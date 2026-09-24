#ifndef XMEGA_PDI_H
#define XMEGA_PDI_H

/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "PIC_ICSP.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define FAMILY_ATXMEGA_AU 7u
#define FAMILY_ATXMEGA_E  8u

#define PDI_PIN_CLK       0u
#define PDI_PIN_DATA      1u


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_ATXMEGA_AU(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS);
bool PROGRAM_ATXMEGA_E(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS);

#endif
