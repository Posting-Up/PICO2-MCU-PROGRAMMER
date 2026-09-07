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

    for (int I = 0; I < COUNT; I++)
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

    for (int I = 0; I < COUNT; I++)
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

    for (int I = 0; I < COUNT; I++)
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
    for (uint8_t I = 0; I < 4; I++)
    {
        gpio_put(PIN_PGD, (BITS & 1u) != 0u);
        BITS >>= 1;

        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }

    SET_PGD_INPUT();

    for (int I = 0; I < 8; I++)
    {
        gpio_put(PIN_PGC, 1);
        gpio_put(PIN_PGC, 0);
    }

    /* Read 8-bits, LSB first */
    uint8_t B = (uint8_t)SHIFT_IN_BITS_LSB_FIRST(8);
    SET_PGD_OUTPUT();

    return B;
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
static void PIC18FXXK80_ERASE_BLOCK(uint8_t REG04, uint8_t REG05)
{
    /* 1. Base Pointer Generation -> TBLPTR = 0x3C0004 */
    PIC18FXXK80_CORE_INSTR(0x0E3C); /* MOVLW 3Ch */
    PIC18FXXK80_CORE_INSTR(0x6EF8); /* MOVWF TBLPTRU */
    PIC18FXXK80_CORE_INSTR(0x0E00); /* MOVLW 00h */
    PIC18FXXK80_CORE_INSTR(0x6EF7); /* MOVWF TBLPTRH */
    PIC18FXXK80_CORE_INSTR(0x0E04); /* MOVLW 04h */
    PIC18FXXK80_CORE_INSTR(0x6EF6); /* MOVWF TBLPTRL */

    /* Load Register 04h Latch Data (Command 0xC = Table Write) */
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)REG04 | ((uint16_t)REG04 << 8));

    /* 2. Advance Latch Register -> TBLPTR = 0x3C0005 */
    PIC18FXXK80_CORE_INSTR(0x0E05); /* MOVLW 05h */
    PIC18FXXK80_CORE_INSTR(0x6EF6); /* MOVWF TBLPTRL */

    /* Load Register 05h Latch Data (Command 0xC = Table Write) */
    PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE, (uint16_t)REG05 | ((uint16_t)REG05 << 8));

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
bool PROGRAM_PIC12F157X(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
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

    for (int I = 0; I < 6; I++)
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
    for (int I = 0; I < MAX_MEM_SIZE_WORDS; I++)
    {
        FLASH_IMAGE[I] = 0x3FFF;
    }

    uint32_t MAX_WORD_ADDR = 0;
    bool HAS_PROGRAM = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA >= PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
        for (uint32_t OFF = 0; OFF < 16; OFF += 2)
        {
            uint32_t WORD_ADDR = (BA + OFF) / 2;
            if (WORD_ADDR < MAX_MEM_SIZE_WORDS)
            {
                FLASH_IMAGE[WORD_ADDR] = (((uint16_t)PAYLOAD[OFF +1] << 8) | PAYLOAD[OFF]) & 0x3FFF;

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
            sleep_ms(5);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR);
            sleep_us(2);
        }
        
        printf("Verifying FLASH...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR);
            sleep_us(5);

        for (uint32_t ADDR = 0; ADDR < MAX_WORD_ADDR; ADDR++)
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
                printf("FLASH VERIFICATION FAILED @ 0x%04: exp 0x%04 got 0x%04\n", (unsigned)ADDR, FLASH_IMAGE[ADDR], ACTUAL);
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

    for (size_t P_IDX = 0; P_IDX < TOTAL_PACKETS; P_IDX++)
    {
        const HEXPacket_t* PKT = &BUFFER[P_IDX];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC12F157X_FLASH_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 4; W++)
        {
            uint32_t UID_BA = PIC12F157X_FLASH_END + ((uint32_t)W * 2u); // 0x10000 maps to 0x8000 User ID
            if (UID_BA >= BA && UID_BA < BA + 16u)
            {
                uint32_t OFF = UID_BA - BA;
                UID_WORDS[W] = (((uint16_t)PAYLOAD[OFF + 1u] << 8) | PAYLOAD[OFF]) & 0x3FFF;
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

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM);
            sleep_us(2);
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(UID_WORDS[W] << 1), 16);

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT);
            sleep_ms(5); // delay = TPINT

            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); // Move PC to next slot
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
                       (unsigned)(0x8000u + W), UID_WORDS[W], ACTUAL); // Direct address logging
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

    for (size_t P_IDX = 0; P_IDX < TOTAL_PACKETS; P_IDX++)
    {
        uint32_t BA = BUFFER[P_IDX].ADDRESS;
        if (BA < PIC12F157X_FLASH_END) continue;

        for (int W = 0; W < 2; W++)
        {
            uint32_t CONFIG_BA = PIC12F157X_CONFIG_BEGIN + ((uint32_t)W * 2u);
            if (CONFIG_BA >= BA && CONFIG_BA < BA + 32u)
            {
                uint32_t OFF = CONFIG_BA - BA;
                CONFIG_WORDS[W] = (((uint16_t)BUFFER[P_IDX].PAYLOAD[OFF + 1u] << 8)
                                  | BUFFER[P_IDX].PAYLOAD[OFF]) & 0x3FFF;
                if (W == 0) CONFIG_WORDS[W] |= 0x0800u; // Code Protection OFF
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

        for (int W = 0; W < 2; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_DATA_PROG_MEM); sleep_us(2); // Load Data For Program Memory
            SHIFT_OUT_BITS_LSB_FIRST((uint32_t)(CONFIG_WORDS[W] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_BGN_PROG_INT); sleep_ms(5);      // Begin Internally Timed Programming
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); S_PC++;               // Move pointer
        }

        printf("Verifying CONFIG Bits...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_RESET_ADDR); sleep_us(5);  // Reset pointer for read pass
        PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_LOAD_CONFIG); sleep_us(2); // Load Configuration
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);
        S_PC = 0x8000u;

        while (S_PC < 0x8007u) { PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_INC_ADDR); S_PC++; }

        for (int W = 0; W < 2; W++)
        {
            SET_PGD_OUTPUT();
            PIC_12_16_SEND_6_BIT_CMD(PIC12F157X_CMD_READ_DATA_PROG_MEM); sleep_us(2); // Read Data From Program Memory
            SET_PGD_INPUT();
            
            uint32_t RAW_STREAM = SHIFT_IN_BITS_LSB_FIRST(16);
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = (uint16_t)((RAW_STREAM >> 1) & 0x3FFF);
            uint16_t CONFIG_EXPECTED = CONFIG_WORDS[W];

            // Clear datasheet unimplemented bits (U-1) to avoid false failures
            if (W == 0) { ACTUAL &= ~0x3104u; CONFIG_EXPECTED &= ~0x3104u; } // Config 1 mask
            if (W == 1) { ACTUAL &= ~0x00FCu; CONFIG_EXPECTED &= ~0x00FCu; } // Config 2 mask (LPBOREN/LVP are real bits, keep them compared)

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
bool PROGRAM_PIC16F183XX(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
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

    size_t P = 0;

    while (P < TOTAL_PACKETS)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA > PIC16F183XX_FLASH_END)
        {
            P++;
            continue;
        }

        uint32_t ROW_BASE = (BA >> 1) & ~0x1Fu;

        uint16_t ROW[32];
        for (int I = 0; I < 32; I++)
        {
            ROW[I] = 0x3FFF;
        }

        // Consume all consecutive packets belonging to this row
        while (P < TOTAL_PACKETS && BUFFER[P].ADDRESS <= PIC16F183XX_FLASH_END)
        {
            const HEXPacket_t* PKT = &BUFFER[P];
            uint32_t DW            = PKT->ADDRESS >> 1;

            if ((DW & ~0x1Fu) != ROW_BASE)
                break;

            uint32_t SLOT          = DW - ROW_BASE;
            const uint8_t* PAYLOAD = PKT->PAYLOAD;

            for (int W = 0; W < 8; W++)
            {
                ROW[SLOT + (uint32_t)W] =
                    (((uint16_t)PAYLOAD[W * 2u + 1u] << 8) |
                    PAYLOAD[W * 2u]) & 0x3FFF;
            }

            P++;
        }

        PIC16F183XX_LOAD_PC_ADDR(ROW_BASE);

        for (int LATCH = 0; LATCH < 32; LATCH++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM_INC);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(ROW[LATCH]) << 1), 16);
        }

        PIC16F183XX_LOAD_PC_ADDR(ROW_BASE);

        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT);
        sleep_ms(3);
    }

    printf("Verifying Flash...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA > PIC16F183XX_FLASH_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;

        PIC16F183XX_LOAD_PC_ADDR(BA >> 1);
        for (int W = 0; W < 8; W++)
        {
            uint16_t EXPECTED = (((uint16_t)PAYLOAD[W * 2u + 1u] << 8) | PAYLOAD[W * 2u]) & 0x3FFF;

            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            if (RAW_STREAM != EXPECTED)
            {
                printf("[PICO] FLASH VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)((BA >> 1) + (uint32_t)W), EXPECTED, RAW_STREAM);
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

        uint32_t BASE_DEV      = BA >> 1;
        const uint8_t* PAYLOAD = BUFFER[P].PAYLOAD;
        for (int B = 0; B < 8; B++)
        {
            PIC16F183XX_LOAD_PC_ADDR(BASE_DEV + (uint32_t)B);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM);
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(PAYLOAD[B * 2u] & 0xFFu) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN INTERNALLY TIMED PROGRAM
            sleep_ms(3);                                                // TPINT
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

            const uint8_t* PAYLOAD = BUFFER[P].PAYLOAD;

            PIC16F183XX_LOAD_PC_ADDR(BA >> 1);
            for (int B = 0; B < 8; B++)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

                SET_PGD_INPUT();
                uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
                RAW_STREAM = (RAW_STREAM >> 1) & 0x3FFF;
                SET_PGD_OUTPUT();

                uint8_t EXPECTED = PAYLOAD[B * 2u] & 0xFF;
                uint8_t ACTUAL = (uint8_t)(RAW_STREAM & 0xFF);

                if (ACTUAL != EXPECTED)
                {
                    printf("[PICO] EEPROM VERIFY FAIL @ 0x%04X: exp 0x%02X got 0x%02X\n",
                           (unsigned)((BA >> 1) + (uint32_t)B), EXPECTED, ACTUAL);

                    gpio_put(PIN_MCLR, 1); // Exit LVP
                    return false;
                }
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify USER ID                          */
    /* -------------------------------------------------------------------------- */
    uint16_t UID_WORDS[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
    bool HAS_UID = false;
    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        
        // Strictly filter to prevent contamination from non-User ID packets
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
                HAS_UID = true;
            }
        }
    }

    if (HAS_UID)
    {
        printf("Programming User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM); // LOAD DATA FOR NVM
            SHIFT_OUT_BITS_LSB_FIRST(((uint32_t)(UID_WORDS[W]) << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN PROGRAM INTERNALLY TIMED
            sleep_ms(6);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC
        }

        printf("Verifying User ID...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int W = 0; W < 4; W++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = RAW_STREAM & 0x3FFF;
            if (ACTUAL != UID_WORDS[W])
            {
                printf("[PICO] CFG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8000u + (uint32_t)W), UID_WORDS[W], ACTUAL);

                gpio_put(PIN_MCLR, 1); // Exit LVP
                return false;
            }
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                      (5) Program & Verify CONFIG                           */
    /* -------------------------------------------------------------------------- */
    uint16_t CFG_WORDS[4] = {0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF};
    bool HAS_CFG = false;
    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        
        // Strictly filter to prevent contamination from non-CONFIG packets
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
                HAS_CFG = true;
            }
        }
    }

    if (HAS_CFG)
    {
        printf("Programming CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        for (int I = 0; I < 7; I++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC to 0x8007
        }

        for (int C = 0; C < 4; C++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_DATA_NVM); // LOAD DATA FOR NVM
            SHIFT_OUT_BITS_LSB_FIRST((CFG_WORDS[C] << 1), 16);
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_BEGIN_PROGRAM_INT); // BEGIN PROGRAM INTERNALLY TIMED
            sleep_ms(6);
            
            if (C < 3)
            {
                PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment PC
            }
        }

        printf("Verifying CONFIG...\n");
        PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_LOAD_CONFIG); // LOAD CONFIGURATION
        SHIFT_OUT_BITS_LSB_FIRST(0x8000u, 16);

        // Index 0-8007h, 1-8008h, 2-8009h, 3-800Ah
        // Masks were shifted one slot too late (index0 was an unmasked 0x3FFF
        // placeholder, and each real mask below was one word ahead of where it
        // belongs; CONFIG4/0x800A's real mask was missing entirely). Corrected
        // against PIC16F18345's real per-word implemented-bit masks: CONFIG1
        // 0x2977, CONFIG2 0x3AEF, CONFIG3 0x2003, CONFIG4 0x0003.
        static const uint16_t CFG_MASKS[4] = {0x2977, 0x3AEF, 0x2003, 0x0003};
        for (int I = 0; I < 7; I++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_INC_ADDR); // Increment Address
        }

        for (int C = 0; C < 4; C++)
        {
            PIC_12_16_SEND_6_BIT_CMD(PIC16F183XX_CMD_READ_DATA_NVM_INC);

            SET_PGD_INPUT();
            uint16_t RAW_STREAM = (uint16_t)SHIFT_IN_BITS_LSB_FIRST(16);
            RAW_STREAM = (RAW_STREAM >> 1) & 0x3FFF;
            SET_PGD_OUTPUT();

            uint16_t ACTUAL = RAW_STREAM & CFG_MASKS[C];
            uint16_t EXPECTED = CFG_WORDS[C] & CFG_MASKS[C];

            if (ACTUAL != EXPECTED)
            {
                printf("[PICO] CONFIG VERIFY FAIL @ 0x%04X: exp 0x%04X got 0x%04X\n",
                       (unsigned)(0x8007u + (uint32_t)C), EXPECTED, ACTUAL);

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
bool PROGRAM_PIC18FXXK80(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
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

    // DEVICE_ID == VALID??
    // IMPORTANT NOTE: Add the check for additional PIC18FXXK80 mcus if you want to expand support
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

    /* CONFIG BITS MUST BE ERASED FIRST. CONFIG6L holds WRT0..WRT3, the write
     * protection bits for code Blocks 0..3 (DS39972B Table 5-1; "WRT0: 0 =
     * Block 0 is write-protected", erased default '---- 1111' = unprotected).
     * While WRT0 is programmed to 0, an Erase Block 0 does nothing and reports
     * no error, so with the config erase LAST every code-block erase was being
     * silently blocked by protection bits left over from the previous run.
     * Erasing config first returns WRTn to 1 and unblocks the block erases.
     *
     * This is what the bench log showed: on PIC18FX5K80 with BBSIZ=1 (the
     * erased default, CONFIG4L<4>) Figure 2-7 puts the 2KW Boot Block at
     * 0000h-0FFFh and starts Block 0 at 1000h. The Boot Block is guarded by
     * WRTB in CONFIG6H, not WRT0, so it kept erasing correctly - which is why
     * rows 0-63 verified and 0x001000, the first address of Block 0, did not. */
    PIC18FXXK80_ERASE_BLOCK(0x02, 0x00); /* Config bits          (Table 3-1: 000002h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x01); /* Code &EEPROM block 0 (Table 3-1: 000104h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x02); /* Code &EEPROM block 1 (Table 3-1: 000204h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x04); /* Code &EEPROM block 2 (Table 3-1: 000404h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x08); /* Code &EEPROM block 3 (Table 3-1: 000804h) */
    PIC18FXXK80_ERASE_BLOCK(0x05, 0x00); /* Boot block           (Table 3-1: 000005h) */
    PIC18FXXK80_ERASE_BLOCK(0x04, 0x00); /* Data EEPROM          (Table 3-1: 000004h) */

    // DIAGNOSTIC: confirm the erase actually reached 0x001000 (the address that
    // has been failing verify) before any programming touches it. Expect FF FF.
    PIC18FXXK80_SET_TBLPTR(0x001000);
    uint8_t PROBE_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
    uint8_t PROBE_HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
    printf("[PICO] post-erase probe @ 0x001000 = %02X %02X (expect FF FF)\n", PROBE_LO, PROBE_HI);


    /* -------------------------------------------------------------------------- */
    /*                         (4) Program & Verify FLASH                         */
    /* -------------------------------------------------------------------------- */
    printf("Programming Flash...\n");

    PIC18FXXK80_CORE_INSTR(0x0000);
    static uint8_t PIC_FLASH_MATRIX[32768];
    memset(PIC_FLASH_MATRIX, 0xFF, sizeof(PIC_FLASH_MATRIX));

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t BA = BUFFER[P].ADDRESS;

        if (BA < PIC18FXXK80_FLASH_END)
        {
            // ba < 0x8000 here, so the writable span is min(32, 32768 - ba) bytes
            size_t N = (size_t)(32768u - BA);
            if (N > 32u)
            {
                N = 32u;
            }
            memcpy(&PIC_FLASH_MATRIX[BA], BUFFER[P].PAYLOAD, N);
        }
    }

    for (uint32_t BLOCK_IDX = 0; BLOCK_IDX < 512; BLOCK_IDX++)
    {
        uint32_t BLOCK_ADDR = BLOCK_IDX * 64u;
        uint8_t *ROW_PTR = &PIC_FLASH_MATRIX[BLOCK_ADDR];

        bool HAS_ACTIVE_DATA = false;
        for (int B = 0; B < 64; B++)
        {
            if (ROW_PTR[B] != 0xFF)
            {
                HAS_ACTIVE_DATA = true;
                break;
            }
        }
        
        if (!HAS_ACTIVE_DATA) continue;

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
            uint8_t T_LSB = ROW_PTR[I * 2];
            uint8_t T_MSB = ROW_PTR[(I * 2) + 1];
            uint16_t T_WORD_DATA = (uint16_t)(T_LSB | ((uint16_t)T_MSB << 8u));
            PIC18FXXK80_CMD_WRITE_WORD(PIC18FXXK80_CMD_TABLE_WRITE_POST2, T_WORD_DATA);
        }

        uint8_t LAST_LSB = ROW_PTR[31 * 2];
        uint8_t LAST_MSB = ROW_PTR[(31 * 2) + 1];
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
        sleep_ms(5);
        gpio_put(PIN_PGC, 0);
        sleep_us(100);
        SHIFT_OUT_BITS_LSB_FIRST(0x0000, 16);

        uint8_t PROGRAM_STATUS_LO = 0;
        absolute_time_t TIMEOUT_START = get_absolute_time();
        do {
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
    for (uint32_t BLOCK_IDX = 0; BLOCK_IDX < 512; BLOCK_IDX++)
    {
        uint32_t BLOCK_ADDR = BLOCK_IDX * 64u;
        uint8_t *ROW_PTR = &PIC_FLASH_MATRIX[BLOCK_ADDR];

        bool HAS_ACTIVE_DATA = false;
        for (int B = 0; B < 64; B++)
        {
            if (ROW_PTR[B] != 0xFF)
            {
                HAS_ACTIVE_DATA = true;
                break;
            }
        }
        if (!HAS_ACTIVE_DATA) continue;

        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 16) & 0x3F));
        PIC18FXXK80_CORE_INSTR(0x6EF8);
        PIC18FXXK80_CORE_INSTR(0x0E00 | ((BLOCK_ADDR >> 8) & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF7);
        PIC18FXXK80_CORE_INSTR(0x0E00 | (BLOCK_ADDR & 0xFF));
        PIC18FXXK80_CORE_INSTR(0x6EF6);

        for (int I = 0; I < 32; I++)
        {
            uint8_t READ_LO = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint8_t READ_HI = PIC18FXXK80_CMD_READ_BYTE(PIC18FXXK80_CMD_TABLE_READ_INC);
            uint16_t ACTUAL_WORD = (uint16_t)(READ_LO | ((uint16_t)READ_HI << 8u));

            uint8_t EXP_LSB = ROW_PTR[I * 2];
            uint8_t EXP_MSB = ROW_PTR[(I * 2) + 1];
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

        uint32_t BASE_EEPROM_ADDR = RAW_ADDRESS - PIC18FXXK80_EEPROM_BGN;
        const uint8_t* PAYLOAD    = BUFFER[P].PAYLOAD;

        for (int I = 0; I < 32; I++)
        {
            uint32_t CURRENT_ADDR = BASE_EEPROM_ADDR + (uint32_t)I;

            if (CURRENT_ADDR >= 0x400u)
                break;

            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)(CURRENT_ADDR & 0x00FFu));
            PIC18FXXK80_CORE_INSTR(0x6E74);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)((CURRENT_ADDR >> 8) & 0x03u));
            PIC18FXXK80_CORE_INSTR(0x6E75);
            PIC18FXXK80_CORE_INSTR(0x0E00 | (uint16_t)PAYLOAD[I]);
            PIC18FXXK80_CORE_INSTR(0x6E73);
            PIC18FXXK80_CORE_INSTR(0x847F);
            PIC18FXXK80_CORE_INSTR(0x827F);

            uint8_t PROGRAM_STATUS_LO;
            do {
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
    bool EEPROM_CELL_ACTIVE[1024];

    memset(EEPROM_LAYOUT_MATRIX, 0xFF, sizeof(EEPROM_LAYOUT_MATRIX));
    memset(EEPROM_CELL_ACTIVE, false, sizeof(EEPROM_CELL_ACTIVE));

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        uint32_t A = BUFFER[P].ADDRESS;
        if (A >= PIC18FXXK80_EEPROM_BGN && A < PIC18FXXK80_EEPROM_END)
        {
            uint32_t LOCAL_ADDR    = A - PIC18FXXK80_EEPROM_BGN;
            const uint8_t* PAYLOAD = BUFFER[P].PAYLOAD;

            for (int I = 0; I < 32; I++)
            {
                if ((LOCAL_ADDR + I) < 1024)
                {
                    EEPROM_LAYOUT_MATRIX[LOCAL_ADDR + I] = PAYLOAD[I];
                    EEPROM_CELL_ACTIVE[LOCAL_ADDR + I] = true;
                }
            }
        }
    }

    PIC18FXXK80_CORE_INSTR(0x9E7F);
    PIC18FXXK80_CORE_INSTR(0x9C7F);

    for (uint32_t ADDR = 0; ADDR < 1024; ADDR++)
    {
        if (!EEPROM_CELL_ACTIVE[ADDR]) continue;

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
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        for (int W = 0; W < 4; W++)
        {
            uint32_t UID_BA = PIC18FXXK80_USER_ID_BGN + (uint32_t)W * 2u;
            if (UID_BA >= BA && UID_BA < BA + 32u)
            {
                uint32_t T_OFF = UID_BA - BA;
                UID_WORDS[W] = ((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u)
                             | PKT->PAYLOAD[T_OFF];
                HAS_UID = true;
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

            uint16_t ACTUAL_UID = (uint16_t)(((uint16_t)HI << 8u) | LO);
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
    
    uint16_t CFG_WORDS[7] = 
    {
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
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        for (int W = 0; W < 7; W++)
        {
            uint32_t CFG_BA = PIC18FXXK80_CONFIG_BGN + (uint32_t)W * 2u;
            if (CFG_BA >= BA && CFG_BA < BA + 32u)
            {
                uint32_t T_OFF = CFG_BA - BA;
                CFG_WORDS[W] = ((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u)
                             | PKT->PAYLOAD[T_OFF];

                // printf("[PICO] Staging CONFIG Word %d at 0x%06X: 0x%04X\n",
                //        w + 1, cfg_ba, cfg_words[w]);
                HAS_CFG = true;
            }
        }
    }

    if (HAS_CFG)
    {
        for (int W = 0; W < 7; W++)
        {
            if (CFG_WORDS[W] == 0xFFFFu) {
                continue;
            }

            uint32_t BASE_ADDR = PIC18FXXK80_CONFIG_BGN + ((uint32_t)W * 2u);
            uint8_t T_LSB = (uint8_t)(CFG_WORDS[W] & 0x00FFu);
            uint8_t T_MSB = (uint8_t)(CFG_WORDS[W] >> 8u);

            PIC18FXXK80_CORE_INSTR(0x8E7F);
            PIC18FXXK80_CORE_INSTR(0x8C7F);
            PIC18FXXK80_SET_TBLPTR(BASE_ADDR);  

            uint16_t EVEN_PAYLOAD = (uint16_t)T_LSB;
            PIC18FXXK80_CMD_WRITE_WORD(0xF, EVEN_PAYLOAD);

            gpio_put(PIN_PGD, 0);
            for (int I = 0; I < 3; I++) {
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
            for (int I = 0; I < 3; I++) {
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

        uint16_t ACTUAL_CFG = (uint16_t)(((uint16_t)T_HI << 8u) | T_LO);
        uint32_t CFG_DEV_ADDR = PIC18FXXK80_CONFIG_BGN + (W * 2);

        uint16_t MASKED_EXPECTED = CFG_WORDS[W] & CFG_MASKS[W];
        uint16_t MASKED_ACTUAL = ACTUAL_CFG & CFG_MASKS[W];

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
bool PROGRAM_PIC18F2XK83(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
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

    // DEVICE_ID == VALID??
    // IMPORTANT NOTE: Add the check for additional PIC18F2XK83 mcus if you want to expand support
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
    sleep_ms(26);                                       // TERAB

    PIC18F2XK83_LOAD_PC_ADDR(PIC18F2XK83_PC_EEPROM);
    SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BULK_ERASE, 8);
    sleep_ms(26);                                       // TERAB


    /* -------------------------------------------------------------------------- */
    /*                (4) Program FLASH Memory (64-Word Rows)                     */
    /* -------------------------------------------------------------------------- */
    printf("Programming FLASH...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; )
    {
        uint32_t BA = BUFFER[P].ADDRESS;
        if (BA > PIC18F2XK83_FLASH_END)
        {
            P++;                        // Safely advance past out-of-bounds packet
            continue;
        }

        uint32_t ROW_BASE               = BA & ~0x7Fu;
        uint16_t ROW[64];
        bool     ROW_NEEDS_PROGRAMMING  = false;

        for (uint32_t I = 0; I < 64u; I++)
        {
            ROW[I] = 0xFFFFu;
        }

        // Consume all consecutive packets belonging to this row
        while (P < TOTAL_PACKETS)
        {
            const HEXPacket_t* PKT   = &BUFFER[P];
            uint32_t PACKET_ADDR     = PKT->ADDRESS;

            if (PACKET_ADDR > PIC18F2XK83_FLASH_END || (PACKET_ADDR & ~0x7Fu) != ROW_BASE)
            {
                break;                  // Do NOT increment p; let next outer iteration process it
            }

            uint32_t OFFSET = PACKET_ADDR - ROW_BASE;
            if ((OFFSET & 0x0Fu) != 0u)
            {
                P++;
                continue;
            }

            uint32_t SLOT = OFFSET >> 1u;
            if ((SLOT + 16u) > 64u)
            {
                P++;
                continue;
            }

            const uint8_t* PAYLOAD = PKT->PAYLOAD;
            for (uint32_t W = 0; W < 16u; W++)
            {
                uint16_t WORD = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8u)
                                          | PAYLOAD[W * 2u]);
                ROW[SLOT + W] = WORD;

                if (WORD != 0xFFFFu)
                {
                    ROW_NEEDS_PROGRAMMING = true;
                }
            }

            P++;                        // Correctly advance inside the processing block
        }

        if (!ROW_NEEDS_PROGRAMMING)
        {
            continue;                   // p is already pointing to the next valid start packet
        }

        PIC18F2XK83_LOAD_PC_ADDR(ROW_BASE);
        for (uint32_t LATCH = 0; LATCH < 64u; LATCH++)
        {
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM_INC, 8);
            SHIFT_OUT_BITS_MSB_FIRST(((uint32_t)ROW[LATCH]) << 1u, 24);
        }

        PIC18F2XK83_LOAD_PC_ADDR(ROW_BASE);
        SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
        sleep_ms(4);                                    // TPINT
    }


    /* -------------------------------------------------------------------------- */
    /*                       (5) Verify FLASH Memory                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying FLASH...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; )
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
            const HEXPacket_t* PKT   = &BUFFER[P];
            uint32_t PACKET_ADDR     = PKT->ADDRESS;

            if (PACKET_ADDR > PIC18F2XK83_FLASH_END || (PACKET_ADDR & ~0x7Fu) != ROW_BASE)
            {
                break;                  // Do NOT increment p
            }

            uint32_t OFFSET = PACKET_ADDR - ROW_BASE;
            if ((OFFSET & 0x0Fu) != 0u)
            {
                P++;
                continue;
            }

            uint32_t SLOT = OFFSET >> 1u;
            if ((SLOT + 16u) > 64u)
            {
                P++;
                continue;
            }

            const uint8_t* PAYLOAD = PKT->PAYLOAD;
            for (uint32_t W = 0; W < 16u; W++)
            {
                EXPECTED_ROW[SLOT + W] = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8u)
                                                   | PAYLOAD[W * 2u]);
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

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18F2XK83_EEPROM_BGN || BA >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t EEPROM_OFFSET = BA - PIC18F2XK83_EEPROM_BGN;
        uint32_t BASE_DEV      = PIC18F2XK83_EEPROM_BGN + EEPROM_OFFSET;
        const uint8_t* PAYLOAD = PKT->PAYLOAD;

        for (uint32_t B = 0; B < HEX_PAYLOAD_SIZE_BYTES; B++)
        {
            if ((BASE_DEV + B) > PIC18F2XK83_EEPROM_END)
            {
                break;                  // do not run past the last EEPROM cell
            }

            uint32_t DATA_BYTE = (uint32_t)(PAYLOAD[B] & 0xFFu);
            if (DATA_BYTE == 0xFFu)
            {
                continue;               // Leave erased cells untouched
            }

            PIC18F2XK83_LOAD_PC_ADDR(BASE_DEV + B);
            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_LOAD_DATA_NVM, 8);

            SHIFT_OUT_BITS_MSB_FIRST(DATA_BYTE << 1u, 24);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18F2XK83_CMD_BEGIN_PROGRAM_INT, 8);
            sleep_ms(6);                                // TPINT
        }
    }


    /* -------------------------------------------------------------------------- */
    /*                        (7) Verify Data EEPROM                              */
    /* -------------------------------------------------------------------------- */
    printf("Verifying EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18F2XK83_EEPROM_BGN || BA >= PIC18F2XK83_EEPROM_END)
        {
            continue;
        }

        uint32_t EEPROM_OFFSET = BA - PIC18F2XK83_EEPROM_BGN;
        uint32_t BASE_DEV      = PIC18F2XK83_EEPROM_BGN + EEPROM_OFFSET;
        const uint8_t* PAYLOAD = PKT->PAYLOAD;

        for (uint32_t B = 0; B < HEX_PAYLOAD_SIZE_BYTES; B++)
        {
            if ((BASE_DEV + B) > PIC18F2XK83_EEPROM_END)
            {
                break;                  // do not run past the last EEPROM cell
            }

            uint8_t EXPECTED_BYTE = PAYLOAD[B];
            if (EXPECTED_BYTE == 0xFFu)
            {
                continue;               // Padding/erased cell - not this packet's data,
                                         // may be owned by a different overlapping packet
            }

            PIC18F2XK83_LOAD_PC_ADDR(BASE_DEV + B);

            uint16_t ACTUAL_WORD = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            uint8_t  ACTUAL_BYTE = (uint8_t)(ACTUAL_WORD & 0xFFu);

            if (ACTUAL_BYTE != EXPECTED_BYTE)
            {
                printf("[-] EEPROM VERIFY ERROR @ 0x%06X: Expected 0x%02X, Got 0x%02X\n",
                       (unsigned)(BASE_DEV + B), EXPECTED_BYTE, ACTUAL_BYTE);

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

    uint16_t UID_WORDS[8];
    for (int I = 0; I < 8; I++)
    {
        UID_WORDS[I] = 0xFFFFu;
    }
    bool HAS_UID = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        for (int W = 0; W < 8; W++)
        {
            uint32_t UID_BA = PIC18F2XK83_USER_ID_BGN + (uint32_t)W * 2u;
            if (UID_BA >= BA && UID_BA < BA + 32u)
            {
                uint32_t T_OFF = UID_BA - BA;
                UID_WORDS[W]   = (uint16_t)(((uint16_t)PKT->PAYLOAD[T_OFF + 1u] << 8u)
                                           | PKT->PAYLOAD[T_OFF]);
                HAS_UID = true;
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
            sleep_ms(6);                                // TPINT
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

    uint16_t CONFIG_WORDS[5];
    for (int I = 0; I < 5; I++)
    {
        CONFIG_WORDS[I] = 0xFFFFu;
    }
    bool HAS_CONFIG = false;

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        for (int W = 0; W < 5; W++)
        {
            uint32_t CONFIG_BA = PIC18F2XK83_CFG_BGN + (uint32_t)W * 2u;
            if (CONFIG_BA >= BA && CONFIG_BA < BA + 32u)
            {
                uint32_t C_OFF   = CONFIG_BA - BA;
                CONFIG_WORDS[W]  = (uint16_t)(((uint16_t)PKT->PAYLOAD[C_OFF + 1u] << 8u)
                                             | PKT->PAYLOAD[C_OFF]);
                HAS_CONFIG = true;
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
            sleep_ms(6);                                // TPINT
        }

        printf("Verifying CONFIG Words...\n");
        // Per-word implemented-bit masks (low byte | high byte << 8), sourced
        // directly from the PIC18(L)F25/26K83 Programming Specification
        // (DS40001927A) Appendix B, Registers B-1 through B-9. Unimplemented
        // bits always read back their Reset value (1) regardless of what is
        // written, so they are excluded here.
        static const uint16_t CFG_WORD_MASKS[5] = {
            0x2B77, /* Word1 (0x300000/1): RSTOSC/FEXTOSC + FCMEN/CSWEN/PR1WAY/CLKOUTEN */
            0xBFFF, /* Word2 (0x300002/3): supervisor - all Word2L bits, Word2H bit6 unimpl. */
            0x3F7F, /* Word3 (0x300004/5): WDT - Word3L bit7, Word3H bits7:6 unimpl.       */
            0x2F9F, /* Word4 (0x300006/7): write-protect - see B-7/B-8 gaps                */
            0x0001  /* Word5 (0x300008/9): only CP (bit0) implemented; Word5H fully unimpl. */
        };
        for (int W = 0; W < 5; W++)
        {
            uint32_t CFG_DEV_ADDR = PIC18F2XK83_CFG_BGN + ((uint32_t)W * 2u);
            PIC18F2XK83_LOAD_PC_ADDR(CFG_DEV_ADDR);

            uint16_t ACTUAL = PIC18F2XK83_READ_WORD_NVM_POST_INC();
            uint16_t MASKED_ACTUAL   = ACTUAL & CFG_WORD_MASKS[W];
            uint16_t MASKED_EXPECTED = CONFIG_WORDS[W] & CFG_WORD_MASKS[W];
            if (MASKED_ACTUAL != MASKED_EXPECTED)
            {
                printf("[-] CONFIG VERIFY ERROR @ 0x%06X: Expected 0x%04X, Got 0x%04X\n",
                       (unsigned)CFG_DEV_ADDR, CONFIG_WORDS[W], ACTUAL);

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
bool PROGRAM_PIC18FXXQ8X(const HEXPacket_t* BUFFER, size_t TOTAL_PACKETS)
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
    printf("[PICO] Device ID: 0x%04X\n", DEVICE_ID);

    if (DEVICE_ID == 0xFFFF || DEVICE_ID == 0x0000)
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

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        PIC18FXXQ8X_LOAD_PC_ADDR(BA);
        sleep_us(2);

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 8; W++)
        {
            uint16_t WORD_TO_PROGRAM = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8)
                                                 | PAYLOAD[W * 2u]);

            if (WORD_TO_PROGRAM == 0xFFFF)
            {
                SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_INC_ADDR, 8);
                sleep_us(2);
                continue;
            }

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA_INC, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(WORD_TO_PROGRAM);
            sleep_us(75);                               // TPINT
        }
    }

    printf("Verifying FLASH and USER IDs...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if ((BA > PIC18FXXQ8X_FLASH_END && BA < PIC18FXXQ8X_USER_ID_BGN) || BA > PIC18FXXQ8X_USER_ID_END)
        {
            continue;
        }

        bool ADDRESS_NEEDS_RELOAD = true;

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
        for (int W = 0; W < 8; W++)
        {
            uint16_t EXPECTED_WORD = (uint16_t)(((uint16_t)PAYLOAD[W * 2u + 1u] << 8)
                                               | PAYLOAD[W * 2u]);

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
                gpio_put(PIN_MCLR, 1); // Exit LVP
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
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_EEPROM_BGN || BA > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
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
                continue;               // Leave erased cells untouched
            }

            // Force exact address targeting for every single byte modification pass
            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            // Command 0xC0: Freezes internal hardware PC (J=0), preventing address drift
            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(BYTE_TO_PROGRAM);
            sleep_ms(11);                               // TPINT
        }
    }

    printf("Verifying EEPROM...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_EEPROM_BGN || BA > PIC18FXXQ8X_EEPROM_END)
        {
            continue;
        }

        bool ADDRESS_NEEDS_RELOAD = true;

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
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
                gpio_put(PIN_MCLR, 1); // Exit LVP
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
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_CONFIG_BGN || BA > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
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

            // Force exact address targeting for every single fuse byte modification pass
            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            // Command 0xC0: Freezes internal hardware PC (J=0), preventing address drift
            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_PROG_DATA, 8);
            sleep_us(2);

            PIC18FXXQ8X_SEND_24_BIT_PAYLOAD(BYTE_TO_PROGRAM);
            sleep_ms(11);                               // TPINT
        }
    }

    printf("Verifying CONFIG fuses...\n");

    for (size_t P = 0; P < TOTAL_PACKETS; P++)
    {
        const HEXPacket_t* PKT = &BUFFER[P];
        uint32_t BA            = PKT->ADDRESS;

        if (BA < PIC18FXXQ8X_CONFIG_BGN || BA > PIC18FXXQ8X_CONFIG_END)
        {
            continue;
        }

        const uint8_t* PAYLOAD = PKT->PAYLOAD;
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

            // Explicitly reload the exact verification address
            PIC18FXXQ8X_LOAD_PC_ADDR(EXACT_BYTE_ADDRESS);
            sleep_us(2);

            SHIFT_OUT_BITS_MSB_FIRST(PIC18FXXQ8X_CMD_READ_DATA_NVM_INC, 8);
            sleep_us(2);
            SET_PGD_INPUT();

            uint32_t READ_RAW = SHIFT_IN_BITS_MSB_FIRST(24);
            SET_PGD_OUTPUT();

            uint8_t ACTUAL_BYTE = (uint8_t)((READ_RAW >> 1) & 0xFF);

            // Per-byte implemented-bit masks for CONFIG1-CONFIG11 (0x300000-
            // 0x30000A), sourced directly from the PIC18-Q83/84 Family
            // Programming Specification (DS40002137D) Appendix B, sections
            // 6.1-6.11. Unimplemented bits always read back their Reset value
            // (1) regardless of what is written, so they are excluded here.
            // Bytes past 0x30000A (up to CONFIG_END) are reserved/unused.
            static const uint8_t CFG_MASKS[11] = {
                0x77, /* CONFIG1  0x300000: RSTOSC/FEXTOSC, bit7+bit3 unimpl. */
                0xFB, /* CONFIG2  0x300001: bit2 unimplemented               */
                0xFF, /* CONFIG3  0x300002: fully implemented                */
                0xBF, /* CONFIG4  0x300003: bit6 unimplemented               */
                0x7F, /* CONFIG5  0x300004: bit7 unimplemented               */
                0x3F, /* CONFIG6  0x300005: bits7:6 unimplemented            */
                0x3F, /* CONFIG7  0x300006: bits7:6 unimplemented            */
                0x8F, /* CONFIG8  0x300007: bits6:4 unimplemented            */
                0x33, /* CONFIG9  0x300008: bits7:6,3:2 unimplemented        */
                0x01, /* CONFIG10 0x300009: only bit0 (CP) implemented       */
                0xFF  /* CONFIG11 0x30000A: fully implemented                */
            };
            uint32_t CFG_IDX = EXACT_BYTE_ADDRESS - PIC18FXXQ8X_CONFIG_BGN;
            if (CFG_IDX < 11u)
            {
                ACTUAL_BYTE   &= CFG_MASKS[CFG_IDX];
                EXPECTED_BYTE &= CFG_MASKS[CFG_IDX];
            }

            if (ACTUAL_BYTE != EXPECTED_BYTE)
            {
                printf("[PICO] VERIFY FAILURE (CONFIG): Address 0x%06X Mismatch! Expected 0x%02X, Read 0x%02X\n",
                       (unsigned)EXACT_BYTE_ADDRESS, EXPECTED_BYTE, ACTUAL_BYTE);
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