#ifndef PDI_PROGRAMMER_H
#define PDI_PROGRAMMER_H
#include "xmega_nvm.h"
#define PDI_PIN_CLK 0u
#define PDI_PIN_DATA 1u
#define XMEGA_MAX_FUSES 7u
bool pdi_program(xmega_region region, uint32_t offset,
                 const uint8_t *data, size_t length);
bool pdi_identify(void);
bool pdi_begin(void);
bool pdi_end(void);
#endif
