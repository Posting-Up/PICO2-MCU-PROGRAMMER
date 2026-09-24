/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "PIC_ICSP.h"

#define PIO_DELAY_IMPLEMENTATION
#include "MCHP_DELAY_NS.pio.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define PIC12F1571_DEV_ID                     0x3051u
#define PIC16F18345_DEV_ID                    0x303Fu
#define PIC18F25K80_DEV_ID                    0x6180u
#define PIC18F66K80_DEV_ID                    0x60E0u
#define PIC18F25K83_DEV_ID                    0x6EE0u
#define PIC18F26Q84_DEV_ID                    0xA300u

#define PIC12F157X_CMD_LOAD_CONFIG            0x00u
#define PIC12F157X_CMD_LOAD_DATA_PROG_MEM     0x02u
#define PIC12F157X_CMD_READ_DATA_PROG_MEM     0x04u
#define PIC12F157X_CMD_INC_ADDR               0x06u
#define PIC12F157X_CMD_RESET_ADDR             0x16u
#define PIC12F157X_CMD_BGN_PROG_INT           0x08u
#define PIC12F157X_CMD_BGN_PROG_EXT           0x18u
#define PIC12F157X_CMD_END_PROG               0x0Au
#define PIC12F157X_CMD_BULK_ERASE             0x09u
#define PIC12F157X_FLASH_END                  0x10000u
#define PIC12F157X_CONFIG_BEGIN               0x1000Eu

#define PIC16F183XX_CMD_LOAD_CONFIG           0x00u
#define PIC16F183XX_CMD_LOAD_DATA_NVM         0x02u
#define PIC16F183XX_CMD_LOAD_DATA_NVM_INC     0x22u
#define PIC16F183XX_CMD_READ_DATA_NVM         0x04u
#define PIC16F183XX_CMD_READ_DATA_NVM_INC     0x24u
#define PIC16F183XX_CMD_INC_ADDR              0x06u
#define PIC16F183XX_CMD_LOAD_PC_ADDR          0x1Du
#define PIC16F183XX_CMD_BEGIN_PROGRAM_INT     0x08u
#define PIC16F183XX_CMD_BEGIN_PROGRAM_EXT     0x18u
#define PIC16F183XX_CMD_END_PROGRAM           0x0Au
#define PIC16F183XX_CMD_BULK_ERASE            0x09u
#define PIC16F183XX_CMD_ROW_ERASE             0x05u
#define PIC16F183XX_PC_DEV_ID                 0x8006u
#define PIC16F183XX_PC_ERASE                  0xE800U
#define PIC16F183XX_FLASH_END                 0xFFFEu
#define PIC16F18345_FLASH_WORDS               8192u
#define PIC16F183XX_ROW_WORDS                 32u
#define PIC16F183XX_EEPROM_BGN                0x1E000u
#define PIC16F183XX_USER_ID_BGN               0x10000u
#define PIC16F183XX_USER_ID_END               0x10008u
#define PIC16F183XX_CFG_BGN                   0x1000Eu
#define PIC16F183XX_CFG_END                   0x10014u

#define PIC18FXXK80_CMD_CORE_INSTR            0x0u
#define PIC18FXXK80_CMD_SHIFT_OUT_TABLAT      0x2u
#define PIC18FXXK80_CMD_TABLE_READ            0x8u
#define PIC18FXXK80_CMD_TABLE_READ_INC        0x9u
#define PIC18FXXK80_CMD_TABLE_READ_DEC        0xAu
#define PIC18FXXK80_CMD_TABLE_READ_PRE_INC    0xBu
#define PIC18FXXK80_CMD_TABLE_WRITE           0xCu
#define PIC18FXXK80_CMD_TABLE_WRITE_POST2     0xDu
#define PIC18FXXK80_CMD_TABLE_WRITE_PGM_POST2 0xEu
#define PIC18FXXK80_CMD_START_PROG            0xFu
#define PIC18FXXK80_PC_DEV_ID                 0x3FFFFEu
#define PIC18FXXK80_FLASH_END                 0x008000u
#define PIC18F66K80_FLASH_END                 0x010000u
#define PIC18FXXK80_EEPROM_BGN                0xF00000u
#define PIC18FXXK80_EEPROM_END                0xF00400u
#define PIC18FXXK80_USER_ID_BGN               0x200000u
#define PIC18FXXK80_CONFIG_BGN                0x300000u

#define PIC18F2XK83_CMD_LOAD_PC_ADDR          0x80u
#define PIC18F2XK83_CMD_BULK_ERASE            0x18u
#define PIC18F2XK83_CMD_ROW_ERASE             0xF0u
#define PIC18F2XK83_CMD_LOAD_DATA_NVM         0x00u
#define PIC18F2XK83_CMD_LOAD_DATA_NVM_INC     0x02u
#define PIC18F2XK83_CMD_READ_DATA_NVM         0xFCu
#define PIC18F2XK83_CMD_READ_DATA_NVM_INC     0xFEu
#define PIC18F2XK83_CMD_INC_ADDR              0xF8u
#define PIC18F2XK83_CMD_BEGIN_PROGRAM_INT     0xE0u
#define PIC18F2XK83_CMD_BEGIN_PROGRAM_EXT     0xC0u
#define PIC18F2XK83_CMD_END_PROGRAM           0x82u
#define PIC18F2XK83_PC_DEV_ID                 0x3FFFFEu
#define PIC18F2XK83_PC_CONFIG                 0x300000u
#define PIC18F2XK83_PC_EEPROM                 0x310000u
#define PIC18F2XK83_FLASH_END                 0x007FFFu
#define PIC18F2XK83_USER_ID_BGN               0x200000u
#define PIC18F2XK83_CFG_BGN                   0x300000u
#define PIC18F2XK83_EEPROM_BGN                0x310000u
#define PIC18F2XK83_EEPROM_END                0x3103FFu

#define PIC18FXXQ8X_CMD_LOAD_PC_ADDR          0x80u
#define PIC18FXXQ8X_CMD_BULK_ERASE            0x18u
#define PIC18FXXQ8X_CMD_PAGE_ERASE_PGM        0xF0u
#define PIC18FXXQ8X_CMD_READ_DATA_NVM         0xFCu
#define PIC18FXXQ8X_CMD_READ_DATA_NVM_INC     0xFEu
#define PIC18FXXQ8X_CMD_INC_ADDR              0xF8u
#define PIC18FXXQ8X_CMD_PROG_DATA             0xC0u
#define PIC18FXXQ8X_CMD_PROG_DATA_INC         0xE0u
#define PIC18FXXQ8X_PC_DEV_ID                 0x3FFFFEu
#define PIC18FXXQ8X_FLASH_END                 0x00FFFFu
#define PIC18FXXQ8X_USER_ID_BGN               0x200000u
#define PIC18FXXQ8X_USER_ID_END               0x20001Fu
#define PIC18FXXQ8X_CONFIG_BGN                0x300000u
#define PIC18FXXQ8X_CONFIG_END                0x300022u
#define PIC18FXXQ8X_EEPROM_BGN                0x380000u
#define PIC18FXXQ8X_EEPROM_END                0x3803FFu


/* -------------------------------------------------------------------------- */
/*                                  Globals                                   */
/* -------------------------------------------------------------------------- */
/* Flash image work buffer, shared by the PIC16F183XX (as 14-bit words) and      */
/* PIC18FXXK80 (as bytes, up to 64 KB on the PIC18F66K80) routines. Only one     */
/* programming routine runs at a time, so one buffer saves RAM.                  */
static union
{
    uint8_t  BYTES[PIC18F66K80_FLASH_END];
    uint16_t WORDS[PIC18F66K80_FLASH_END / 2u];
} PIC_IMAGE;


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: [ANY] Selects the ICSP bit-bang setup/hold window for the target family.
 *              The PIC18F(2/4)5K80 datasheet permits a 40ns clock-high/clock-low
 *              window; every other supported ICSP target keeps the 100ns default.
 *              Called once, at the top of each PROGRAM_* entry point, before any
 *              line is toggled.
 * INPUT:       Device family code (FAMILY_* from PIC_ICSP.h)
 * RETURN:      ---
 */
static void ICSP_SELECT_TIMING(uint8_t FAMILY)
{
    if (FAMILY == FAMILY_PIC18FXXK80)
    {
        PIO_DELAY_ICSP_SET_NS(ICSP_BIT_DELAY_K80_NS);
    }
    else
    {
        PIO_DELAY_ICSP_SET_NS(ICSP_BIT_DELAY_NS);
    }
}

/**
 * DESCRIPTION: [ANY] PGD(DATA)=INPUT
 * INPUT:       ---
 * RETURN:      ---
 */
static void SET_PGD_INPUT(void)
{
    gpio_disable_pulls(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_IN);
    PIO_DELAY_ICSP_BIT();
}

/**
 * DESCRIPTION: [ANY] PGD(DATA)=OUTPUT
 * INPUT:       ---
 * RETURN:      ---
 */
static void SET_PGD_OUTPUT(void)
{
    gpio_disable_pulls(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_put(PIN_PGD, 0);
    PIO_DELAY_ICSP_BIT();
}

/**
 * DESCRIPTION: [ANY] Shifts in X-COUNT bits, LSB first
 * INPUT:       # of bits to shift in
 * RETURN:      return bits
 */
static uint32_t SHIFT_IN_BITS_LSB_FIRST(int COUNT)
{
    SET_PGD_INPUT();

    uint32_t DATA = 0;
    uint32_t MASK = 1u;

    for (int i = 0; i < COUNT; i++)
    {
        gpio_put(PIN_PGC, 1);
        PIO_DELAY_ICSP_BIT();

        if (gpio_get(PIN_PGD))
        {
            DATA |= MASK;
        }

        gpio_put(PIN_PGC, 0);
        PIO_DELAY_ICSP_BIT();

        MASK <<= 1;
    }

    return DATA;
}

/**
 * DESCRIPTION: [ANY] Shifts in X-COUNT bits, MSB first
 * INPUT:       # of bits to shift in
 * RETURN:      return bits
 */
static uint32_t SHIFT_IN_BITS_MSB_FIRST(int COUNT)
{
    SET_PGD_INPUT();

    uint32_t DATA = 0;

    for (int i = 0; i < COUNT; i++)
    {
        gpio_put(PIN_PGC, 1);
        PIO_DELAY_ICSP_BIT();

        gpio_put(PIN_PGC, 0);
        PIO_DELAY_ICSP_BIT();

        DATA = (DATA << 1) | (gpio_get(PIN_PGD) ? 1u : 0u);
    }

    return DATA;
}

/**
 * DESCRIPTION: [ANY] Shifts out X-COUNT bits, LSB first
 * INPUT:       Incoming data, # of bits to send
 * RETURN:      ---
 */
static void SHIFT_OUT_BITS_LSB_FIRST(uint32_t DATA, int COUNT)
{
    SET_PGD_OUTPUT();

    for (int i = 0; i < COUNT; i++)
    {
        gpio_put(PIN_PGD, DATA & 1);
        DATA >>= 1;
        PIO_DELAY_ICSP_BIT();

        gpio_put(PIN_PGC, 1);
        PIO_DELAY_ICSP_BIT();

        gpio_put(PIN_PGC, 0);
        PIO_DELAY_ICSP_BIT();
    }
}

/**
 * DESCRIPTION: [ANY] Shifts out X-COUNT bits, MSB first
 * INPUT:       Incoming data, # of bits to send
 * RETURN:      ---
 */
static void SHIFT_OUT_BITS_MSB_FIRST(uint32_t DATA, int COUNT)
{
    SET_PGD_OUTPUT();

    for (uint32_t MASK = (COUNT > 0) ? (1u << (COUNT - 1)) : 0u; MASK != 0u; MASK >>= 1)
    {
        gpio_put(PIN_PGD, (DATA & MASK) != 0u);
        PIO_DELAY_ICSP_BIT();

        gpio_put(PIN_PGC, 1);
        PIO_DELAY_ICSP_BIT();

        gpio_put(PIN_PGC, 0);
        PIO_DELAY_ICSP_BIT();
    }
}

/**
 * DESCRIPTION: [PIC12F157X] [PIC16F183XX] Sends a 6-bit command LSB-first
 * INPUT:       6-bits
 * RETURN:      ---
 */
static void PIC_12_16_SEND_6_BIT_CMD(uint8_t CMD)
{
    gpio_put(PIN_PGC, 0);
    sleep_us(1);

    SHIFT_OUT_BITS_LSB_FIRST(CMD, 6);
    sleep_us(2);
}

/**
 * DESCRIPTION: [PIC16F183XX] Loads the PC address
 * INPUT:       32-bit address (actual address is 24 bits)
 * RETURN:      ---
 */
static void PIC16F183XX_LOAD_PC_ADDR(uint32_t ADDR)
{
    PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_PC_ADDR);

    uint32_t PAYLOAD = (ADDR & 0x3FFFFFu) << 1;
    SHIFT_OUT_BITS_LSB_FIRST(PAYLOAD, 24);
    sleep_us(2);
}

/**
 * DESCRIPTION: [PIC18F2XK83] Loads the PC address
 * INPUT:       24-bit payload = {start=0, addr[21:0] MSb, stop=0}
 * RETURN:      ---
 */
static void PIC18F2XK83_LOAD_PC_ADDR(uint32_t ADDR)
{
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_PC_ADDR, 8);
    sleep_us(2);
    SHIFT_OUT_BITS_MSB_FIRST((ADDR & 0x3FFFFFu) << 1, 24);
}

/**
 * DESCRIPTION: [PIC18FXXQ8X] Load PC Address (0x80)
 * INPUT:       24-bit payload = {start=0, addr[21:0] MSb, stop=0}
 * RETURN:      ---
 */
static void PIC18FXXQ8X_LOAD_PC_ADDR(uint32_t ADDR)
{
    SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_LOAD_PC_ADDR, 8);
    sleep_us(2);
    SHIFT_OUT_BITS_MSB_FIRST((ADDR & 0x3FFFFFu) << 1, 24);
}

/**
 * DESCRIPTION: [PIC18FXXQ8X] Unified 24-bit ICSP transmission payload helper
 * INPUT:       24-bit payload = {start=0, addr[21:0] MSb, stop=0}
 * RETURN:      ---
 */
static void PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(uint32_t DATA)
{
    uint32_t PAYLOAD = (DATA & 0x3FFFFFu) << 1u;
    SHIFT_OUT_BITS_MSB_FIRST(PAYLOAD, 24);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sends a 4-bit command, waits 8 execution clocks, and reads back an 8-bit response
 * INPUT:       Command Opcode
 * RETURN:      8-bits of data
 */
static uint8_t PIC18FXXK80_CMD_READ_BYTE(uint8_t CMD)
{
    SET_PGD_OUTPUT();

    uint8_t BITS = CMD;
    for (uint8_t i = 0; i < 4; i++)
    {
        gpio_put(PIN_PGD, (BITS & 1u) != 0u);
        BITS >>= 1;

        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }

    SET_PGD_INPUT();

    for (int i = 0; i < 8; i++)
    {
        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }

    uint8_t B = (uint8_t)SHIFT_IN_BITS_LSB_FIRST(8);
    SET_PGD_OUTPUT();

    return B;
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sends a 4-bit command and a 16-bit payload
 * INPUT:       Command Opcode
 * RETURN:      ---
 */
static void PIC18FXXK80_CMD_WRITE_WORD(uint8_t CMD, uint16_t DATA)
{
    SET_PGD_OUTPUT();

    SHIFT_OUT_BITS_LSB_FIRST(CMD, 4);
    sleep_us(1);
    SHIFT_OUT_BITS_LSB_FIRST(DATA, 16);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sends a 16-bit core instruction
 * INPUT:       Instruction Opcode
 * RETURN:      ---
 */
static void PIC18FXXK80_CORE_INSTR(uint16_t INSTR)
{
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_CORE_INSTR, INSTR);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sets the Table Pointer to given address
 * INPUT:       Desired Table Pointer Address (32-bits)
 * RETURN:      ---
 */
static void PIC18FXXK80_SET_TBLPTR(uint32_t ADDR)
{
    PIC18FXXK80_CORE_INSTR(0x0E00 | ((ADDR >> 16) & 0xFF));
    PIC18FXXK80_CORE_INSTR(0x6EF8);
    PIC18FXXK80_CORE_INSTR(0x0E00 | ((ADDR >> 8) & 0xFF));
    PIC18FXXK80_CORE_INSTR(0x6EF7);
    PIC18FXXK80_CORE_INSTR(0x0E00 | (ADDR & 0xFF));
    PIC18FXXK80_CORE_INSTR(0x6EF6);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Erase one block per DS39972B Table 3-2 through 3-7
 * INPUT:       reg04/reg05 are the values for 3C0004h and 3C0005h
 * RETURN:      ---
 */
static void PIC18FXXK80_ERASE_BLOCK(uint8_t REG04, uint8_t REG05)
{
    PIC18FXXK80_CORE_INSTR(0x0E3C);
    PIC18FXXK80_CORE_INSTR(0x6EF8);
    PIC18FXXK80_CORE_INSTR(0x0E00);
    PIC18FXXK80_CORE_INSTR(0x6EF7);
    PIC18FXXK80_CORE_INSTR(0x0E04);
    PIC18FXXK80_CORE_INSTR(0x6EF6);

    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)REG04 | ((uint16_t)REG04 << 8));

    PIC18FXXK80_CORE_INSTR(0x0E05);
    PIC18FXXK80_CORE_INSTR(0x6EF6);

    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)REG05 | ((uint16_t)REG05 << 8));

    PIC18FXXK80_CORE_INSTR(0x0E06);
    PIC18FXXK80_CORE_INSTR(0x6EF6);

    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, 0x8080);

    SET_PGD_OUTPUT();
    gpio_put(PIN_PGD, 0);

    SHIFT_OUT_BITS_LSB_FIRST(0x0, 4);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
    sleep_us(2);

    SHIFT_OUT_BITS_LSB_FIRST(0x0, 4);
    gpio_put(PIN_PGD, 0);
    sleep_ms(11);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
}

/**
 * DESCRIPTION: [PIC18F2XK83] Read 1 data word from NVM (post increment)
 * INPUT:       ---
 * RETURN:      16-bit payload
 */
static uint16_t PIC18F2XK83_READ_WORD_NVM_POST_INC(void)
{
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_READ_DATA_NVM_INC, 8);
    sleep_us(1);

    SET_PGD_INPUT();
    sleep_us(2);
    uint32_t RAW = SHIFT_IN_BITS_MSB_FIRST(24);

    SET_PGD_OUTPUT();

    return (uint16_t)((RAW >> 1) & 0xFFFF);
}

/**
 * DESCRIPTION: Programs PIC12F157X MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
 */
bool PROGRAM_PIC12F157X(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS)
{
    ICSP_SELECT_TIMING(FAMILY_PIC12F157X);

    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR);
    gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);
    gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    gpio_put(PIN_MCLR, 0);
    sleep_us(500);

    SHIFT_OUT_BITS_LSB_FIRST(0x4D434850, 32);

    gpio_put(PIN_PGC, 1);
    sleep_us(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(1);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
    SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(0x3FFF << 1), 16);

    for (int I = 0; I < 6; I++)
    {
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
    }

    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
    uint32_t RAW = SHIFT_IN_BITS_LSB_FIRST(16);
    SET_PGD_OUTPUT();

    uint16_t DEV_ID = (uint16_t)((RAW >> 1) & 0x3FFF);

    if (DEV_ID != PIC12F1571_DEV_ID)
    {
        printf("MCU DEVICE ID invalid\n");
        return false;
    }
    else
    {
        printf("MCU DEVICE ID valid: 0x%X\n", DEV_ID);
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase MCU                                      */
    /* -------------------------------------------------------------------------- */
    printf("Erasing...\n");

    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BULK_ERASE);
    sleep_ms(5);

    printf("Erased SUCCESSFULLY...\n");


    /* -------------------------------------------------------------------------- */
    /*                     (4) Program & Verify FLASH                             */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH...\n");

    uint16_t FLASH_IMAGE[MAX_MEM_SIZE_WORDS];
    for (int I = 0; I < MAX_MEM_SIZE_WORDS; I++)
    {
        FLASH_IMAGE[I] = 0x3FFF;
    }

    uint32_t MAX_WORD_ADDR = 0;
    bool     HAS_PROGRAM   = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA >= PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (uint32_t OFF = 0; OFF < 16; OFF += 2)
        {
            uint32_t WORD_ADDR = (BA + OFF) / 2;
            if (WORD_ADDR < MAX_MEM_SIZE_WORDS)
            {
                FLASH_IMAGE[WORD_ADDR] = (((uint16_t)PAYLOAD[OFF + 1] << 8) | PAYLOAD[OFF]) & 0x3FFF;

                HAS_PROGRAM = true;
                if (WORD_ADDR > MAX_WORD_ADDR)
                {
                    MAX_WORD_ADDR = WORD_ADDR;
                }
            }
        }
    }

    if (HAS_PROGRAM)
    {
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
        sleep_us(5);

        uint32_t TOTAL_ROWS = (MAX_WORD_ADDR / 16) + 1;
        for (uint32_t R = 0; R < TOTAL_ROWS; R++)
        {
            uint32_t ROW_START = R * 16;

            for (int I = 0; I < 16; I++)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
                sleep_us(2);

                SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(FLASH_IMAGE[ROW_START + I] << 1), 16);

                if (I < 15)
                {
                    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
                }
            }

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(3); // DS40001713A: TPINT (program memory) 2.5 ms max

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            sleep_us(2);
        }

        printf("Verifying FLASH...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
        sleep_us(5);

        for (uint32_t ADDR = 0; ADDR <= MAX_WORD_ADDR; ADDR++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
            sleep_us(2);

            SET_PGD_INPUT();
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);

            if (ACTUAL != FLASH_IMAGE[ADDR])
            {
                printf("FLASH VERIFICATION FAILED @ 0x%04X: exp 0x%04X got 0x%04X\n", (unsigned)ADDR, FLASH_IMAGE[ADDR], ACTUAL);
                gpio_put(PIN_MCLR, 1);
                return false;
            }

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                    (5) Program & Verify USER ID                            */
    /* -------------------------------------------------------------------------- */
    uint16_t UID_WORDS[4] = { 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF };
    bool     HAS_UID      = false;

    for (size_t P_IDX = 0; P_IDX < TOTAL_PACKETS; P_IDX++)
    {
        const HEXPacket_t *PKT = &BUFFER[P_IDX];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 4; W++)
        {
            uint32_t UID_BA = PIC12F157X_FLASH_END + ((uint32_t)W * 2u);
            if (UID_BA >= BA && UID_BA < BA + 16u)
            {
                uint32_t OFF = UID_BA - BA;
                UID_WORDS[W] = (((uint16_t)PAYLOAD[OFF + 1u] << 8) | PAYLOAD[OFF]) & 0x3FFF;
                HAS_UID      = true;
            }
        }
    }

    if (HAS_UID)
    {
        printf("Programming User IDs...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
            sleep_us(2);
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(UID_WORDS[W] << 1), 16);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(5);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
        }

        printf("Verifying User IDs...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
            sleep_us(2);

            SET_PGD_INPUT();
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);

            if (ACTUAL != UID_WORDS[W])
            {
                printf("[PICO] UID VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8000u + W), UID_WORDS[W], ACTUAL);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                   (6) Program & Verify CONFIG BITS                         */
    /* -------------------------------------------------------------------------- */
    uint16_t CONFIG_WORDS[2] = { 0x3FFF, 0x3FFF };
    bool     HAS_CONFIG      = false;
    uint32_t S_PC            = 0;

    for (size_t P_IDX = 0; P_IDX < TOTAL_PACKETS; P_IDX++)
    {
        uint32_t BA = BUFFER[P_IDX].ADDRESS;
        if (BA < PIC12F157X_FLASH_END)
        {
            continue;
        }

        for (int W = 0; W < 2; W++)
        {
            uint32_t CONFIG_BA = PIC12F157X_CONFIG_BEGIN + ((uint32_t)W * 2u);
            if (CONFIG_BA >= BA && CONFIG_BA < BA + 32u)
            {
                uint32_t OFF    = CONFIG_BA - BA;
                CONFIG_WORDS[W] = (((uint16_t)BUFFER[P_IDX].PAYLOAD[OFF + 1u] << 8) | BUFFER[P_IDX].PAYLOAD[OFF]) & 0x3FFF;
                if (W == 0)
                {
                    CONFIG_WORDS[W] |= 0x0800u;
                }
                HAS_CONFIG = true;
            }
        }
    }

    if (HAS_CONFIG)
    {
        printf("Writing CONFIG Bits...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
        sleep_us(5);
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        while (S_PC < 0x8007u)
        {
            PIC_12_16_SEND_6_BIT_CMD(0x06);
            S_PC++;
        }

        for (int W = 0; W < 2; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
            sleep_us(2);
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(CONFIG_WORDS[W] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(5);
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            S_PC++;
        }

        printf("Verifying CONFIG Bits...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
        sleep_us(5);
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);
        S_PC = 0x8000u;

        while (S_PC < 0x8007u)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            S_PC++;
        }

        for (int W = 0; W < 2; W++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL          = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);
            uint16_t CONFIG_EXPECTED = CONFIG_WORDS[W];

            if (W == 0)
            {
                ACTUAL &= ~0x3104u;
                CONFIG_EXPECTED &= ~0x3104u;
            }
            if (W == 1)
            {
                ACTUAL &= ~0x20FCu;
                CONFIG_EXPECTED &= ~0x20FCu;
            }

            if (ACTUAL != CONFIG_EXPECTED)
            {
                printf("[PICO] CONFIG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)S_PC, CONFIG_EXPECTED, ACTUAL);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            S_PC++;
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */
    gpio_put(PIN_MCLR, 1);

    return true;
}

/**
 * DESCRIPTION: Programs PIC16F183XX MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
 */
bool PROGRAM_PIC16F183XX(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS)
{
    ICSP_SELECT_TIMING(FAMILY_PIC16F183XX);

    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR);
    gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);
    gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_PGD, 0);
    sleep_us(1);

    gpio_put(PIN_MCLR, 0);
    sleep_us(250);

    SHIFT_OUT_BITS_LSB_FIRST(0x4D434850, 32);

    gpio_put(PIN_PGC, 1);
    sleep_us(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(1);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC16F183XX_LOAD_PC_ADDR(PIC16F183XX_PC_DEV_ID);
    PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM);

    uint16_t RAW = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
    SET_PGD_OUTPUT();

    uint16_t DEVICE_ID = (RAW >> 1) & 0x3FFF;
    printf("Device ID:         0x%04X\n", DEVICE_ID);

    if (DEVICE_ID != PIC16F18345_DEV_ID)
    {
        printf("DEVICE ID not a PIC16F18345. Check connections or add addtional MCU support.\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase MCU                                      */
    /* -------------------------------------------------------------------------- */
    printf("Erasing...\n");

    PIC16F183XX_LOAD_PC_ADDR(PIC16F183XX_PC_ERASE);
    PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BULK_ERASE);
    sleep_ms(5);

    printf("MCU Erased Successfully.\n");


    /* -------------------------------------------------------------------------- */
    /*                       (4) Program & Verify FLASH                           */
    /* -------------------------------------------------------------------------- */
    printf("Programming Flash...\n");

    // Merge every packet into one flash image first, so a short HEX record's 0xFF padding is
    // overwritten by any later record that covers the same words (e.g. an interrupt vector
    // at word 4 after a 2-word reset vector), and no record can spill past its row
    uint16_t *IMAGE = PIC_IMAGE.WORDS;
    bool      ROW_USED[PIC16F18345_FLASH_WORDS / PIC16F183XX_ROW_WORDS];

    for (uint32_t I = 0; I < PIC16F18345_FLASH_WORDS; I++)
    {
        IMAGE[I] = 0x3FFF;
    }
    memset(ROW_USED, 0, sizeof(ROW_USED));

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA > PIC16F183XX_FLASH_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = BUFFER[P].PAYLOAD;

        for (uint32_t W = 0; W < 8u; W++)
        {
            uint32_t WORD = (BA >> 1) + W;

            if (WORD < PIC16F18345_FLASH_WORDS)
            {
                IMAGE[WORD]                            = (((uint16_t)PAYLOAD[W * 2u + 1u] << 8) | PAYLOAD[W * 2u]) & 0x3FFF;
                ROW_USED[WORD / PIC16F183XX_ROW_WORDS] = true;
            }
        }
    }

    for (uint32_t ROW = 0; ROW < PIC16F18345_FLASH_WORDS / PIC16F183XX_ROW_WORDS; ROW++)
    {
        if (!ROW_USED[ROW])
        {
            continue;
        }

        uint32_t ROW_BASE = ROW * PIC16F183XX_ROW_WORDS;

        PIC16F183XX_LOAD_PC_ADDR(ROW_BASE);

        for (uint32_t LATCH = 0; LATCH < PIC16F183XX_ROW_WORDS; LATCH++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM_INC);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(IMAGE[ROW_BASE + LATCH]) << 1), 16);
        }

        PIC16F183XX_LOAD_PC_ADDR(ROW_BASE);

        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
        sleep_ms(3);
    }

    printf("Verifying Flash...\n");

    for (uint32_t ROW = 0; ROW < PIC16F18345_FLASH_WORDS / PIC16F183XX_ROW_WORDS; ROW++)
    {
        if (!ROW_USED[ROW])
        {
            continue;
        }

        uint32_t ROW_BASE = ROW * PIC16F183XX_ROW_WORDS;

        PIC16F183XX_LOAD_PC_ADDR(ROW_BASE);

        for (uint32_t W = 0; W < PIC16F183XX_ROW_WORDS; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM          = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            if (RAW_STREAM != IMAGE[ROW_BASE + W])
            {
                printf("[PICO] FLASH VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(ROW_BASE + W), IMAGE[ROW_BASE + W], RAW_STREAM);

                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                       (4) Program & Verify EEPROM                          */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");
    bool HAS_EEPROM = false;
    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        if (BA < PIC16F183XX_EEPROM_BGN)
        {
            continue;
        }

        uint32_t       BASE_DEV = BA >> 1;
        const uint8_t *PAYLOAD  = BUFFER[P].PAYLOAD;
        for (int B = 0; B < 8; B++)
        {
            PIC16F183XX_LOAD_PC_ADDR(BASE_DEV + (uint32_t)B);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(PAYLOAD[B * 2u] & 0xFFu) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
            sleep_ms(3);
        }

        HAS_EEPROM = true;
    }

    if (HAS_EEPROM)
    {
        printf("Verifying EEPROM region...\n");
        for (size_t P = 0; P < TOTAL_PACKETS; P++)
        {
            uint32_t BA = BUFFER[P].ADDRESS;
            if (BA < PIC16F183XX_EEPROM_BGN)
            {
                continue;
            }

            const uint8_t *PAYLOAD = BUFFER[P].PAYLOAD;

            PIC16F183XX_LOAD_PC_ADDR(BA >> 1);
            for (int B = 0; B < 8; B++)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

                SET_PGD_INPUT();
                uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
                RAW_STREAM          = (RAW_STREAM >> 1) & 0x3FFF;
                SET_PGD_OUTPUT();

                uint8_t EXPECTED = PAYLOAD[B * 2u] & 0xFF;
                uint8_t ACTUAL   = (uint8_t)(RAW_STREAM & 0xFF);

                if (ACTUAL != EXPECTED)
                {
                    printf("[PICO] EEPROM VERIFY FAIL @ 0x%04X: exp 0x%02X got 0x%02X\n",
                           (unsigned)((BA >> 1) + (uint32_t)B), EXPECTED, ACTUAL);

                    gpio_put(PIN_MCLR, 1);
                    return false;
                }
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify USER ID                          */
    /* -------------------------------------------------------------------------- */
    uint16_t UID_WORDS[4] = { 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF };
    bool     HAS_UID      = false;
    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA < PIC16F183XX_USER_ID_BGN || BA >= PIC16F183XX_USER_ID_END)
        {
            continue;
        }

        for (int W = 0; W < 4; W++)
        {
            uint32_t UID_BA = PIC16F183XX_USER_ID_BGN + (uint32_t)W * 2u;
            if (UID_BA >= BA && UID_BA < BA + 32u)
            {
                uint32_t OFF = UID_BA - BA;
                UID_WORDS[W] = (((uint16_t)BUFFER[P].PAYLOAD[OFF + 1u] << 8) | BUFFER[P].PAYLOAD[OFF]) & 0x3FFF;
                HAS_UID      = true;
            }
        }
    }

    if (HAS_UID)
    {
        printf("Programming User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(UID_WORDS[W]) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
            sleep_ms(6);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR);
        }

        printf("Verifying User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM          = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = RAW_STREAM & 0x3FFF;
            if (ACTUAL != UID_WORDS[W])
            {
                printf("[PICO] CFG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8000u + (uint32_t)W), UID_WORDS[W], ACTUAL);

                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify CONFIG                           */
    /* -------------------------------------------------------------------------- */
    uint16_t CFG_WORDS[4] = { 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF };
    bool     HAS_CFG      = false;
    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA < PIC16F183XX_CFG_BGN || BA > PIC16F183XX_CFG_END)
        {
            continue;
        }

        for (int C = 0; C < 4; C++)
        {
            uint32_t CFG_BA = PIC16F183XX_CFG_BGN + (uint32_t)C * 2u;
            if (CFG_BA >= BA && CFG_BA < BA + 32u)
            {
                uint32_t OFF = CFG_BA - BA;
                CFG_WORDS[C] = (((uint16_t)BUFFER[P].PAYLOAD[OFF + 1u] << 8) | BUFFER[P].PAYLOAD[OFF]) & 0x3FFF;
                HAS_CFG      = true;
            }
        }
    }

    if (HAS_CFG)
    {
        printf("Programming CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int I = 0; I < 7; I++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR);
        }

        for (int C = 0; C < 4; C++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM);
            SHIFT_OUT_BITS_LSB_FIRST((CFG_WORDS[C] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
            sleep_ms(6);

            if (C < 3)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR);
            }
        }

        printf("Verifying CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        static const uint16_t CFG_MASKS[4] = { 0x2977, 0x3AEF, 0x2003, 0x0003 };
        for (int I = 0; I < 7; I++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR);
        }

        for (int C = 0; C < 4; C++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM          = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t ACTUAL   = RAW_STREAM & CFG_MASKS[C];
            uint16_t EXPECTED = CFG_WORDS[C] & CFG_MASKS[C];

            if (ACTUAL != EXPECTED)
            {
                printf("[PICO] CONFIG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8007u + (uint32_t)C), EXPECTED, ACTUAL);

                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */
    gpio_put(PIN_MCLR, 1);
    return true;
}

/**
 * DESCRIPTION: Programs PIC18FXXK80 MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
 */
bool PROGRAM_PIC18FXXK80(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS)
{
    ICSP_SELECT_TIMING(FAMILY_PIC18FXXK80);

    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR);
    gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);
    gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 0);
    sleep_ms(10);

    gpio_put(PIN_MCLR, 1);
    sleep_ms(10);

    gpio_put(PIN_MCLR, 0);
    sleep_us(250);

    uint32_t MAGIC = 0x4D434850;
    SET_PGD_OUTPUT();
    for (int I = 0; I < 32; I++)
    {
        bool BIT = (MAGIC & 0x80000000u) != 0;
        gpio_put(PIN_PGD, BIT);
        sleep_us(1);
        gpio_put(PIN_PGC, 1);
        sleep_us(1);
        gpio_put(PIN_PGC, 0);
        sleep_us(1);
        MAGIC <<= 1;
    }
    sleep_ms(10);

    gpio_put(PIN_MCLR, 1);
    sleep_ms(10);

    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_PC_DEV_ID);
    uint8_t LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC) & 0xF0;
    uint8_t HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

    uint16_t DEVICE_ID = (uint16_t)(LO | ((uint16_t)HI << 8));
    printf("Device ID:       0x%04X\n", DEVICE_ID);

    if (DEVICE_ID != PIC18F25K80_DEV_ID && DEVICE_ID != PIC18F66K80_DEV_ID)
    {
        printf("DEVICE ID not a PIC18F25K80 or PIC18F66K80. Check connections or add additional MCU support.\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase MCU                                      */
    /* -------------------------------------------------------------------------- */
    PIC18FXXK80_CORE_INSTR(0x8E7F);
    PIC18FXXK80_CORE_INSTR(0x9C7F);
    PIC18FXXK80_CORE_INSTR(0x847F);
    PIC18FXXK80_SET_TBLPTR(0x200000);
    PIC18FXXK80_CORE_INSTR(0x887F);
    PIC18FXXK80_CORE_INSTR(0x827F);
    gpio_put(PIN_PGD, 0);
    for (int I = 0; I < 3; I++)
    {
        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }
    gpio_put(PIN_PGC, 1);
    sleep_ms(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(100);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

    PIC18FXXK80_ERASE_BLOCK(0x02, 0x00);
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x01);
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x02);
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x04);
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x08);
    PIC18FXXK80_ERASE_BLOCK(0x05, 0x00);
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x00);

    PIC18FXXK80_SET_TBLPTR(0x001000);
    uint8_t PROBE_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
    uint8_t PROBE_HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
    printf("[PICO] post-erase probe @ 0x001000 = %02X %02X (expect FF FF)\n", PROBE_LO, PROBE_HI);


    /* -------------------------------------------------------------------------- */
    /*                         (4) Program & Verify FLASH                         */
    /* -------------------------------------------------------------------------- */
    printf("Programming Flash...\n");

    PIC18FXXK80_CORE_INSTR(0x0000);

    // PIC18F66K80 has 64 KB of code memory, the PIC18F25K80 32 KB (DS39972B Table 2-2)
    uint32_t FLASH_SIZE       = (DEVICE_ID == PIC18F66K80_DEV_ID) ? PIC18F66K80_FLASH_END : PIC18FXXK80_FLASH_END;
    uint8_t *PIC_FLASH_MATRIX = PIC_IMAGE.BYTES;
    memset(PIC_FLASH_MATRIX, 0xFF, FLASH_SIZE);

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA < FLASH_SIZE)
        {
            size_t N = (size_t)(FLASH_SIZE - BA);
            if (N > 32u)
            {
                N = 32u;
            }
            memcpy(&PIC_FLASH_MATRIX[BA], BUFFER[P].PAYLOAD, N);
        }
    }

    for (uint32_t BLOCK_IDX = 0; BLOCK_IDX < FLASH_SIZE / 64u; BLOCK_IDX++)
    {
        uint32_t BLOCK_ADDR = BLOCK_IDX * 64u;
        uint8_t *ROW_PTR    = &PIC_FLASH_MATRIX[BLOCK_ADDR];

        bool HAS_ACTIVE_DATA = false;
        for (int B = 0; B < 64; B++)
        {
            if (ROW_PTR[B] != 0xFF)
            {
                HAS_ACTIVE_DATA = true;
                break;
            }
        }

        if (!HAS_ACTIVE_DATA)
        {
            continue;
        }

        PIC18FXXK80_CORE_INSTR(0x8E7F);
        PIC18FXXK80_CORE_INSTR(0x9C7F);
        PIC18FXXK80_CORE_INSTR(0x847F);

        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 16) & 0x3F));
        PIC18FXXK80_CORE_INSTR(0x6EF8);
        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 8) & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF7);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (BLOCK_ADDR & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF6);

        for (int I = 0; I < 31; I++)
        {
            uint8_t  T_LSB       = ROW_PTR[I * 2];
            uint8_t  T_MSB       = ROW_PTR[(I * 2) + 1];
            uint16_t T_WORD_DATA = (uint16_t)(T_LSB | ((uint16_t)T_MSB << 8u));
            PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE_POST2, T_WORD_DATA);
        }

        uint8_t  LAST_LSB  = ROW_PTR[31 * 2];
        uint8_t  LAST_MSB  = ROW_PTR[(31 * 2) + 1];
        uint16_t LAST_WORD = (uint16_t)(LAST_LSB | ((uint16_t)LAST_MSB << 8u));
        PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_START_PROG, LAST_WORD);

        gpio_put(PIN_PGD, 0);
        for (int I = 0; I < 3; I++)
        {
            gpio_put(PIN_PGC, 1);
            gpio_put(PIN_PGC, 0);
        }
        gpio_put(PIN_PGD, 0);

        gpio_put(PIN_PGC, 1);
        sleep_ms(5); // Bench-proven. 2 ms (2x the DS39972B P9 1 ms minimum) left bytes unprogrammed
        gpio_put(PIN_PGC, 0);
        sleep_us(100);
        SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

        uint8_t         PROGRAM_STATUS_LO = 0;
        absolute_time_t TIMEOUT_START     = get_absolute_time();
        do
        {
            if (absolute_time_diff_us(TIMEOUT_START, get_absolute_time()) > 1000000LL)
            {
                printf("[-] Flash write timeout at block index %d!\n", (int)BLOCK_IDX);
                break;
            }
            PIC18FXXK80_CORE_INSTR(0x507F);
            PIC18FXXK80_CORE_INSTR(0x6EF5);
            PIC18FXXK80_CORE_INSTR(0x0000);
            PROGRAM_STATUS_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
            PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
        } while (PROGRAM_STATUS_LO & (1 << 1));

        PIC18FXXK80_CORE_INSTR(0x947F);
    }

    printf("Verifying Flash...\n");
    for (uint32_t BLOCK_IDX = 0; BLOCK_IDX < FLASH_SIZE / 64u; BLOCK_IDX++)
    {
        uint32_t BLOCK_ADDR = BLOCK_IDX * 64u;
        uint8_t *ROW_PTR    = &PIC_FLASH_MATRIX[BLOCK_ADDR];

        bool HAS_ACTIVE_DATA = false;
        for (int B = 0; B < 64; B++)
        {
            if (ROW_PTR[B] != 0xFF)
            {
                HAS_ACTIVE_DATA = true;
                break;
            }
        }
        if (!HAS_ACTIVE_DATA)
        {
            continue;
        }

        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 16) & 0x3F));
        PIC18FXXK80_CORE_INSTR(0x6EF8);
        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 8) & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF7);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (BLOCK_ADDR & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF6);

        for (int I = 0; I < 32; I++)
        {
            uint8_t  READ_LO     = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint8_t  READ_HI     = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint16_t ACTUAL_WORD = (uint16_t)(READ_LO | ((uint16_t)READ_HI << 8u));

            uint8_t  EXP_LSB       = ROW_PTR[I * 2];
            uint8_t  EXP_MSB       = ROW_PTR[(I * 2) + 1];
            uint16_t EXPECTED_WORD = (uint16_t)(EXP_LSB | ((uint16_t)EXP_MSB << 8u));

            if (ACTUAL_WORD != EXPECTED_WORD)
            {
                printf("[-] PFM VERIFY ERROR @ 0x%06X: Expected 0x%04X, Read 0x%04X\n",
                       BLOCK_ADDR + (I * 2), EXPECTED_WORD, ACTUAL_WORD);
                gpio_put(PIN_MCLR, 0);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                         (5) Program & Verify EEPROM                        */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");

    PIC18FXXK80_CORE_INSTR(0x0000);
    PIC18FXXK80_CORE_INSTR(0x9E7F);
    PIC18FXXK80_CORE_INSTR(0x9C7F);

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t RAW_ADDRESS = BUFFER[P].ADDRESS;

        if (RAW_ADDRESS < PIC18FXXK80_EEPROM_BGN || RAW_ADDRESS >= PIC18FXXK80_EEPROM_END)
        {
            continue;
        }

        uint32_t       BASE_EEPROM_ADDR = RAW_ADDRESS - PIC18FXXK80_EEPROM_BGN;
        const uint8_t *PAYLOAD          = BUFFER[P].PAYLOAD;

        for (int I = 0; I < 32; I++)
        {
            uint32_t CURRENT_ADDR = BASE_EEPROM_ADDR + (uint32_t)I;

            if (CURRENT_ADDR >= 0x400u)
            {
                break;
            }

            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)(CURRENT_ADDR & 0x00FFu));
            PIC18FXXK80_CORE_INSTR(0x6E74);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)((CURRENT_ADDR >> 8) & 0x03u));
            PIC18FXXK80_CORE_INSTR(0x6E75);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)PAYLOAD[I]);
            PIC18FXXK80_CORE_INSTR(0x6E73);
            PIC18FXXK80_CORE_INSTR(0x847F);
            PIC18FXXK80_CORE_INSTR(0x827F);

            uint8_t PROGRAM_STATUS_LO;
            do
            {
                PIC18FXXK80_CORE_INSTR(0x507F);
                PIC18FXXK80_CORE_INSTR(0x6EF5);
                PIC18FXXK80_CORE_INSTR(0x0000);

                PROGRAM_STATUS_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
                (void)PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
            } while (PROGRAM_STATUS_LO & (1u << 1));

            gpio_put(PIN_PGC, 0);
            sleep_us(100);

            PIC18FXXK80_CORE_INSTR(0x947F);
        }
    }

    printf("Verifying EEPROM...\n");

    static uint8_t EEPROM_LAYOUT_MATRIX[1024];
    bool           EEPROM_CELL_ACTIVE[1024];

    memset(EEPROM_LAYOUT_MATRIX, 0xFF, sizeof(EEPROM_LAYOUT_MATRIX));
    memset(EEPROM_CELL_ACTIVE, false, sizeof(EEPROM_CELL_ACTIVE));

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t A = BUFFER[P].ADDRESS;
        if (A >= PIC18FXXK80_EEPROM_BGN && A < PIC18FXXK80_EEPROM_END)
        {
            uint32_t       LOCAL_ADDR = A - PIC18FXXK80_EEPROM_BGN;
            const uint8_t *PAYLOAD    = BUFFER[P].PAYLOAD;

            for (int I = 0; I < 32; I++)
            {
                if ((LOCAL_ADDR + I) < 1024)
                {
                    EEPROM_LAYOUT_MATRIX[LOCAL_ADDR + I] = PAYLOAD[I];
                    EEPROM_CELL_ACTIVE[LOCAL_ADDR + I]   = true;
                }
            }
        }
    }

    PIC18FXXK80_CORE_INSTR(0x9E7F);
    PIC18FXXK80_CORE_INSTR(0x9C7F);

    for (uint32_t ADDR = 0; ADDR < 1024; ADDR++)
    {
        if (!EEPROM_CELL_ACTIVE[ADDR])
        {
            continue;
        }

        PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)(ADDR & 0xFFu));
        PIC18FXXK80_CORE_INSTR(0x6E74);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)((ADDR >> 8) & 0x03u));
        PIC18FXXK80_CORE_INSTR(0x6E75);

        PIC18FXXK80_CORE_INSTR(0x807F);

        PIC18FXXK80_CORE_INSTR(0x5073);
        PIC18FXXK80_CORE_INSTR(0x6EF5);
        PIC18FXXK80_CORE_INSTR(0x0000);

        uint8_t READ_VAL = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
        PIC18FXXK80_CMD_READ_BYTE(0x2);

        if (READ_VAL != EEPROM_LAYOUT_MATRIX[ADDR])
        {
            printf("[-] EEPROM VERIFY ERROR @ 0x%04X: Expected 0x%02X, Read 0x%02X\n",
                   (unsigned int)ADDR, EEPROM_LAYOUT_MATRIX[ADDR], READ_VAL);
            gpio_put(PIN_MCLR, 0);
            return false;
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                         (6) Program & Verify USER ID                       */
    /* -------------------------------------------------------------------------- */
    printf("Programming User ID...\n");
    uint16_t UID_WORDS[4];
    for (int I = 0; I < 4; I++)
    {
        UID_WORDS[I] = 0xFFFFu;
    }
    bool HAS_UID = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        for (int W = 0; W < 4; W++)
        {
            uint32_t UID_BA = PIC18FXXK80_USER_ID_BGN + (uint32_t)W * 2u;
            if (UID_BA >= BA && UID_BA < BA + 32u)
            {
                uint32_t T_OFF = UID_BA - BA;
                UID_WORDS[W]   = ((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u) | PKT->PAYLOAD[T_OFF];
                HAS_UID        = true;
            }
        }
    }

    if (HAS_UID)
    {
        PIC18FXXK80_CORE_INSTR(0x8E7F);
        PIC18FXXK80_CORE_INSTR(0x9C7F);
        PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_USER_ID_BGN);

        for (int I = 0; I < 3; I++)
        {
            PIC18FXXK80_CMD_WRITE_WORD(0x0D, UID_WORDS[I]);
        }
        PIC18FXXK80_CMD_WRITE_WORD(0x0F, UID_WORDS[3]);

        gpio_put(PIN_PGD, 0);
        for (int I = 0; I < 3; I++)
        {
            gpio_put(PIN_PGC, 1);
            gpio_put(PIN_PGC, 0);
        }

        gpio_put(PIN_PGC, 1);
        sleep_ms(5);
        gpio_put(PIN_PGC, 0);
        sleep_us(100);
        SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

        printf("Verifying User ID...\n");

        PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_USER_ID_BGN);
        for (int I = 0; I < 4; I++)
        {
            uint8_t LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint8_t HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

            uint16_t ACTUAL_UID   = (uint16_t)(((uint16_t)HI << 8u) | LO);
            uint32_t UID_DEV_ADDR = PIC18FXXK80_USER_ID_BGN + (I * 2);

            if (ACTUAL_UID != UID_WORDS[I])
            {
                printf("[-] USER ID VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       UID_DEV_ADDR, UID_WORDS[I], ACTUAL_UID);
                gpio_put(PIN_MCLR, 0);
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                         (7) Program & Verify CONFIG                        */
    /* -------------------------------------------------------------------------- */
    printf("Programming CONFIG Words...\n");

    uint16_t CFG_WORDS[7] = {
        0x085D,
        0x7F7F,
        0x891F,
        0x0F91,
        0x0FC0,
        0xF0E0,
        0x0040
    };
    bool HAS_CFG = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        for (int W = 0; W < 7; W++)
        {
            uint32_t CFG_BA = PIC18FXXK80_CONFIG_BGN + (uint32_t)W * 2u;
            if (CFG_BA >= BA && CFG_BA < BA + 32u)
            {
                uint32_t T_OFF = CFG_BA - BA;
                CFG_WORDS[W]   = ((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u) | PKT->PAYLOAD[T_OFF];

                HAS_CFG = true;
            }
        }
    }

    if (HAS_CFG)
    {
        for (int W = 0; W < 7; W++)
        {
            if (CFG_WORDS[W] == 0xFFFFu)
            {
                continue;
            }

            uint32_t BASE_ADDR = PIC18FXXK80_CONFIG_BGN + ((uint32_t)W * 2u);
            uint8_t  T_LSB     = (uint8_t)(CFG_WORDS[W] & 0x00FFu);
            uint8_t  T_MSB     = (uint8_t)(CFG_WORDS[W] >> 8u);

            PIC18FXXK80_CORE_INSTR(0x8E7F);
            PIC18FXXK80_CORE_INSTR(0x8C7F);
            PIC18FXXK80_SET_TBLPTR(BASE_ADDR);

            uint16_t EVEN_PAYLOAD = (uint16_t)T_LSB;
            PIC18FXXK80_CMD_WRITE_WORD(0xF, EVEN_PAYLOAD);

            gpio_put(PIN_PGD, 0);
            for (int I = 0; I < 3; I++)
            {
                gpio_put(PIN_PGC, 1);
                gpio_put(PIN_PGC, 0);
            }

            gpio_put(PIN_PGC, 1);
            sleep_ms(5);
            gpio_put(PIN_PGC, 0);
            sleep_us(100);
            SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

            PIC18FXXK80_CORE_INSTR(0x8E7F);
            PIC18FXXK80_CORE_INSTR(0x8C7F);
            PIC18FXXK80_SET_TBLPTR(BASE_ADDR + 1u);

            uint16_t ODD_PAYLOAD = ((uint16_t)T_MSB << 8u);
            PIC18FXXK80_CMD_WRITE_WORD(0xF, ODD_PAYLOAD);

            gpio_put(PIN_PGD, 0);
            for (int I = 0; I < 3; I++)
            {
                gpio_put(PIN_PGC, 1);
                gpio_put(PIN_PGC, 0);
            }

            gpio_put(PIN_PGC, 1);
            sleep_ms(5);
            gpio_put(PIN_PGC, 0);
            sleep_us(100);
            SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
        }
    }

    printf("Verifying CONFIG Words...\n");
    PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_CONFIG_BGN);

    const uint16_t CFG_MASKS[7] = {
        0xDF5D,
        0x7F7F,
        0x8900,
        0x0011,
        0xC00F,
        0xE00F,
        0x400F
    };

    for (int W = 0; W < 7; W++)
    {
        uint8_t T_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
        uint8_t T_HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

        uint16_t ACTUAL_CFG   = (uint16_t)(((uint16_t)T_HI << 8u) | T_LO);
        uint32_t CFG_DEV_ADDR = PIC18FXXK80_CONFIG_BGN + (W * 2);

        uint16_t MASKED_EXPECTED = CFG_WORDS[W] & CFG_MASKS[W];
        uint16_t MASKED_ACTUAL   = ACTUAL_CFG & CFG_MASKS[W];

        if (CFG_WORDS[W] != 0xFFFFu && MASKED_ACTUAL != MASKED_EXPECTED)
        {
            printf("[-] CONFIG VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                   CFG_DEV_ADDR, CFG_WORDS[W], ACTUAL_CFG);
            gpio_put(PIN_MCLR, 0);
            sleep_ms(1);
            return false;
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */
    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 0);

    return true;
}

/**
 * DESCRIPTION: Programs PIC18F2XK83 MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
 */
bool PROGRAM_PIC18F2XK83(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS)
{
    ICSP_SELECT_TIMING(FAMILY_PIC18F2XK83);

    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR);
    gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);
    gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    gpio_put(PIN_MCLR, 0);
    sleep_ms(1);

    uint32_t MAGIC = 0x4D434850;

    SET_PGD_OUTPUT();
    for (int I = 0; I < 32; I++)
    {
        bool BIT = (MAGIC & 0x80000000u) != 0;
        gpio_put(PIN_PGD, BIT);
        sleep_us(1);
        gpio_put(PIN_PGC, 1);
        sleep_us(1);
        gpio_put(PIN_PGC, 0);
        sleep_us(1);
        MAGIC <<= 1;
    }
    sleep_ms(2);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_DEV_ID);
    uint16_t DEVICE_ID = PIC18F2XK83_READ_WORD_NVM_POST_INC();
    printf("Device ID:       0x%04X\n", DEVICE_ID);

    if (DEVICE_ID != PIC18F25K83_DEV_ID)
    {
        printf("DEVICE ID not a PIC18F25K83. Check connections or add additional MCU support.\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Bulk Erase MCU                                 */
    /* -------------------------------------------------------------------------- */
    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_CONFIG);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BULK_ERASE, 8);
    sleep_ms(26);

    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_EEPROM);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BULK_ERASE, 8);
    sleep_ms(26);


    /* -------------------------------------------------------------------------- */
    /*                (4) Program FLASH Memory (64-Word Rows)                     */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH...\n");

    for (size_t P = 0; P < TOTAL_PACKETS;)
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        if (BA > PIC18F2XK83_FLASH_END)
        {
            P++;
            continue;
        }

        uint32_t ROW_BASE = BA & ~0x7Fu;
        uint16_t ROW[64];
        bool     ROW_NEEDS_PROGRAMMING = false;

        for (uint32_t I = 0; I < 64u; I++)
        {
            ROW[I] = 0xFFFFu;
        }

        while (P < TOTAL_PACKETS)
        {
            const HEXPacket_t *PKT         = &BUFFER[P];
            uint32_t           PACKET_ADDR = PKT->ADDRESS;

            if (PACKET_ADDR > PIC18F2XK83_FLASH_END || (PACKET_ADDR & ~0x7Fu) != ROW_BASE)
            {
                break;
            }

            uint32_t OFFSET = PACKET_ADDR - ROW_BASE;
            if ((OFFSET & 0x0Fu) != 0u)
            {
                P++;
                continue;
            }

            uint32_t SLOT = OFFSET >> 1u;
            if ((SLOT + 8u) > 64u)
            {
                P++;
                continue;
            }

            const uint8_t *PAYLOAD = PKT->PAYLOAD;
            for (uint32_t W = 0; W < 8u; W++)
            {
                uint16_t WORD = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8u) | PAYLOAD[W * 2u]);
                ROW[SLOT + W] = WORD;

                if (WORD != 0xFFFFu)
                {
                    ROW_NEEDS_PROGRAMMING = true;
                }
            }

            P++;
        }

        if (!ROW_NEEDS_PROGRAMMING)
        {
            continue;
        }

        PIC18F2XK83_LOAD_PC_ADDR(ROW_BASE);
        for (uint32_t LATCH = 0; LATCH < 64u; LATCH++)
        {
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM_INC, 8);
            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)ROW[LATCH]) << 1u, 24);
        }

        PIC18F2XK83_LOAD_PC_ADDR(ROW_BASE);
        SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
        sleep_ms(3); // DS40001927A: TPINT (program memory) 2.8 ms max
    }


    /* -------------------------------------------------------------------------- */
    /*                       (5) Verify FLASH Memory                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying FLASH...\n");

    for (size_t P = 0; P < TOTAL_PACKETS;)
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        if (BA > PIC18F2XK83_FLASH_END)
        {
            P++;
            continue;
        }

        uint32_t ROW_BASE = BA & ~0x7Fu;
        uint16_t EXPECTED_ROW[64];

        for (uint32_t I = 0; I < 64u; I++)
        {
            EXPECTED_ROW[I] = 0xFFFFu;
        }

        while (P < TOTAL_PACKETS)
        {
            const HEXPacket_t *PKT         = &BUFFER[P];
            uint32_t           PACKET_ADDR = PKT->ADDRESS;

            if (PACKET_ADDR > PIC18F2XK83_FLASH_END || (PACKET_ADDR & ~0x7Fu) != ROW_BASE)
            {
                break;
            }

            uint32_t OFFSET = PACKET_ADDR - ROW_BASE;
            if ((OFFSET & 0x0Fu) != 0u)
            {
                P++;
                continue;
            }

            uint32_t SLOT = OFFSET >> 1u;
            if ((SLOT + 8u) > 64u)
            {
                P++;
                continue;
            }

            const uint8_t *PAYLOAD = PKT->PAYLOAD;
            for (uint32_t W = 0; W < 8u; W++)
            {
                EXPECTED_ROW[SLOT + W] = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8u) | PAYLOAD[W * 2u]);
            }

            P++;
        }

        PIC18F2XK83_LOAD_PC_ADDR(ROW_BASE);
        for (uint32_t LATCH = 0; LATCH < 64u; LATCH++)
        {
            uint16_t ACTUAL_WORD = PIC18F2XK83_READ_WORD_NVM_POST_INC();

            if (ACTUAL_WORD != EXPECTED_ROW[LATCH])
            {
                printf("[-] FLASH VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)(ROW_BASE + (LATCH << 1u)), EXPECTED_ROW[LATCH], ACTUAL_WORD);

                gpio_put(PIN_MCLR, 1);
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*             (6) Program Data EEPROM (Granular Byte Skipping)               */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18F2XK83_EEPROM_BGN || BA >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t       EEPROM_OFFSET = BA - PIC18F2XK83_EEPROM_BGN;
        uint32_t       BASE_DEV      = PIC18F2XK83_EEPROM_BGN + EEPROM_OFFSET;
        const uint8_t *PAYLOAD       = PKT->PAYLOAD;

        for (uint32_t B = 0; B < 8u; B++)
        {
            uint32_t DEV_ADDR = BASE_DEV + (B << 1u);
            if (DEV_ADDR > PIC18F2XK83_EEPROM_END)
            {
                break;
            }

            uint32_t DATA_BYTE = (uint32_t)(PAYLOAD[B * 2u] & 0xFFu);
            if (DATA_BYTE == 0xFFu)
            {
                continue;
            }

            PIC18F2XK83_LOAD_PC_ADDR(DEV_ADDR);
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);

            SHIFT_OUT_BITS_MSB_FIRST(DATA_BYTE << 1u, 24);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                        (7) Verify Data EEPROM                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18F2XK83_EEPROM_BGN || BA >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t       EEPROM_OFFSET = BA - PIC18F2XK83_EEPROM_BGN;
        uint32_t       BASE_DEV      = PIC18F2XK83_EEPROM_BGN + EEPROM_OFFSET;
        const uint8_t *PAYLOAD       = PKT->PAYLOAD;

        for (uint32_t B = 0; B < 8u; B++)
        {
            uint32_t DEV_ADDR = BASE_DEV + (B << 1u);
            if (DEV_ADDR > PIC18F2XK83_EEPROM_END)
            {
                break;
            }

            uint8_t EXPECTED_BYTE = PAYLOAD[B * 2u];

            PIC18F2XK83_LOAD_PC_ADDR(DEV_ADDR);

            uint16_t ACTUAL_WORD = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            uint8_t  ACTUAL_BYTE = (uint8_t)(ACTUAL_WORD & 0xFFu);

            if (ACTUAL_BYTE != EXPECTED_BYTE)
            {
                printf("[-] EEPROM VERIFY ERROR @ 0x%06X: Expected 0x%02X, Got 0x%02X\n",
                       (unsigned)DEV_ADDR, EXPECTED_BYTE, ACTUAL_BYTE);

                gpio_put(PIN_MCLR, 1);
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                    (8) Program & Verify USER ID                            */
    /* -------------------------------------------------------------------------- */
    printf("Programming User IDs...\n");

    uint16_t UID_WORDS[8];
    for (int I = 0; I < 8; I++)
    {
        UID_WORDS[I] = 0xFFFFu;
    }
    bool HAS_UID = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        for (int W = 0; W < 8; W++)
        {
            uint32_t UID_BA = PIC18F2XK83_USER_ID_BGN + (uint32_t)W * 2u;
            if (UID_BA >= BA && UID_BA < BA + 32u)
            {
                uint32_t T_OFF = UID_BA - BA;
                UID_WORDS[W]   = (uint16_t)(((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u) | PKT->PAYLOAD[T_OFF]);
                HAS_UID        = true;
            }
        }
    }

    if (HAS_UID)
    {
        for (int W = 0; W < 8; W++)
        {
            uint32_t UID_DEV_ADDR = PIC18F2XK83_USER_ID_BGN + ((uint32_t)W * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(UID_DEV_ADDR);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)UID_WORDS[W]) << 1u, 24);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);
        }

        printf("Verifying User IDs...\n");
        for (int W = 0; W < 8; W++)
        {
            uint32_t UID_DEV_ADDR = PIC18F2XK83_USER_ID_BGN + ((uint32_t)W * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(UID_DEV_ADDR);

            uint16_t ACTUAL = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            if (ACTUAL != UID_WORDS[W])
            {
                printf("[-] USER ID VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)UID_DEV_ADDR, UID_WORDS[W], ACTUAL);

                gpio_put(PIN_MCLR, 1);
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                   (9) Program & Verify CONFIG WORDS                        */
    /* -------------------------------------------------------------------------- */
    printf("Programming CONFIG Words...\n");

    uint16_t CONFIG_WORDS[5];
    for (int I = 0; I < 5; I++)
    {
        CONFIG_WORDS[I] = 0xFFFFu;
    }
    bool HAS_CONFIG = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        for (int W = 0; W < 5; W++)
        {
            uint32_t CONFIG_BA = PIC18F2XK83_CFG_BGN + (uint32_t)W * 2u;
            if (CONFIG_BA >= BA && CONFIG_BA < BA + 32u)
            {
                uint32_t C_OFF  = CONFIG_BA - BA;
                CONFIG_WORDS[W] = (uint16_t)(((uint16_t)PKT->PAYLOAD[C_OFF + 1u] << 8u) | PKT->PAYLOAD[C_OFF]);
                HAS_CONFIG      = true;
            }
        }
    }

    if (HAS_CONFIG)
    {
        for (int W = 0; W < 5; W++)
        {
            uint32_t CONFIG_DEV_ADDR = PIC18F2XK83_CFG_BGN + ((uint32_t)W * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(CONFIG_DEV_ADDR);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)CONFIG_WORDS[W]) << 1u, 24);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);
        }

        printf("Verifying CONFIG Words...\n");
        static const uint16_t CFG_WORD_MASKS[5] = {
            0x2B77,
            0xBFFF,
            0x3F7F,
            0x2F9F,
            0x0001
        };
        for (int W = 0; W < 5; W++)
        {
            uint32_t CFG_DEV_ADDR = PIC18F2XK83_CFG_BGN + ((uint32_t)W * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(CFG_DEV_ADDR);

            uint16_t ACTUAL          = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            uint16_t MASKED_ACTUAL   = ACTUAL & CFG_WORD_MASKS[W];
            uint16_t MASKED_EXPECTED = CONFIG_WORDS[W] & CFG_WORD_MASKS[W];
            if (MASKED_ACTUAL != MASKED_EXPECTED)
            {
                printf("[-] CONFIG VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)CFG_DEV_ADDR, CONFIG_WORDS[W], ACTUAL);

                gpio_put(PIN_MCLR, 1);
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */
    gpio_put(PIN_MCLR, 1);
    sleep_ms(1);

    return true;
}

/**
 * DESCRIPTION: Programs PIC18FXXQ8X MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
 */
bool PROGRAM_PIC18FXXQ8X(const HEXPacket_t *BUFFER, size_t TOTAL_PACKETS)
{
    ICSP_SELECT_TIMING(FAMILY_PIC18FXXQ8X);

    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR);
    gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);
    gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_MCLR, 0);

    sleep_ms(50);

    uint32_t MAGIC = 0x4D434850;

    SET_PGD_OUTPUT();

    for (int I = 0; I < 32; I++)
    {
        bool BIT = (MAGIC & 0x80000000u) != 0;

        gpio_put(PIN_PGD, BIT);
        sleep_us(1);

        gpio_put(PIN_PGC, 1);
        sleep_us(1);

        gpio_put(PIN_PGC, 0);
        sleep_us(1);

        MAGIC <<= 1;
    }

    sleep_ms(2);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC18FXXQ8X_LOAD_PC_ADDR(PIC18FXXQ8X_PC_DEV_ID);
    sleep_us(2);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM, 8);
    sleep_us(2);
    SET_PGD_INPUT();

    uint32_t RAW = SHIFT_IN_BITS_MSB_FIRST(24);

    SET_PGD_OUTPUT();
    uint16_t DEVICE_ID = (uint16_t)((RAW >> 1) & 0xFFFF);
    printf("Device ID: 0x%04X\n", DEVICE_ID);

    if (DEVICE_ID == 0xFFFF || DEVICE_ID == 0x0000)
    {
        printf("[PICO] ERROR: Invalid Device ID!\n");

        gpio_put(PIN_MCLR, 1);
        sleep_us(1000);

        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase All                                      */
    /* -------------------------------------------------------------------------- */
    printf("Erasing...\n");
    PIC18FXXQ8X_LOAD_PC_ADDR(0x000000);
    sleep_us(2);

    SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_BULK_ERASE, 8);
    sleep_us(2);

    PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(0xF);
    sleep_ms(12); // DS40002137: TERAB (bulk erase) 11 ms max


    /* -------------------------------------------------------------------------- */
    /*               (4) Program & Verify - FLASH & USER ID                       */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH and USER IDs...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        PIC18FXXQ8X_LOAD_PC_ADDR(BA);
        sleep_us(2);

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 8; W++)
        {
            uint16_t WORD_TO_PROGRAM = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8) | PAYLOAD[W * 2u]);

            if (WORD_TO_PROGRAM == 0xFFFF)
            {
                SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_INC_ADDR, 8);
                sleep_us(2);
                continue;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA_INC, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(WORD_TO_PROGRAM);
            sleep_us(75);
        }
    }

    printf("Verifying FLASH and USER IDs...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if ((BA > PIC18FXXQ8X_FLASH_END && BA < PIC18FXXQ8X_USER_ID_BGN) || BA > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        bool ADDRESS_NEEDS_RELOAD = true;

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 8; W++)
        {
            uint16_t EXPECTED_WORD = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8) | PAYLOAD[W * 2u]);

            if (EXPECTED_WORD == 0xFFFF)
            {
                ADDRESS_NEEDS_RELOAD = true;
                continue;
            }

            if (ADDRESS_NEEDS_RELOAD)
            {
                PIC18FXXQ8X_LOAD_PC_ADDR(BA + ((uint32_t)W * 2u));
                sleep_us(2);
                ADDRESS_NEEDS_RELOAD = false;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t READ_RAW = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL_WORD = (uint16_t)((READ_RAW >> 1) & 0xFFFF);

            if (ACTUAL_WORD != EXPECTED_WORD)
            {
                printf("[PICO] VERIFY FAILURE (FLASH/UID): Address 0x%06X Mismatch! Expected 0x%04X, Read 0x%04X\n",
                       (unsigned)(BA + ((uint32_t)W * 2u)), EXPECTED_WORD, ACTUAL_WORD);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                 (5) Program and Verify Data EEPROM                         */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_EEPROM_BGN || BA > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int B = 0; B < 16; B++)
        {
            uint32_t EXACT_BYTE_ADDRESS = BA + (uint32_t)B;

            if (EXACT_BYTE_ADDRESS > PIC18FXXQ8X_EEPROM_END)
            {
                break;
            }

            uint8_t BYTE_TO_PROGRAM = PAYLOAD[B];

            if (BYTE_TO_PROGRAM == 0xFF)
            {
                continue;
            }

            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(BYTE_TO_PROGRAM);
            sleep_ms(11);
        }
    }

    printf("Verifying EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_EEPROM_BGN || BA > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        bool ADDRESS_NEEDS_RELOAD = true;

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int B = 0; B < 16; B++)
        {
            uint32_t EXACT_BYTE_ADDRESS = BA + (uint32_t)B;

            if (EXACT_BYTE_ADDRESS > PIC18FXXQ8X_EEPROM_END)
            {
                break;
            }

            uint8_t EXPECTED_BYTE = PAYLOAD[B];

            if (EXPECTED_BYTE == 0xFF)
            {
                ADDRESS_NEEDS_RELOAD = true;
                continue;
            }

            if (ADDRESS_NEEDS_RELOAD)
            {
                PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
                sleep_us(2);
                ADDRESS_NEEDS_RELOAD = false;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t READ_RAW = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint8_t ACTUAL_BYTE = (uint8_t)((READ_RAW >> 1) & 0xFF);

            if (ACTUAL_BYTE != EXPECTED_BYTE)
            {
                printf("[PICO] VERIFY FAILURE (EEPROM): Address 0x%06X Mismatch! Expected 0x%02X, Read 0x%02X\n",
                       (unsigned)EXACT_BYTE_ADDRESS, EXPECTED_BYTE, ACTUAL_BYTE);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*              (6) Program and Verify CONFIGURATION BYTES                    */
    /* -------------------------------------------------------------------------- */
    printf("Programming CONFIG Fuses...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_CONFIG_BGN || BA > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int B = 0; B < 16; B++)
        {
            uint32_t EXACT_BYTE_ADDRESS = BA + (uint32_t)B;

            if (EXACT_BYTE_ADDRESS > PIC18FXXQ8X_CONFIG_END)
            {
                break;
            }

            uint8_t BYTE_TO_PROGRAM = PAYLOAD[B];

            if (BYTE_TO_PROGRAM == 0xFF)
            {
                continue;
            }

            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(BYTE_TO_PROGRAM);
            sleep_ms(11);
        }
    }

    printf("Verifying CONFIG fuses...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t *PKT = &BUFFER[P];
        uint32_t           BA  = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_CONFIG_BGN || BA > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t *PAYLOAD = PKT->PAYLOAD;
        for (int B = 0; B < 16; B++)
        {
            uint32_t EXACT_BYTE_ADDRESS = BA + (uint32_t)B;

            if (EXACT_BYTE_ADDRESS > PIC18FXXQ8X_CONFIG_END)
            {
                break;
            }

            uint8_t EXPECTED_BYTE = PAYLOAD[B];

            if (EXPECTED_BYTE == 0xFF)
            {
                continue;
            }

            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t READ_RAW = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint8_t ACTUAL_BYTE = (uint8_t)((READ_RAW >> 1) & 0xFF);

            static const uint8_t CFG_MASKS[11] = {
                0x77,
                0xFB,
                0xFF,
                0xBF,
                0x7F,
                0x3F,
                0x3F,
                0x8F,
                0x33,
                0x01,
                0xFF
            };
            uint32_t CFG_IDX = EXACT_BYTE_ADDRESS - PIC18FXXQ8X_CONFIG_BGN;
            if (CFG_IDX < 11u)
            {
                ACTUAL_BYTE &= CFG_MASKS[CFG_IDX];
                EXPECTED_BYTE &= CFG_MASKS[CFG_IDX];
            }

            if (ACTUAL_BYTE != EXPECTED_BYTE)
            {
                printf("[PICO] VERIFY FAILURE (CONFIG): Address 0x%06X Mismatch! Expected 0x%02X, Read 0x%02X\n",
                       (unsigned)EXACT_BYTE_ADDRESS, EXPECTED_BYTE, ACTUAL_BYTE);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                         (7) LVP Exit / SUCCESS                             */
    /* -------------------------------------------------------------------------- */
    gpio_put(PIN_PGD, 0);
    gpio_put(PIN_PGC, 0);
    sleep_us(10);

    gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    return true;
}
