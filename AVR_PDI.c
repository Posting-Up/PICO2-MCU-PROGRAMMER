/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "AVR_PDI.h"


/* ==========================================================================
 *                         SUPPORTED CHIP TABLE
 *
 *
 *    fuse_mask[] index = FUSEBYTE index:
 * 
 *    [0] JTAG user ID  - only exists on the A/B series (JTAG parts). Masked
 *                        to 0x00 on C/D/E so an absent register cannot fail
 *                        verification.
 *    [1] WDT           - all 8 bits readable.
 *    [2] reset config  - 0x63, but 0x43 on the E series [.xmega-e].
 *    [3] RESERVED      - never written (XMEGA_FUSE_RESERVED_IDX).
 *    [4] start-up      - 0x1F on A/B (bit0 = JTAGEN), 0x1E on C/D/E.
 *    [5] EESAVE/BOD    - 0x3F.
 *    [6] E series only - fault-detect actions on Px0..5; 0x00 elsewhere.
 * ========================================================================== */
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


static bool PROGRAM_XMEGA_PDI(const xmega_chip_t *CHIP, const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
{
   return true;
}

bool PROGRAM_ATXMEGA192A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA192A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA128A3U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA128A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA128A4U(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA128A4U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA64AU(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA64A3U, buffer, total_packets);
}

bool PROGRAM_ATXMEGA32C3(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA32C3, buffer, total_packets);
}

bool PROGRAM_ATXMEGA32E5(const HEXPacket_t* buffer, size_t total_packets)
{
    return PROGRAM_XMEGA_PDI(&XMEGA_ATXMEGA32E5, buffer, total_packets);
}
