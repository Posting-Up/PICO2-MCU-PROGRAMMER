#ifndef MC9S08PA4_H
#define MC9S08PA4_H


/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/pio.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// System clock (default: ~150MHz)
#define SM_MHZ                                  ((uint8_t)(clock_get_hz(clk_sys) / 1000000UL))

// GPIO Pins
#define PIN_RESET                               0u           // GP0
#define PIN_BKGD                                1u           // GP1
#define PIN_VDD_DISABLE                         2u           // GP2

// S08 Command Opcodes
#define S08_CMD_READ_BYTE                       0xE0u        // READ_BYTE
#define S08_CMD_READ_PROGRAM_PROGRAM_STATUS     0xE4u        // READ_PROGRAM_PROGRAM_STATUS
#define S08_CMD_WRITE_BYTE                      0xC0u        // WRITE_BYTE

// Register Addresses
#define SYS_SDIDH                               0x3002u      // Device ID High register
#define SYS_SDIDL                               0x3003u      // Device ID Low  register
#define NVM_FCLKDIV                             0x3020u      // FDIVLD=bit[7], FDIVCK=bit[6], FDIV=bits[5:0]
#define NVM_FCCOBIX                             0x3022u       
#define NVM_FCCOBHI                             0x302Au
#define NVM_FCCOBLO                             0x302Bu
#define NVM_FSTAT                               0x3026u
#define NVM_FPROT                               0x3028u
#define NVM_EEPROT                              0x3029u
#define NVM_FSEC                                0x3021u

// Flash & EEPROM Command Opcodes
#define S08_PROGRAM_FLASH                       0x06u        // Program up to 2 longwords
#define S08_ERASE_ALL_BLOCK                     0x08u        
#define S08_UNSECURE_NVM                        0x0Bu        
#define S08_PROGRAM_EEPROM                      0x11u

// Protection Register unlock values
#define NVM_FPROT_UNPROTECT_ALL                 0xFFu        // FPOPEN=1, FPHDIS=1 -> no flash protection
#define NVM_EEPROT_UNPROTECT_ALL                0x80u        // DPOPEN=1           -> no EEPROM protection

// Flash Configuration Field 
#define NVM_CFG_RESERVED_BYTE                   0xFFu        // 0xFF78-0xFF7B reserved, must be programmed to 0xFF
#define NVM_CFG_FPROT_BYTE                      0xFFu        // 0xFF7C flash protection byte  -> unprotected
#define NVM_CFG_EEPROT_BYTE                     0xFFu        // 0xFF7D EEPROM protection byte -> unprotected
#define NVM_CFG_FOPT_BYTE                       0xFFu        // 0xFF7E flash nonvolatile byte
#define NVM_CFG_FSEC_UNSECURED                  0xFEu        // 0xFF7F KEYEN=11 (backdoor disabled), SEC=10 (unsecured)
#define NVM_FSEC_BYTE_ADDRESS                   0xFF7Fu      // Flash security byte, reloaded into FSEC on every reset
#define NVM_CFG_FIELD_START                     0xFF78u      // First byte of the configuration field phrase
#define NVM_CFG_FIELD_END                       0xFF7Fu      // Last  byte of the configuration field phrase
#define NVM_FSEC_SEC_MASK                       0x03u        // FSEC[SEC] bits [1:0]
#define NVM_FSEC_SEC_UNSECURED                  0x02u        // SEC = 10 -> Unsecured (00/01/11 are all Secured)

// Flag bits
#define NVM_FSTAT_CCIF                          0x80u        // Command Complete Interrupt Flag
#define NVM_FSTAT_ACCERR                        0x20u        // Access Error
#define NVM_FSTAT_FPVIOL                        0x10u        // Protection Violation
#define NVM_FSTAT_ERR_CLR                       0x30u        // Write 1s to clear ACCERR | FPVIOL before a command
#define NVM_FSTAT_ERR_MASK                      0x33u        // ACCERR | FPVIOL | MGSTAT1 | MGSTAT0

// Device ID & Device ID Mask
#define MC9S08PA4_DEV_ID                        0x0043u
#define MC9S08PA4_DEV_ID_MASK                   0x0FFFu      // Upper 4-bits are Revision

// NVM array ranges & program granularity
#define NVM_FLASH_START                         0xF000u      // 4 KB flash block
#define NVM_FLASH_END                           0xFFFFu
#define NVM_FLASH_SIZE_BYTES                    4096u
#define NVM_EEPROM_START                        0x3100u      // 128 byte EEPROM block
#define NVM_EEPROM_END                          0x317Fu
#define NVM_EEPROM_SIZE_BYTES                   128u
#define NVM_ERASED_BYTE                         0xFFu        // Value an erased NVM byte reads back as
#define NVM_FLASH_GLOBAL_HI                     0x00u        // Global address [23:16] selecting the flash block
#define NVM_EEPROM_GLOBAL_HI                    0x00u        // Global address [23:16] selecting the EEPROM block
#define NVM_FLASH_PHRASE_BYTES                  8u           // Program Flash  writes two longwords (4 words) per command
#define NVM_EEPROM_BURST_BYTES                  4u           // Program EEPROM writes one to four bytes per command

// Data sizes
#define S19_PACKET_SIZE_BYTES                   66u          // Wire frame: 2 address bytes + 64 payload bytes
#define S19_PAYLOAD_SIZE_BYES                   64u          // Payload WIDTH on the wire, NOT the address stride

// Staging capacity
#define S19_MIN_RECORD_STRIDE_BYTES             4u           // Smallest S1 record data length supported
#define S19_MAX_PACKETS                         ((((NVM_FLASH_SIZE_BYTES + NVM_EEPROM_SIZE_BYTES) / S19_MIN_RECORD_STRIDE_BYTES)) + 64u)

// Packet Structure
typedef struct
{
    uint16_t ADDRESS;
    uint8_t  PAYLOAD[64];
} S19Packet_t;


/* -------------------------------------------------------------------------- */
/*                                  Statics                                   */
/* -------------------------------------------------------------------------- */
// Clock Divider
static uint8_t     FCLKDIV;

// Image Sizes
static uint8_t     FLASH_IMAGE[NVM_FLASH_SIZE_BYTES];   // 0xF000 - 0xFFFF
static uint8_t     EEPROM_IMAGE[NVM_EEPROM_SIZE_BYTES]; // 0x3100 - 0x317F

// Bit-Banging Timing Constraints (in RP2040 system clock cycles)
static uint16_t    BIT_TIME_SM_CYCLES;                  // Total duration of a single BDM bit window
static uint16_t    TX_1_LOW_TIME_SM_CYCLES;             // Low phase duration when transmitting a logical '1'
static uint16_t    TX_1_HIGH_TIME_SM_CYCLES;            // High phase duration when transmitting a logical '1'
static uint16_t    TX_0_LOW_TIME_SM_CYCLES;             // Low phase duration when transmitting a logical '0'
static uint16_t    TX_0_HIGH_TIME_SM_CYCLES;            // High phase duration when transmitting a logical '0'
static uint16_t    RX_LOW_TIME_SM_CYCLES;               // Host-driven low phase to signal the target to start transmitting a bit
static uint16_t    RX_WAIT_TO_SAMPLE_TIME_SM_CYCLES;    // Delay after host release before sampling the data line
static uint16_t    RX_FINISH_HIGH_TIME_SM_CYCLES;       // Required high/idle recovery time before the next bit can start
static uint16_t    RX_CMD_TO_DATA_DELAY_SM_CYCLES;      // Turnaround delay required between a BDM command byte and its data packet

// PIO Configuration for Transmitting (TX) Data
static const PIO   TX_BYTE_PIO = pio0;                  // PIO hardware block instance designated for BDM serial transmission
static const uint  TX_BYTE_SM  = 0;                     // State machine index within the TX PIO instance
static uint        TX_BYTE_OFFSET;                      // Program counter memory offset where the TX assembly code is loaded

// PIO Configuration for Receiving (RX) Data
static const PIO   RX_BYTE_PIO = pio1;                  // PIO hardware block instance designated for BDM serial reception
static const uint  RX_BYTE_SM  = 0;                     // State machine index within the RX PIO instance
static uint        RX_BYTE_OFFSET;                      // Program counter memory offset where the RX assembly code is loaded


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
void S08_PIO_INIT(void);
bool PROGRAM_MC9S08PA4(const S19Packet_t* BUFFER, size_t TOTAL_PACKETS);


#endif
