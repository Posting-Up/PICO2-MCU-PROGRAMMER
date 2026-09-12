/* Derived from Posting-Up/PICO2-MCU-PROGRAMMER src/AVR_PDI.c
 * upstream commit fe31fce17efb8c8ad0d565a938c3f82ab5517cd4.
 * See README.md for local changes and hardware validation limits. */
/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "avr_pdi.pio.h"
#include "pdi_programmer.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define PDI_CLOCK_HZ                    125000u

/* Retain upstream RP2350 PIO2/SM0 allocation. One SM owns both directions. */
#define PDI_TX_CHUNK_BITS               32u
#define PDI_FRAME_BITS                  12u
#define PDI_BREAK_BITS                  12u

// ATxmega192A3U signature bytes
#define ATXMEGA192A3U_DEVID0            0x1Eu
#define ATXMEGA192A3U_DEVID1            0x97u
#define ATXMEGA192A3U_DEVID2            0x44u

// PDI instruction opcodes
#define PDI_CMD_LDS(ADDR_SZ, DATA_SZ)   (0x00u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_STS(ADDR_SZ, DATA_SZ)   (0x40u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_LDCS(CSREG)             (0x80u | (CSREG))
#define PDI_CMD_STCS(CSREG)             (0xC0u | (CSREG))
#define PDI_CMD_KEY                     (0xE0u)
#define PDI_SIZE_1BYTE                  0u
#define PDI_SIZE_4BYTES                 3u

// PDI Control/Status Register space (CSRS)
#define PDI_CSR_STATUS                  0u
#define PDI_CSR_RESET                   1u
#define PDI_CSR_CTRL                    2u

#define PDI_STATUS_NVMEN                (1u << 1)   // STATUS bit 1 = NVMEN
#define PDI_RESET_SIGNATURE             0x59u       // write to RESET reg to force reset
#define PDI_CTRL_GUARDTIME_32           0x02u       // CTRL[2:0]=GUARDTIME; 32 IDLE bits on RX->TX turnaround

// PDI address space
#define PDI_DATAMEM_BASE                0x01000000u
#define NVM_BASE                        (PDI_DATAMEM_BASE + 0x01C0u)
#define NVM_REG_CMD                     0x0Au
#define NVM_REG_STATUS                  0x0Fu
#define NVM_STATUS_NVMBUSY              (1u << 7)   // STATUS bit 7 = NVMBUSY, bit 6 = FBUSY
#define MCU_DEVID0_ADDR                 (PDI_DATAMEM_BASE + 0x0090u)

#define NVM_CMD_NOOP 0x00u
#define NVM_CMD_READ_NVM 0x43u

// Start-bit search window in PDI_CLK bits
#define PDI_RX_START_BIT_TIMEOUT_BITS   4096u

// NVM completion budgets in WALL-CLOCK ms
#define PDI_NVMEN_TIMEOUT_MS            4000u   /* chip erase of a full 192KB image measured > 1656ms */
#define PDI_NVM_BUSY_TIMEOUT_MS         2000u
#define PDI_RESET_RELEASE_ATTEMPTS      64u

// Clocked settle delay between a fuse write and its verify read, in PDI_CLK idle bits (~8.2ms)


// Host-side (Intel HEX) section bases


/* -------------------------------------------------------------------------- */
/*                               Structures                                   */
/* -------------------------------------------------------------------------- */
typedef enum {
    PDI_OK = 0,
    PDI_ERR_RX_TIMEOUT,      /* target never produced a start bit           */
    PDI_ERR_PARITY,          /* even-parity mismatch on a received frame    */
    PDI_ERR_FRAME,           /* stop bit(s) not high                        */
    PDI_ERR_NVMEN_TIMEOUT,   /* NVMEN never asserted after the KEY sequence */
    PDI_ERR_NVM_BUSY,        /* NVM controller BUSY never cleared           */
    PDI_ERR_RESET_RELEASE,   /* PDI RESET register would not clear on exit  */
    PDI_ERR_ID_MISMATCH      /* device ID did not match the expected value  */
} pdi_status_t;

static bool PDI_DATA_IS_OUTPUT;
static bool PDI_LINK_RUNNING;

static const PIO  PDI_PIO = pio2;
static const uint PDI_SM  = 0u;
static uint       PDI_OFFSET;
static bool       PDI_PIO_LOADED;

static const uint8_t PDI_NVM_PROG_KEY[8] =
{
    0xFFu, 0x88u, 0xD8u, 0xCDu, 0x45u, 0xABu, 0x89u, 0x12u
};

/* -------------------------------------------------------------------------- */
/*                              Static Handlers                               */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Loads the PDI PIO program, configures the state machine and hands it the PDI_CLK/PDI_DATA pins
 * INPUT:       sm_clkdiv (float) - State machine clock divider against clk_sys
 * RETURN:      ---
 */
static void PDI_PIO_INIT(float sm_clkdiv)
{
    if (!PDI_PIO_LOADED)
    {
        pio_sm_claim(PDI_PIO, PDI_SM);
        PDI_OFFSET     = pio_add_program(PDI_PIO, &avr_pdi_program);
        PDI_PIO_LOADED = true;
    }

    avr_pdi_program_init(PDI_PIO, PDI_SM, PDI_OFFSET, PDI_PIN_CLK, PDI_PIN_DATA, sm_clkdiv);
    avr_pdi_claim_pins(PDI_PIO, PDI_SM, PDI_PIN_CLK, PDI_PIN_DATA);
}

/**
 * DESCRIPTION: Statically parks PDI_CLK at a level and drives or releases PDI_DATA with the state machine halted
 * INPUT:       clk_level (bool) - Level to hold on PDI_CLK
 *              data_mode (int)  - AVR_PDI_DATA_LOW, AVR_PDI_DATA_HIGH or AVR_PDI_DATA_RELEASE
 * RETURN:      ---
 */
static void PDI_HOLD_LINES(bool clk_level, int data_mode)
{
    avr_pdi_hold_lines(PDI_PIO, PDI_SM, clk_level, data_mode);
}

/**
 * DESCRIPTION: Returns the state machine to the job dispatcher so clocked transfers can resume
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_PIO_RUN(void)
{
    avr_pdi_run(PDI_PIO, PDI_SM, PDI_OFFSET);
    PDI_LINK_RUNNING = true;
}

/**
 * DESCRIPTION: Clocks out a pattern of up to 32 bits, least significant bit first, with the data line driven
 * INPUT:       pattern (uint32_t) - Bit pattern to shift out
 *              bits (uint32_t)    - Number of bits to clock, 1 to 32
 * RETURN:      ---
 */
static void PDI_CLOCK_OUT_BITS(uint32_t pattern, uint32_t bits)
{
    avr_pdi_tx_bits(PDI_PIO, PDI_SM, pattern, bits);
    PDI_DATA_IS_OUTPUT = true;
}

/**
 * DESCRIPTION: Generates a specified number of idle clock cycles with the data line driven high
 * INPUT:       n (uint32_t) - The number of idle bits to clock out
 * RETURN:      ---
 */
static void PDI_CLOCK_IDLE_BITS(uint32_t n)
{
    while (n)
    {
        uint32_t chunk = (n > PDI_TX_CHUNK_BITS) ? PDI_TX_CHUNK_BITS : n;

        PDI_CLOCK_OUT_BITS(0xFFFFFFFFu, chunk);
        n -= chunk;
    }
}

/**
 * DESCRIPTION: Transmits a PDI BREAK condition by holding the data line low for 12 clock cycles
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_SEND_BREAK(void)
{
    PDI_CLOCK_OUT_BITS(0x00000000u, PDI_BREAK_BITS);
}

/**
 * DESCRIPTION: Safely shifts the PDI link from transmit to receive mode by clocking two idle bits before line release
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_ENTER_RX(void)
{
    if (PDI_DATA_IS_OUTPUT)
    {
        PDI_CLOCK_OUT_BITS(0x3u, 2u);
        PDI_DATA_IS_OUTPUT = false;
    }
}

/**
 * DESCRIPTION: Prepares the PDI link for transmission by providing initial idle bits if the data line is not driven
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_ENTER_TX(void)
{
    if (!PDI_DATA_IS_OUTPUT)
    {
        PDI_CLOCK_OUT_BITS(0xFFFFFFFFu, 32u);
    }
}

/**
 * DESCRIPTION: Receives a byte over PDI by handling start bit synchronization, data parsing, parity checking, and framing verification
 * INPUT:       out (uint8_t*) - Pointer to store the successfully received byte
 * RETURN:      pdi_status_t   - Execution status (PDI_OK, PDI_ERR_RX_TIMEOUT, PDI_ERR_PARITY, or PDI_ERR_FRAME)
 */
static pdi_status_t PDI_RX_BYTE(uint8_t *out)
{
    PDI_ENTER_RX();

    uint32_t word = avr_pdi_rx_frame(PDI_PIO, PDI_SM, PDI_RX_START_BIT_TIMEOUT_BITS);

    if (word == AVR_PDI_RX_TIMEOUT)
    {
        return PDI_ERR_RX_TIMEOUT;
    }

    uint32_t frame = (word >> AVR_PDI_RX_FRAME_SHIFT) & AVR_PDI_RX_FRAME_MASK;

    uint8_t value  = (uint8_t)(frame & 0xFFu);
    bool    parity = 0;

    for (int i = 0; i < 8; i++)
    {
        parity ^= (bool)((value >> i) & 1u);
    }

    bool parity_rx = (bool)((frame >> 8) & 1u);
    bool stop1     = (bool)((frame >> 9) & 1u);
    bool stop2     = (bool)((frame >> 10) & 1u);

    if (parity_rx != parity)
    {
        return PDI_ERR_PARITY;
    }
    if (!stop1 || !stop2)
    {
        return PDI_ERR_FRAME;
    }

    *out = value;
    return PDI_OK;
}

/**
 * DESCRIPTION: Transmits a single byte over PDI by framing it with a start bit, 8 data bits, an even parity bit, and 2 stop bits
 * INPUT:       byte (uint8_t) - The data byte to transmit
 * RETURN:      ---
 */
static void PDI_TX_BYTE(uint8_t byte)
{
    PDI_ENTER_TX();

    uint32_t parity = 0;

    for (int i = 0; i < 8; i++)
    {
        parity ^= (uint32_t)((byte >> i) & 1u);
    }

    uint32_t frame = ((uint32_t)byte << 1)      /* start bit 0 at bit 0      */
                   | (parity << 9)
                   | (3u << 10);                /* stop bits 1 and 2         */

    PDI_CLOCK_OUT_BITS(frame, PDI_FRAME_BITS);
}

/**
 * DESCRIPTION: Transmits a 32-bit address over PDI in little-endian byte order
 * INPUT:       addr (uint32_t) - The 32-bit destination address to transmit
 * RETURN:      ---
 */
static void PDI_TX_ADDR32(uint32_t addr)
{
    PDI_TX_BYTE((uint8_t)(addr & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 8) & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 16) & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 24) & 0xFFu));
}

/**
 * DESCRIPTION: Executes a PDI Store Direct (STS) command to write a 1-byte value to a 32-bit address
 * INPUT:       addr (uint32_t)  - The target 32-bit memory address
 *              value (uint8_t)  - The data byte to write
 * RETURN:      ---
 */
static void PDI_STS_BYTE(uint32_t addr, uint8_t value)
{
    PDI_TX_BYTE(PDI_CMD_STS(PDI_SIZE_4BYTES, PDI_SIZE_1BYTE));
    PDI_TX_ADDR32(addr);
    PDI_TX_BYTE(value);
}

/**
 * DESCRIPTION: Executes a PDI Load Direct (LDS) command to read a 1-byte value from a 32-bit address
 * INPUT:       addr (uint32_t)   - The target 32-bit memory address to read from
 *              value (uint8_t*)  - Pointer to store the retrieved data byte
 * RETURN:      pdi_status_t      - Execution status of the underlying frame reception
 */
static pdi_status_t PDI_LDS_BYTE(uint32_t addr, uint8_t *value)
{
    PDI_TX_BYTE(PDI_CMD_LDS(PDI_SIZE_4BYTES, PDI_SIZE_1BYTE));
    PDI_TX_ADDR32(addr);
    return PDI_RX_BYTE(value);
}

/**
 * DESCRIPTION: Forces the PDI hardware into synchronization by sending double break states separated by idle bits
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_RESYNC(void)
{
    PDI_ENTER_TX();
    PDI_SEND_BREAK();
    PDI_CLOCK_IDLE_BITS(2);
    PDI_SEND_BREAK();
    PDI_CLOCK_IDLE_BITS(4);
}

/**
 * DESCRIPTION: Executes a PDI Store Control and Status (STCS) command to write a value to a system register
 * INPUT:       csreg (uint8_t) - The target Control/Status register index
 *              value (uint8_t) - The data byte to write to the register
 * RETURN:      ---
 */
static void PDI_STCS(uint8_t csreg, uint8_t value)
{
    PDI_TX_BYTE(PDI_CMD_STCS(csreg));
    PDI_TX_BYTE(value);
}

/**
 * DESCRIPTION: Executes a PDI Load Control and Status (LDCS) command to read a value from a system register
 * INPUT:       csreg (uint8_t)  - The target Control/Status register index to read from
 *              value (uint8_t*) - Pointer to store the retrieved register byte
 * RETURN:      pdi_status_t     - Execution status of the underlying frame reception
 */
static pdi_status_t PDI_LDCS(uint8_t csreg, uint8_t *value)
{
    PDI_TX_BYTE(PDI_CMD_LDCS(csreg));
    return PDI_RX_BYTE(value);
}

/**
 * DESCRIPTION: Polls the PDI Status register until the NVM controller becomes active or times out
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if NVM becomes active, or PDI_ERR_NVMEN_TIMEOUT on failure
 */
static pdi_status_t PDI_WAIT_NVM_BUS_ACTIVE(void)
{
     absolute_time_t deadline = make_timeout_time_ms(PDI_NVMEN_TIMEOUT_MS);
     while (!time_reached(deadline))
     {
        uint8_t status = 0;
        pdi_status_t st = PDI_LDCS(PDI_CSR_STATUS, &status);

        if (st != PDI_OK)
        {
            PDI_RESYNC();
            continue;
        }
        if (status & PDI_STATUS_NVMEN)
        {
            return PDI_OK;
        }
    }
    return PDI_ERR_NVMEN_TIMEOUT;
}

/**
 * DESCRIPTION: Polls the NVM controller status register until the busy flag clears or the execution limit is reached
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if the NVM engine is free, or PDI_ERR_NVM_BUSY on failure
 */
static pdi_status_t PDI_WAIT_NVM_NOT_BUSY(void)
{
    absolute_time_t deadline = make_timeout_time_ms(PDI_NVM_BUSY_TIMEOUT_MS);
    while (!time_reached(deadline))
    {
        uint8_t status = 0;

        pdi_status_t st = PDI_LDS_BYTE(NVM_BASE + NVM_REG_STATUS, &status);
        if (st != PDI_OK)
        {
            PDI_RESYNC();
            continue;
        }
        if (!(status & NVM_STATUS_NVMBUSY))
        {
            return PDI_OK;
        }
    }

    return PDI_ERR_NVM_BUSY;
}

/**
 * DESCRIPTION: Initializes the physical PDI link, enters programming mode by unlocking the NVM security key, and verifies NVM bus activity
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if programming mode is enabled, or an error status code on failure
 */
static pdi_status_t PDI_ENABLE(void)
{
    PDI_HOLD_LINES(0, AVR_PDI_DATA_LOW);        /* CLK low = target in reset */
    PDI_DATA_IS_OUTPUT = true;
    sleep_ms(1);                                /* settle / assert reset     */

    PDI_HOLD_LINES(0, AVR_PDI_DATA_HIGH);       /* disable RESET function    */
    busy_wait_us_32(10);                        /* > tEXT(max) = 1us         */

    PDI_PIO_RUN();
    PDI_CLOCK_IDLE_BITS(32);                    /* spec minimum is 16        */
    PDI_RESYNC();                               /* known RX state            */

    PDI_STCS(PDI_CSR_RESET, PDI_RESET_SIGNATURE);
    PDI_STCS(PDI_CSR_CTRL, PDI_CTRL_GUARDTIME_32);

    PDI_TX_BYTE(PDI_CMD_KEY);
    for (int i = 0; i < 8; i++)
    {
        PDI_TX_BYTE(PDI_NVM_PROG_KEY[i]);
    }

    pdi_status_t st = PDI_WAIT_NVM_BUS_ACTIVE();
    if (st != PDI_OK)
    {
        return st;
    }

    return PDI_OK;
}

/**
 * DESCRIPTION: Disables the PDI interface, attempts to clear the target's reset state, and releases the GPIO lines
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if the target reset was released successfully, or PDI_ERR_RESET_RELEASE on failure
 */
static pdi_status_t PDI_DISABLE(void)
{
    if (!PDI_LINK_RUNNING) return PDI_OK;
    bool released = false;

    for (uint32_t i = 0; i < PDI_RESET_RELEASE_ATTEMPTS; i++)
    {
        PDI_STCS(PDI_CSR_RESET, 0x00u);

        uint8_t value = 0xFFu;
        if (PDI_LDCS(PDI_CSR_RESET, &value) == PDI_OK && value == 0x00u)
        {
            released = true;
            break;
        }
        PDI_RESYNC();
    }

    if (released)
    {
        printf("PDI: reset register cleared - target released\n");
    }
    else
    {
        printf("PDI: WARNING - reset register would not clear\n");
    }

    PDI_HOLD_LINES(1, AVR_PDI_DATA_LOW);
    PDI_DATA_IS_OUTPUT = true;
    sleep_ms(2);

    avr_pdi_release_pins(PDI_PIO, PDI_SM, PDI_PIN_CLK, PDI_PIN_DATA);
    PDI_LINK_RUNNING = false;
    PDI_DATA_IS_OUTPUT = false;

    printf("PDI: interface disabled, lines released\n");

    return released ? PDI_OK : PDI_ERR_RESET_RELEASE;
}

/**
 * DESCRIPTION: Reads the 3-byte signature/device ID from the target MCU and verifies it matches the expected ATxmega192A3U signature
 * INPUT:       id (uint8_t[3]) - Destination array to store the three signature bytes
 * RETURN:      pdi_status_t    - PDI_OK if reading succeeds and signature matches, or an error code on failure
 */
static pdi_status_t PDI_READ_DEVICE_ID(uint8_t id[3])
{
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);

    for (uint32_t i = 0; i < 3; i++)
    {
        pdi_status_t st = PDI_LDS_BYTE(MCU_DEVID0_ADDR + i, &id[i]);
        if (st != PDI_OK)
        {
            return st;
        }
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

    printf("PDI: DEVICE ID = 0x%02X 0x%02X 0x%02X\n",
            id[0], id[1], id[2]);

    if (id[0] != ATXMEGA192A3U_DEVID0 ||
        id[1] != ATXMEGA192A3U_DEVID1 ||
        id[2] != ATXMEGA192A3U_DEVID2) {
        printf("PDI: MISMATCH - expected 0x%02X 0x%02X 0x%02X (ATxmega192A3U)\n",
                ATXMEGA192A3U_DEVID0, ATXMEGA192A3U_DEVID1,
                ATXMEGA192A3U_DEVID2);
        return PDI_ERR_ID_MISMATCH;
    }

    return PDI_OK;
}


/* ATxmega192A3U-only adapter. The upstream multi-device entry point is not used. */
static bool bus_read(uint32_t address, uint8_t *value)
{
    return PDI_LDS_BYTE(address, value) == PDI_OK;
}
static bool bus_wait(void) { return PDI_WAIT_NVM_NOT_BUSY() == PDI_OK; }
static bool connect_target(void)
{
    uint8_t id[3];
    PDI_PIO_INIT((float)clock_get_hz(clk_sys) / (2.0f * PDI_CLOCK_HZ));
    PDI_DATA_IS_OUTPUT = false;
    pdi_status_t st = PDI_ENABLE();
    if (st == PDI_OK) st = PDI_READ_DEVICE_ID(id);
    if (st != PDI_OK) printf("PDI: connect error %u\n", (unsigned)st);
    return st == PDI_OK;
}
static bool bus_reset(void)
{
    if (PDI_DISABLE() != PDI_OK) return false;
    sleep_ms(100);
    return connect_target();
}
static bool session_active;
bool pdi_begin(void)
{
    if (session_active) return false;
    bool ok = connect_target();
    if (!ok) PDI_DISABLE();
    session_active = ok;
    return ok;
}
bool pdi_end(void)
{
    if (!session_active) return true;
    session_active = false;
    return PDI_DISABLE() == PDI_OK;
}
bool pdi_identify(void)
{
    if (session_active) return false;
    bool ok = pdi_begin();
    return pdi_end() && ok;
}
bool pdi_program(xmega_region region, uint32_t offset,
                 const uint8_t *data, size_t length)
{
    if (!session_active || !xmega_valid_request(region, offset, data, length)) return false;
    bool ok = true;
    if (ok) {
        const xmega_bus bus = {bus_read, PDI_STS_BYTE, bus_wait, bus_reset};
        xmega_result result = xmega_write(&bus, region, offset, data, length);
        printf("PDI: write result %u (0=verified, 1=invalid, 2=I/O, 3=mismatch)\n",
               (unsigned)result);
        ok = result == XMEGA_OK;
    }
    if (!ok) pdi_end();
    return ok;
}
