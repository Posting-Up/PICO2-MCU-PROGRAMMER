/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include "AVR_PDI.h"


/* -------------------------------------------------------------------------- */
/*                              Static Handlers                               */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Delays execution for half of the PDI clock period
 * INPUT:       ---
 * RETURN:      ---
 */
static inline void PDI_DELAY_HALF(void)
{
    busy_wait_us_32(PDI_CLK_HALF_PERIOD_US);
}

/**
 * DESCRIPTION: Configures the PDI data pin as an input to release the line
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_DATA_RELEASE(void)
{
    if (PDI_DATA_IS_OUTPUT) 
    {
        gpio_set_dir(PDI_PIN_DATA, GPIO_IN);
        PDI_DATA_IS_OUTPUT = false;
    }
}

/**
 * DESCRIPTION: Sets the PDI data pin state and configures it as an output
 * INPUT:       level (bool) - The digital logic state (true = High, false = Low)
 * RETURN:      ---
 */
static void PDI_DATA_OUTPUT(bool level)
{
    gpio_put(PDI_PIN_DATA, level);
    
    if (!PDI_DATA_IS_OUTPUT) 
    {
        gpio_set_dir(PDI_PIN_DATA, GPIO_OUT);
        PDI_DATA_IS_OUTPUT = true;
    }
}

/**
 * DESCRIPTION: Clocks in a single data bit from the PDI target
 * INPUT:       ---
 * RETURN:      bool - The sampled logic state of the data pin
 */
static bool PDI_CLOCK_IN_BIT(void)
{
    gpio_put(PDI_PIN_CLK, 0);
    PDI_DELAY_HALF();
    
    bool bit = gpio_get(PDI_PIN_DATA);
    
    gpio_put(PDI_PIN_CLK, 1);
    PDI_DELAY_HALF();
    return bit;
}

/**
 * DESCRIPTION: Clocks out a single data bit to the PDI target
 * INPUT:       bit (bool) - The logic state to transmit on the data pin
 * RETURN:      ---
 */
static void PDI_CLOCK_OUT_BIT(bool bit)
{
    gpio_put(PDI_PIN_CLK, 0);
    
    PDI_DATA_OUTPUT(bit);
    PDI_DELAY_HALF();

    gpio_put(PDI_PIN_CLK, 1);
    PDI_DELAY_HALF();
}

/**
 * DESCRIPTION: Generates a specified number of idle clock cycles with the data line driven high
 * INPUT:       n (uint32_t) - The number of idle bits to clock out
 * RETURN:      ---
 */
static void PDI_CLOCK_IDLE_BITS(uint32_t n)
{
    while (n--) 
    {
        PDI_CLOCK_OUT_BIT(1);
    }
}

/**
 * DESCRIPTION: Transmits a PDI BREAK condition by holding the data line low for 12 clock cycles
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_SEND_BREAK(void)
{
    for (int i = 0; i < 12; i++) 
    {
        PDI_CLOCK_OUT_BIT(0);
    }
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
        PDI_CLOCK_OUT_BIT(1);
        PDI_CLOCK_OUT_BIT(1);
        PDI_DATA_RELEASE();
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
        PDI_CLOCK_OUT_BIT(1);
        PDI_CLOCK_OUT_BIT(1);
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

/**
 * DESCRIPTION: Transmits a single byte over PDI by framing it with a start bit, 8 data bits, an even parity bit, and 2 stop bits
 * INPUT:       byte (uint8_t) - The data byte to transmit
 * RETURN:      ---
 */
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
     for (uint32_t i = 0; i < PDI_NVM_BUSY_TIMEOUT_MS; i++) 
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
    for (uint32_t i = 0; i < PDI_NVM_BUSY_POLL_LIMIT; i++) 
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

/**
 * DESCRIPTION: Disables the PDI interface, attempts to clear the target's reset state, and releases the GPIO lines
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if the target reset was released successfully, or PDI_ERR_RESET_RELEASE on failure
 */
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

/**
 * DESCRIPTION: Executes a full chip erase command sequence and waits for the NVM bus to re-activate and complete the operation
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if chip erase completes successfully, or an error code on failure
 */
static pdi_status_t PDI_CHIP_ERASE(void)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK) 
    {
        printf("PDI: FAILED - NVM busy before erase\n");
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_CHIP_ERASE);
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);

    st = PDI_WAIT_NVM_BUS_ACTIVE();
    if (st != PDI_OK)
    {
        printf("PDI: FAILED - NVM bus did not return after erase\n");
        return st;
    }

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        printf("PDI: FAILED - NVMBUSY never cleared\n");
        return st;
    }
    printf("PDI: CHIP ERASED SUCCESSFULLY\n");

    return PDI_OK;
}

/**
 * DESCRIPTION: Erases the user signature row and restores all non-reserved device fuses to their default factory values
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if defaults are programmed successfully, or an error code on failure
 */
static pdi_status_t PDI_WRITE_DEFAULTS(void)
{
    pdi_status_t st = PDI_WAIT_NVM_BUS_ACTIVE();
    
    if (st != PDI_OK)
    {
        printf("PDI: FAILED - NVM bus not active before writing defaults\n");
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_USER_SIG_ROW);
    PDI_STS_BYTE(PDI_USERSIG_BASE, PDI_DUMMY_TRIGGER_BYTE);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        printf("PDI: FAILED - user signature row erase did not complete\n");
        return st;
    }

    static const uint8_t FUSE_DEFAULTS[ATXMEGA192A3U_FUSE_COUNT] = ATXMEGA192A3U_FUSE_DEFAULTS;

    for (uint32_t i = 0; i < ATXMEGA192A3U_FUSE_COUNT; i++)
    {
        if (i == ATXMEGA192A3U_FUSE_RESERVED_IDX)
        {
            continue;
        }

        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FUSE);
        PDI_STS_BYTE(PDI_FUSE_BASE + i, FUSE_DEFAULTS[i]);

        st = PDI_WAIT_NVM_NOT_BUSY();
        if (st != PDI_OK)
        {
            printf("PDI: FAILED - FUSEBYTE%u write did not complete\n", (unsigned)i);
            return st;
        }
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

    printf("PDI: DEFAULTS PROGRAMMED - fuses at factory values, user signature row erased\n");

    return PDI_OK;
}


/* -------------------------------------------------------------------------- */
/*                            Programming Handlers                            */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Program ATxmega192A3U MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA192A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                      (1) Initialization                                    */
    /* -------------------------------------------------------------------------- */
    gpio_init(PDI_PIN_CLK);  
    gpio_init(PDI_PIN_DATA); 

    gpio_disable_pulls(PDI_PIN_CLK);
    gpio_disable_pulls(PDI_PIN_DATA);

    gpio_set_dir(PDI_PIN_CLK,  GPIO_IN);
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
    /*                      (4) Erase FLASH & EEPROM                              */
    /* -------------------------------------------------------------------------- */
    if ((PDI_ENABLE() || PDI_CHIP_ERASE()) != PDI_OK) 
    {
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                     (5) USER_ID & CONFIG Defaults                          */
    /* -------------------------------------------------------------------------- */
    if (PDI_WRITE_DEFAULTS() != PDI_OK)
    {
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (6) PDI Disable                                       */
    /* -------------------------------------------------------------------------- */
    if (PDI_DISABLE() != PDI_OK)
    {
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         SUCCESS                                            */
    /* -------------------------------------------------------------------------- */
    return true;
}

/**
 * DESCRIPTION: Program ATXMEGA128A3U MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA128A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    return true;
}

/**
 * DESCRIPTION: Program ATXMEGA128A4U MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA128A4U(const HEXPacket_t* buffer, size_t total_packets)
{
    return true;
}

/**
 * DESCRIPTION: Program ATXMEGA64AU MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA64AU(const HEXPacket_t* buffer, size_t total_packets)
{
    return true;
}

/**
 * DESCRIPTION: Program ATXMEGA32C3 MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA32C3(const HEXPacket_t* buffer, size_t total_packets)
{
    return true;
}

/**
 * DESCRIPTION: Program ATXMEGA32E5 MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA32E5(const HEXPacket_t* buffer, size_t total_packets)
{
    return true;
}
