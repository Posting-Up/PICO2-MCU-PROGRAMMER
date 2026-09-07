/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include "AVR_PDI.h"


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
static inline void PDI_DELAY_HALF(void)
{
    busy_wait_us_32(PDI_CLK_HALF_PERIOD_US);
}

static void PDI_DATA_RELEASE(void)
{
    if (PDI_DATA_IS_OUTPUT) 
    {
        gpio_set_dir(PDI_PIN_DATA, GPIO_IN);
        PDI_DATA_IS_OUTPUT = false;
    }
}

static void PDI_DATA_OUTPUT(bool level)
{
    gpio_put(PDI_PIN_DATA, level);
    
    if (!PDI_DATA_IS_OUTPUT) 
    {
        gpio_set_dir(PDI_PIN_DATA, GPIO_OUT);
        PDI_DATA_IS_OUTPUT = true;
    }
}

static bool PDI_CLOCK_IN_BIT(void)
{
    gpio_put(PDI_PIN_CLK, 0);
    PDI_DELAY_HALF();
    
    bool bit = gpio_get(PDI_PIN_DATA);
    
    gpio_put(PDI_PIN_CLK, 1);
    PDI_DELAY_HALF();
    return bit;
}

static void PDI_CLOCK_OUT_BIT(bool bit)
{
    gpio_put(PDI_PIN_CLK, 0);
    
    PDI_DATA_OUTPUT(bit);
    PDI_DELAY_HALF();

    gpio_put(PDI_PIN_CLK, 1);
    PDI_DELAY_HALF();
}

static void PDI_CLOCK_IDLE_BITS(uint32_t n)
{
    while (n--) 
    {
        PDI_CLOCK_OUT_BIT(1);
    }
}

static void PDI_SEND_BREAK(void)
{
    for (int i = 0; i < 12; i++) 
    {
        PDI_CLOCK_OUT_BIT(0);
    }
}

static void PDI_ENTER_RX(void)
{
    if (PDI_DATA_IS_OUTPUT) 
    {
        PDI_CLOCK_OUT_BIT(1);
        PDI_CLOCK_OUT_BIT(1);
        PDI_DATA_RELEASE();
    }
}

static void PDI_ENTER_TX(void)
{
    if (!PDI_DATA_IS_OUTPUT) 
    {
        PDI_CLOCK_OUT_BIT(1);
        PDI_CLOCK_OUT_BIT(1);
    }
}

static pdi_status_t PDI_RX_BYTE(uint8_t *out)
{
    PDI_ENTER_RX();

    bool got_start = false;
    for (uint32_t i = 0; i < PDI_RX_START_BIT_TIMEOUT_BITS; i++) 
    {
        if (PDI_CLOCK_IN_BIT() == 0) 
        {
            got_start = true;
            break;
        }
    }
    if (!got_start) 
    {
        return PDI_ERR_RX_TIMEOUT;
    }

    uint8_t value  = 0;
    bool    parity = 0;

    for (int i = 0; i < 8; i++) 
    {
        bool bit = PDI_CLOCK_IN_BIT();
        parity ^= bit;
        value |= (uint8_t)(bit << i);
    }

    bool parity_rx = PDI_CLOCK_IN_BIT();
    bool stop1     = PDI_CLOCK_IN_BIT();
    bool stop2     = PDI_CLOCK_IN_BIT();

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

static void PDI_TX_BYTE(uint8_t byte)
{
    PDI_ENTER_TX();

    bool parity = 0;

    PDI_CLOCK_OUT_BIT(0);                       /* start bit                 */
    for (int i = 0; i < 8; i++) 
    {
        bool bit = (byte >> i) & 1u;
        parity ^= bit;
        PDI_CLOCK_OUT_BIT(bit);
    }
    PDI_CLOCK_OUT_BIT(parity);
    PDI_CLOCK_OUT_BIT(1);                       /* stop bit 1                */
    PDI_CLOCK_OUT_BIT(1);                       /* stop bit 2                */
}

static void PDI_TX_ADDR32(uint32_t addr)
{
    PDI_TX_BYTE((uint8_t)(addr & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 8) & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 16) & 0xFFu));
    PDI_TX_BYTE((uint8_t)((addr >> 24) & 0xFFu));
}

static void PDI_STS_BYTE(uint32_t addr, uint8_t value)
{
    PDI_TX_BYTE(PDI_CMD_STS(PDI_SIZE_4BYTES, PDI_SIZE_1BYTE));
    PDI_TX_ADDR32(addr);
    PDI_TX_BYTE(value);
}

static pdi_status_t PDI_LDS_BYTE(uint32_t addr, uint8_t *value)
{
    PDI_TX_BYTE(PDI_CMD_LDS(PDI_SIZE_4BYTES, PDI_SIZE_1BYTE));
    PDI_TX_ADDR32(addr);
    return PDI_RX_BYTE(value);
}

static void PDI_RESYNC(void)
{
    PDI_ENTER_TX();
    PDI_SEND_BREAK();
    PDI_CLOCK_IDLE_BITS(2);
    PDI_SEND_BREAK();
    PDI_CLOCK_IDLE_BITS(4);
}

static void PDI_STCS(uint8_t csreg, uint8_t value)
{
    PDI_TX_BYTE(PDI_CMD_STCS(csreg));
    PDI_TX_BYTE(value);
}

static pdi_status_t PDI_LDCS(uint8_t csreg, uint8_t *value)
{
    PDI_TX_BYTE(PDI_CMD_LDCS(csreg));
    return PDI_RX_BYTE(value);
}

static pdi_status_t PDI_WAIT_NVM_BUS_ACTIVE(void)
{
    for (uint32_t i = 0; i < PDI_NVMEN_POLL_LIMIT; i++) 
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

static pdi_status_t PDI_ENABLE(void)
{
    gpio_put(PDI_PIN_CLK, 0);
    gpio_set_dir(PDI_PIN_CLK, GPIO_OUT);        /* CLK low = target in reset */

    PDI_DATA_IS_OUTPUT = false;
    PDI_DATA_OUTPUT(0);
    sleep_ms(1);                                /* settle / assert reset     */

    PDI_DATA_OUTPUT(1);                         /* disable RESET function    */
    busy_wait_us_32(10);                        /* > tEXT(max) = 1us         */
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

static pdi_status_t PDI_DISABLE(void)
{
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

    gpio_put(PDI_PIN_CLK, 1);
    PDI_DATA_OUTPUT(0);
    sleep_ms(2);

    gpio_set_dir(PDI_PIN_CLK, GPIO_IN);
    PDI_DATA_RELEASE();

    printf("PDI: interface disabled, lines released (target running)\n");

    return released ? PDI_OK : PDI_ERR_RESET_RELEASE;
}

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

static bool PROGRAM_XMEGA_PDI(const xmega_chip_t *CHIP, const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
{
    /* -------------------------------------------------------------------------- */
    /*                      (1) Initialization                                    */
    /* -------------------------------------------------------------------------- */
    gpio_init(PDI_PIN_CLK);  
    gpio_init(PDI_PIN_DATA); 

    gpio_disable_pulls(PDI_PIN_CLK);
    gpio_disable_pulls(PDI_PIN_DATA);

    gpio_set_dir(PDI_PIN_CLK, GPIO_IN);
    gpio_set_dir(PDI_PIN_DATA, GPIO_IN);
    
    PDI_DATA_IS_OUTPUT = false;

    /* -------------------------------------------------------------------------- */
    /*                       (2) PDI Enable                                       */
    /* -------------------------------------------------------------------------- */
    if (PDI_ENABLE() != PDI_OK) 
    {
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                      (3) Read Device ID                                    */
    /* -------------------------------------------------------------------------- */
    uint8_t DEVICE_ID[3] = {0};
    
    if (PDI_READ_DEVICE_ID(DEVICE_ID) != PDI_OK) 
    {
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                      (4) PDI Disable                                       */
    /* -------------------------------------------------------------------------- */
    if (PDI_DISABLE() != PDI_OK) 
    {
        return false;
    }
    
    return true;
}

bool PROGRAM_ATXMEGA192A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA192A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA128A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA128A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA128A4U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA128A4U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA64AU(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA64A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA32C3(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA32C3, buffer, total_packets);
}

bool PROGRAM_ATXMEGA32E5(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA32E5, buffer, total_packets);
}
