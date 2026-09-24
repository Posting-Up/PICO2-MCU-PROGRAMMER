/* ------------------------------------------------------------------------------------------ */
/* ATMEGA168PB_STRESS - SPI ISP programming stress image for PICO2_MCU_PROGRAMMER               */
/*                                                                                              */
/*   FLASH    : all 16384 bytes. This program at 0x0000, every remaining byte filled by         */
/*              make_hex.py with a xorshift32 pattern (seed 0x168BA5A5) mapped to 0x00..0xFE,  */
/*              so no location is left at the erased value 0xFF                                  */
/*   EEPROM   : all 512 bytes, byte n = (n * 7 + 0x21) % 255  (never 0xFF)                      */
/*   FUSES    : LOW 0xE2 (int. 8 MHz, CKDIV8 off), HIGH 0xDE (BOD 1.8 V, SPIEN kept),          */
/*              EXT 0xFD (BOOTSZ=10, BOOTRST unprogrammed)                                      */
/*   LOCK     : 0xEF (BLB1 mode 2: SPM may not write the boot section; chip erase clears it)   */
/*   USER ID  : none - the ATmega168PB has no user signature row                               */
/*                                                                                              */
/* Behaviour: sums the whole flash and EEPROM, then blinks PB5 (Arduino D13 LED) at 2 Hz       */
/* ------------------------------------------------------------------------------------------ */
#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

int main(void)
{
    DDRB |= (1 << PB5);

    uint8_t sum = 0;
    for (uint16_t i = 0; i < 16384u; i++)
    {
        sum += pgm_read_byte((const uint8_t *)i);
    }
    for (uint16_t i = 0; i < 512u; i++)
    {
        sum += eeprom_read_byte((const uint8_t *)i);
    }
    GPIOR0 = sum;

    while (1)
    {
        PINB = (1 << PB5);
        _delay_ms(250);
    }
}
