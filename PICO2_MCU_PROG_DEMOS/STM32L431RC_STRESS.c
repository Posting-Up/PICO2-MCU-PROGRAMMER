/* ------------------------------------------------------------------------------------------ */
/* STM32L431RC_STRESS - SWD programming stress image for PICO2_MCU_PROGRAMMER (OpenOCD)         */
/*                                                                                              */
/*   FLASH : all 262144 bytes (0x08000000-0x0803FFFF). This program at the start, every         */
/*           remaining byte filled by STM32L431RC_STRESS_make_bin.py with a xorshift32 pattern  */
/*           (seed 0x431CA5A5) mapped to 0x00..0xFE, so no location is left at the erased 0xFF  */
/*                                                                                              */
/* Behaviour: sums the whole flash into RAM (FLASH_SUM), then toggles PA5 about twice a second  */
/*            on the 4 MHz MSI reset clock                                                      */
/*                                                                                              */
/* Build: arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os -nostdlib -nostartfiles                 */
/*        -T STM32L431RC_STRESS.ld -o prog.elf STM32L431RC_STRESS.c                             */
/*        arm-none-eabi-objcopy -O binary prog.elf prog.bin                                     */
/* ------------------------------------------------------------------------------------------ */
#include <stdint.h>

#define FLASH_BASE_ADDR  0x08000000u
#define FLASH_SIZE_BYTES 0x00040000u

#define RCC_AHB2ENR (*(volatile uint32_t *)0x4002104Cu)
#define GPIOA_MODER (*(volatile uint32_t *)0x48000000u)
#define GPIOA_ODR   (*(volatile uint32_t *)0x48000014u)

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

    RCC_AHB2ENR |= 1u << 0;                                  // GPIOA clock
    GPIOA_MODER = (GPIOA_MODER & ~(3u << 10)) | (1u << 10); // PA5 output

    while (1)
    {
        GPIOA_ODR ^= 1u << 5;
        for (volatile uint32_t i = 0; i < 200000u; i++)
        {
        }
    }
}
