/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "avr_pdi.pio.h"
#include "src/AVR_PDI.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define PDI_SM_CLKDIV                   30.0f

/* pio0/pio1 instruction memory and SM0 are owned by HCS08_BDM at boot, so PDI
 * takes the RP2350's third block; one SM carries both directions because both
 * directions drive PDI_CLK and the DATA pindir must never be contested. */
#define PDI_TX_CHUNK_BITS               32u
#define PDI_FRAME_BITS                  12u
#define PDI_BREAK_BITS                  12u

// ATxmega192A3U signature bytes
#define ATXMEGA192A3U_DEVID0            0x1Eu
#define ATXMEGA192A3U_DEVID1            0x97u
#define ATXMEGA192A3U_DEVID2            0x44u

// PDI instruction opcodes
#define PDI_CMD_LDS(ADDR_SZ, DATA_SZ)   (0x00u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_LD(PTR_MODE, DATA_SZ)   (0x20u | ((PTR_MODE) << 2) | (DATA_SZ))
#define PDI_CMD_STS(ADDR_SZ, DATA_SZ)   (0x40u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_ST(PTR_MODE, DATA_SZ)   (0x60u | ((PTR_MODE) << 2) | (DATA_SZ))
#define PDI_CMD_LDCS(CSREG)             (0x80u | (CSREG))
#define PDI_CMD_REPEAT(DATA_SZ)         (0xA0u | (DATA_SZ))
#define PDI_CMD_STCS(CSREG)             (0xC0u | (CSREG))
#define PDI_CMD_KEY                     (0xE0u)
#define PDI_SIZE_1BYTE                  0u
#define PDI_SIZE_2BYTES                 1u
#define PDI_SIZE_3BYTES                 2u
#define PDI_SIZE_4BYTES                 3u
#define PDI_PTR_INDIRECT                0u          /* *(ptr)                       */
#define PDI_PTR_INDIRECT_PI             1u          /* *(ptr++)                     */
#define PDI_PTR_DIRECT                  2u          /* ptr  (load pointer register) */

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
#define NVM_REG_ADDR0                   0x00u
#define NVM_REG_ADDR1                   0x01u
#define NVM_REG_ADDR2                   0x02u
#define NVM_REG_DATA0                   0x04u
#define NVM_REG_CMD                     0x0Au
#define NVM_REG_CTRLA                   0x0Bu
#define NVM_REG_CTRLB                   0x0Cu
#define NVM_REG_STATUS                  0x0Fu
#define NVM_REG_LOCKBITS                0x10u
#define NVM_CTRLA_CMDEX                 (1u << 0)   // CTRLA bit 0 = CMDEX, "execute loaded command" trigger
#define NVM_STATUS_NVMBUSY              (1u << 7)   // STATUS bit 7 = NVMBUSY, bit 6 = FBUSY 
#define MCU_DEVID0_ADDR                 (PDI_DATAMEM_BASE + 0x0090u)

// NVM command opcodes 
#define NVM_CMD_NOOP                    0x00u
#define NVM_CMD_CHIP_ERASE              0x40u  /* TRIGGER: CMDEX             */
#define NVM_CMD_READ_NVM                0x43u  /* TRIGGER: PDI read          */

// Flash (application + boot), page granular; buffer shared with user signature row.
#define NVM_CMD_ERASE_FLASH_BUFFER      0x26u  /* TRIGGER: CMDEX             */
#define NVM_CMD_LOAD_FLASH_BUFFER       0x23u  /* TRIGGER: PDI data          */
#define NVM_CMD_ERASE_FLASH_PAGE        0x2Bu  /* TRIGGER: PDI write         */
#define NVM_CMD_WRITE_FLASH_PAGE        0x2Eu  /* TRIGGER: PDI write         */
#define NVM_CMD_ERASE_WRITE_FLASH_PAGE  0x2Fu  /* TRIGGER: PDI write         */

// EEPROM, page granular, own separate page buffer.
#define NVM_CMD_ERASE_EEPROM_BUFFER     0x36u  /* TRIGGER: CMDEX             */
#define NVM_CMD_LOAD_EEPROM_BUFFER      0x33u  /* TRIGGER: PDI data          */
#define NVM_CMD_ERASE_EEPROM_PAGE       0x32u  /* TRIGGER: PDI write         */
#define NVM_CMD_WRITE_EEPROM_PAGE       0x34u  /* TRIGGER: PDI write         */
#define NVM_CMD_ERASE_WRITE_EEPROM_PAGE 0x35u  /* TRIGGER: PDI write         */

// User signature row: NOT touched by chip erase; uses the flash page buffer.
#define NVM_CMD_ERASE_USER_SIG_ROW      0x18u  /* TRIGGER: PDI write         */
#define NVM_CMD_WRITE_USER_SIG_ROW      0x1Au  /* TRIGGER: PDI write         */

// Fuses: no erase-fuse command on XMEGA; WRITE_FUSE overwrites a byte in place.
#define NVM_CMD_WRITE_FUSE              0x4Cu  /* TRIGGER: PDI data          */

// Dummy PDI-write value for page-commit/erase
#define PDI_DUMMY_TRIGGER_BYTE          0x55u

// flash: base 0x800000, size 0x32000 (192KB app + 8KB boot)
#define PDI_FLASH_BASE                  0x00800000u
#define ATXMEGA192A3U_FLASH_SIZE        0x32000u
#define ATXMEGA192A3U_FLASH_PAGE_SIZE   512u

// eeprom: base 0x8c0000, size 2048
#define PDI_EEPROM_BASE                 0x008C0000u
#define ATXMEGA192A3U_EEPROM_SIZE       2048u
#define ATXMEGA192A3U_EEPROM_PAGE_SIZE  32u

// usersig: base 0x8e0400, size/page 512 
#define PDI_USERSIG_BASE                0x008E0400u
#define ATXMEGA192A3U_USERSIG_SIZE      512u

// fuses: base 0x8f0020, 6 bytes (FUSEBYTE0..5); FUSEBYTE3 is reserved and never written
#define PDI_FUSE_BASE                   0x008F0020u
#define ATXMEGA192A3U_FUSE_COUNT        6u
#define ATXMEGA192A3U_FUSE_RESERVED_IDX 3u

// lockbits: base 0x8f0027 
#define PDI_LOCKBITS_ADDR               0x008F0027u

// Host-side (Intel HEX) address map
#define ATXMEGA192A3U_FLASH_BGN         0x000000u
#define ATXMEGA192A3U_FLASH_END         (ATXMEGA192A3U_FLASH_BGN + ATXMEGA192A3U_FLASH_SIZE - 1u)   /* 0x031FFF */
#define ATXMEGA192A3U_EEPROM_BGN        0x810000u
#define ATXMEGA192A3U_EEPROM_END        (ATXMEGA192A3U_EEPROM_BGN + ATXMEGA192A3U_EEPROM_SIZE - 1u) /* 0x8107FF */
#define ATXMEGA192A3U_CONFIG_BGN        0x820000u
#define ATXMEGA192A3U_CONFIG_END        (ATXMEGA192A3U_CONFIG_BGN + ATXMEGA192A3U_FUSE_COUNT - 1u)  /* 0x820005 */
#define ATXMEGA192A3U_LOCK_BGN          0x830000u
#define ATXMEGA192A3U_LOCK_END          0x830000u
#define ATXMEGA192A3U_SIGNATURE_BGN     0x840000u
#define ATXMEGA192A3U_SIGNATURE_END     0x840002u
#define ATXMEGA192A3U_USER_ID_BGN       0x850000u
#define ATXMEGA192A3U_USER_ID_END       (ATXMEGA192A3U_USER_ID_BGN + ATXMEGA192A3U_USERSIG_SIZE - 1u) /* 0x8501FF */

// Fuse verify masks
#define ATXMEGA192A3U_FUSE_MASKS        { 0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu }

// Fuse factory defaults (XMEGA AU manual 4.16); index 3 reserved and never written
#define ATXMEGA192A3U_FUSE_DEFAULTS     { 0xFFu, 0x00u, 0xFFu, 0x00u, 0xFEu, 0xFFu }

// Start-bit search window in PDI_CLK bits
#define PDI_RX_START_BIT_TIMEOUT_BITS   4096u

// NVM completion budgets in WALL-CLOCK ms
#define PDI_NVMEN_TIMEOUT_MS            4000u   /* chip erase of a full 192KB image measured > 1656ms */
#define PDI_NVM_BUSY_TIMEOUT_MS         2000u
#define PDI_RESET_RELEASE_ATTEMPTS      64u

// Clocked settle delay between a fuse write and its verify read, in PDI_CLK idle bits (~8.2ms)
#define PDI_FUSE_SETTLE_IDLE_BITS       2048u

#define XMEGA_EEPROM_PAGE_SIZE          32u   // 32 bytes on every supported part

// Host-side (Intel HEX) section bases
#define XMEGA_HEX_FLASH_BGN             0x000000u

#define PDI_RX_START_BIT_TIMEOUT_BITS   4096u   /* >> 128-bit max guard time */
#define PDI_NVM_BUSY_POLL_LIMIT         40000u  /* chip erase takes ~ms      */
#define PDI_RESET_RELEASE_ATTEMPTS      64u

/* -------------------------------------------------------------------------- */
/*                               Structures                                   */
/* -------------------------------------------------------------------------- */
typedef struct
{
    const char *NAME;              /* for log lines                         */
    uint8_t     DEVID[3];          /* MCU.DEVID0..2, [4] signature          */
    uint32_t    FLASH_SIZE;        /* bytes, application + boot             */
    uint32_t    FLASH_PAGE;        /* bytes                                 */
    uint32_t    EEPROM_SIZE;       /* bytes                                 */
    uint32_t    USERSIG_SIZE;      /* bytes; also the user sig page size    */
    uint8_t     FUSE_COUNT;        /* 6, or 7 on the E series               */
    uint8_t     FUSE_MASK[XMEGA_MAX_FUSES];  /* verify masks, see below     */
} xmega_chip_t;

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

static const PIO  PDI_PIO = pio2;
static const uint PDI_SM  = 0u;
static uint       PDI_OFFSET;
static bool       PDI_PIO_LOADED;

static const uint8_t PDI_NVM_PROG_KEY[8] =
{
    0xFFu, 0x88u, 0xD8u, 0xCDu, 0x45u, 0xABu, 0x89u, 0x12u
};

static const xmega_chip_t XMEGA_ATXMEGA192A3U = 
{
    "ATxmega192A3U", {0x1Eu, 0x97u, 0x44u}, 0x32000u, 512u, 2048u, 512u, 6u,
    {0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu, 0x00u}
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
        PDI_CLOCK_OUT_BITS(0x3u, 2u);
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
    PDI_DATA_IS_OUTPUT = false;

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

/**
 * DESCRIPTION: Loads a full 512-byte page into the flash write buffer and triggers an atomic erase+write
 * INPUT:       pdi_page_addr (uint32_t)       - PDI flash address of the page start (must be 512-byte aligned)
 *              page_data     (const uint8_t*) - 512-byte buffer to write
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_WRITE_FLASH_PAGE(uint32_t pdi_page_addr, const uint8_t *page_data)
{
    pdi_status_t st;

    bool has_data = false;
    for (uint32_t i = 0; i < ATXMEGA192A3U_FLASH_PAGE_SIZE; i++)
    {
        if (page_data[i] != 0xFFu) { has_data = true; break; }
    }
    if (!has_data) return PDI_OK;

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK) return st;

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_FLASH_BUFFER);

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(pdi_page_addr);

    uint32_t repeat_val = ATXMEGA192A3U_FLASH_PAGE_SIZE - 1u;
    PDI_TX_BYTE(PDI_CMD_REPEAT(PDI_SIZE_2BYTES));
    PDI_TX_BYTE((uint8_t)(repeat_val & 0xFFu));
    PDI_TX_BYTE((uint8_t)((repeat_val >> 8) & 0xFFu));

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
    for (uint32_t i = 0; i < ATXMEGA192A3U_FLASH_PAGE_SIZE; i++)
    {
        PDI_TX_BYTE(page_data[i]);
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_WRITE_FLASH_PAGE);
    PDI_STS_BYTE(pdi_page_addr, PDI_DUMMY_TRIGGER_BYTE);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK) return st;

    return PDI_OK;
}

/**
 * DESCRIPTION: Program and verify EEPROM pages supplied by the HEX packets.
 *              The existing erase stage has already initialized omitted bytes.
 */
static bool PDI_WRITE_EEPROM(const HEXPacket_t *buffer, size_t total_packets)
{
    static uint8_t eeprom[ATXMEGA192A3U_EEPROM_SIZE];
    bool page_present[ATXMEGA192A3U_EEPROM_SIZE / ATXMEGA192A3U_EEPROM_PAGE_SIZE] = {false};
    memset(eeprom, 0xFF, sizeof(eeprom));

    for (size_t pkt = 0; pkt < total_packets; pkt++)
    {
        uint32_t address = buffer[pkt].ADDRESS;
        if (address < ATXMEGA192A3U_EEPROM_BGN || address > ATXMEGA192A3U_EEPROM_END)
            continue;

        uint32_t offset = address - ATXMEGA192A3U_EEPROM_BGN;
        uint32_t count = ATXMEGA192A3U_EEPROM_SIZE - offset;
        if (count > HEX_PAYLOAD_SIZE_BYTES) count = HEX_PAYLOAD_SIZE_BYTES;
        memcpy(eeprom + offset, buffer[pkt].PAYLOAD, count);
        for (uint32_t i = 0; i < count; i++)
            page_present[(offset + i) / ATXMEGA192A3U_EEPROM_PAGE_SIZE] = true;
    }

    for (uint32_t page = 0; page < sizeof(page_present) / sizeof(page_present[0]); page++)
    {
        if (!page_present[page]) continue;
        uint32_t offset = page * ATXMEGA192A3U_EEPROM_PAGE_SIZE;
        uint32_t address = PDI_EEPROM_BASE + offset;

        if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
        PDI_TX_ADDR32(0u);
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_EEPROM_BUFFER);
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);
        if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;

        // Use the pointer/page-stream sequence from AVR1612 and the flash writer.
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_EEPROM_BUFFER);
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
        PDI_TX_ADDR32(address);
        PDI_TX_BYTE(PDI_CMD_REPEAT(PDI_SIZE_1BYTE));
        PDI_TX_BYTE((uint8_t)(ATXMEGA192A3U_EEPROM_PAGE_SIZE - 1u));
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
        for (uint32_t i = 0; i < ATXMEGA192A3U_EEPROM_PAGE_SIZE; i++)
            PDI_TX_BYTE(eeprom[offset + i]);

        // Erase+write also handles supplied all-FF pages if EESAVE preserved EEPROM.
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_WRITE_EEPROM_PAGE);
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
        PDI_TX_ADDR32(address);
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
        PDI_TX_BYTE(PDI_DUMMY_TRIGGER_BYTE);
        if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;

        uint8_t readback[ATXMEGA192A3U_EEPROM_PAGE_SIZE];
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);
        PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
        PDI_TX_ADDR32(address);
        for (uint32_t i = 0; i < ATXMEGA192A3U_EEPROM_PAGE_SIZE; i++)
        {
            // Request one byte at a time; retain the existing PIO RX framing.
            PDI_TX_BYTE(PDI_CMD_LD(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
            pdi_status_t st = PDI_RX_BYTE(&readback[i]);
            if (st != PDI_OK)
            {
                printf("PDI: EEPROM read failed at 0x%08X, status=%u\n",
                       (unsigned)(address + i), (unsigned)st);
                return false;
            }
        }
        for (uint32_t i = 0; i < ATXMEGA192A3U_EEPROM_PAGE_SIZE; i++)
        {
            if (readback[i] != eeprom[offset + i])
            {
                printf("PDI: EEPROM verify failed at 0x%08X: expected=0x%02X, actual=0x%02X\n",
                       (unsigned)(address + i), (unsigned)eeprom[offset + i], (unsigned)readback[i]);
                printf("PDI: EEPROM page 0x%08X expected:", (unsigned)address);
                for (uint32_t j = 0; j < ATXMEGA192A3U_EEPROM_PAGE_SIZE; j++)
                    printf(" %02X", (unsigned)eeprom[offset + j]);
                printf("\nPDI: EEPROM page 0x%08X actual:  ", (unsigned)address);
                for (uint32_t j = 0; j < ATXMEGA192A3U_EEPROM_PAGE_SIZE; j++)
                    printf(" %02X", (unsigned)readback[j]);
                printf("\n");
                return false;
            }
        }
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
    return true;
}

/**
 * DESCRIPTION: Program and verify the USER_ID user signature row.
 *              PDI_WRITE_DEFAULTS has already erased this row.
 */
static bool PDI_WRITE_USER_ID(const HEXPacket_t *buffer, size_t total_packets)
{
    static uint8_t user_id[ATXMEGA192A3U_USERSIG_SIZE];
    bool present = false;
    memset(user_id, 0xFF, sizeof(user_id));

    for (size_t pkt = 0; pkt < total_packets; pkt++)
    {
        uint32_t address = buffer[pkt].ADDRESS;
        if (address < ATXMEGA192A3U_USER_ID_BGN || address > ATXMEGA192A3U_USER_ID_END)
            continue;

        uint32_t offset = address - ATXMEGA192A3U_USER_ID_BGN;
        uint32_t count = ATXMEGA192A3U_USERSIG_SIZE - offset;
        if (count > HEX_PAYLOAD_SIZE_BYTES) count = HEX_PAYLOAD_SIZE_BYTES;
        memcpy(user_id + offset, buffer[pkt].PAYLOAD, count);
        present = true;
    }
    if (!present) return true;

    if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_FLASH_BUFFER);
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);
    if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_FLASH_BUFFER);
    // Flash buffer words must be loaded low byte first, then high byte.
    for (uint32_t i = 0; i < ATXMEGA192A3U_USERSIG_SIZE; i++)
        PDI_STS_BYTE(PDI_USERSIG_BASE + i, user_id[i]);

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_USER_SIG_ROW);
    PDI_STS_BYTE(PDI_USERSIG_BASE, PDI_DUMMY_TRIGGER_BYTE);
    if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);
    for (uint32_t i = 0; i < ATXMEGA192A3U_USERSIG_SIZE; i++)
    {
        uint8_t value;
        if (PDI_LDS_BYTE(PDI_USERSIG_BASE + i, &value) != PDI_OK) return false;
        if (value != user_id[i])
        {
            printf("PDI: USER_ID verify failed at 0x%08X\n", (unsigned)(PDI_USERSIG_BASE + i));
            return false;
        }
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
    return true;
}

/**
 * DESCRIPTION: Write the supplied configuration fuses after all memory writes.
 *              Clip the fixed-size HEX payload to FUSEBYTE0..5 and skip byte 3.
 */
static bool PDI_WRITE_CONFIG(const HEXPacket_t *buffer, size_t total_packets)
{
    uint8_t config[ATXMEGA192A3U_FUSE_COUNT] = {0};
    bool present[ATXMEGA192A3U_FUSE_COUNT] = {false};

    for (size_t pkt = 0; pkt < total_packets; pkt++)
    {
        uint32_t address = buffer[pkt].ADDRESS;
        if (address < ATXMEGA192A3U_CONFIG_BGN || address > ATXMEGA192A3U_CONFIG_END)
            continue;

        uint32_t offset = address - ATXMEGA192A3U_CONFIG_BGN;
        uint32_t count = ATXMEGA192A3U_FUSE_COUNT - offset;
        if (count > HEX_PAYLOAD_SIZE_BYTES) count = HEX_PAYLOAD_SIZE_BYTES;
        for (uint32_t i = 0; i < count; i++)
        {
            config[offset + i] = buffer[pkt].PAYLOAD[i];
            present[offset + i] = true;
        }
    }

    for (uint32_t i = 0; i < ATXMEGA192A3U_FUSE_COUNT; i++)
    {
        if (!present[i] || i == ATXMEGA192A3U_FUSE_RESERVED_IDX) continue;
        if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;
        PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FUSE);
        PDI_STS_BYTE(PDI_FUSE_BASE + i, config[i]);
        if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK) return false;
        PDI_CLOCK_IDLE_BITS(PDI_FUSE_SETTLE_IDLE_BITS);
    }

    // Some fuse bits read back correctly only after reset. The caller performs
    // the existing PDI_DISABLE/reset release after this final write stage.
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
    return true;
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
    PDI_PIO_INIT(PDI_SM_CLKDIV);

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
    /*                     (6) Write & Verify FLASH                               */
    /* -------------------------------------------------------------------------- */
    uint32_t flash_pages_written = 0u;

    static uint8_t flash_page_buf[ATXMEGA192A3U_FLASH_PAGE_SIZE];
    uint32_t current_page = 0xFFFFFFFFu;

    for (size_t pkt = 0; pkt < total_packets; pkt++)
    {
        uint32_t hex_addr = buffer[pkt].ADDRESS;

        if (hex_addr >= ATXMEGA192A3U_FLASH_END)
        {
            continue;
        }

        uint32_t page_num  = hex_addr / ATXMEGA192A3U_FLASH_PAGE_SIZE;
        uint32_t page_base_hex = page_num * ATXMEGA192A3U_FLASH_PAGE_SIZE;

        if (page_num != current_page)
        {
            if (current_page != 0xFFFFFFFFu)
            {
                uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA192A3U_FLASH_PAGE_SIZE;

                if (PDI_WRITE_FLASH_PAGE(pdi_addr, flash_page_buf) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - Flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }

                flash_pages_written++;
            }

            current_page = page_num;
            memset(flash_page_buf, 0xFF, sizeof(flash_page_buf));
        }

        uint32_t offset     = hex_addr - page_base_hex;
        uint32_t bytes_left = HEX_PAYLOAD_SIZE_BYTES;
        uint32_t src_off    = 0u;

        while (bytes_left > 0u)
        {
            uint32_t space    = ATXMEGA192A3U_FLASH_PAGE_SIZE - offset;
            uint32_t copy_len = (bytes_left < space) ? bytes_left : space;

            memcpy(&flash_page_buf[offset], buffer[pkt].PAYLOAD + src_off, copy_len);

            bytes_left -= copy_len;
            src_off    += copy_len;
            offset     += copy_len;

            if (bytes_left > 0u)
            {
                /* Payload crosses a page boundary - flush and start next page. */
                uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA192A3U_FLASH_PAGE_SIZE;
                if (PDI_WRITE_FLASH_PAGE(pdi_addr, flash_page_buf) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - Flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }

                flash_pages_written++;
                current_page++;
                memset(flash_page_buf, 0xFF, sizeof(flash_page_buf));
                offset = 0u;
            }
        }
    }

    if (current_page != 0xFFFFFFFFu)
    {
        uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA192A3U_FLASH_PAGE_SIZE;

        if (PDI_WRITE_FLASH_PAGE(pdi_addr, flash_page_buf) != PDI_OK)
        {
            PDI_DISABLE();
            printf("PDI: FAILED - Flash page write error at 0x%08X\n", (unsigned)pdi_addr);
            return false;
        }

        flash_pages_written++;
    }

    /* -------------------------------------------------------------------------- */
    /*                      (7) Write EEPROM                                      */
    /* -------------------------------------------------------------------------- */
    if (!PDI_WRITE_EEPROM(buffer, total_packets))
    {
        PDI_DISABLE();
        printf("PDI: FAILED - EEPROM programming\n");
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                      (8) Write USER ID                                     */
    /* -------------------------------------------------------------------------- */
    if (!PDI_WRITE_USER_ID(buffer, total_packets))
    {
        PDI_DISABLE();
        printf("PDI: FAILED - USER_ID programming\n");
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                      (9) Write CONFIG fuses                                */
    /* -------------------------------------------------------------------------- */
    if (!PDI_WRITE_CONFIG(buffer, total_packets))
    {
        PDI_DISABLE();
        printf("PDI: FAILED - CONFIG fuse programming\n");
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                     (10) PDI Disable                                       */
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
