#ifndef AVR_PDI_H
#define AVR_PDI_H


/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "PIC.h"        

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

#define PDI_PTR_INDIRECT                0u   /* *(ptr)                       */
#define PDI_PTR_INDIRECT_PI             1u   /* *(ptr++)                     */
#define PDI_PTR_DIRECT                  2u   /* ptr  (load pointer register) */

// PDI Control/Status Register space (CSRS)
#define PDI_CSR_STATUS                  0u
#define PDI_CSR_RESET                   1u
#define PDI_CSR_CTRL                    2u

#define PDI_STATUS_NVMEN                (1u << 1)   // STATUS bit 1 = NVMEN [2] 32.7.1
#define PDI_RESET_SIGNATURE             0x59u       // write to RESET reg to force reset; anything else releases 
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
#define MCU_DEVID0_ADDR                 (PDI_DATAMEM_BASE + 0x0090u)   /

// NVM command opcodes 
// TRIGGER = what actually starts the command once loaded into NVM.CMD: CMDEX, a dummy PDI write, or the PDI data write itself.
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

// Dummy PDI-write value for page-commit/erase triggers (matches vendor driver's DUMMY_BYTE, [1]); address is what matters, not this value.
#define PDI_DUMMY_TRIGGER_BYTE          0x55u

// PDI-bus addresses for ATxmega192A3U ([4] avrdude part chain x192a3u -> .xmega-ab -> .xmega-cd). PDI-bus space, NOT the HEX address map below.

// flash: base 0x800000, size 0x32000 (192KB app + 8KB boot), page 512 [.xmega-cd / x192a3u]
#define PDI_FLASH_BASE                  0x00800000u
#define ATXMEGA192A3U_FLASH_SIZE        0x32000u
#define ATXMEGA192A3U_FLASH_PAGE_SIZE   512u

// eeprom: base 0x8c0000, size 2048, page 32 [.xmega-cd]
#define PDI_EEPROM_BASE                 0x008C0000u
#define ATXMEGA192A3U_EEPROM_SIZE       2048u
#define ATXMEGA192A3U_EEPROM_PAGE_SIZE  32u

// usersig: base 0x8e0400, size/page 512 [x192a3u override]; one flash page, uses the flash page buffer
#define PDI_USERSIG_BASE                0x008E0400u
#define ATXMEGA192A3U_USERSIG_SIZE      512u

// fuses: base 0x8f0020, 6 bytes (FUSEBYTE0..5); FUSEBYTE3 is reserved and never written
#define PDI_FUSE_BASE                   0x008F0020u
#define ATXMEGA192A3U_FUSE_COUNT        6u
#define ATXMEGA192A3U_FUSE_RESERVED_IDX 3u

// lockbits: base 0x8f0027 [.xmega-cd]; present for completeness, this driver does not program lock bits
#define PDI_LOCKBITS_ADDR               0x008F0027u

// Host-side (Intel HEX) address map - what USB_RP_COM.py's PARSE_HEX_FILE() actually produces in HEXPacket_t.ADDRESS.
// avr-gcc flat/linear VMAs per [7]: .text 0x000000, .eeprom 0x810000, .fuse 0x820000, .lock 0x830000, .user_signatures 0x850000.
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

// Fuse verify masks - reserved/unimplemented bits don't read back as written, so a raw compare would false-fail. Source [4]. Index 3 unused (reserved, skipped outright).
#define ATXMEGA192A3U_FUSE_MASKS        { 0xFFu, 0xFFu, 0x63u, 0x00u, 0x1Fu, 0x3Fu }

// Loop bounds - everything is bounded so a dead target can never hang the Pico.

// Start-bit search window in PDI_CLK bits (4x the 128-bit max guard time [2] 32.3.7); scales with clock rate since it's bit-time, not wall-clock.
#define PDI_RX_START_BIT_TIMEOUT_BITS   4096u

// NVM completion budgets in WALL-CLOCK ms, not attempt counts: silicon completion time is independent of PDI_CLK rate. ~2 orders of magnitude over a real chip-erase.
#define PDI_NVMEN_TIMEOUT_MS            2000u
#define PDI_NVM_BUSY_TIMEOUT_MS         2000u
#define PDI_RESET_RELEASE_ATTEMPTS      64u

// Clocked settle delay between a fuse write and its verify read, in PDI_CLK idle bits (~8.2ms); clocked (not sleep) so PDI_CLK never goes idle >100us [2] 32.3.2.
#define PDI_FUSE_SETTLE_IDLE_BITS       2048u

// Per-chip configuration below. Shared PDI/NVM layout verified identical across all 6 supported parts via [4]'s part hierarchy (.xmega-ab / .xmega-e both inherit .xmega-cd without override) - only sizes, page sizes, and fuse masks differ per chip.

#define XMEGA_EEPROM_PAGE_SIZE          32u   // 32 bytes on every supported part [4], never overridden

// Host-side (Intel HEX) section bases [7]; chip-independent, only each region's END varies with chip size.
// HEX (avr-gcc flat map) section bases. Only flash is programmed by the current sequence.
#define XMEGA_HEX_FLASH_BGN             0x000000u

#define XMEGA_MAX_PAGE_SIZE             512u   // largest flash/usersig page across supported parts; sizes the static page buffers
#define XMEGA_MAX_FUSES                 7u     // FUSEBYTE0..5, plus FUSEBYTE6 on the E series only
#define XMEGA_FUSE_RESERVED_IDX         3u     // FUSEBYTE3 is reserved on every part and never written

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

/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_ATXMEGA192A3U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA32C3   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA32E5   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA64AU   (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA128A3U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA128A4U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);

#endif /* AVR_PDI_H */
