#ifndef CYPRESS_HCIUART_H
#define CYPRESS_HCIUART_H

/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include "pico/stdlib.h"
#include "hardware/uart.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define FAMILY_CYBT213043    9u

#define PIN_CYBT_UART_TX     0u /* Pico TX  -> Target RX (valid UART0 TX pin) */
#define PIN_CYBT_UART_RX     1u /* Pico RX  <- Target TX (valid UART0 RX pin) */
#define PIN_CYBT_MCLR        2u 
#define PIN_CYBT_CTS         3u 
#define PIN_CYBT_RADIO_RST   4u 

#define CYBT_UART_INSTANCE   uart0
#define CYBT_UART_BAUDRATE   115200u /* Start rate; the boot ROM autobauds on the host's first command */

/* Bridge session control */
#define CYBT_EXIT_BAUDRATE   2400u  
#define CYBT_IDLE_TIMEOUT_MS 10000u 
#define CYBT_RING_SIZE       8192u  /* RX ring, must stay a power of two */
#define CYBT_BRIDGE_CHUNK    256u   /* Per-iteration USB <-> UART copy size */


/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */
void CYBT213043_HCI_BRIDGE(void);

#endif
