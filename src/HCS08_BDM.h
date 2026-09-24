#ifndef HCS08_BDM_H
#define HCS08_BDM_H

/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "hardware/pio.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define FAMILY_HCS08                1u

#define PIN_RESET                   0u
#define PIN_BKGD                    1u
#define PIN_VDD_DISABLE             2u

#define S19_MIN_RECORD_STRIDE_BYTES 4u
#define NVM_FLASH_SIZE_BYTES        4096u
#define NVM_EEPROM_SIZE_BYTES       128u

#define S19_PACKET_SIZE_BYTES       66u
#define S19_PAYLOAD_SIZE_BYES       64u
#define S19_MAX_PACKETS             ((((NVM_FLASH_SIZE_BYTES + NVM_EEPROM_SIZE_BYTES) / S19_MIN_RECORD_STRIDE_BYTES)) + 64u)

typedef struct
{
    uint16_t ADDRESS;
    uint8_t  PAYLOAD[64];
} S19Packet_t;


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
void S08_PIO_INIT(void);
bool PROGRAM_MC9S08PA4(const S19Packet_t *BUFFER, size_t TOTAL_PACKETS);

#endif
