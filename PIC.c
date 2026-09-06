/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "PIC.h"


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: [ANY] PGD(DATA)=INPUT
 * INPUT:           ---
 * RETURN:          ---
*/
static void SET_PGD_INPUT(void)
{
    gpio_disable_pulls(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_IN);
    sleep_us(1);
}

/**
 * DESCRIPTION: [ANY] PGD(DATA)=OUTPUT
 * INPUT:           ---
 * RETURN:          ---
*/
static void SET_PGD_OUTPUT(void)
{
    gpio_disable_pulls(PIN_PGD);
    gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_put(PIN_PGD, 0);
    sleep_us(1);
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
    uint32_t MASK = 1u;                 // Walking mask: avoids a variable shift per bit

    for (int i = 0; i < COUNT; i++)
    {
        gpio_put(PIN_PGC, 1);
        sleep_us(1);

        if(gpio_get(PIN_PGD))
        {
            DATA |= MASK;
        }

        gpio_put(PIN_PGC, 0);
        sleep_us(1);

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
        sleep_us(1);
        
        gpio_put(PIN_PGC, 0);
        sleep_us(1);

        DATA = (DATA << 1) | (gpio_get(PIN_PGD) ? 1u : 0u);
    }
    
    return DATA;
}

/**
 * DESCRIPTION: [ANY] Shifts out X-COUNT bits, LSB first
 * INPUT:       Incoming data, # of bits to send
 * RETURN:          ---
*/
static void SHIFT_OUT_BITS_LSB_FIRST(uint32_t DATA, int COUNT)
{
    SET_PGD_OUTPUT();

    for (int i = 0; i < COUNT; i++)
    {
        gpio_put(PIN_PGD, DATA & 1);
        DATA >>= 1;
        sleep_us(1);

        gpio_put(PIN_PGC, 1);
        sleep_us(1);
        
        gpio_put(PIN_PGC, 0);
        sleep_us(1);
    }
}

/**
 * DESCRIPTION: [ANY] Shifts out X-COUNT bits, MSB first
 * INPUT:       Incoming data, # of bits to send
 * RETURN:          ---
*/
static void SHIFT_OUT_BITS_MSB_FIRST(uint32_t DATA, int COUNT)
{
    SET_PGD_OUTPUT();

    // Walking mask from bit (COUNT-1) down to bit 0: avoids a variable shift per bit
    for (uint32_t MASK = (COUNT > 0) ? (1u << (COUNT - 1)) : 0u; MASK != 0u; MASK >>= 1)
    {
        gpio_put(PIN_PGD, (DATA & MASK) != 0u);
        sleep_us(1);

        gpio_put(PIN_PGC, 1);
        sleep_us(1);

        gpio_put(PIN_PGC, 0);
        sleep_us(1);
    }
}

/**
 * DESCRIPTION: [PIC12F157X] [PIC16F183XX] Sebds a 6-bit command LSB-first
 * INPUT:       6-bits
 * RETURN:          ---
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
 * RETURN:          ---
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
 * RETURN:          ---
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
 * RETURN:          ---
 */
static void PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(uint32_t DATA)
{
    // 24-bit framing layout: 1 Start bit (0), 6 Pad bits (0), 16 Data bits, 1 Stop bit (0)
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

    /* Send 4-bit command, LSb first */
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

    /* Read 8-bits, LSB first */
    uint8_t b = (uint8_t)SHIFT_IN_BITS_LSB_FIRST(8);
    SET_PGD_OUTPUT();

    return b;
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sends a 4-bit command and a 16-bit payload
 * INPUT:       Command Opcode
 * RETURN:          ---
 */
static void PIC18FXXK80_CMD_WRITE_WORD(uint8_t CMD, uint16_t DATA) {
    SET_PGD_OUTPUT();
    
    SHIFT_OUT_BITS_LSB_FIRST(CMD, 4);
    sleep_us(1);
    SHIFT_OUT_BITS_LSB_FIRST(DATA, 16);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sends a 16-bit core instruction
 * INPUT:       Instruction Opcode
 * RETURN:          ---
 */
static void PIC18FXXK80_CORE_INSTR(uint16_t INSTR)
{
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_CORE_INSTR, INSTR);
}

/**
 * DESCRIPTION: [PIC18FXXK80] Sets the Table Pointer to given address
 * INPUT:       Desired Table Pointer Address (32-bits)
 * RETURN:          ---
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
 * RETURN:          ---
 */
static void PIC18FXXK80_ERASE_BLOCK(uint8_t reg04, uint8_t reg05)
{
    /* 1. Base Pointer Generation -> TBLPTR = 0x3C0004 */
    PIC18FXXK80_CORE_INSTR(0x0E3C); /* MOVLW 3Ch */
    PIC18FXXK80_CORE_INSTR(0x6EF8); /* MOVWF TBLPTRU */
    PIC18FXXK80_CORE_INSTR(0x0E00); /* MOVLW 00h */
    PIC18FXXK80_CORE_INSTR(0x6EF7); /* MOVWF TBLPTRH */
    PIC18FXXK80_CORE_INSTR(0x0E04); /* MOVLW 04h */
    PIC18FXXK80_CORE_INSTR(0x6EF6); /* MOVWF TBLPTRL */

    /* Load Register 04h Latch Data (Command 0xC = Table Write) */
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)reg04 | ((uint16_t)reg04 << 8));

    /* 2. Advance Latch Register -> TBLPTR = 0x3C0005 */
    PIC18FXXK80_CORE_INSTR(0x0E05); /* MOVLW 05h */
    PIC18FXXK80_CORE_INSTR(0x6EF6); /* MOVWF TBLPTRL */

    /* Load Register 05h Latch Data (Command 0xC = Table Write) */
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)reg05 | ((uint16_t)reg05 << 8));

    /* 3. Advance to Execution Register -> TBLPTR = 0x3C0006 */
    PIC18FXXK80_CORE_INSTR(0x0E06); /* MOVLW 06h */
    PIC18FXXK80_CORE_INSTR(0x6EF6); /* MOVWF TBLPTRL */

    /* Write 0x80 using Command 0xF (Table Write, Start Programming) */
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, 0x8080);

    SET_PGD_OUTPUT();
    gpio_put(PIN_PGD, 0);

    /* Complete required ICSP padding clocks */
    SHIFT_OUT_BITS_LSB_FIRST(0x0, 4);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
    sleep_us(2);

    SHIFT_OUT_BITS_LSB_FIRST(0x0, 4);
    /* Enforce self-timed physical matrix discharge delay window (P11) */
    gpio_put(PIN_PGD, 0);
    sleep_ms(11);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);
}

/**
 * DESCRIPTION: [PIC18F2XK83] Read 1 data word from NVM (post increment)
 * INPUT:           ---
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
bool PROGRAM_PIC12F157X(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR); gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);  gpio_set_dir(PIN_PGD,  GPIO_OUT);
    gpio_init(PIN_PGC);  gpio_set_dir(PIN_PGC,  GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC, GPIO_DRIVE_STRENGTH_12MA);
    
    gpio_put(PIN_PGD,  0);
    gpio_put(PIN_PGC,  0);
    gpio_put(PIN_MCLR, 1);      // MCLR HIGH
    sleep_ms(5);

    gpio_put(PIN_MCLR, 0);      // Pull MCLR Low      
    sleep_us(500);          
    
    // Send LVP Key
    SHIFT_OUT_BITS_LSB_FIRST(0x4D434850, 32);
    
    // 33rd Clock of LVP Key
    gpio_put(PIN_PGC, 1);
    sleep_us(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(1);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
    SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(0x3FFF << 1), 16);

    for (int i = 0; i < 6; i++)
    {
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
    }
    
    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
    uint32_t RAW = SHIFT_IN_BITS_LSB_FIRST(16);
    SET_PGD_OUTPUT();

    uint16_t DEV_ID = (uint16_t)((RAW >> 1) & 0x3FFF);

    // Valid DEV_ID? **NOTE: Currently only checks for PIC12F1571 MCU**
    if(DEV_ID != PIC12F1571_DEV_ID)
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
    for (int i = 0; i < MAX_MEM_SIZE_WORDS; i++)
    {
        FLASH_IMAGE[i] = 0x3FFF;
    }

    uint32_t MAX_WORD_ADDR = 0;
    bool HAS_PROGRAM = false;

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* PKT = &buffer[p];
        uint32_t BA            = PKT->address;

        if (BA >= PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->payload;
        for (uint32_t off = 0; off < 16; off += 2)
        {
            uint32_t WORD_ADDR = (BA + off) / 2;
            if (WORD_ADDR < MAX_MEM_SIZE_WORDS)
            {
                FLASH_IMAGE[WORD_ADDR] = (((uint16_t)PAYLOAD[off +1] << 8) | PAYLOAD[off]) & 0x3FFF;

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
        for (uint32_t r = 0; r < TOTAL_ROWS; r++)
        {
           uint32_t ROW_START = r * 16;

           for (int i = 0; i < 16; i++)
           {
                PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
                sleep_us(2);
                
                SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(FLASH_IMAGE[ROW_START + i] << 1), 16);

                if (i < 15)
                {
                    PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
                }
            }

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(5);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            sleep_us(2);
        }
        
        printf("Verifying FLASH...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
            sleep_us(5);

        for (uint32_t addr = 0; addr < MAX_WORD_ADDR; addr++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM);
            sleep_us(2);

            SET_PGD_INPUT();
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);
                
            if (ACTUAL != FLASH_IMAGE[addr])
            {
                printf("FLASH VERIFICATION FAILED @ 0x%04: exp 0x%04 got 0x%04\n", (unsigned)addr, FLASH_IMAGE[addr], ACTUAL);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
                
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                    (5) Program & Verify USER ID                            */
    /* -------------------------------------------------------------------------- */
    uint16_t UID_WORDS[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
    bool HAS_UID          = false;

    for (size_t p_idx = 0; p_idx < total_packets; p_idx++)
    {
        const HEXPacket_t* PKT = &buffer[p_idx];
        uint32_t BA            = PKT->address;

        if (BA < PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->payload;
        for (int w = 0; w < 4; w++)
        {
            uint32_t UID_BA = PIC12F157X_FLASH_END + ((uint32_t)w * 2u); // 0x10000 maps to 0x8000 User ID
            if (UID_BA >= BA && UID_BA < BA + 16u)
            {
                uint32_t off = UID_BA - BA;
                UID_WORDS[w] = (((uint16_t)PAYLOAD[off + 1u] << 8) | PAYLOAD[off]) & 0x3FFF;
                HAS_UID = true;
            }
        }
    }

    if (HAS_UID)
    {
        printf("Programming User IDs...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG);
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int w = 0; w < 4; w++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
            sleep_us(2);
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(UID_WORDS[w] << 1), 16);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(5); // delay = TPINT

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); // Move PC to next slot
        }

        printf("Verifying User IDs...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG); 
        sleep_us(2);
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int w = 0; w < 4; w++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM); 
            sleep_us(2);

            SET_PGD_INPUT();
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);

            if (ACTUAL != UID_WORDS[w])
            {
                printf("[PICO] UID VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8000u + w), UID_WORDS[w], ACTUAL); // Direct address logging
                gpio_put(PIN_MCLR, 1);
                return false;
            }
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); // Move pointer forward
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                   (6) Program & Verify CONFIG BITS                         */
    /* -------------------------------------------------------------------------- */
    uint16_t CONFIG_WORDS[2] = {0x3FFF, 0x3FFF};
    bool     HAS_CONFIG      = false;
    uint32_t S_PC = 0;

    for (size_t p_idx = 0; p_idx < total_packets; p_idx++)
    {
        uint32_t BA = buffer[p_idx].address;
        if (BA < PIC12F157X_FLASH_END) continue;

        for (int w = 0; w < 2; w++)
        {
            uint32_t config_BA = PIC12F157X_CONFIG_BEGIN + ((uint32_t)w * 2u);
            if (config_BA >= BA && config_BA < BA + 32u)
            {
                uint32_t off = config_BA - BA;
                CONFIG_WORDS[w] = (((uint16_t)buffer[p_idx].payload[off + 1u] << 8)
                                  | buffer[p_idx].payload[off]) & 0x3FFF;
                if (w == 0) CONFIG_WORDS[w] |= 0x0800u; // Code Protection OFF
                HAS_CONFIG = true;
            }
        }
    }

    if (HAS_CONFIG)
    {
        printf("Writing CONFIG Bits...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR); sleep_us(5); // Reset Address
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG); sleep_us(2); // Load Configuration
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        while (S_PC < 0x8007u) { PIC_12_16_SEND_6_BIT_CMD(0x06); S_PC++; } // Fast-forward to 0x8007

        for (int w = 0; w < 2; w++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM); sleep_us(2); // Load Data For Program Memory
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(CONFIG_WORDS[w] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT); sleep_ms(5);      // Begin Internally Timed Programming
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); S_PC++;               // Move pointer
        }

        printf("Verifying CONFIG Bits...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR); sleep_us(5);  // Reset pointer for read pass
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG); sleep_us(2); // Load Configuration
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);
        S_PC = 0x8000u;

        while (S_PC < 0x8007u) { PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); S_PC++; }

        for (int w = 0; w < 2; w++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM); sleep_us(2); // Read Data From Program Memory
            SET_PGD_INPUT();
            
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);
            uint16_t CONFIG_EXPECTED = CONFIG_WORDS[w];

            // Clear datasheet unimplemented bits (U-1) to avoid false failures
            if (w == 0) { ACTUAL &= ~0x3104u; CONFIG_EXPECTED &= ~0x3104u; } // Config 1 mask
            if (w == 1) { ACTUAL &= ~0x28FCu; CONFIG_EXPECTED &= ~0x28FCu; } // Config 2 mask

            if (ACTUAL != CONFIG_EXPECTED)
            {
                printf("[PICO] CONFIG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n", 
                       (unsigned)S_PC, CONFIG_EXPECTED, ACTUAL);
                gpio_put(PIN_MCLR, 1);
                return false;
            }
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); S_PC++; // Move pointer
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
bool PROGRAM_PIC16F183XX(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR); gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);  gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);  gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD,  GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC,  GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0); gpio_put(PIN_PGC, 0); gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    // Make sure PGD and PGC are LOW before Pulling MCLR Low
    gpio_put(PIN_PGC, 0);
    gpio_put(PIN_PGD, 0);
    sleep_us(1);

    // Pull MCLR Low
    gpio_put(PIN_MCLR, 0);
    sleep_us(250);

    // Send LVP key
    SHIFT_OUT_BITS_LSB_FIRST(0x4D434850, 32);

    // 33rd clock of LVP key
    gpio_put(PIN_PGC, 1);
    sleep_us(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(1);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC16F183XX_LOAD_PC_ADDR(PIC16F183XX_PC_DEV_ID);                 // Device ID starting address
    PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM);         // Read Data from NVM command

    uint16_t RAW = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
    SET_PGD_OUTPUT();

    // Isolate the 14-bit DEVICE ID
    uint16_t DEVICE_ID = (RAW >> 1) & 0x3FFF;
    printf("Device ID:         0x%04X\n", DEVICE_ID);

    // DEVICE_ID == VALID??
    // IMPORTANT NOTE: Add the check for additional PIC16F183XX mcus if you want to expand support
    if (DEVICE_ID != PIC16F18345_DEV_ID)
    {
        printf("DEVICE ID not a PIC16F18345. Check connections or add addtional MCU support.\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase MCU                                      */
    /* -------------------------------------------------------------------------- */
    printf("Erasing...\n");

    PIC16F183XX_LOAD_PC_ADDR(PIC16F183XX_PC_ERASE);              // Set PC address to ERASE ALL region
    PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BULK_ERASE);        // Bulk Erase CMD
    sleep_ms(5);                                                 // Bulk Erase Cycle Time Delay

    printf("MCU Erased Successfully.\n");


    /* -------------------------------------------------------------------------- */
    /*                       (4) Program & Verify FLASH                           */
    /* -------------------------------------------------------------------------- */
    printf("Programming Flash...\n");

    size_t p = 0;

    while (p < total_packets)
    {
        uint32_t ba = buffer[p].address;

        if (ba > PIC16F183XX_FLASH_END)
        {
            p++;
            continue;
        }

        uint32_t row_base = (ba >> 1) & ~0x1Fu;

        uint16_t row[32];
        for (int i = 0; i < 32; i++)
        {
            row[i] = 0x3FFF;
        }

        // Consume all consecutive packets belonging to this row
        while (p < total_packets && buffer[p].address <= PIC16F183XX_FLASH_END)
        {
            const HEXPacket_t* pkt = &buffer[p];
            uint32_t dw            = pkt->address >> 1;

            if ((dw & ~0x1Fu) != row_base)
                break;

            uint32_t slot          = dw - row_base;
            const uint8_t* payload = pkt->payload;

            for (int w = 0; w < 8; w++)
            {
                row[slot + (uint32_t)w] =
                    (((uint16_t)payload[w * 2u + 1u] << 8) |
                    payload[w * 2u]) & 0x3FFF;
            }

            p++;
        }

        PIC16F183XX_LOAD_PC_ADDR(row_base);

        for (int latch = 0; latch < 32; latch++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM_INC);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(row[latch]) << 1), 16);
        }

        PIC16F183XX_LOAD_PC_ADDR(row_base);

        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
        sleep_ms(3);
    }

    printf("Verifying Flash...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba > PIC16F183XX_FLASH_END)
        {
            continue;
        }

        const uint8_t* payload = pkt->payload;

        PIC16F183XX_LOAD_PC_ADDR(ba >> 1);
        for (int w = 0; w < 8; w++)
        {
            uint16_t expected = (((uint16_t)payload[w * 2u + 1u] << 8) | payload[w * 2u]) & 0x3FFF;

            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t raw_stream = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            raw_stream = (raw_stream >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            if (raw_stream != expected)
            {
                printf("[PICO] FLASH VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)((ba >> 1) + (uint32_t)w), expected, raw_stream);
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                       (4) Program & Verify EEPROM                          */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");
    bool has_eeprom = false;
    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t ba = buffer[p].address;
        if (ba < PIC16F183XX_EEPROM_BGN)
        {
            continue;
        }

        uint32_t base_dev      = ba >> 1;
        const uint8_t* payload = buffer[p].payload;
        for (int b = 0; b < 8; b++)
        {
            PIC16F183XX_LOAD_PC_ADDR(base_dev + (uint32_t)b);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(payload[b * 2u] & 0xFFu) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN INTERNALLY TIMED PROGRAM
            sleep_ms(3);                                                // TPINT
        }

        has_eeprom = true;
    }

    if (has_eeprom)
    {
        printf("Verifying EEPROM region...\n");
        for (size_t p = 0; p < total_packets; p++)
        {
            uint32_t ba = buffer[p].address;
            if (ba < PIC16F183XX_EEPROM_BGN)
            {
                continue;
            }

            const uint8_t* payload = buffer[p].payload;

            PIC16F183XX_LOAD_PC_ADDR(ba >> 1);
            for (int b = 0; b < 8; b++)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

                SET_PGD_INPUT();
                uint16_t raw_stream = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
                raw_stream = (raw_stream >> 1) & 0x3FFF;
                SET_PGD_OUTPUT();

                uint8_t expected = payload[b * 2u] & 0xFF;
                uint8_t actual = (uint8_t)(raw_stream & 0xFF);

                if (actual != expected)
                {
                    printf("[PICO] EEPROM VERIFY FAIL @ 0x%04X: exp 0x%02X got 0x%02X\n",
                           (unsigned)((ba >> 1) + (uint32_t)b), expected, actual);

                    gpio_put(PIN_MCLR, 1); // Exit LVP
                    return false;
                }
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify USER ID                          */
    /* -------------------------------------------------------------------------- */
    uint16_t uid_words[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
    bool has_uid = false;
    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t ba = buffer[p].address;
        
        // Strictly filter to prevent contamination from non-User ID packets
        if (ba < PIC16F183XX_USER_ID_BGN || ba >= PIC16F183XX_USER_ID_END)
        {
            continue;
        }

        for (int w = 0; w < 4; w++)
        {
            uint32_t uid_ba = PIC16F183XX_USER_ID_BGN + (uint32_t)w * 2u;
            if (uid_ba >= ba && uid_ba < ba + 32u)
            {
                uint32_t off = uid_ba - ba;
                uid_words[w] = (((uint16_t)buffer[p].payload[off + 1u] << 8) | buffer[p].payload[off]) & 0x3FFF;
                has_uid = true;
            }
        }
    }

    if (has_uid)
    {
        printf("Programming User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int w = 0; w < 4; w++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM); // LOAD DATA FOR NVM
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(uid_words[w]) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN PROGRAM INTERNALLY TIMED
            sleep_ms(6);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC
        }

        printf("Verifying User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int w = 0; w < 4; w++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t raw_stream = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            raw_stream = (raw_stream >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t actual = raw_stream & 0x3FFF;
            if (actual != uid_words[w])
            {
                printf("[PICO] CFG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8000u + (uint32_t)w), uid_words[w], actual);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify CONFIG                           */
    /* -------------------------------------------------------------------------- */
    uint16_t cfg_words[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
    bool has_cfg = false;
    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t ba = buffer[p].address;
        
        // Strictly filter to prevent contamination from non-CONFIG packets
        if (ba < PIC16F183XX_CFG_BGN || ba > PIC16F183XX_CFG_END)
        {
            continue;
        }

        for (int c = 0; c < 4; c++)
        {
            uint32_t cfg_ba = PIC16F183XX_CFG_BGN + (uint32_t)c * 2u;
            if (cfg_ba >= ba && cfg_ba < ba + 32u)
            {
                uint32_t off = cfg_ba - ba;
                cfg_words[c] = (((uint16_t)buffer[p].payload[off + 1u] << 8) | buffer[p].payload[off]) & 0x3FFF;
                has_cfg = true;
            }
        }
    }

    if (has_cfg)
    {
        printf("Programming CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int i = 0; i < 7; i++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC to 0x8007
        }

        for (int c = 0; c < 4; c++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM); // LOAD DATA FOR NVM
            SHIFT_OUT_BITS_LSB_FIRST((cfg_words[c] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN PROGRAM INTERNALLY TIMED
            sleep_ms(6);
            
            if (c < 3)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC
            }
        }

        printf("Verifying CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        // Index 0-8007h, 1-8008h, 2-8009h, 3-800Ah
        static const uint16_t cfg_masks[4] = {0x3FFF, 0x2977, 0x3AEF, 0x2003};
        for (int i = 0; i < 7; i++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment Address
        }

        for (int c = 0; c < 4; c++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t raw_stream = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            raw_stream = (raw_stream >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t actual = raw_stream & cfg_masks[c];
            uint16_t expected = cfg_words[c] & cfg_masks[c];

            if (actual != expected)
            {
                printf("[PICO] CONFIG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8007u + (uint32_t)c), expected, actual);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                return false;
            }
        }
    }

    
    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */ 
    gpio_put(PIN_MCLR, 1); // Exit LVP
    return true;
}

/**
 * DESCRIPTION: Programs PIC18FXXK80 MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
*/
bool PROGRAM_PIC18FXXK80(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR); gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);  gpio_set_dir(PIN_PGD, GPIO_OUT);
    gpio_init(PIN_PGC);  gpio_set_dir(PIN_PGC, GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD,  GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC,  GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0); gpio_put(PIN_PGC, 0); gpio_put(PIN_MCLR, 0);
    sleep_ms(10);

    gpio_put(PIN_MCLR, 1);
    sleep_ms(10);

    gpio_put(PIN_MCLR, 0);
    sleep_us(250);

    uint32_t magic = 0x4D434850;
    SET_PGD_OUTPUT();
    for (int i = 0; i < 32; i++)
    {
        bool bit = (magic & 0x80000000u) != 0;
        gpio_put(PIN_PGD, bit);
        sleep_us(1);
        gpio_put(PIN_PGC, 1);
        sleep_us(1);
        gpio_put(PIN_PGC, 0);
        sleep_us(1);
        magic <<= 1;
    }
    sleep_ms(10);

    gpio_put(PIN_MCLR, 1);
    sleep_ms(10);
    
    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_PC_DEV_ID);
    uint8_t lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC) & 0xF0;
    uint8_t hi = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

    uint16_t device_id = (uint16_t)(lo | ((uint16_t)hi << 8));
    printf("Device ID:       0x%04X\n", device_id);

    // DEVICE_ID == VALID??
    // IMPORTANT NOTE: Add the check for additional PIC18FXXK80 mcus if you want to expand support
    if (device_id != PIC18F25K80_DEV_ID && device_id != PIC18F66K80_DEV_ID)
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
    for (int i = 0; i < 3; i++)
    {
        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }
    gpio_put(PIN_PGC, 1);
    sleep_ms(1);
    gpio_put(PIN_PGC, 0);
    sleep_us(100);
    SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

    PIC18FXXK80_ERASE_BLOCK(0x04, 0x01); /* Code &EEPROM block 0 (Table 3-1: 000104h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x02); /* Code &EEPROM block 1 (Table 3-1: 000204h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x04); /* Code &EEPROM block 2 (Table 3-1: 000404h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x08); /* Code &EEPROM block 3 (Table 3-1: 000804h) */
    PIC18FXXK80_ERASE_BLOCK(0x05, 0x00); /* Boot block           (Table 3-1: 000005h) */
    PIC18FXXK80_ERASE_BLOCK(0x02, 0x00); /* Config bits          (Table 3-1: 000002h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x00); /* Data EEPROM          (Table 3-1: 000004h) */


    /* -------------------------------------------------------------------------- */
    /*                         (4) Program & Verify FLASH                         */
    /* -------------------------------------------------------------------------- */
    printf("Programming Flash...\n");

    PIC18FXXK80_CORE_INSTR(0x0000);
    static uint8_t pic_flash_matrix[32768];
    memset(pic_flash_matrix, 0xFF, sizeof(pic_flash_matrix));

    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t ba = buffer[p].address;

        if (ba < PIC18FXXK80_FLASH_END)
        {
            // ba < 0x8000 here, so the writable span is min(32, 32768 - ba) bytes
            size_t n = (size_t)(32768u - ba);
            if (n > 32u)
            {
                n = 32u;
            }
            memcpy(&pic_flash_matrix[ba], buffer[p].payload, n);
        }
    }

    for (uint32_t block_idx = 0; block_idx < 512; block_idx++)
    {
        uint32_t block_addr = block_idx * 64u;
        uint8_t *row_ptr = &pic_flash_matrix[block_addr];

        bool has_active_data = false;
        for (int b = 0; b < 64; b++)
        {
            if (row_ptr[b] != 0xFF)
            {
                has_active_data = true;
                break;
            }
        }
        
        if (!has_active_data) continue;

        PIC18FXXK80_CORE_INSTR(0x8E7F);
        PIC18FXXK80_CORE_INSTR(0x9C7F);
        PIC18FXXK80_CORE_INSTR(0x847F);

        PIC18FXXK80_CORE_INSTR(0x0E00 | ((block_addr >> 16) & 0x3F));
        PIC18FXXK80_CORE_INSTR(0x6EF8);
        PIC18FXXK80_CORE_INSTR(0x0E00 | ((block_addr >> 8) & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF7);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (block_addr & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF6);

        for (int i = 0; i < 31; i++)
        {
            uint8_t t_lsb = row_ptr[i * 2];
            uint8_t t_msb = row_ptr[(i * 2) + 1];
            uint16_t t_word_data = (uint16_t)(t_lsb | ((uint16_t)t_msb << 8u));
            PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE_POST2, t_word_data);
        }

        uint8_t last_lsb = row_ptr[31 * 2];
        uint8_t last_msb = row_ptr[(31 * 2) + 1];
        uint16_t last_word = (uint16_t)(last_lsb | ((uint16_t)last_msb << 8u));
        PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_START_PROG, last_word);

        gpio_put(PIN_PGD, 0);
        for (int i = 0; i < 3; i++)
        {
            gpio_put(PIN_PGC, 1);
            gpio_put(PIN_PGC, 0);
        }
            gpio_put(PIN_PGD, 0);
        
        gpio_put(PIN_PGC, 1);
        sleep_ms(5);
        gpio_put(PIN_PGC, 0);
        sleep_us(100);
        SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

        uint8_t PROGRAM_STATUS_lo = 0;
        absolute_time_t timeout_start = get_absolute_time();
        do {
            if (absolute_time_diff_us(timeout_start, get_absolute_time()) > 1000000LL)
            {
                printf("[-] Flash write timeout at block index %d!\n", (int)block_idx);
                break;
            }
            PIC18FXXK80_CORE_INSTR(0x507F);
            PIC18FXXK80_CORE_INSTR(0x6EF5);
            PIC18FXXK80_CORE_INSTR(0x0000);
            PROGRAM_STATUS_lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
            PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
        } while (PROGRAM_STATUS_lo & (1 << 1));

        PIC18FXXK80_CORE_INSTR(0x947F);
    }

    printf("Verifying Flash...\n");
    for (uint32_t block_idx = 0; block_idx < 512; block_idx++)
    {
        uint32_t block_addr = block_idx * 64u;
        uint8_t *row_ptr = &pic_flash_matrix[block_addr];

        bool has_active_data = false;
        for (int b = 0; b < 64; b++)
        {
            if (row_ptr[b] != 0xFF)
            {
                has_active_data = true;
                break;
            }
        }
        if (!has_active_data) continue;

        PIC18FXXK80_CORE_INSTR(0x0E00 | ((block_addr >> 16) & 0x3F));
        PIC18FXXK80_CORE_INSTR(0x6EF8);
        PIC18FXXK80_CORE_INSTR(0x0E00 | ((block_addr >> 8) & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF7);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (block_addr & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF6);

        for (int i = 0; i < 32; i++)
        {
            uint8_t read_lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint8_t read_hi = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint16_t actual_word = (uint16_t)(read_lo | ((uint16_t)read_hi << 8u));

            uint8_t exp_lsb = row_ptr[i * 2];
            uint8_t exp_msb = row_ptr[(i * 2) + 1];
            uint16_t expected_word = (uint16_t)(exp_lsb | ((uint16_t)exp_msb << 8u));

            if (actual_word != expected_word)
            {
                printf("[-] PFM VERIFY ERROR @ 0x%06X: Expected 0x%04X, Read 0x%04X\n",
                    block_addr + (i * 2), expected_word, actual_word);
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

    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t raw_address = buffer[p].address;

        if (raw_address < PIC18FXXK80_EEPROM_BGN || raw_address >= PIC18FXXK80_EEPROM_END)
        {
            continue;
        }

        uint32_t base_eeprom_addr = raw_address - PIC18FXXK80_EEPROM_BGN;
        const uint8_t* payload    = buffer[p].payload;

        for (int i = 0; i < 32; i++)
        {
            uint32_t current_addr = base_eeprom_addr + (uint32_t)i;

            if (current_addr >= 0x400u)
                break;

            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)(current_addr & 0x00FFu));
            PIC18FXXK80_CORE_INSTR(0x6E74);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)((current_addr >> 8) & 0x03u));
            PIC18FXXK80_CORE_INSTR(0x6E75);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)payload[i]);
            PIC18FXXK80_CORE_INSTR(0x6E73);
            PIC18FXXK80_CORE_INSTR(0x847F);
            PIC18FXXK80_CORE_INSTR(0x827F);

            uint8_t PROGRAM_STATUS_lo;
            do {
                PIC18FXXK80_CORE_INSTR(0x507F);
                PIC18FXXK80_CORE_INSTR(0x6EF5);
                PIC18FXXK80_CORE_INSTR(0x0000);

                PROGRAM_STATUS_lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
                (void)PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
            } while (PROGRAM_STATUS_lo & (1u << 1));

            gpio_put(PIN_PGC, 0);
            sleep_us(100);

            PIC18FXXK80_CORE_INSTR(0x947F);
        }
    }

    printf("Verifying EEPROM...\n");

    static uint8_t eeprom_layout_matrix[1024];
    bool eeprom_cell_active[1024];

    memset(eeprom_layout_matrix, 0xFF, sizeof(eeprom_layout_matrix));
    memset(eeprom_cell_active, false, sizeof(eeprom_cell_active));

    for (size_t p = 0; p < total_packets; p++)
    {
        uint32_t a = buffer[p].address;
        if (a >= PIC18FXXK80_EEPROM_BGN && a < PIC18FXXK80_EEPROM_END)
        {
            uint32_t local_addr    = a - PIC18FXXK80_EEPROM_BGN;
            const uint8_t* payload = buffer[p].payload;

            for (int i = 0; i < 32; i++)
            {
                if ((local_addr + i) < 1024)
                {
                    eeprom_layout_matrix[local_addr + i] = payload[i];
                    eeprom_cell_active[local_addr + i] = true;
                }
            }
        }
    }

    PIC18FXXK80_CORE_INSTR(0x9E7F);
    PIC18FXXK80_CORE_INSTR(0x9C7F);

    for (uint32_t addr = 0; addr < 1024; addr++)
    {
        if (!eeprom_cell_active[addr]) continue;

        PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)(addr & 0xFFu));
        PIC18FXXK80_CORE_INSTR(0x6E74);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)((addr >> 8) & 0x03u));
        PIC18FXXK80_CORE_INSTR(0x6E75);

        PIC18FXXK80_CORE_INSTR(0x807F);

        PIC18FXXK80_CORE_INSTR(0x5073);
        PIC18FXXK80_CORE_INSTR(0x6EF5);
        PIC18FXXK80_CORE_INSTR(0x0000);

        uint8_t read_val = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_SHIFT_OUT_TABLAT);
        PIC18FXXK80_CMD_READ_BYTE(0x2);

        if (read_val != eeprom_layout_matrix[addr])
        {
            printf("[-] EEPROM VERIFY ERROR @ 0x%04X: Expected 0x%02X, Read 0x%02X\n",
                   (unsigned int)addr, eeprom_layout_matrix[addr], read_val);
            gpio_put(PIN_MCLR, 0);
            return false;
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                         (6) Program & Verify USER ID                       */
    /* -------------------------------------------------------------------------- */
    printf("Programming User ID...\n");
    uint16_t uid_words[4];
    for (int i = 0; i < 4; i++)
    {
        uid_words[i] = 0xFFFFu;
    }
    bool has_uid = false;

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        for (int w = 0; w < 4; w++)
        {
            uint32_t uid_ba = PIC18FXXK80_USER_ID_BGN + (uint32_t)w * 2u;
            if (uid_ba >= ba && uid_ba < ba + 32u)
            {
                uint32_t t_off = uid_ba - ba;
                uid_words[w] = ((uint16_t)pkt->payload[t_off + 1u] << 8u)
                             | pkt->payload[t_off];
                has_uid = true;
            }
        }
    }

    if (has_uid)
    {
        PIC18FXXK80_CORE_INSTR(0x8E7F);
        PIC18FXXK80_CORE_INSTR(0x9C7F);
        PIC18FXXK80_SET_TBLPTR(PIC18FXXK80_USER_ID_BGN);

        for (int i = 0; i < 3; i++)
        {
            PIC18FXXK80_CMD_WRITE_WORD(0x0D, uid_words[i]);
        }
        PIC18FXXK80_CMD_WRITE_WORD(0x0F, uid_words[3]);

        gpio_put(PIN_PGD, 0);
        for (int i = 0; i < 3; i++)
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
        for (int i = 0; i < 4; i++)
        {
            uint8_t lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint8_t hi = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

            uint16_t actual_uid = (uint16_t)(((uint16_t)hi << 8u) | lo);
            uint32_t uid_dev_addr = PIC18FXXK80_USER_ID_BGN + (i * 2);

            if (actual_uid != uid_words[i])
            {
                printf("[-] USER ID VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                    uid_dev_addr, uid_words[i], actual_uid);
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
    
    uint16_t cfg_words[7] = 
    {
        0x085D,
        0x7F7F,
        0x891F,
        0x0F91,
        0x0FC0,
        0xF0E0,
        0x0040
    };
    bool has_cfg = false;

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        for (int w = 0; w < 7; w++)
        {
            uint32_t cfg_ba = PIC18FXXK80_CONFIG_BGN + (uint32_t)w * 2u;
            if (cfg_ba >= ba && cfg_ba < ba + 32u)
            {
                uint32_t t_off = cfg_ba - ba;
                cfg_words[w] = ((uint16_t)pkt->payload[t_off + 1u] << 8u)
                             | pkt->payload[t_off];

                // printf("[PICO] Staging CONFIG Word %d at 0x%06X: 0x%04X\n",
                //        w + 1, cfg_ba, cfg_words[w]);
                has_cfg = true;
            }
        }
    }

    if (has_cfg)
    {
        for (int w = 0; w < 7; w++)
        {
            if (cfg_words[w] == 0xFFFFu) {
                continue;
            }

            uint32_t base_addr = PIC18FXXK80_CONFIG_BGN + ((uint32_t)w * 2u);
            uint8_t t_lsb = (uint8_t)(cfg_words[w] & 0x00FFu);
            uint8_t t_msb = (uint8_t)(cfg_words[w] >> 8u);

            PIC18FXXK80_CORE_INSTR(0x8E7F);
            PIC18FXXK80_CORE_INSTR(0x8C7F);
            PIC18FXXK80_SET_TBLPTR(base_addr);  

            uint16_t even_payload = (uint16_t)t_lsb;
            PIC18FXXK80_CMD_WRITE_WORD(0xF, even_payload);

            gpio_put(PIN_PGD, 0);
            for (int i = 0; i < 3; i++) {
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
            PIC18FXXK80_SET_TBLPTR(base_addr + 1u);

            uint16_t odd_payload = ((uint16_t)t_msb << 8u);
            PIC18FXXK80_CMD_WRITE_WORD(0xF, odd_payload);

            gpio_put(PIN_PGD, 0);
            for (int i = 0; i < 3; i++) {
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

    const uint16_t cfg_masks[7] = {
        0xDF5D,
        0x7F7F,
        0x8F00,
        0x0091,
        0xC00F,
        0xE00F,
        0x400F
    };

    for (int w = 0; w < 7; w++)
    {
        uint8_t t_lo = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
        uint8_t t_hi = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);

        uint16_t actual_cfg = (uint16_t)(((uint16_t)t_hi << 8u) | t_lo);
        uint32_t cfg_dev_addr = PIC18FXXK80_CONFIG_BGN + (w * 2);

        uint16_t masked_expected = cfg_words[w] & cfg_masks[w];
        uint16_t masked_actual = actual_cfg & cfg_masks[w];

        if (cfg_words[w] != 0xFFFFu && masked_actual != masked_expected)
        {
            printf("[-] CONFIG VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                   cfg_dev_addr, cfg_words[w], actual_cfg);
            gpio_put(PIN_MCLR, 0);
            sleep_ms(1);
            return false;
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */ 
    // LVP Exit
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
bool PROGRAM_PIC18F2XK83(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR); gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);  gpio_set_dir(PIN_PGD,  GPIO_OUT);
    gpio_init(PIN_PGC);  gpio_set_dir(PIN_PGC,  GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD,  GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC,  GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD, 0); gpio_put(PIN_PGC, 0); gpio_put(PIN_MCLR, 1);
    sleep_ms(5);

    // Pull MCLR Low
    gpio_put(PIN_MCLR, 0);
    sleep_ms(1);

    uint32_t magic = 0x4D434850;

    SET_PGD_OUTPUT();
    for (int i = 0; i < 32; i++)
    {
        bool bit = (magic & 0x80000000u) != 0;
        gpio_put(PIN_PGD, bit);
        sleep_us(1);
        gpio_put(PIN_PGC, 1);
        sleep_us(1);
        gpio_put(PIN_PGC, 0);
        sleep_us(1);
        magic <<= 1;
    }
    sleep_ms(2);


    /* -------------------------------------------------------------------------- */
    /*                         (2) Read Device ID                                 */
    /* -------------------------------------------------------------------------- */
    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_DEV_ID);
    uint16_t device_id = PIC18F2XK83_READ_WORD_NVM_POST_INC();
    printf("Device ID:       0x%04X\n", device_id);

    // DEVICE_ID == VALID??
    // IMPORTANT NOTE: Add the check for additional PIC18F2XK83 mcus if you want to expand support
    if (device_id != PIC18F25K83_DEV_ID)
    {
        printf("DEVICE ID not a PIC18F25K83. Check connections or add additional MCU support.\n");
        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Bulk Erase MCU                                 */
    /* -------------------------------------------------------------------------- */
    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_CONFIG);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BULK_ERASE, 8);
    sleep_ms(26);                                       // TERAB

    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_EEPROM);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BULK_ERASE, 8);
    sleep_ms(26);                                       // TERAB


    /* -------------------------------------------------------------------------- */
    /*                (4) Program FLASH Memory (64-Word Rows)                     */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH...\n");

    for (size_t p = 0; p < total_packets; )
    {
        uint32_t ba = buffer[p].address;
        if (ba > PIC18F2XK83_FLASH_END)
        {
            p++;                        // Safely advance past out-of-bounds packet
            continue;
        }

        uint32_t row_base               = ba & ~0x7Fu;
        uint16_t row[64];
        bool     row_needs_programming  = false;

        for (uint32_t i = 0; i < 64u; i++)
        {
            row[i] = 0xFFFFu;
        }

        // Consume all consecutive packets belonging to this row
        while (p < total_packets)
        {
            const HEXPacket_t* pkt   = &buffer[p];
            uint32_t packet_addr     = pkt->address;

            if (packet_addr > PIC18F2XK83_FLASH_END || (packet_addr & ~0x7Fu) != row_base)
            {
                break;                  // Do NOT increment p; let next outer iteration process it
            }

            uint32_t offset = packet_addr - row_base;
            if ((offset & 0x0Fu) != 0u)
            {
                p++;
                continue;
            }

            uint32_t slot = offset >> 1u;
            if ((slot + 8u) > 64u)
            {
                p++;
                continue;
            }

            const uint8_t* payload = pkt->payload;
            for (uint32_t w = 0; w < 8u; w++)
            {
                uint16_t word = (uint16_t)(((uint16_t)payload[w * 2u + 1u] << 8u)
                                          | payload[w * 2u]);
                row[slot + w] = word;

                if (word != 0xFFFFu)
                {
                    row_needs_programming = true;
                }
            }

            p++;                        // Correctly advance inside the processing block
        }

        if (!row_needs_programming)
        {
            continue;                   // p is already pointing to the next valid start packet
        }

        PIC18F2XK83_LOAD_PC_ADDR(row_base);
        for (uint32_t latch = 0; latch < 64u; latch++)
        {
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM_INC, 8);
            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)row[latch]) << 1u, 24);
        }

        PIC18F2XK83_LOAD_PC_ADDR(row_base);
        SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
        sleep_ms(4);                                    // TPINT
    }


    /* -------------------------------------------------------------------------- */
    /*                       (5) Verify FLASH Memory                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying FLASH...\n");

    for (size_t p = 0; p < total_packets; )
    {
        uint32_t ba = buffer[p].address;
        if (ba > PIC18F2XK83_FLASH_END)
        {
            p++;
            continue;
        }

        uint32_t row_base = ba & ~0x7Fu;
        uint16_t expected_row[64];

        for (uint32_t i = 0; i < 64u; i++)
        {
            expected_row[i] = 0xFFFFu;
        }

        while (p < total_packets)
        {
            const HEXPacket_t* pkt   = &buffer[p];
            uint32_t packet_addr     = pkt->address;

            if (packet_addr > PIC18F2XK83_FLASH_END || (packet_addr & ~0x7Fu) != row_base)
            {
                break;                  // Do NOT increment p
            }

            uint32_t offset = packet_addr - row_base;
            if ((offset & 0x0Fu) != 0u)
            {
                p++;
                continue;
            }

            uint32_t slot = offset >> 1u;
            if ((slot + 8u) > 64u)
            {
                p++;
                continue;
            }

            const uint8_t* payload = pkt->payload;
            for (uint32_t w = 0; w < 8u; w++)
            {
                expected_row[slot + w] = (uint16_t)(((uint16_t)payload[w * 2u + 1u] << 8u)
                                                   | payload[w * 2u]);
            }

            p++;
        }

        PIC18F2XK83_LOAD_PC_ADDR(row_base);
        for (uint32_t latch = 0; latch < 64u; latch++)
        {
            uint16_t actual_word = PIC18F2XK83_READ_WORD_NVM_POST_INC();

            if (actual_word != expected_row[latch])
            {
                printf("[-] FLASH VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)(row_base + (latch << 1u)), expected_row[latch], actual_word);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*             (6) Program Data EEPROM (Granular Byte Skipping)               */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18F2XK83_EEPROM_BGN || ba >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t eeprom_offset = ba - PIC18F2XK83_EEPROM_BGN;
        uint32_t base_dev      = PIC18F2XK83_EEPROM_BGN + eeprom_offset;
        const uint8_t* payload = pkt->payload;

        for (uint32_t b = 0; b < 8u; b++)
        {
            uint32_t data_byte = (uint32_t)(payload[b * 2u] & 0xFFu);
            if (data_byte == 0xFFu)
            {
                continue;               // Leave erased cells untouched
            }

            PIC18F2XK83_LOAD_PC_ADDR(base_dev + (b << 1u));
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);

            SHIFT_OUT_BITS_MSB_FIRST(data_byte << 1u, 24);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);                                // TPINT
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                        (7) Verify Data EEPROM                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying EEPROM...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18F2XK83_EEPROM_BGN || ba >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t eeprom_offset = ba - PIC18F2XK83_EEPROM_BGN;
        uint32_t base_dev      = PIC18F2XK83_EEPROM_BGN + eeprom_offset;
        const uint8_t* payload = pkt->payload;

        for (uint32_t b = 0; b < 8u; b++)
        {
            uint8_t expected_byte = payload[b * 2u];
            PIC18F2XK83_LOAD_PC_ADDR(base_dev + (b << 1u));

            uint16_t actual_word = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            uint8_t  actual_byte = (uint8_t)(actual_word & 0xFFu);

            if (actual_byte != expected_byte)
            {
                printf("[-] EEPROM VERIFY ERROR @ 0x%06X: Expected 0x%02X, Got 0x%02X\n",
                       (unsigned)(base_dev + (b << 1u)), expected_byte, actual_byte);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                    (8) Program & Verify USER ID                            */
    /* -------------------------------------------------------------------------- */
    printf("Programming User IDs...\n");

    uint16_t uid_words[8];
    for (int i = 0; i < 8; i++)
    {
        uid_words[i] = 0xFFFFu;
    }
    bool has_uid = false;

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        for (int w = 0; w < 8; w++)
        {
            uint32_t uid_ba = PIC18F2XK83_USER_ID_BGN + (uint32_t)w * 2u;
            if (uid_ba >= ba && uid_ba < ba + 32u)
            {
                uint32_t t_off = uid_ba - ba;
                uid_words[w]   = (uint16_t)(((uint16_t)pkt->payload[t_off + 1u] << 8u)
                                           | pkt->payload[t_off]);
                has_uid = true;
            }
        }
    }

    if (has_uid)
    {
        for (int w = 0; w < 8; w++)
        {
            uint32_t uid_dev_addr = PIC18F2XK83_USER_ID_BGN + ((uint32_t)w * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(uid_dev_addr);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)uid_words[w]) << 1u, 24);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);                                // TPINT
        }

        printf("Verifying User IDs...\n");
        for (int w = 0; w < 8; w++)
        {
            uint32_t uid_dev_addr = PIC18F2XK83_USER_ID_BGN + ((uint32_t)w * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(uid_dev_addr);

            uint16_t actual = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            if (actual != uid_words[w])
            {
                printf("[-] USER ID VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)uid_dev_addr, uid_words[w], actual);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                   (9) Program & Verify CONFIG WORDS                        */
    /* -------------------------------------------------------------------------- */
    printf("Programming CONFIG Words...\n");

    uint16_t config_words[5];
    for (int i = 0; i < 5; i++)
    {
        config_words[i] = 0xFFFFu;
    }
    bool has_config = false;

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        for (int w = 0; w < 5; w++)
        {
            uint32_t config_ba = PIC18F2XK83_CFG_BGN + (uint32_t)w * 2u;
            if (config_ba >= ba && config_ba < ba + 32u)
            {
                uint32_t c_off   = config_ba - ba;
                config_words[w]  = (uint16_t)(((uint16_t)pkt->payload[c_off + 1u] << 8u)
                                             | pkt->payload[c_off]);
                has_config = true;
            }
        }
    }

    if (has_config)
    {
        for (int w = 0; w < 5; w++)
        {
            uint32_t config_dev_addr = PIC18F2XK83_CFG_BGN + ((uint32_t)w * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(config_dev_addr);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)config_words[w]) << 1u, 24);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);                                // TPINT
        }

        printf("Verifying CONFIG Words...\n");
        for (int w = 0; w < 5; w++)
        {
            uint32_t cfg_dev_addr = PIC18F2XK83_CFG_BGN + ((uint32_t)w * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(cfg_dev_addr);

            uint16_t actual = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            if (actual != config_words[w])
            {
                printf("[-] CONFIG VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)cfg_dev_addr, config_words[w], actual);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                sleep_ms(1);
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                               SUCCESS                                      */
    /* -------------------------------------------------------------------------- */
    // LVP Exit
    gpio_put(PIN_MCLR, 1);
    sleep_ms(1);

    return true;
}

/**
 * DESCRIPTION: Programs PIC18FXXQ8X MCU
 * INPUT:       HEX Packet Staging Buffer, # of packets
 * RETURN:      TRUE=SUCCESS, FALSE=FAILURE
*/
bool PROGRAM_PIC18FXXQ8X(const HEXPacket_t* buffer, size_t total_packets)
{
    /* -------------------------------------------------------------------------- */
    /*                             (1) LVP Entry                                  */
    /* -------------------------------------------------------------------------- */
    gpio_init(PIN_MCLR); gpio_set_dir(PIN_MCLR, GPIO_OUT);
    gpio_init(PIN_PGD);  gpio_set_dir(PIN_PGD,  GPIO_OUT);
    gpio_init(PIN_PGC);  gpio_set_dir(PIN_PGC,  GPIO_OUT);

    gpio_set_drive_strength(PIN_MCLR, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGD,  GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PIN_PGC,  GPIO_DRIVE_STRENGTH_12MA);

    gpio_put(PIN_PGD,  0);
    gpio_put(PIN_PGC,  0);
    gpio_put(PIN_MCLR, 0);

    sleep_ms(50);

    uint32_t magic = 0x4D434850;

    SET_PGD_OUTPUT();

    for (int i = 0; i < 32; i++)
    {
        bool bit = (magic & 0x80000000u) != 0;

        gpio_put(PIN_PGD, bit);
        sleep_us(1);

        gpio_put(PIN_PGC, 1);
        sleep_us(1);

        gpio_put(PIN_PGC, 0);
        sleep_us(1);

        magic <<= 1;
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

    uint32_t raw = SHIFT_IN_BITS_MSB_FIRST(24);

    SET_PGD_OUTPUT();
    uint16_t device_id = (uint16_t)((raw >> 1) & 0xFFFF);
    printf("[PICO] Device ID: 0x%04X\n", device_id);

    if (device_id == 0xFFFF || device_id == 0x0000)
    {
        printf("[PICO] ERROR: Invalid Device ID!\n");

        gpio_put(PIN_MCLR, 1); // Exit LVP
        sleep_us(1000);

        return false;
    }


    /* -------------------------------------------------------------------------- */
    /*                         (3) Erase All                                      */
    /* -------------------------------------------------------------------------- */
    printf("[PICO] Erasing...\n");
    PIC18FXXQ8X_LOAD_PC_ADDR(0x000000);
    sleep_us(2);

    SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_BULK_ERASE, 8);
    sleep_us(2);

    PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(0xF);
    sleep_ms(20);                                       // TERAB


    /* -------------------------------------------------------------------------- */
    /*               (4) Program & Verify - FLASH & USER ID                       */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH and USER IDs...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        PIC18FXXQ8X_LOAD_PC_ADDR(ba);
        sleep_us(2);

        const uint8_t* payload = pkt->payload;
        for (int w = 0; w < 8; w++)
        {
            uint16_t word_to_program = (uint16_t)(((uint16_t)payload[w * 2u + 1u] << 8)
                                                 | payload[w * 2u]);

            if (word_to_program == 0xFFFF)
            {
                SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_INC_ADDR, 8);
                sleep_us(2);
                continue;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA_INC, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(word_to_program);
            sleep_us(75);                               // TPINT
        }
    }

    printf("Verifying FLASH and USER IDs...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if ((ba > PIC18FXXQ8X_FLASH_END && ba < PIC18FXXQ8X_USER_ID_BGN) || ba > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        bool address_needs_reload = true;

        const uint8_t* payload = pkt->payload;
        for (int w = 0; w < 8; w++)
        {
            uint16_t expected_word = (uint16_t)(((uint16_t)payload[w * 2u + 1u] << 8)
                                               | payload[w * 2u]);

            if (expected_word == 0xFFFF)
            {
                address_needs_reload = true;
                continue;
            }

            if (address_needs_reload)
            {
                PIC18FXXQ8X_LOAD_PC_ADDR(ba + ((uint32_t)w * 2u));
                sleep_us(2);
                address_needs_reload = false;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t read_raw = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint16_t actual_word = (uint16_t)((read_raw >> 1) & 0xFFFF);

            if (actual_word != expected_word)
            {
                printf("[PICO] VERIFY FAILURE (FLASH/UID): Address 0x%06X Mismatch! Expected 0x%04X, Read 0x%04X\n",
                       (unsigned)(ba + ((uint32_t)w * 2u)), expected_word, actual_word);
                gpio_put(PIN_MCLR, 1); // Exit LVP
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                 (5) Program and Verify Data EEPROM                         */
    /* -------------------------------------------------------------------------- */
    printf("Programming EEPROM...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18FXXQ8X_EEPROM_BGN || ba > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        const uint8_t* payload = pkt->payload;
        for (int b = 0; b < 16; b++)
        {
            uint32_t exact_byte_address = ba + (uint32_t)b;

            if (exact_byte_address > PIC18FXXQ8X_EEPROM_END)
            {
                break;
            }

            uint8_t byte_to_program = payload[b];

            if (byte_to_program == 0xFF)
            {
                continue;               // Leave erased cells untouched
            }

            // Force exact address targeting for every single byte modification pass
            PIC18FXXQ8X_LOAD_PC_ADDR(exact_byte_address);
            sleep_us(2);

            // Command 0xC0: Freezes internal hardware PC (J=0), preventing address drift
            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(byte_to_program);
            sleep_ms(11);                               // TPINT
        }
    }

    printf("Verifying EEPROM...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18FXXQ8X_EEPROM_BGN || ba > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        bool address_needs_reload = true;

        const uint8_t* payload = pkt->payload;
        for (int b = 0; b < 16; b++)
        {
            uint32_t exact_byte_address = ba + (uint32_t)b;

            if (exact_byte_address > PIC18FXXQ8X_EEPROM_END)
            {
                break;
            }

            uint8_t expected_byte = payload[b];

            if (expected_byte == 0xFF)
            {
                address_needs_reload = true;
                continue;
            }

            if (address_needs_reload)
            {
                PIC18FXXQ8X_LOAD_PC_ADDR(exact_byte_address);
                sleep_us(2);
                address_needs_reload = false;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t read_raw = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint8_t actual_byte = (uint8_t)((read_raw >> 1) & 0xFF);

            if (actual_byte != expected_byte)
            {
                printf("[PICO] VERIFY FAILURE (EEPROM): Address 0x%06X Mismatch! Expected 0x%02X, Read 0x%02X\n",
                       (unsigned)exact_byte_address, expected_byte, actual_byte);
                gpio_put(PIN_MCLR, 1); // Exit LVP
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*              (6) Program and Verify CONFIGURATION BYTES                    */
    /* -------------------------------------------------------------------------- */
    printf("Programming CONFIG Fuses...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18FXXQ8X_CONFIG_BGN || ba > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t* payload = pkt->payload;
        for (int b = 0; b < 16; b++)
        {
            uint32_t exact_byte_address = ba + (uint32_t)b;

            if (exact_byte_address > PIC18FXXQ8X_CONFIG_END)
            {
                break;
            }

            uint8_t byte_to_program = payload[b];

            if (byte_to_program == 0xFF)
            {
                continue;
            }

            // Force exact address targeting for every single fuse byte modification pass
            PIC18FXXQ8X_LOAD_PC_ADDR(exact_byte_address);
            sleep_us(2);

            // Command 0xC0: Freezes internal hardware PC (J=0), preventing address drift
            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(byte_to_program);
            sleep_ms(11);                               // TPINT
        }
    }

    printf("Verifying CONFIG fuses...\n");

    for (size_t p = 0; p < total_packets; p++)
    {
        const HEXPacket_t* pkt = &buffer[p];
        uint32_t ba            = pkt->address;

        if (ba < PIC18FXXQ8X_CONFIG_BGN || ba > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t* payload = pkt->payload;
        for (int b = 0; b < 16; b++)
        {
            uint32_t exact_byte_address = ba + (uint32_t)b;

            if (exact_byte_address > PIC18FXXQ8X_CONFIG_END)
            {
                break;
            }

            uint8_t expected_byte = payload[b];

            if (expected_byte == 0xFF)
            {
                continue;
            }

            // Explicitly reload the exact verification address
            PIC18FXXQ8X_LOAD_PC_ADDR(exact_byte_address);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t read_raw = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint8_t actual_byte = (uint8_t)((read_raw >> 1) & 0xFF);

            if (actual_byte != expected_byte)
            {
                printf("[PICO] VERIFY FAILURE (CONFIG): Address 0x%06X Mismatch! Expected 0x%02X, Read 0x%02X\n",
                       (unsigned)exact_byte_address, expected_byte, actual_byte);
                gpio_put(PIN_MCLR, 1); // Exit LVP
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