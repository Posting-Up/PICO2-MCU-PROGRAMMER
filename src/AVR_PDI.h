#ifndef AVR_PDI_H
#define AVR_PDI_H

/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "src/PIC_ISP.h"        


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// GPIO
#define PDI_PIN_CLK                     0u
#define PDI_PIN_DATA                    1u

// Device family code
#define FAMILY_ATXMEGA192A3U            7u

// Data Sizing
#define XMEGA_MAX_PAGE_SIZE             512u   // largest flash/usersig page across supported parts; sizes the static page buffers
#define XMEGA_MAX_FUSES                 7u     // FUSEBYTE0..5, plus FUSEBYTE6 on the E series only
#define XMEGA_FUSE_RESERVED_IDX         3u     // FUSEBYTE3 is reserved on every part and never written

#define PDI_RX_START_BIT_TIMEOUT_BITS   4096u   /* >> 128-bit max guard time */
#define PDI_NVM_BUSY_POLL_LIMIT         40000u  /* chip erase takes ~ms      */
#define PDI_RESET_RELEASE_ATTEMPTS      64u


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
bool PROGRAM_ATXMEGA192A3U (const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS);

#endif
