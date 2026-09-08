#ifndef HCS08_BDM_H
#define HCS08_BDM_H


/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "hardware/pio.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// Device Family Code
#define FAMILY_HCS08                1u

// GPIO Pins
#define PIN_RESET                               0u           // GP0
#define PIN_BKGD                                1u           // GP1
#define PIN_VDD_DISABLE                         2u           // GP2

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
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
void S08_PIO_INIT(void);
bool PROGRAM_MC9S08PA4(const S19Packet_t* BUFFER, size_t TOTAL_PACKETS);


#endif
