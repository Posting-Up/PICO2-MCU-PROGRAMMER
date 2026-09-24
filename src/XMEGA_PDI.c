/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "XMEGA_PDI.h"
#include "XMEGA_PDI.pio.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define ATXMEGA64A4U_DEVID0              0x1Eu
#define ATXMEGA64A4U_DEVID1              0x96u
#define ATXMEGA64A4U_DEVID2              0x46u

#define ATXMEGA128A3U_DEVID0             0x1Eu
#define ATXMEGA128A3U_DEVID1             0x97u
#define ATXMEGA128A3U_DEVID2             0x42u

#define ATXMEGA128A4U_DEVID0             0x1Eu
#define ATXMEGA128A4U_DEVID1             0x97u
#define ATXMEGA128A4U_DEVID2             0x46u

#define ATXMEGA192A3U_DEVID0             0x1Eu
#define ATXMEGA192A3U_DEVID1             0x97u
#define ATXMEGA192A3U_DEVID2             0x44u

#define PDI_CMD_LDS(ADDR_SZ, DATA_SZ)    (0x00u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_LD(PTR_MODE, DATA_SZ)    (0x20u | ((PTR_MODE) << 2) | (DATA_SZ))
#define PDI_CMD_STS(ADDR_SZ, DATA_SZ)    (0x40u | ((ADDR_SZ) << 2) | (DATA_SZ))
#define PDI_CMD_ST(PTR_MODE, DATA_SZ)    (0x60u | ((PTR_MODE) << 2) | (DATA_SZ))
#define PDI_CMD_LDCS(CSREG)              (0x80u | (CSREG))
#define PDI_CMD_REPEAT(DATA_SZ)          (0xA0u | (DATA_SZ))
#define PDI_CMD_STCS(CSREG)              (0xC0u | (CSREG))
#define PDI_CMD_KEY                      (0xE0u)
#define PDI_SIZE_1BYTE                   0u
#define PDI_SIZE_2BYTES                  1u
#define PDI_SIZE_3BYTES                  2u
#define PDI_SIZE_4BYTES                  3u
#define PDI_PTR_INDIRECT                 0u
#define PDI_PTR_INDIRECT_PI              1u
#define PDI_PTR_DIRECT                   2u

#define NVM_CMD_NOOP                     0x00u
#define NVM_CMD_CHIP_ERASE               0x40u
#define NVM_CMD_READ_NVM                 0x43u

#define PDI_CSR_STATUS                   0u
#define PDI_CSR_RESET                    1u
#define PDI_CSR_CTRL                     2u
#define PDI_STATUS_NVMEN                 (1u << 1)
#define PDI_RESET_SIGNATURE              0x59u
#define PDI_CTRL_GUARDTIME_32            0x02u

#define PDI_DATAMEM_BASE                 0x01000000u
#define NVM_BASE                         (PDI_DATAMEM_BASE + 0x01C0u)
#define NVM_REG_ADDR0                    0x00u
#define NVM_REG_ADDR1                    0x01u
#define NVM_REG_ADDR2                    0x02u
#define NVM_REG_DATA0                    0x04u
#define NVM_REG_CMD                      0x0Au
#define NVM_REG_CTRLA                    0x0Bu
#define NVM_REG_CTRLB                    0x0Cu
#define NVM_REG_STATUS                   0x0Fu
#define NVM_REG_LOCKBITS                 0x10u
#define NVM_CTRLA_CMDEX                  (1u << 0)
#define NVM_STATUS_EELOAD                (1u << 1)
#define NVM_STATUS_NVMBUSY               (1u << 7)
#define MCU_DEVID0_ADDR                  (PDI_DATAMEM_BASE + 0x0090u)
#define NVM_CMD_ERASE_FLASH_BUFFER       0x26u
#define NVM_CMD_LOAD_FLASH_BUFFER        0x23u
#define NVM_CMD_ERASE_FLASH_PAGE         0x2Bu
#define NVM_CMD_WRITE_FLASH_PAGE         0x2Eu
#define NVM_CMD_ERASE_WRITE_FLASH_PAGE   0x2Fu
#define NVM_CMD_ERASE_EEPROM_BUFFER      0x36u
#define NVM_CMD_LOAD_EEPROM_BUFFER       0x33u
#define NVM_CMD_ERASE_EEPROM_PAGE        0x32u
#define NVM_CMD_WRITE_EEPROM_PAGE        0x34u
#define NVM_CMD_ERASE_WRITE_EEPROM_PAGE  0x35u
#define NVM_CMD_ERASE_USER_SIG_ROW       0x18u
#define NVM_CMD_WRITE_USER_SIG_ROW       0x1Au
#define NVM_CMD_WRITE_FUSE               0x4Cu
#define NVM_CMD_WRITE_LOCK_BITS          0x08u
#define PDI_DUMMY_TRIGGER_BYTE           0x55u
#define PDI_FLASH_BASE                   0x00800000u
#define ATXMEGA_AU_FLASH_SIZE            0x32000u
#define ATXMEGA_AU_FLASH_PAGE_SIZE       512u
#define PDI_EEPROM_BASE                  0x008C0000u
#define ATXMEGA_AU_EEPROM_SIZE           2048u
#define ATXMEGA_AU_EEPROM_PAGE_SIZE      32u
#define PDI_USERSIG_BASE                 0x008E0400u
#define ATXMEGA_AU_USERSIG_SIZE          512u
#define PDI_FUSE_BASE                    0x008F0020u
#define ATXMEGA_AU_FUSE_COUNT            6u
#define ATXMEGA_AU_FUSE_RESERVED_IDX     3u
#define ATXMEGA_AU_FUSE_IS_RESERVED(IDX) ((IDX) == ATXMEGA_AU_FUSE_RESERVED_IDX)
#define PDI_LOCKBITS_ADDR                0x008F0027u

#define ATXMEGA_AU_FLASH_BGN             0x000000u
#define ATXMEGA_AU_FLASH_END             (ATXMEGA_AU_FLASH_BGN + ATXMEGA_AU_FLASH_SIZE - 1u)
#define ATXMEGA_AU_EEPROM_BGN            0x810000u
#define ATXMEGA_AU_EEPROM_END            (ATXMEGA_AU_EEPROM_BGN + ATXMEGA_AU_EEPROM_SIZE - 1u)
#define ATXMEGA_AU_CONFIG_BGN            0x820000u
#define ATXMEGA_AU_CONFIG_END            (ATXMEGA_AU_CONFIG_BGN + ATXMEGA_AU_FUSE_COUNT - 1u)
#define ATXMEGA_AU_LOCK_BGN              0x830000u
#define ATXMEGA_AU_LOCK_END              0x830000u
#define ATXMEGA_AU_SIGNATURE_BGN         0x840000u
#define ATXMEGA_AU_SIGNATURE_END         0x840002u
#define ATXMEGA_AU_USER_ID_BGN           0x850000u
#define ATXMEGA_AU_USER_ID_END           (ATXMEGA_AU_USER_ID_BGN + ATXMEGA_AU_USERSIG_SIZE - 1u)

#define ATXMEGA_AU_FUSE_MASKS            { 0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu }
#define ATXMEGA192A3U_FUSE_DEFAULTS      { 0xFFu, 0x00u, 0xFFu, 0x00u, 0xFEu, 0xFFu }

#define ATXMEGA32E5_DEVID0               0x1Eu
#define ATXMEGA32E5_DEVID1               0x95u
#define ATXMEGA32E5_DEVID2               0x4Cu

#define ATXMEGA_E_FLASH_SIZE             0x9000u
#define ATXMEGA_E_FLASH_PAGE_SIZE        128u
#define ATXMEGA_E_EEPROM_SIZE            1024u
#define ATXMEGA_E_EEPROM_PAGE_SIZE       32u
#define ATXMEGA_E_USERSIG_SIZE           128u
#define ATXMEGA_E_FUSE_COUNT             7u
#define ATXMEGA_E_FUSE_UNUSED_IDX        0u
#define ATXMEGA_E_FUSE_RESERVED_IDX      3u
#define ATXMEGA_E_FUSE_IS_RESERVED(IDX)  (((IDX) == ATXMEGA_E_FUSE_UNUSED_IDX) || \
                                          ((IDX) == ATXMEGA_E_FUSE_RESERVED_IDX))

#define ATXMEGA_E_FLASH_BGN              0x000000u
#define ATXMEGA_E_FLASH_END              (ATXMEGA_E_FLASH_BGN + ATXMEGA_E_FLASH_SIZE - 1u)
#define ATXMEGA_E_EEPROM_BGN             0x810000u
#define ATXMEGA_E_EEPROM_END             (ATXMEGA_E_EEPROM_BGN + ATXMEGA_E_EEPROM_SIZE - 1u)
#define ATXMEGA_E_CONFIG_BGN             0x820000u
#define ATXMEGA_E_CONFIG_END             (ATXMEGA_E_CONFIG_BGN + ATXMEGA_E_FUSE_COUNT - 1u)
#define ATXMEGA_E_LOCK_BGN               0x830000u
#define ATXMEGA_E_LOCK_END               0x830000u
#define ATXMEGA_E_SIGNATURE_BGN          0x840000u
#define ATXMEGA_E_SIGNATURE_END          0x840002u
#define ATXMEGA_E_USER_ID_BGN            0x850000u
#define ATXMEGA_E_USER_ID_END            (ATXMEGA_E_USER_ID_BGN + ATXMEGA_E_USERSIG_SIZE - 1u)

#define ATXMEGA_E_FUSE_MASKS             { 0x00u, 0xFFu, 0x43u, 0x00u, 0x1Eu, 0x3Fu, 0xFFu }
#define ATXMEGA32E5_FUSE_DEFAULTS        { 0x00u, 0x00u, 0xFFu, 0x00u, 0xFFu, 0xFFu, 0xFFu }

#define PDI_CLK_HZ                       2500000u
#define PDI_TX_CHUNK_BITS                32u
#define PDI_FRAME_BITS                   12u
#define PDI_BREAK_BITS                   12u
#define PDI_RX_START_BIT_TIMEOUT_BITS    4096u
#define PDI_NVMEN_TIMEOUT_MS             4000u
#define PDI_NVM_BUSY_TIMEOUT_MS          2000u
#define PDI_RESET_RELEASE_ATTEMPTS       64u
#define PDI_FUSE_SETTLE_IDLE_BITS        2048u

#define XMEGA_HEX_FLASH_BGN              0x000000u
#define XMEGA_MAX_PAGE_SIZE              512u
#define XMEGA_MAX_FUSES                  7u
#define XMEGA_FUSE_RESERVED_IDX          3u
#define PDI_NVM_BUSY_POLL_LIMIT          40000u
#define PDI_FUSE_VERIFY_SKIP_IDX         4u

static bool PDI_DATA_IS_OUTPUT;

static const PIO  PDI_PIO = pio2;
static const uint PDI_SM  = 1u;
static uint       PDI_OFFSET;
static bool       PDI_PIO_LOADED;

static const uint8_t PDI_NVM_PROG_KEY[8] = {
    0xFFu, 0x88u, 0xD8u, 0xCDu, 0x45u, 0xABu, 0x89u, 0x12u
};


/* -------------------------------------------------------------------------- */
/*                               Structures                                   */
/* -------------------------------------------------------------------------- */
typedef struct
{
    const char *NAME;
    uint8_t     DEVID[3];
    uint32_t    FLASH_SIZE;
    uint32_t    FLASH_PAGE;
    uint32_t    EEPROM_SIZE;
    uint32_t    USERSIG_SIZE;
    uint8_t     FUSE_COUNT;
    uint8_t     FUSE_MASK[XMEGA_MAX_FUSES];
} xmega_chip_t;

typedef struct
{
    const char *NAME;
    uint8_t     DEVID[3];
} xmega_devid_t;

typedef enum
{
    PDI_OK = 0,
    PDI_ERR_RX_TIMEOUT,
    PDI_ERR_PARITY,
    PDI_ERR_FRAME,
    PDI_ERR_NVMEN_TIMEOUT,
    PDI_ERR_NVM_BUSY,
    PDI_ERR_RESET_RELEASE,
    PDI_ERR_ID_MISMATCH,
    PDI_ERR_VERIFY
} pdi_status_t;

static const xmega_devid_t XMEGA_KNOWN_DEVICES[] = {
    { "ATxmega32E5", { ATXMEGA32E5_DEVID0, ATXMEGA32E5_DEVID1, ATXMEGA32E5_DEVID2 } },
    { "ATxmega64A4U", { ATXMEGA64A4U_DEVID0, ATXMEGA64A4U_DEVID1, ATXMEGA64A4U_DEVID2 } },
    { "ATxmega128A3U", { ATXMEGA128A3U_DEVID0, ATXMEGA128A3U_DEVID1, ATXMEGA128A3U_DEVID2 } },
    { "ATxmega128A4U", { ATXMEGA128A4U_DEVID0, ATXMEGA128A4U_DEVID1, ATXMEGA128A4U_DEVID2 } },
    { "ATxmega192A3U", { ATXMEGA192A3U_DEVID0, ATXMEGA192A3U_DEVID1, ATXMEGA192A3U_DEVID2 } }
};

#define XMEGA_KNOWN_DEVICE_COUNT (sizeof(XMEGA_KNOWN_DEVICES) / sizeof(XMEGA_KNOWN_DEVICES[0]))


/* -------------------------------------------------------------------------- */
/*                              Handlers                                      */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Loads the PDI program into pio2 once, then configures the state machine and hands it the
 *              PDI_CLK/PDI_DATA pins (PDI_CLK driven low, PDI_DATA released)
 * INPUT:       ---
 * RETURN:      ---
 */
static void PDI_PIO_INIT(void)
{
    if (!PDI_PIO_LOADED)
    {
        pio_sm_claim(PDI_PIO, PDI_SM);
        PDI_OFFSET     = pio_add_program(PDI_PIO, &XMEGA_PDI_program);
        PDI_PIO_LOADED = true;
    }

    XMEGA_PDI_program_init(PDI_PIO, PDI_SM, PDI_OFFSET, PDI_PIN_CLK, PDI_PIN_DATA, PDI_CLK_HZ);
    XMEGA_PDI_claim_pins(PDI_PIO, PDI_SM, PDI_PIN_CLK, PDI_PIN_DATA);

    PDI_DATA_IS_OUTPUT = false;
}

/**
 * DESCRIPTION: Clocks out a pattern of up to 32 bits, least significant bit first, with the data line driven
 * INPUT:       pattern (uint32_t) - Bit pattern to shift out
 *              bits    (uint32_t) - Number of bits to clock, 1 to 32
 * RETURN:      ---
 */
static void PDI_CLOCK_OUT_BITS(uint32_t pattern, uint32_t bits)
{
    XMEGA_PDI_tx_bits(PDI_PIO, PDI_SM, pattern, bits);
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
 *              (the RX job itself releases PDI_DATA on its first instruction)
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
 * DESCRIPTION: Checks one raw RX word from the state machine (start-bit timeout, even parity, both stop bits)
 *              and extracts the data byte
 * INPUT:       word (uint32_t) - Raw word from XMEGA_PDI_rx_word()
 *              out  (uint8_t*) - Destination for the data byte
 * RETURN:      pdi_status_t    - PDI_OK, PDI_ERR_RX_TIMEOUT, PDI_ERR_PARITY or PDI_ERR_FRAME
 */
static pdi_status_t PDI_DECODE_FRAME(uint32_t word, uint8_t *out)
{
    if (word == XMEGA_PDI_RX_TIMEOUT)
    {
        return PDI_ERR_RX_TIMEOUT;
    }

    uint32_t frame     = (word >> XMEGA_PDI_RX_FRAME_SHIFT) & XMEGA_PDI_RX_FRAME_MASK;
    uint8_t  value     = (uint8_t)(frame & 0xFFu);
    bool     parity    = (__builtin_popcount(value) & 1) != 0;
    bool     parity_rx = ((frame >> 8) & 1u) != 0u;
    bool     stop1     = ((frame >> 9) & 1u) != 0u;
    bool     stop2     = ((frame >> 10) & 1u) != 0u;

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
 * DESCRIPTION: Receives a byte over PDI by handling start bit synchronization, data parsing, parity checking, and framing verification
 * INPUT:       out (uint8_t*) - Pointer to store the successfully received byte
 * RETURN:      pdi_status_t   - Execution status (PDI_OK, PDI_ERR_RX_TIMEOUT, PDI_ERR_PARITY, or PDI_ERR_FRAME)
 */
static pdi_status_t PDI_RX_BYTE(uint8_t *out)
{
    PDI_ENTER_RX();

    return PDI_DECODE_FRAME(XMEGA_PDI_rx_frame(PDI_PIO, PDI_SM, PDI_RX_START_BIT_TIMEOUT_BITS), out);
}

/**
 * DESCRIPTION: Transmits a single byte over PDI by framing it with a start bit, 8 data bits, an even parity bit, and 2 stop bits
 * INPUT:       byte (uint8_t) - The data byte to transmit
 * RETURN:      ---
 */
static void PDI_TX_BYTE(uint8_t byte)
{
    PDI_ENTER_TX();

    uint32_t parity = (uint32_t)(__builtin_popcount(byte) & 1);
    uint32_t frame  = ((uint32_t)byte << 1)         /* start bit 0 at bit 0 */
                      | (parity << 9) | (3u << 10); /* stop bits 1 and 2    */

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
        uint8_t      status = 0;
        pdi_status_t st     = PDI_LDCS(PDI_CSR_STATUS, &status);

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
    XMEGA_PDI_hold_lines(PDI_PIO, PDI_SM, 0, XMEGA_PDI_DATA_LOW);
    PDI_DATA_IS_OUTPUT = true;
    sleep_ms(1);

    XMEGA_PDI_hold_lines(PDI_PIO, PDI_SM, 0, XMEGA_PDI_DATA_HIGH);
    busy_wait_us_32(10);

    XMEGA_PDI_run(PDI_PIO, PDI_SM, PDI_OFFSET);
    PDI_CLOCK_IDLE_BITS(32);
    PDI_RESYNC();

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
 * DESCRIPTION: Confirms NVM access is still granted and only re-runs the full PDI enable sequence when the
 *              status register says the link lost it, avoiding a redundant key exchange between stages
 * INPUT:       ---
 * RETURN:      pdi_status_t - PDI_OK if programming mode is available, or an error status code on failure
 */
static pdi_status_t PDI_ENSURE_NVM_ACTIVE(void)
{
    uint8_t status = 0;

    if (PDI_LDCS(PDI_CSR_STATUS, &status) == PDI_OK && (status & PDI_STATUS_NVMEN))
    {
        return PDI_OK;
    }

    return PDI_ENABLE();
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

    XMEGA_PDI_hold_lines(PDI_PIO, PDI_SM, 1, XMEGA_PDI_DATA_LOW);
    PDI_DATA_IS_OUTPUT = true;
    sleep_ms(2);

    XMEGA_PDI_release_pins(PDI_PIO, PDI_SM, PDI_PIN_CLK, PDI_PIN_DATA);
    PDI_DATA_IS_OUTPUT = false;

    printf("PDI: interface disabled, lines released (target running)\n");

    return released ? PDI_OK : PDI_ERR_RESET_RELEASE;
}

/**
 * DESCRIPTION: Reads the 3-byte signature/device ID from the target MCU and matches it against every XMEGA part
 *              this driver supports. The AU and E series share the device ID register layout, only the signature
 *              value itself differs per part, so a single read serves both families
 * INPUT:       id   (uint8_t[3])    - Destination array to store the three signature bytes
 *              name (const char**)  - Destination for the matched part name, or NULL if the caller does not need it
 * RETURN:      pdi_status_t         - PDI_OK if reading succeeds and the signature matches a known part, or an error code on failure
 */
static pdi_status_t PDI_READ_DEVICE_ID(uint8_t id[3], const char **name)
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

    for (uint32_t i = 0; i < XMEGA_KNOWN_DEVICE_COUNT; i++)
    {
        if (id[0] == XMEGA_KNOWN_DEVICES[i].DEVID[0] &&
            id[1] == XMEGA_KNOWN_DEVICES[i].DEVID[1] &&
            id[2] == XMEGA_KNOWN_DEVICES[i].DEVID[2])
        {
            if (name != NULL)
            {
                *name = XMEGA_KNOWN_DEVICES[i].NAME;
            }
            return PDI_OK;
        }
    }

    printf("PDI: FAILED - unknown device ID = 0x%02X 0x%02X 0x%02X\n",
           (unsigned)id[0], (unsigned)id[1], (unsigned)id[2]);

    return PDI_ERR_ID_MISMATCH;
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
    return PDI_OK;
}

/**
 * DESCRIPTION: Reports how many configuration fuse bytes the selected chip family exposes
 * INPUT:       family (uint8_t) - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 * RETURN:      uint32_t         - Number of FUSEBYTE slots, reserved entries included
 */
static uint32_t PDI_FUSE_COUNT(uint8_t family)
{
    return (family == FAMILY_ATXMEGA_E) ? ATXMEGA_E_FUSE_COUNT : ATXMEGA_AU_FUSE_COUNT;
}

/**
 * DESCRIPTION: Reports whether a FUSEBYTE index must be skipped on the selected chip family. The AU series only
 *              reserves FUSEBYTE3, while on the E series FUSEBYTE0 does not exist either
 * INPUT:       family (uint8_t)  - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              index  (uint32_t) - FUSEBYTE index relative to the fuse base address
 * RETURN:      bool              - true if the fuse byte must not be written, false otherwise
 */
static bool PDI_FUSE_IS_RESERVED(uint8_t family, uint32_t index)
{
    if (family == FAMILY_ATXMEGA_E)
    {
        return ATXMEGA_E_FUSE_IS_RESERVED(index);
    }

    return ATXMEGA_AU_FUSE_IS_RESERVED(index);
}

/**
 * DESCRIPTION: Reports the implemented-bit mask for a FUSEBYTE index on the selected chip family, so verification
 *              only compares bits the datasheet actually defines for that byte (reserved bits are always written
 *              as one but are not guaranteed to read back as anything meaningful)
 * INPUT:       family (uint8_t)  - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              index  (uint32_t) - FUSEBYTE index relative to the fuse base address
 * RETURN:      uint8_t           - Bitmask of the fuse bits implemented at that index
 */
static uint8_t PDI_FUSE_MASK(uint8_t family, uint32_t index)
{
    static const uint8_t AU_MASKS[ATXMEGA_AU_FUSE_COUNT] = ATXMEGA_AU_FUSE_MASKS;
    static const uint8_t E_MASKS[ATXMEGA_E_FUSE_COUNT]   = ATXMEGA_E_FUSE_MASKS;

    if (family == FAMILY_ATXMEGA_E)
    {
        return E_MASKS[index];
    }

    return AU_MASKS[index];
}

/**
 * DESCRIPTION: Erases the user signature row and restores all non-reserved device fuses to their default factory
 *              values, using the fuse table and reserved-index rules of the selected chip family
 * INPUT:       family (uint8_t) - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 * RETURN:      pdi_status_t     - PDI_OK if defaults are programmed successfully, or an error code on failure
 */
static pdi_status_t PDI_WRITE_DEFAULTS(uint8_t family)
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

    static const uint8_t AU_FUSE_DEFAULTS[ATXMEGA_AU_FUSE_COUNT] = ATXMEGA192A3U_FUSE_DEFAULTS;
    static const uint8_t E_FUSE_DEFAULTS[ATXMEGA_E_FUSE_COUNT]   = ATXMEGA32E5_FUSE_DEFAULTS;

    const uint8_t *FUSE_DEFAULTS = (family == FAMILY_ATXMEGA_E) ? E_FUSE_DEFAULTS : AU_FUSE_DEFAULTS;
    uint32_t       fuse_count    = PDI_FUSE_COUNT(family);

    for (uint32_t i = 0; i < fuse_count; i++)
    {
        if (PDI_FUSE_IS_RESERVED(family, i))
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

    return PDI_OK;
}


/* -------------------------------------------------------------------------- */
/*                            Programming Handlers                            */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Reports whether a page buffer is still fully erased and therefore has nothing to program
 * INPUT:       page_data (const uint8_t*) - Buffer to inspect
 *              length    (uint32_t)       - Number of bytes to inspect
 * RETURN:      bool                       - true if every byte is 0xFF, false otherwise
 */
static bool PDI_PAGE_IS_BLANK(const uint8_t *page_data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        if (page_data[i] != 0xFFu)
        {
            return false;
        }
    }

    return true;
}

/**
 * DESCRIPTION: Clears the flash page buffer once before the flash stage, so the first page load starts from a
 *              known-erased buffer (every later page write auto-erases it again)
 * INPUT:       ---
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_ERASE_FLASH_BUFFER(void)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_FLASH_BUFFER);
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);

    return PDI_WAIT_NVM_NOT_BUSY();
}

/**
 * DESCRIPTION: Loads a full flash page into the flash write buffer and triggers a page write. The chip erase at the
 *              start of every session has already erased all flash, so the write skips the per-page erase that an
 *              atomic erase+write would repeat. The page size follows the selected chip family (512 bytes on AU,
 *              128 bytes on E)
 * INPUT:       family        (uint8_t)        - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              pdi_page_addr (uint32_t)       - PDI flash address of the page start (must be page aligned)
 *              page_data     (const uint8_t*) - Page sized buffer to write
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_WRITE_FLASH_PAGE(uint8_t family, uint32_t pdi_page_addr, const uint8_t *page_data)
{
    pdi_status_t st;

    uint32_t page_size = (family == FAMILY_ATXMEGA_E) ? ATXMEGA_E_FLASH_PAGE_SIZE : ATXMEGA_AU_FLASH_PAGE_SIZE;

    if (PDI_PAGE_IS_BLANK(page_data, page_size))
    {
        return PDI_OK;
    }

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_FLASH_BUFFER);

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(pdi_page_addr);

    uint32_t repeat_val = page_size - 1u;
    PDI_TX_BYTE(PDI_CMD_REPEAT(PDI_SIZE_2BYTES));
    PDI_TX_BYTE((uint8_t)(repeat_val & 0xFFu));
    PDI_TX_BYTE((uint8_t)((repeat_val >> 8) & 0xFFu));

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
    for (uint32_t i = 0; i < page_size; i++)
    {
        PDI_TX_BYTE(page_data[i]);
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FLASH_PAGE);
    PDI_STS_BYTE(pdi_page_addr, PDI_DUMMY_TRIGGER_BYTE);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    return PDI_OK;
}

/**
 * DESCRIPTION: Reads a page back over PDI as a single REPEAT burst and compares it against the expected page
 *              buffer, costing one bus turnaround for the whole page instead of one per byte
 * INPUT:       pdi_page_addr (uint32_t)       - PDI address of the page start
 *              page_data     (const uint8_t*) - Buffer holding the expected contents
 *              length        (uint32_t)       - Number of bytes to read back and compare
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_VERIFY_PAGE(uint32_t pdi_page_addr, const uint8_t *page_data, uint32_t length)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(pdi_page_addr);

    uint32_t repeat_val = length - 1u;
    PDI_TX_BYTE(PDI_CMD_REPEAT(PDI_SIZE_2BYTES));
    PDI_TX_BYTE((uint8_t)(repeat_val & 0xFFu));
    PDI_TX_BYTE((uint8_t)((repeat_val >> 8) & 0xFFu));

    PDI_TX_BYTE(PDI_CMD_LD(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));

    // One RX job for the whole burst: the target sends the frames back to back, so every
    // frame must be collected (the state machine holds PDI_CLK low while its RX FIFO is
    // full). Errors are only recorded here and reported once the burst has drained
    PDI_ENTER_RX();
    XMEGA_PDI_rx_start(PDI_PIO, PDI_SM, PDI_RX_START_BIT_TIMEOUT_BITS, length);

    pdi_status_t first_st   = PDI_OK;
    uint32_t     first_idx  = 0u;
    uint8_t      first_read = 0u;

    for (uint32_t i = 0; i < length; i++)
    {
        uint8_t      value = 0;
        uint32_t     word  = XMEGA_PDI_rx_word(PDI_PIO, PDI_SM);
        pdi_status_t rx    = PDI_DECODE_FRAME(word, &value);

        if (rx == PDI_OK && value != page_data[i])
        {
            rx = PDI_ERR_VERIFY;
        }
        if (rx != PDI_OK && first_st == PDI_OK)
        {
            first_st   = rx;
            first_idx  = i;
            first_read = value;
        }
        if (rx == PDI_ERR_RX_TIMEOUT)
        {
            break; // no start bit: the state machine has already ended the job
        }
    }

    if (first_st == PDI_ERR_VERIFY)
    {
        printf("PDI: FAILED - verify at 0x%08X: expected=0x%02X, actual=0x%02X\n",
               (unsigned)(pdi_page_addr + first_idx), (unsigned)page_data[first_idx], (unsigned)first_read);
        PDI_RESYNC();
        return PDI_ERR_VERIFY;
    }
    if (first_st != PDI_OK)
    {
        printf("PDI: FAILED - readback at 0x%08X (status=%u)\n",
               (unsigned)(pdi_page_addr + first_idx), (unsigned)first_st);
        PDI_RESYNC();
        return first_st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

    return PDI_OK;
}

/**
 * DESCRIPTION: Loads a full EEPROM page into the EEPROM write buffer and triggers an atomic erase+write. The page
 *              size follows the selected chip family
 * INPUT:       family        (uint8_t)        - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              pdi_page_addr (uint32_t)       - PDI EEPROM address of the page start (must be page aligned)
 *              page_data     (const uint8_t*) - Page sized buffer to write
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_WRITE_EEPROM_PAGE(uint8_t family, uint32_t pdi_page_addr, const uint8_t *page_data)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    uint32_t page_size = (family == FAMILY_ATXMEGA_E) ? ATXMEGA_E_EEPROM_PAGE_SIZE : ATXMEGA_AU_EEPROM_PAGE_SIZE;

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(0u);

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_EEPROM_BUFFER);
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_EEPROM_BUFFER);

    uint8_t command = 0;
    st              = PDI_LDS_BYTE(NVM_BASE + NVM_REG_CMD, &command);
    if (st != PDI_OK)
    {
        return st;
    }

    if (command != NVM_CMD_LOAD_EEPROM_BUFFER)
    {
        printf("PDI: FAILED - EEPROM command check (expected=0x%02X, actual=0x%02X)\n",
               (unsigned)NVM_CMD_LOAD_EEPROM_BUFFER, (unsigned)command);
        return PDI_ERR_VERIFY;
    }

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(pdi_page_addr);

    uint32_t repeat_val = page_size - 1u;
    PDI_TX_BYTE(PDI_CMD_REPEAT(PDI_SIZE_1BYTE));
    PDI_TX_BYTE((uint8_t)(repeat_val & 0xFFu));

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
    for (uint32_t i = 0; i < page_size; i++)
    {
        PDI_TX_BYTE(page_data[i]);
    }

    uint8_t nvm_status = 0;
    st                 = PDI_LDS_BYTE(NVM_BASE + NVM_REG_STATUS, &nvm_status);
    if (st != PDI_OK)
    {
        return st;
    }

    if (!(nvm_status & NVM_STATUS_EELOAD))
    {
        printf("PDI: FAILED - EEPROM buffer load at 0x%08X (NVM_STATUS=0x%02X)\n",
               (unsigned)pdi_page_addr, (unsigned)nvm_status);
        return PDI_ERR_VERIFY;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_WRITE_EEPROM_PAGE);

    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_DIRECT, PDI_SIZE_4BYTES));
    PDI_TX_ADDR32(pdi_page_addr);
    PDI_TX_BYTE(PDI_CMD_ST(PDI_PTR_INDIRECT_PI, PDI_SIZE_1BYTE));
    PDI_TX_BYTE(PDI_DUMMY_TRIGGER_BYTE);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    return PDI_OK;
}

/**
 * DESCRIPTION: Loads the full user signature row into the flash write buffer and commits it, relying on
 *              PDI_WRITE_DEFAULTS having already erased the row. The row size follows the selected chip
 *              family (512 bytes on AU, one 128-byte flash page on E)
 * INPUT:       family   (uint8_t)        - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              row_data (const uint8_t*) - Row sized buffer to write
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_WRITE_USERSIG_ROW(uint8_t family, const uint8_t *row_data)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    uint32_t row_size = (family == FAMILY_ATXMEGA_E) ? ATXMEGA_E_USERSIG_SIZE : ATXMEGA_AU_USERSIG_SIZE;

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_ERASE_FLASH_BUFFER);
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CTRLA, NVM_CTRLA_CMDEX);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_LOAD_FLASH_BUFFER);
    for (uint32_t i = 0; i < row_size; i++)
    {
        PDI_STS_BYTE(PDI_USERSIG_BASE + i, row_data[i]);
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_USER_SIG_ROW);
    PDI_STS_BYTE(PDI_USERSIG_BASE, PDI_DUMMY_TRIGGER_BYTE);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    return PDI_OK;
}

/**
 * DESCRIPTION: Writes a single configuration fuse byte and clocks the idle bits the fuse needs to settle
 * INPUT:       index (uint32_t) - FUSEBYTE index relative to the fuse base address
 *              value (uint8_t)  - The fuse value to program
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_WRITE_FUSE_BYTE(uint32_t index, uint8_t value)
{
    pdi_status_t st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FUSE);
    PDI_STS_BYTE(PDI_FUSE_BASE + index, value);

    st = PDI_WAIT_NVM_NOT_BUSY();
    if (st != PDI_OK)
    {
        return st;
    }

    PDI_CLOCK_IDLE_BITS(PDI_FUSE_SETTLE_IDLE_BITS);

    return PDI_OK;
}

/**
 * DESCRIPTION: Reads back the configuration fuse bytes of the selected chip family over PDI so they can be
 *              printed/verified
 * INPUT:       family (uint8_t)  - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              fuses  (uint8_t*) - Destination array to store the fuse bytes, sized for the family's fuse count
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_READ_FUSES(uint8_t family, uint8_t *fuses)
{
    uint32_t fuse_count = PDI_FUSE_COUNT(family);

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);

    for (uint32_t i = 0; i < fuse_count; i++)
    {
        pdi_status_t st = PDI_LDS_BYTE(PDI_FUSE_BASE + i, &fuses[i]);
        if (st != PDI_OK)
        {
            return st;
        }
    }

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

    return PDI_OK;
}

/**
 * DESCRIPTION: Reads back the LOCKBITS register over PDI so it can be printed after programming
 * INPUT:       value (uint8_t*) - Destination for the LOCKBITS byte
 * RETURN:      pdi_status_t
 */
static pdi_status_t PDI_READ_LOCKBITS(uint8_t *value)
{
    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_READ_NVM);

    pdi_status_t st = PDI_LDS_BYTE(PDI_LOCKBITS_ADDR, value);

    PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

    return st;
}

/**
 * DESCRIPTION: Re-reads the configuration fuse bytes just written and confirms each one matches, masking off bits
 *              the datasheet does not define for that FUSEBYTE index. FUSEBYTE4 is skipped because its bits
 *              (RSTDISBL/STARTUPTIME/WDLOCK) only read back correctly after a device reset, per the fuse register
 *              description, so an immediate post-write readback would always mismatch even when the write worked
 * INPUT:       family       (uint8_t)        - FAMILY_ATXMEGA_AU or FAMILY_ATXMEGA_E
 *              expected     (const uint8_t*) - Fuse bytes that were requested to be written
 *              written_flag (const bool*)    - Marks which indices were actually written and need checking
 * RETURN:      pdi_status_t - PDI_OK if every checked byte matches, PDI_ERR_VERIFY on mismatch, or a bus error
 */
static pdi_status_t PDI_VERIFY_FUSES(uint8_t family, const uint8_t *expected, const bool *written_flag)
{
    uint8_t  readback[XMEGA_MAX_FUSES] = { 0 };
    uint32_t fuse_count                = PDI_FUSE_COUNT(family);

    pdi_status_t st = PDI_READ_FUSES(family, readback);
    if (st != PDI_OK)
    {
        return st;
    }

    for (uint32_t i = 0; i < fuse_count; i++)
    {
        if (!written_flag[i] || PDI_FUSE_IS_RESERVED(family, i) || i == PDI_FUSE_VERIFY_SKIP_IDX)
        {
            continue;
        }

        uint8_t mask = PDI_FUSE_MASK(family, i);

        if ((readback[i] & mask) != (expected[i] & mask))
        {
            printf("PDI: FAILED - FUSEBYTE%u verify (expected=0x%02X, actual=0x%02X, mask=0x%02X)\n",
                   (unsigned)i, (unsigned)(expected[i] & mask), (unsigned)(readback[i] & mask), (unsigned)mask);
            return PDI_ERR_VERIFY;
        }
    }

    return PDI_OK;
}


/**
 * DESCRIPTION: Program ATxmega_AU MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA_AU(const HEXPacket_t *buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                      (1) Initialization                                    */
    /* -------------------------------------------------------------------------- */
    PDI_PIO_INIT();


    /* -------------------------------------------------------------------------- */
    /*                       (2) PDI Enable                                       */
    /* -------------------------------------------------------------------------- */
    if (PDI_ENABLE() != PDI_OK)
    {
        printf("PDI: FAILED - could not enter programming mode\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (3) Read Device ID                                    */
    /* -------------------------------------------------------------------------- */
    uint8_t     DEVICE_ID[3] = { 0 };
    const char *DEVICE_NAME  = "ATxmega_AU";

    if (PDI_READ_DEVICE_ID(DEVICE_ID, &DEVICE_NAME) != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - device ID read/verify error\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (4) Chip Erase                                        */
    /* -------------------------------------------------------------------------- */
    if ((PDI_CHIP_ERASE() != PDI_OK) || (PDI_ENABLE() != PDI_OK))
    {
        PDI_DISABLE();
        printf("PDI: FAILED - chip erase error\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                     (5) USER_ID & CONFIG Defaults                          */
    /* -------------------------------------------------------------------------- */
    if (PDI_WRITE_DEFAULTS(FAMILY_ATXMEGA_AU) != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - writing factory defaults\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (6) Program & Verify FLASH                            */
    /* -------------------------------------------------------------------------- */
    if (PDI_ERASE_FLASH_BUFFER() != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - flash page buffer erase\n");
        return false;
    }

    uint32_t flash_pages_written = 0u;
    {
        static uint8_t flash_page_buf[ATXMEGA_AU_FLASH_PAGE_SIZE];
        uint32_t       current_page = 0xFFFFFFFFu;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr > ATXMEGA_AU_FLASH_END)
            {
                continue;
            }

            uint32_t page_num      = hex_addr / ATXMEGA_AU_FLASH_PAGE_SIZE;
            uint32_t page_base_hex = page_num * ATXMEGA_AU_FLASH_PAGE_SIZE;

            if (page_num != current_page)
            {
                if (current_page != 0xFFFFFFFFu &&
                    !PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE))
                {
                    uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_AU_FLASH_PAGE_SIZE;

                    if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_AU, pdi_addr, flash_page_buf) != PDI_OK)
                    {
                        PDI_DISABLE();
                        printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                        return false;
                    }
                    if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE) != PDI_OK)
                    {
                        PDI_DISABLE();
                        printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
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
                uint32_t space    = ATXMEGA_AU_FLASH_PAGE_SIZE - offset;
                uint32_t copy_len = (bytes_left < space) ? bytes_left : space;

                memcpy(&flash_page_buf[offset], buffer[pkt].PAYLOAD + src_off, copy_len);

                bytes_left -= copy_len;
                src_off += copy_len;
                offset += copy_len;

                if (bytes_left > 0u)
                {
                    uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_AU_FLASH_PAGE_SIZE;

                    if (!PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE))
                    {
                        if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_AU, pdi_addr, flash_page_buf) != PDI_OK)
                        {
                            PDI_DISABLE();
                            printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                            return false;
                        }
                        if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE) != PDI_OK)
                        {
                            PDI_DISABLE();
                            printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
                            return false;
                        }
                        flash_pages_written++;
                    }

                    current_page++;
                    memset(flash_page_buf, 0xFF, sizeof(flash_page_buf));
                    offset = 0u;
                }
            }
        }

        if (current_page != 0xFFFFFFFFu &&
            !PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE))
        {
            uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_AU_FLASH_PAGE_SIZE;

            if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_AU, pdi_addr, flash_page_buf) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                return false;
            }
            if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_AU_FLASH_PAGE_SIZE) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
                return false;
            }
            flash_pages_written++;
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                      (7) Program & Verify EEPROM                           */
    /* -------------------------------------------------------------------------- */
    uint32_t eeprom_pages_written = 0u;
    {
        static uint8_t eeprom_buf[ATXMEGA_AU_EEPROM_SIZE];
        bool           page_present[ATXMEGA_AU_EEPROM_SIZE / ATXMEGA_AU_EEPROM_PAGE_SIZE] = { false };
        bool           present                                                            = false;

        memset(eeprom_buf, 0xFF, sizeof(eeprom_buf));

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_AU_EEPROM_BGN || hex_addr > ATXMEGA_AU_EEPROM_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_AU_EEPROM_BGN;
            uint32_t count  = ATXMEGA_AU_EEPROM_SIZE - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            memcpy(&eeprom_buf[offset], buffer[pkt].PAYLOAD, count);

            for (uint32_t i = 0; i < count; i++)
            {
                page_present[(offset + i) / ATXMEGA_AU_EEPROM_PAGE_SIZE] = true;
            }
            present = true;
        }

        if (present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - EEPROM NVM entry\n");
                return false;
            }

            for (uint32_t page = 0; page < (ATXMEGA_AU_EEPROM_SIZE / ATXMEGA_AU_EEPROM_PAGE_SIZE); page++)
            {
                if (!page_present[page])
                {
                    continue;
                }

                uint32_t offset   = page * ATXMEGA_AU_EEPROM_PAGE_SIZE;
                uint32_t pdi_addr = PDI_EEPROM_BASE + offset;

                if (PDI_WRITE_EEPROM_PAGE(FAMILY_ATXMEGA_AU, pdi_addr, &eeprom_buf[offset]) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - EEPROM page write error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }
                if (PDI_VERIFY_PAGE(pdi_addr, &eeprom_buf[offset], ATXMEGA_AU_EEPROM_PAGE_SIZE) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - EEPROM page verify error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }
                eeprom_pages_written++;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (8) Program & Verify USER ID                          */
    /* -------------------------------------------------------------------------- */
    uint32_t user_id_bytes_written = 0u;
    {
        static uint8_t user_id_buf[ATXMEGA_AU_USERSIG_SIZE];
        bool           present = false;

        memset(user_id_buf, 0xFF, sizeof(user_id_buf));

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_AU_USER_ID_BGN || hex_addr > ATXMEGA_AU_USER_ID_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_AU_USER_ID_BGN;
            uint32_t count  = ATXMEGA_AU_USERSIG_SIZE - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            memcpy(&user_id_buf[offset], buffer[pkt].PAYLOAD, count);
            present = true;
        }

        if (present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID NVM entry\n");
                return false;
            }

            if (PDI_WRITE_USERSIG_ROW(FAMILY_ATXMEGA_AU, user_id_buf) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID row write error at 0x%08X\n", (unsigned)PDI_USERSIG_BASE);
                return false;
            }
            if (PDI_VERIFY_PAGE(PDI_USERSIG_BASE, user_id_buf, ATXMEGA_AU_USERSIG_SIZE) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID row verify error at 0x%08X\n", (unsigned)PDI_USERSIG_BASE);
                return false;
            }
            user_id_bytes_written = ATXMEGA_AU_USERSIG_SIZE;

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (9) Program & Verify CONFIG                           */
    /* -------------------------------------------------------------------------- */
    uint32_t fuses_written = 0u;
    {
        uint8_t config[ATXMEGA_AU_FUSE_COUNT]       = { 0 };
        bool    fuse_present[ATXMEGA_AU_FUSE_COUNT] = { false };
        bool    any_fuse                            = false;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_AU_CONFIG_BGN || hex_addr > ATXMEGA_AU_CONFIG_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_AU_CONFIG_BGN;
            uint32_t count  = ATXMEGA_AU_FUSE_COUNT - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            for (uint32_t i = 0; i < count; i++)
            {
                config[offset + i]       = buffer[pkt].PAYLOAD[i];
                fuse_present[offset + i] = true;

                if ((offset + i) != ATXMEGA_AU_FUSE_RESERVED_IDX)
                {
                    any_fuse = true;
                }
            }
        }

        if (any_fuse)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - CONFIG NVM entry\n");
                return false;
            }

            for (uint32_t i = 0; i < ATXMEGA_AU_FUSE_COUNT; i++)
            {
                if (!fuse_present[i] || i == ATXMEGA_AU_FUSE_RESERVED_IDX)
                {
                    continue;
                }

                if (PDI_WRITE_FUSE_BYTE(i, config[i]) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - FUSEBYTE%u write error\n", (unsigned)i);
                    return false;
                }
                fuses_written++;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

            if (PDI_VERIFY_FUSES(FAMILY_ATXMEGA_AU, config, fuse_present) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - CONFIG fuse verification\n");
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                     (10) Program LOCKBITS                                  */
    /* -------------------------------------------------------------------------- */
    uint32_t lockbits_written = 0u;
    {
        uint8_t lock_value   = 0xFFu;
        bool    lock_present = false;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_AU_LOCK_BGN || hex_addr > ATXMEGA_AU_LOCK_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_AU_LOCK_BGN;
            lock_value      = buffer[pkt].PAYLOAD[offset];
            lock_present    = true;
        }

        if (lock_present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - LOCKBITS NVM entry\n");
                return false;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FUSE);
            PDI_STS_BYTE(PDI_LOCKBITS_ADDR, lock_value);

            if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - LOCKBITS write error\n");
                return false;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
            lockbits_written++;
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                     (11) Program Exit & PDI Disable                        */
    /* -------------------------------------------------------------------------- */
    uint8_t fuse_bytes[ATXMEGA_AU_FUSE_COUNT] = { 0 };
    bool    fuses_read_ok                     = false;

    if (PDI_ENSURE_NVM_ACTIVE() == PDI_OK)
    {
        fuses_read_ok = (PDI_READ_FUSES(FAMILY_ATXMEGA_AU, fuse_bytes) == PDI_OK);
    }

    if (!fuses_read_ok)
    {
        printf("PDI: WARNING - could not read back CONFIG fuses\n");
    }

    uint8_t lockbits_value   = 0xFFu;
    bool    lockbits_read_ok = false;

    if (PDI_ENSURE_NVM_ACTIVE() == PDI_OK)
    {
        lockbits_read_ok = (PDI_READ_LOCKBITS(&lockbits_value) == PDI_OK);
    }

    if (!lockbits_read_ok)
    {
        printf("PDI: WARNING - could not read back LOCKBITS\n");
    }

    if (PDI_DISABLE() != PDI_OK)
    {
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         SUCCESS — print all results                        */
    /* -------------------------------------------------------------------------- */
    printf("PDI: DEVICE ID = 0x%02X 0x%02X 0x%02X (%s confirmed)\n",
           DEVICE_ID[0], DEVICE_ID[1], DEVICE_ID[2], DEVICE_NAME);
    printf("PDI: CHIP ERASED SUCCESSFULLY\n");
    printf("PDI: FLASH PROGRAMMED & VERIFIED SUCCESSFULLY (%u pages)\n", (unsigned)flash_pages_written);
    printf("PDI: EEPROM PROGRAMMED & VERIFIED SUCCESSFULLY (%u pages)\n", (unsigned)eeprom_pages_written);
    printf("PDI: USER_ID PROGRAMMED & VERIFIED SUCCESSFULLY (%u bytes)\n", (unsigned)user_id_bytes_written);
    printf("PDI: CONFIG FUSES PROGRAMMED SUCCESSFULLY (%u fuses)\n", (unsigned)fuses_written);
    printf("PDI: LOCKBITS PROGRAMMED SUCCESSFULLY (%u fuses)\n", (unsigned)lockbits_written);

    if (fuses_read_ok)
    {
        printf("PDI: CONFIG FUSES =");
        for (uint32_t i = 0; i < ATXMEGA_AU_FUSE_COUNT; i++)
        {
            if (ATXMEGA_AU_FUSE_IS_RESERVED(i))
            {
                continue;
            }

            printf(" FUSEBYTE%u=0x%02X", (unsigned)i, (unsigned)fuse_bytes[i]);
        }
        printf("\n");
    }

    if (lockbits_read_ok)
    {
        printf("PDI: LOCKBITS = 0x%02X\n", (unsigned)lockbits_value);
    }

    printf("PDI: ATXMEGA_AU PROGRAMMED SUCCESSFULLY\n");
    return true;
}

/**
 * DESCRIPTION: Program ATxmega_E MCU
 * INPUT:       buffer (const HEXPacket_t*) - Pointer to the array of parsed HEX data records
 *              total_packets (size_t)      - Total number of records present within the data buffer
 * RETURN:      bool                        - true if the device was fully verified and programmed successfully, false otherwise
 */
bool PROGRAM_ATXMEGA_E(const HEXPacket_t *buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                      (1) Initialization                                    */
    /* -------------------------------------------------------------------------- */
    PDI_PIO_INIT();


    /* -------------------------------------------------------------------------- */
    /*                       (2) PDI Enable                                       */
    /* -------------------------------------------------------------------------- */
    if (PDI_ENABLE() != PDI_OK)
    {
        printf("PDI: FAILED - could not enter programming mode\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (3) Read Device ID                                    */
    /* -------------------------------------------------------------------------- */
    uint8_t     DEVICE_ID[3] = { 0 };
    const char *DEVICE_NAME  = "ATxmega_E";

    if (PDI_READ_DEVICE_ID(DEVICE_ID, &DEVICE_NAME) != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - device ID read/verify error\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (4) Chip Erase                                        */
    /* -------------------------------------------------------------------------- */
    if ((PDI_CHIP_ERASE() != PDI_OK) || (PDI_ENABLE() != PDI_OK))
    {
        PDI_DISABLE();
        printf("PDI: FAILED - chip erase error\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                     (5) USER_ID & CONFIG Defaults                          */
    /* -------------------------------------------------------------------------- */
    if (PDI_WRITE_DEFAULTS(FAMILY_ATXMEGA_E) != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - writing factory defaults\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                      (6) Program & Verify FLASH                            */
    /* -------------------------------------------------------------------------- */
    if (PDI_ERASE_FLASH_BUFFER() != PDI_OK)
    {
        PDI_DISABLE();
        printf("PDI: FAILED - flash page buffer erase\n");
        return false;
    }

    uint32_t flash_pages_written = 0u;
    {
        static uint8_t flash_page_buf[ATXMEGA_E_FLASH_PAGE_SIZE];
        uint32_t       current_page = 0xFFFFFFFFu;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr > ATXMEGA_E_FLASH_END)
            {
                continue;
            }

            uint32_t page_num      = hex_addr / ATXMEGA_E_FLASH_PAGE_SIZE;
            uint32_t page_base_hex = page_num * ATXMEGA_E_FLASH_PAGE_SIZE;

            if (page_num != current_page)
            {
                if (current_page != 0xFFFFFFFFu &&
                    !PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE))
                {
                    uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_E_FLASH_PAGE_SIZE;

                    if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_E, pdi_addr, flash_page_buf) != PDI_OK)
                    {
                        PDI_DISABLE();
                        printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                        return false;
                    }
                    if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE) != PDI_OK)
                    {
                        PDI_DISABLE();
                        printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
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
                uint32_t space    = ATXMEGA_E_FLASH_PAGE_SIZE - offset;
                uint32_t copy_len = (bytes_left < space) ? bytes_left : space;

                memcpy(&flash_page_buf[offset], buffer[pkt].PAYLOAD + src_off, copy_len);

                bytes_left -= copy_len;
                src_off += copy_len;
                offset += copy_len;

                if (bytes_left > 0u)
                {
                    uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_E_FLASH_PAGE_SIZE;

                    if (!PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE))
                    {
                        if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_E, pdi_addr, flash_page_buf) != PDI_OK)
                        {
                            PDI_DISABLE();
                            printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                            return false;
                        }
                        if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE) != PDI_OK)
                        {
                            PDI_DISABLE();
                            printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
                            return false;
                        }
                        flash_pages_written++;
                    }

                    current_page++;
                    memset(flash_page_buf, 0xFF, sizeof(flash_page_buf));
                    offset = 0u;
                }
            }
        }

        if (current_page != 0xFFFFFFFFu &&
            !PDI_PAGE_IS_BLANK(flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE))
        {
            uint32_t pdi_addr = PDI_FLASH_BASE + current_page * ATXMEGA_E_FLASH_PAGE_SIZE;

            if (PDI_WRITE_FLASH_PAGE(FAMILY_ATXMEGA_E, pdi_addr, flash_page_buf) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - flash page write error at 0x%08X\n", (unsigned)pdi_addr);
                return false;
            }
            if (PDI_VERIFY_PAGE(pdi_addr, flash_page_buf, ATXMEGA_E_FLASH_PAGE_SIZE) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - flash page verify error at 0x%08X\n", (unsigned)pdi_addr);
                return false;
            }
            flash_pages_written++;
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                      (7) Program & Verify EEPROM                           */
    /* -------------------------------------------------------------------------- */
    uint32_t eeprom_pages_written = 0u;
    {
        static uint8_t eeprom_buf[ATXMEGA_E_EEPROM_SIZE];
        bool           page_present[ATXMEGA_E_EEPROM_SIZE / ATXMEGA_E_EEPROM_PAGE_SIZE] = { false };
        bool           present                                                          = false;

        memset(eeprom_buf, 0xFF, sizeof(eeprom_buf));

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_E_EEPROM_BGN || hex_addr > ATXMEGA_E_EEPROM_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_E_EEPROM_BGN;
            uint32_t count  = ATXMEGA_E_EEPROM_SIZE - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            memcpy(&eeprom_buf[offset], buffer[pkt].PAYLOAD, count);

            for (uint32_t i = 0; i < count; i++)
            {
                page_present[(offset + i) / ATXMEGA_E_EEPROM_PAGE_SIZE] = true;
            }
            present = true;
        }

        if (present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - EEPROM NVM entry\n");
                return false;
            }

            for (uint32_t page = 0; page < (ATXMEGA_E_EEPROM_SIZE / ATXMEGA_E_EEPROM_PAGE_SIZE); page++)
            {
                if (!page_present[page])
                {
                    continue;
                }

                uint32_t offset   = page * ATXMEGA_E_EEPROM_PAGE_SIZE;
                uint32_t pdi_addr = PDI_EEPROM_BASE + offset;

                if (PDI_WRITE_EEPROM_PAGE(FAMILY_ATXMEGA_E, pdi_addr, &eeprom_buf[offset]) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - EEPROM page write error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }
                if (PDI_VERIFY_PAGE(pdi_addr, &eeprom_buf[offset], ATXMEGA_E_EEPROM_PAGE_SIZE) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - EEPROM page verify error at 0x%08X\n", (unsigned)pdi_addr);
                    return false;
                }
                eeprom_pages_written++;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (8) Program & Verify USER ID                          */
    /* -------------------------------------------------------------------------- */
    uint32_t user_id_bytes_written = 0u;
    {
        static uint8_t user_id_buf[ATXMEGA_E_USERSIG_SIZE];
        bool           present = false;

        memset(user_id_buf, 0xFF, sizeof(user_id_buf));

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_E_USER_ID_BGN || hex_addr > ATXMEGA_E_USER_ID_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_E_USER_ID_BGN;
            uint32_t count  = ATXMEGA_E_USERSIG_SIZE - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            memcpy(&user_id_buf[offset], buffer[pkt].PAYLOAD, count);
            present = true;
        }

        if (present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID NVM entry\n");
                return false;
            }

            if (PDI_WRITE_USERSIG_ROW(FAMILY_ATXMEGA_E, user_id_buf) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID row write error at 0x%08X\n", (unsigned)PDI_USERSIG_BASE);
                return false;
            }
            if (PDI_VERIFY_PAGE(PDI_USERSIG_BASE, user_id_buf, ATXMEGA_E_USERSIG_SIZE) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - USER_ID row verify error at 0x%08X\n", (unsigned)PDI_USERSIG_BASE);
                return false;
            }
            user_id_bytes_written = ATXMEGA_E_USERSIG_SIZE;

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (9) Program & Verify CONFIG                           */
    /* -------------------------------------------------------------------------- */
    uint32_t fuses_written = 0u;
    {
        uint8_t config[ATXMEGA_E_FUSE_COUNT]       = { 0 };
        bool    fuse_present[ATXMEGA_E_FUSE_COUNT] = { false };
        bool    any_fuse                           = false;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_E_CONFIG_BGN || hex_addr > ATXMEGA_E_CONFIG_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_E_CONFIG_BGN;
            uint32_t count  = ATXMEGA_E_FUSE_COUNT - offset;
            if (count > HEX_PAYLOAD_SIZE_BYTES)
            {
                count = HEX_PAYLOAD_SIZE_BYTES;
            }

            for (uint32_t i = 0; i < count; i++)
            {
                config[offset + i]       = buffer[pkt].PAYLOAD[i];
                fuse_present[offset + i] = true;

                if (!ATXMEGA_E_FUSE_IS_RESERVED(offset + i))
                {
                    any_fuse = true;
                }
            }
        }

        if (any_fuse)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - CONFIG NVM entry\n");
                return false;
            }

            for (uint32_t i = 0; i < ATXMEGA_E_FUSE_COUNT; i++)
            {
                if (!fuse_present[i] || ATXMEGA_E_FUSE_IS_RESERVED(i))
                {
                    continue;
                }

                if (PDI_WRITE_FUSE_BYTE(i, config[i]) != PDI_OK)
                {
                    PDI_DISABLE();
                    printf("PDI: FAILED - FUSEBYTE%u write error\n", (unsigned)i);
                    return false;
                }
                fuses_written++;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);

            if (PDI_VERIFY_FUSES(FAMILY_ATXMEGA_E, config, fuse_present) != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - CONFIG fuse verification\n");
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                     (10) Program LOCKBITS                                  */
    /* -------------------------------------------------------------------------- */
    uint32_t lockbits_written = 0u;
    {
        uint8_t lock_value   = 0xFFu;
        bool    lock_present = false;

        for (size_t pkt = 0; pkt < total_packets; pkt++)
        {
            if ((pkt & 15u) == 0u)
            {
                PDI_CLOCK_IDLE_BITS(16u);
            }

            uint32_t hex_addr = buffer[pkt].ADDRESS;

            if (hex_addr < ATXMEGA_AU_LOCK_BGN || hex_addr > ATXMEGA_AU_LOCK_END)
            {
                continue;
            }

            uint32_t offset = hex_addr - ATXMEGA_AU_LOCK_BGN;
            lock_value      = buffer[pkt].PAYLOAD[offset];
            lock_present    = true;
        }

        if (lock_present)
        {
            if (PDI_ENSURE_NVM_ACTIVE() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - LOCKBITS NVM entry\n");
                return false;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_WRITE_FUSE);
            PDI_STS_BYTE(PDI_LOCKBITS_ADDR, lock_value);

            if (PDI_WAIT_NVM_NOT_BUSY() != PDI_OK)
            {
                PDI_DISABLE();
                printf("PDI: FAILED - LOCKBITS write error\n");
                return false;
            }

            PDI_STS_BYTE(NVM_BASE + NVM_REG_CMD, NVM_CMD_NOOP);
            lockbits_written++;
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                     (11) Program Exit & PDI Disable                        */
    /* -------------------------------------------------------------------------- */
    uint8_t fuse_bytes[ATXMEGA_E_FUSE_COUNT] = { 0 };
    bool    fuses_read_ok                    = false;

    if (PDI_ENSURE_NVM_ACTIVE() == PDI_OK)
    {
        fuses_read_ok = (PDI_READ_FUSES(FAMILY_ATXMEGA_E, fuse_bytes) == PDI_OK);
    }

    if (!fuses_read_ok)
    {
        printf("PDI: WARNING - could not read back CONFIG fuses\n");
    }

    uint8_t lockbits_value   = 0xFFu;
    bool    lockbits_read_ok = false;

    if (PDI_ENSURE_NVM_ACTIVE() == PDI_OK)
    {
        lockbits_read_ok = (PDI_READ_LOCKBITS(&lockbits_value) == PDI_OK);
    }

    if (!lockbits_read_ok)
    {
        printf("PDI: WARNING - could not read back LOCKBITS\n");
    }

    if (PDI_DISABLE() != PDI_OK)
    {
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         SUCCESS — print all results                        */
    /* -------------------------------------------------------------------------- */
    printf("PDI: DEVICE ID = 0x%02X 0x%02X 0x%02X (%s confirmed)\n",
           DEVICE_ID[0], DEVICE_ID[1], DEVICE_ID[2], DEVICE_NAME);
    printf("PDI: CHIP ERASED SUCCESSFULLY\n");
    printf("PDI: FLASH PROGRAMMED & VERIFIED SUCCESSFULLY (%u pages)\n", (unsigned)flash_pages_written);
    printf("PDI: EEPROM PROGRAMMED & VERIFIED SUCCESSFULLY (%u pages)\n", (unsigned)eeprom_pages_written);
    printf("PDI: USER_ID PROGRAMMED & VERIFIED SUCCESSFULLY (%u bytes)\n", (unsigned)user_id_bytes_written);
    printf("PDI: CONFIG FUSES PROGRAMMED SUCCESSFULLY (%u fuses)\n", (unsigned)fuses_written);
    printf("PDI: LOCKBITS PROGRAMMED SUCCESSFULLY (%u fuses)\n", (unsigned)lockbits_written);

    if (fuses_read_ok)
    {
        printf("PDI: CONFIG FUSES =");
        for (uint32_t i = 0; i < ATXMEGA_E_FUSE_COUNT; i++)
        {
            if (ATXMEGA_E_FUSE_IS_RESERVED(i))
            {
                continue;
            }

            printf(" FUSEBYTE%u=0x%02X", (unsigned)i, (unsigned)fuse_bytes[i]);
        }
        printf("\n");
    }

    if (lockbits_read_ok)
    {
        printf("PDI: LOCKBITS = 0x%02X\n", (unsigned)lockbits_value);
    }

    printf("PDI: ATXMEGA_E PROGRAMMED SUCCESSFULLY\n");
    return true;
}
