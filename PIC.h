#ifndef PIC_H
#define PIC_H


/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// GPIO Pins
#define PIN_MCLR                                0u           // GP0
#define PIN_PGD                                 1u           // GP1
#define PIN_PGC                                 2u           // GP2

// Device ID
#define PIC12F1571_DEV_ID                       0x3051u
#define PIC16F18345_DEV_ID                      0x303Fu
#define PIC18F25K80_DEV_ID                      0x6180u
#define PIC18F66K80_DEV_ID                      0x60E0u
#define PIC18F25K83_DEV_ID                      0x6EE0u
#define PIC18F26Q84_DEV_ID                      0xA300u

/* PIC12F157X Specific */
// Program Command Opcodes
#define PIC12F157X_CMD_LOAD_CONFIG              0x00u
#define PIC12F157X_CMD_LOAD_DATA_PROG_MEM       0x02u
#define PIC12F157X_CMD_READ_DATA_PROG_MEM       0x04u
#define PIC12F157X_CMD_INC_ADDR                 0x06u
#define PIC12F157X_CMD_RESET_ADDR               0x16u
#define PIC12F157X_CMD_BGN_PROG_INT             0x08u
#define PIC12F157X_CMD_BGN_PROG_EXT             0x18u
#define PIC12F157X_CMD_END_PROG                 0x0Au
#define PIC12F157X_CMD_BULK_ERASE               0x09u
// Memory Addresses
#define PIC12F157X_FLASH_END                    0x10000u
#define PIC12F157X_CONFIG_BEGIN                 0x1000Eu

/* PIC16F183XX Specific */
// Prog CMD Opcodes 
#define PIC16F183XX_CMD_LOAD_CONFIG             0x00u
#define PIC16F183XX_CMD_LOAD_DATA_NVM           0x02u
#define PIC16F183XX_CMD_LOAD_DATA_NVM_INC       0x22u
#define PIC16F183XX_CMD_READ_DATA_NVM           0x04u
#define PIC16F183XX_CMD_READ_DATA_NVM_INC       0x24u
#define PIC16F183XX_CMD_INC_ADDR                0x06u
#define PIC16F183XX_CMD_LOAD_PC_ADDR            0x1Du
#define PIC16F183XX_CMD_BEGIN_PROGRAM_INT       0x08u
#define PIC16F183XX_CMD_BEGIN_PROGRAM_EXT       0x18u
#define PIC16F183XX_CMD_END_PROGRAM             0x0Au
#define PIC16F183XX_CMD_BULK_ERASE              0x09u
#define PIC16F183XX_CMD_ROW_ERASE               0x05u
// PC Addresses
#define PIC16F183XX_PC_DEV_ID                   0x8006u
#define PIC16F183XX_PC_ERASE                    0xE800U
// Memory Addresses
#define PIC16F183XX_FLASH_END                   0xFFFEu   // // 0x3FFF * 2
#define PIC16F183XX_EEPROM_BGN                  0x1E000u  // // 0xF000 * 2
#define PIC16F183XX_USER_ID_BGN                 0x10000u  // // 0x8000 * 2
#define PIC16F183XX_USER_ID_END                 0x10008u  // // 0x8003 * 2
#define PIC16F183XX_CFG_BGN                     0x1000Eu  // // 0x8007 * 2
#define PIC16F183XX_CFG_END                     0x10014u  // // 0x800A * 2

/* PIC18FXXK80 Specific */
// Prog CMD Opcodes
#define PIC18FXXK80_CMD_CORE_INSTR              0x0u
#define PIC18FXXK80_CMD_SHIFT_OUT_TABLAT        0x2u
#define PIC18FXXK80_CMD_TABLE_READ              0x8u
#define PIC18FXXK80_CMD_TABLE_READ_INC          0x9u
#define PIC18FXXK80_CMD_TABLE_READ_DEC          0xAu
#define PIC18FXXK80_CMD_TABLE_READ_PRE_INC      0xBu
#define PIC18FXXK80_CMD_TABLE_WRITE             0xCu
#define PIC18FXXK80_CMD_TABLE_WRITE_POST2       0xDu
#define PIC18FXXK80_CMD_TABLE_WRITE_PGM_POST2   0xEu
#define PIC18FXXK80_CMD_START_PROG              0xFu
// PC Addresses
#define PIC18FXXK80_PC_DEV_ID                   0x3FFFFEu
// Memory Addresses
#define PIC18FXXK80_FLASH_END                   0x008000u
#define PIC18FXXK80_EEPROM_BGN                  0xF00000u
#define PIC18FXXK80_EEPROM_END                  0xF00400u
#define PIC18FXXK80_USER_ID_BGN                 0x200000u
#define PIC18FXXK80_CONFIG_BGN                  0x300000u

/* PIC18F2XK83 Specific */
// Prog CMD Opcodes
#define PIC18F2XK83_CMD_LOAD_PC_ADDR            0x80u
#define PIC18F2XK83_CMD_BULK_ERASE              0x18u
#define PIC18F2XK83_CMD_ROW_ERASE               0xF0u
#define PIC18F2XK83_CMD_LOAD_DATA_NVM           0x00u
#define PIC18F2XK83_CMD_LOAD_DATA_NVM_INC       0x02u
#define PIC18F2XK83_CMD_READ_DATA_NVM           0xFCu
#define PIC18F2XK83_CMD_READ_DATA_NVM_INC       0xFEu
#define PIC18F2XK83_CMD_INC_ADDR                0xF8u
#define PIC18F2XK83_CMD_BEGIN_PROGRAM_INT       0xE0u
#define PIC18F2XK83_CMD_BEGIN_PROGRAM_EXT       0xC0u
#define PIC18F2XK83_CMD_END_PROGRAM             0x82u
// PC Addresses
#define PIC18F2XK83_PC_DEV_ID                   0x3FFFFEu
#define PIC18F2XK83_PC_CONFIG                   0x300000u
#define PIC18F2XK83_PC_EEPROM                   0x310000u
// Memory Addresses
#define PIC18F2XK83_FLASH_END                   0x007FFFu
#define PIC18F2XK83_USER_ID_BGN                 0x200000u
#define PIC18F2XK83_CFG_BGN                     0x300000u
#define PIC18F2XK83_EEPROM_BGN                  0x310000u
#define PIC18F2XK83_EEPROM_END                  0x3103FFu

/* PIC18FXXQ8X Specific */
// Prog CMD Opcodes
#define PIC18FXXQ8X_CMD_LOAD_PC_ADDR            0x80u
#define PIC18FXXQ8X_CMD_BULK_ERASE              0x18u
#define PIC18FXXQ8X_CMD_PAGE_ERASE_PGM          0xF0u
#define PIC18FXXQ8X_CMD_READ_DATA_NVM           0xFCu
#define PIC18FXXQ8X_CMD_READ_DATA_NVM_INC       0xFEu
#define PIC18FXXQ8X_CMD_INC_ADDR                0xF8u
#define PIC18FXXQ8X_CMD_PROG_DATA               0xC0u
#define PIC18FXXQ8X_CMD_PROG_DATA_INC           0xE0u
// PC Addresses
#define PIC18FXXQ8X_PC_DEV_ID                   0x3FFFFEu
// Memory Addresses
#define PIC18FXXQ8X_FLASH_END                   0x00FFFFu
#define PIC18FXXQ8X_USER_ID_BGN                 0x200000u
#define PIC18FXXQ8X_USER_ID_END                 0x20001Fu
#define PIC18FXXQ8X_CONFIG_BGN                  0x300000u
#define PIC18FXXQ8X_CONFIG_END                  0x300022u
#define PIC18FXXQ8X_EEPROM_BGN                  0x380000u
#define PIC18FXXQ8X_EEPROM_END                  0x3803FFu

// HEX Payload Sizing
#define HEX_PACKET_SIZE_BYTES                   36u
#define HEX_PAYLOAD_SIZE_BYTES                  32u
#define MAX_MEM_SIZE_WORDS                      2048u
#define HEX_MAX_PACKETS                         4032u
// HEX Packet Structure
typedef struct
{
    uint32_t address;       // 32-bit address
    uint8_t  payload[32];   // 32-byte payload
} HEXPacket_t; 


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_PIC12F157X(const HEXPacket_t* buffer, size_t total_packets);
bool PROGRAM_PIC16F183XX(const HEXPacket_t* buffer, size_t total_packets);
bool PROGRAM_PIC18FXXK80(const HEXPacket_t* buffer, size_t total_packets);
bool PROGRAM_PIC18F2XK83(const HEXPacket_t* buffer, size_t total_packets);
bool PROGRAM_PIC18FXXQ8X(const HEXPacket_t* buffer, size_t total_packets);


#endif