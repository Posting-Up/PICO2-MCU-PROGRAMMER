/* ------------------------------------------------------------------------------------------ */
/* NRF52832_STRESS - SWD programming stress image for PICO2_MCU_PROGRAMMER (OpenOCD, --MCU NRF52) */
/*                                                                                              */
/*   FLASH : all 524288 bytes (0x00000000-0x0007FFFF). This program at the start, every         */
/*           remaining byte filled by NRF52832_STRESS_make_bin.py with a xorshift32 pattern     */
/*           (seed 0x52832A5A) mapped to 0x00..0xFE, so no location is left at the erased 0xFF  */
/*                                                                                              */
/* Behaviour: sums the whole flash into RAM (FLASH_SUM), then toggles P0.17 (nRF52-DK LED1)     */
/*            roughly twice a second on the 64 MHz reset clock                                 */
/*                                                                                              */
/* Build: arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os -nostdlib -nostartfiles                 */
/*        -T NRF52832_STRESS.ld -o prog.elf NRF52832_STRESS.c                                   */
/*        arm-none-eabi-objcopy -O binary prog.elf prog.bin                                     */
/* ------------------------------------------------------------------------------------------ */
#include <stdint.h>

#define FLASH_BASE_ADDR  0x00000000u
#define FLASH_SIZE_BYTES 0x00080000u

#define P0_OUT    (*(volatile uint32_t *)0x50000504u)
#define P0_DIRSET (*(volatile uint32_t *)0x50000518u)
#define LED_PIN   17u

extern uint32_t _estack;

volatile uint32_t FLASH_SUM;

void Reset_Handler(void);
void Default_Handler(void);

__attribute__((section(".isr_vector"), used)) const void *const VECTOR_TABLE[16] = {
    &_estack,        Reset_Handler,   Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, 0,
    0,               0,               0,               Default_Handler,
    Default_Handler, 0,               Default_Handler, Default_Handler,
};

void Default_Handler(void)
{
    while (1)
    {
    }
}

void Reset_Handler(void)
{
    uint32_t sum = 0;
    for (const volatile uint8_t *p = (const uint8_t *)FLASH_BASE_ADDR; p < (const uint8_t *)(FLASH_BASE_ADDR + FLASH_SIZE_BYTES); p++)
    {
        sum += *p;
    }
    FLASH_SUM = sum;

    P0_DIRSET = 1u << LED_PIN;

    while (1)
    {
        P0_OUT ^= 1u << LED_PIN;
        for (volatile uint32_t i = 0; i < 2000000u; i++)
        {
        }
    }
}
