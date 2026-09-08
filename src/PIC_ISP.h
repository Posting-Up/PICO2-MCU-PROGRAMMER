#ifndef PIC_H
#define PIC_H

/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "pico/stdlib.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// GPIO Pins
#define PIN_MCLR                                0u       // GP0
#define PIN_PGD                                 1u       // GP1
#define PIN_PGC                                 2u       // GP2

// Device Family Code
#define FAMILY_PIC12F157X           2u
#define FAMILY_PIC16F183XX          3u
#define FAMILY_PIC18FXXK80          4u
#define FAMILY_PIC18F2XK83          5u
#define FAMILY_PIC18FXXQ8X          6u

// HEX Payload Sizing
#define HEX_PACKET_SIZE_BYTES                   36u
#define HEX_PAYLOAD_SIZE_BYTES                  32u
#define MAX_MEM_SIZE_WORDS                      2048u
// HEX Staging Buffer Capacity
#define HEX_MAX_PACKETS                         8192u
// HEX Packet Structure
typedef struct
{
    uint32_t ADDRESS;       // 32-bit address
    uint8_t  PAYLOAD[32];   // 32-byte payload
} HEXPacket_t; 


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_PIC12F157X (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_PIC16F183XX(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_PIC18FXXK80(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_PIC18F2XK83(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_PIC18FXXQ8X(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);


#endif