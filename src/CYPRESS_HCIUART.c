/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "CYPRESS_HCIUART.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "pico/stdio.h"
#include "tusb.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define CYBT_RESET_ASSERT_MS 10u
#define CYBT_RESET_SETTLE_MS 10u
#define CYBT_DRAIN_PASSES    64u


/* -------------------------------------------------------------------------- */
/*                                 Structures                                 */
/* -------------------------------------------------------------------------- */
static volatile uint8_t  UART_RING[CYBT_RING_SIZE];
static volatile uint32_t RING_HEAD;
static volatile uint32_t RING_TAIL;

typedef struct
{
    gpio_function_t FUNCTION;
    bool            PULL_UP;
    bool            PULL_DOWN;
    bool            IS_OUTPUT;
    bool            OUT_LEVEL;
} CYBT_PIN_STATE_t;

static const uint       CYBT_PINS[] = { PIN_CYBT_UART_TX, PIN_CYBT_UART_RX, PIN_CYBT_MCLR,
                                        PIN_CYBT_CTS, PIN_CYBT_RADIO_RST };
static CYBT_PIN_STATE_t CYBT_PIN_SNAPSHOT[count_of(CYBT_PINS)];


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */

/**
 * DESCRIPTION: UART0 RX interrupt. Drains the PL011 FIFO into the ring the moment bytes
 *              land, so USB scheduling latency in the bridge loop can never overrun the
 *              32-byte hardware FIFO at 3 Mbaud
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_UART_RX_IRQ(void)
{
    while (uart_is_readable(CYBT_UART_INSTANCE))
    {
        uint32_t RAW = uart_get_hw(CYBT_UART_INSTANCE)->dr;

        if ((RAW & (UART_UARTDR_OE_BITS | UART_UARTDR_BE_BITS |
                    UART_UARTDR_PE_BITS | UART_UARTDR_FE_BITS)) != 0u)
        {
            uart_get_hw(CYBT_UART_INSTANCE)->rsr = 0xFu;
            continue;
        }

        if ((RING_HEAD - RING_TAIL) < CYBT_RING_SIZE)
        {
            UART_RING[RING_HEAD & (CYBT_RING_SIZE - 1u)] = (uint8_t)RAW;
            RING_HEAD++;
        }
    }
}

/**
 * DESCRIPTION: Records how the pins this driver is about to take over were left by whoever
 *              configured them last - boot, or an earlier programming session
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_SAVE_PIN_STATE(void)
{
    for (uint I = 0u; I < count_of(CYBT_PINS); I++)
    {
        CYBT_PIN_SNAPSHOT[I].FUNCTION  = gpio_get_function(CYBT_PINS[I]);
        CYBT_PIN_SNAPSHOT[I].PULL_UP   = gpio_is_pulled_up(CYBT_PINS[I]);
        CYBT_PIN_SNAPSHOT[I].PULL_DOWN = gpio_is_pulled_down(CYBT_PINS[I]);
        CYBT_PIN_SNAPSHOT[I].IS_OUTPUT = gpio_is_dir_out(CYBT_PINS[I]);
        CYBT_PIN_SNAPSHOT[I].OUT_LEVEL = gpio_get_out_level(CYBT_PINS[I]);
    }
}

/**
 * DESCRIPTION: Puts those pins back exactly as CYBT213043_SAVE_PIN_STATE() found them. The
 *              PIO programs the HCS08 driver loads at startup keep their pin claims, and the
 *              PIC/XMEGA drivers re-run gpio_init() on entry, so after this the next family
 *              sees the hardware it expects
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_RESTORE_PIN_STATE(void)
{
    for (uint I = 0u; I < count_of(CYBT_PINS); I++)
    {
        gpio_put(CYBT_PINS[I], CYBT_PIN_SNAPSHOT[I].OUT_LEVEL);
        gpio_set_dir(CYBT_PINS[I], CYBT_PIN_SNAPSHOT[I].IS_OUTPUT);
        gpio_set_pulls(CYBT_PINS[I], CYBT_PIN_SNAPSHOT[I].PULL_UP, CYBT_PIN_SNAPSHOT[I].PULL_DOWN);
        gpio_set_function(CYBT_PINS[I], CYBT_PIN_SNAPSHOT[I].FUNCTION);
    }
}

/**
 * DESCRIPTION: Drives one control pin to a level before switching it to an output, so the
 *              line never glitches through the opposite state
 * INPUT:       Pin number, output level
 * RETURN:      ---
 */
static void CYBT213043_DRIVE(uint PIN, bool LEVEL)
{
    gpio_init(PIN);
    gpio_put(PIN, LEVEL);
    gpio_set_dir(PIN, GPIO_OUT);
}

/**
 * DESCRIPTION: Configures the GPIOs used to talk to the CYBT-213043-02 module
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_INIT(void)
{
    CYBT213043_SAVE_PIN_STATE();

    CYBT213043_DRIVE(PIN_CYBT_MCLR, 0);

    CYBT213043_DRIVE(PIN_CYBT_UART_TX, 1);

    CYBT213043_DRIVE(PIN_CYBT_CTS, 0);       /* tell module it is clear-to-send */
    CYBT213043_DRIVE(PIN_CYBT_RADIO_RST, 0); /* hold module in reset            */
}

/**
 * DESCRIPTION: Power/reset sequence that lands the CYW20819 boot ROM in the autobaud
 *              (HCI download) state
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_ENTER_DOWNLOAD_MODE(void)
{
    gpio_put(PIN_CYBT_CTS, 0);
    gpio_put(PIN_CYBT_RADIO_RST, 0);
    sleep_ms(CYBT_RESET_ASSERT_MS);

    gpio_put(PIN_CYBT_RADIO_RST, 1);
    sleep_ms(CYBT_RESET_SETTLE_MS);
}

/**
 * DESCRIPTION: Brings up UART0 on the HCI pins and arms the RX interrupt. Nothing is sent
 *              from here: the boot ROM autobauds on the FIRST command it receives, so the
 *              PC has to be the one that opens the session, at the rate it wants
 * INPUT:       ---
 * RETURN:      ---
 */
static void CYBT213043_UART_START(void)
{
    RING_HEAD = 0u;
    RING_TAIL = 0u;

    uart_init(CYBT_UART_INSTANCE, CYBT_UART_BAUDRATE);
    gpio_set_function(PIN_CYBT_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_CYBT_UART_RX, GPIO_FUNC_UART);
    gpio_pull_up(PIN_CYBT_UART_RX);

    uart_set_format(CYBT_UART_INSTANCE, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(CYBT_UART_INSTANCE, true);

    irq_set_exclusive_handler(UART0_IRQ, CYBT213043_UART_RX_IRQ);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irqs_enabled(CYBT_UART_INSTANCE, true, false);
}

/**
 * DESCRIPTION: Programs the CYBT-213043-02 (CYW20819) Bluetooth module over the WICED HCI
 *              UART download protocol. The module is reset into its boot-ROM download mode
 *              here, then USB CDC <-> UART0 is bridged byte for byte so that USB_RP_COM.py
 *              can run the HCI sequence (minidriver load, chip erase, Write_RAM, Read_RAM
 *              verify, Launch_RAM) straight against the module. The UART follows the
 *              COM-port baud rate the host sets, which is what lets the boot ROM autobaud
 *              and the minidriver then move to the high-speed download rate.
 *
 *              Returns to family select once the host selects CYBT_EXIT_BAUDRATE or goes
 *              silent, with UART0 released and GP0-GP4 restored, so back-to-back modules can
 *              be programmed over one USB session without rebooting the Pico.
 * INPUT:       ---
 * RETURN:      ---
 */
void CYBT213043_HCI_BRIDGE(void)
{
    uint32_t        ACTIVE_BAUD = CYBT_UART_BAUDRATE;
    char            BRIDGE_BUF[CYBT_BRIDGE_CHUNK];
    absolute_time_t IDLE_DEADLINE;

    /* -------------------------------------------------------------------------- */
    /*                      (1) Initialization                                    */
    /* -------------------------------------------------------------------------- */
    CYBT213043_INIT();

    /* -------------------------------------------------------------------------- */
    /*                      (2) Enter Download Mode                               */
    /* -------------------------------------------------------------------------- */
    CYBT213043_ENTER_DOWNLOAD_MODE();
    CYBT213043_UART_START();

    /* -------------------------------------------------------------------------- */
    /*                      (3) Bridge USB CDC <-> HCI UART                       */
    /* -------------------------------------------------------------------------- */
    IDLE_DEADLINE = make_timeout_time_ms(CYBT_IDLE_TIMEOUT_MS);

    while (!time_reached(IDLE_DEADLINE))
    {
        int BYTES_IN = stdio_get_until(BRIDGE_BUF, sizeof(BRIDGE_BUF), make_timeout_time_us(0));

        cdc_line_coding_t LINE_CODING;
        tud_cdc_get_line_coding(&LINE_CODING);

        if (LINE_CODING.bit_rate == CYBT_EXIT_BAUDRATE)
        {
            break;
        }

        if ((LINE_CODING.bit_rate != ACTIVE_BAUD) && (LINE_CODING.bit_rate != 0u))
        {
            uart_tx_wait_blocking(CYBT_UART_INSTANCE);
            ACTIVE_BAUD = uart_set_baudrate(CYBT_UART_INSTANCE, LINE_CODING.bit_rate)
                              ? LINE_CODING.bit_rate
                              : ACTIVE_BAUD;
        }

        if (BYTES_IN > 0)
        {
            uart_write_blocking(CYBT_UART_INSTANCE, (const uint8_t *)BRIDGE_BUF, BYTES_IN);
        }

        int BYTES_OUT = 0;
        while ((BYTES_OUT < (int)sizeof(BRIDGE_BUF)) && (RING_TAIL != RING_HEAD))
        {
            BRIDGE_BUF[BYTES_OUT++] = UART_RING[RING_TAIL & (CYBT_RING_SIZE - 1u)];
            RING_TAIL++;
        }

        if (BYTES_OUT > 0)
        {
            stdio_put_string(BRIDGE_BUF, BYTES_OUT, false, false);
            stdio_flush();
        }

        if ((BYTES_IN > 0) || (BYTES_OUT > 0))
        {
            IDLE_DEADLINE = make_timeout_time_ms(CYBT_IDLE_TIMEOUT_MS);
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                      (4) HCI Release                                       */
    /* -------------------------------------------------------------------------- */
    uart_set_irqs_enabled(CYBT_UART_INSTANCE, false, false);
    irq_set_enabled(UART0_IRQ, false);
    irq_remove_handler(UART0_IRQ, CYBT213043_UART_RX_IRQ);
    uart_deinit(CYBT_UART_INSTANCE);

    for (uint32_t DRAINED = 0u; DRAINED < CYBT_DRAIN_PASSES; DRAINED++)
    {
        if (stdio_get_until(BRIDGE_BUF, sizeof(BRIDGE_BUF), make_timeout_time_us(0)) <= 0)
        {
            break;
        }
    }

    CYBT213043_RESTORE_PIN_STATE();
}
