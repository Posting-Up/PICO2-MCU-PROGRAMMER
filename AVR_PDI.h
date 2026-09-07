#ifndef AVR_PDI_H
#define AVR_PDI_H


/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "PIC.h"        


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// GPIO
#define PDI_PIN_CLK                     0u
#define PDI_PIN_DATA                    1u

// Bit-bang half-period in microseconds
#define PDI_CLK_HALF_PERIOD_US          2u

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

// NVM PROG KEY 0x1289AB45CDD888FF

// PDI address space: NVM controller I/O base 0x1C0, MCU control base 0x0090 
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

// Loop bounds - everything is bounded so a dead target can never hang the Pico.

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

#define XMEGA_MAX_PAGE_SIZE             512u   // largest flash/usersig page across supported parts; sizes the static page buffers
#define XMEGA_MAX_FUSES                 7u     // FUSEBYTE0..5, plus FUSEBYTE6 on the E series only
#define XMEGA_FUSE_RESERVED_IDX         3u     // FUSEBYTE3 is reserved on every part and never written

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


/* -------------------------------------------------------------------------- */
/*                                  Statics                                   */
/* -------------------------------------------------------------------------- */
static bool PDI_DATA_IS_OUTPUT;

static const uint8_t PDI_NVM_PROG_KEY[8] = 
{
    0xFFu, 0x88u, 0xD8u, 0xCDu, 0x45u, 0xABu, 0x89u, 0x12u
};

static const xmega_chip_t XMEGA_ATXMEGA192A3U = 
{
    "ATxmega192A3U", {0x1Eu, 0x97u, 0x44u}, 0x32000u, 512u, 2048u, 512u, 6u,
    {0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu, 0x00u}
};
static const xmega_chip_t XMEGA_ATXMEGA128A3U = 
{
    "ATxmega128A3U", {0x1Eu, 0x97u, 0x42u}, 0x22000u, 512u, 2048u, 512u, 6u,
    {0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu, 0x00u}
};
static const xmega_chip_t XMEGA_ATXMEGA128A4U = 
{
    "ATxmega128A4U", {0x1Eu, 0x97u, 0x46u}, 0x22000u, 256u, 2048u, 256u, 6u,
    {0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu, 0x00u}
};
static const xmega_chip_t XMEGA_ATXMEGA64A3U = 
{
    "ATxmega64A3U", {0x1Eu, 0x96u, 0x42u}, 0x11000u, 256u, 2048u, 256u, 6u,
    {0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu, 0x00u}
};
static const xmega_chip_t XMEGA_ATXMEGA32C3 = 
{
    "ATxmega32C3", {0x1Eu, 0x95u, 0x49u}, 0x9000u, 256u, 1024u, 256u, 6u,
    {0x00u, 0xFFu, 0x63u, 0x00u, 0x1Eu, 0x3Fu, 0x00u}
};
static const xmega_chip_t XMEGA_ATXMEGA32E5 = 
{
    "ATxmega32E5", {0x1Eu, 0x95u, 0x4Cu}, 0x9000u, 128u, 1024u, 128u, 7u,
    {0x00u, 0xFFu, 0x43u, 0x00u, 0x1Eu, 0x3Fu, 0xFFu}
};


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_ATXMEGA192A3U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA32C3   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA32E5   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA64AU   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA128A3U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA128A4U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);

#endif
