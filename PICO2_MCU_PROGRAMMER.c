/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "MC9S08PA4.h"
#include "PIC.h"
#include "AVR_PDI.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// Device family code
#define FAMILY_MC9S08PA4            1u
#define FAMILY_PIC12F157X           2u
#define FAMILY_PIC16F183XX          3u
#define FAMILY_PIC18FXXK80          4u
#define FAMILY_PIC18F2XK83          5u
#define FAMILY_PIC18FXXQ8X          6u
#define FAMILY_ATXMEGA192A3U        7u
#define FAMILY_ATXMEGA32C3          8u
#define FAMILY_ATXMEGA32E5          9u
#define FAMILY_ATXMEGA64AU          10u
#define FAMILY_ATXMEGA128A3U        11u
#define FAMILY_ATXMEGA128A4U        12u
#define FAMILY_MCU_LAST             12u
// S19 Payload Sizing
#define S19_PACKET_SIZE_BYTES       66u          
#define S19_PAYLOAD_SIZE_BYES       64u 
// HEX Payload Sizing
#define HEX_PACKET_SIZE_BYTES       36u
#define HEX_PAYLOAD_SIZE_BYTES      32u


/* -------------------------------------------------------------------------- */
/*                                  Statics                                   */
/* -------------------------------------------------------------------------- */
static bool        PROGRAM_STATUS;                         // Programming State
static S19Packet_t S19_STAGING_BUFFER[S19_MAX_PACKETS];    // S19 packet staging buffer
static HEXPacket_t HEX_STAGING_BUFFER[HEX_MAX_PACKETS];    // HEX packet staging buffer


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Identifies MCU from USB_RP_COM.py
 * INPUT:           ---
 * RETURN:      Valid MCU #
 */
static uint8_t IDENTIFY_MCU(void)
{
    char INIT_BUF[64];
    int  IDX = 0;

    while (true) 
    {
        int C = getchar_timeout_us(1000);
        if (C != PICO_ERROR_TIMEOUT) 
        {
            if (C == '\n' || C == '\r') 
            {
                INIT_BUF[IDX] = '\0';

                if (strcmp(INIT_BUF, "INIT_FAMILY:MC9S08PA4") == 0)     // MCU == MC9S08PA4?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_MC9S08PA4;
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:PIC12F157X") == 0)    // MCU == PIC12F157X? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_PIC12F157X; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:PIC16F183XX") == 0)   // MCU == PIC16F183XX? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_PIC16F183XX; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:PIC18FXXK80") == 0)   // MCU == PIC18FXXK80? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_PIC18FXXK80; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:PIC18F2XK83") == 0)   // MCU == PIC18F2XK83? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_PIC18F2XK83; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:PIC18FXXQ8X") == 0)   // MCU == PIC18FXXQ8X? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_PIC18FXXQ8X; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA192A3U") == 0)  // MCU == ATXMEGA192A3U? 
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n"); 
                    return FAMILY_ATXMEGA192A3U; 
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA32C3") == 0)   // MCU == ATXMEGA32C3?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_ATXMEGA32C3;
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA32E5") == 0)   // MCU == ATXMEGA32E5?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_ATXMEGA32E5;
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA64AU") == 0)   // MCU == ATXMEGA64AU?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_ATXMEGA64AU;
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA128A3U") == 0)   // MCU == ATXMEGA128A3U?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_ATXMEGA128A3U;
                }
                if (strcmp(INIT_BUF, "INIT_FAMILY:ATXMEGA128A4U") == 0)   // MCU == ATXMEGA128A4U?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_ATXMEGA128A4U;
                }
                
                IDX = 0; // Clear index if a stray/malformed line is captured
            } 
            else if (IDX < sizeof(INIT_BUF) - 1) 
            {
                INIT_BUF[IDX++] = (char)C;
            }
        }
    }
}

/**
 * DESCRIPTION: Loads S19 packets into a staging buffer
 * INPUT:       S19 Packets
 * RETURN:      True=PASS, False=FAIL
 */
static bool LOAD_S19_DATA(S19Packet_t* OUT_PACKET)
{
    uint8_t PACKET_ARRARY[S19_PACKET_SIZE_BYTES];

    int BYTES_COLLECTED = 0;

    // Collect 66-byte payload
    while (BYTES_COLLECTED < S19_PACKET_SIZE_BYTES)
    {
        int BYTE_IN = getchar_timeout_us(400000);

        if (BYTE_IN == PICO_ERROR_TIMEOUT)
        {
            // TIMEOUT
            return false;
        }

        PACKET_ARRARY[BYTES_COLLECTED++] = (uint8_t)BYTE_IN;
    }

    // Unpack Address
    OUT_PACKET->ADDRESS = (PACKET_ARRARY[0] << 8) | PACKET_ARRARY[1];
    memcpy(OUT_PACKET->PAYLOAD, &PACKET_ARRARY[2], S19_PAYLOAD_SIZE_BYES);

    // ACK
    printf("S19_LINE_SUCCESS\n");
    return true;
}

/**
 * DESCRIPTION: Loads HEX packets into a staging buffer
 * INPUT:       HEX Packets
 * RETURN:      True=PASS, False=FAIL
 */
static bool LOAD_HEX_DATA(HEXPacket_t* OUT_PACKET)
{
    uint8_t PACKET_ARRARY[HEX_PACKET_SIZE_BYTES];

    int BYTES_COLLECTED = 0;

    // Collect 36-byte payload
    while (BYTES_COLLECTED < HEX_PACKET_SIZE_BYTES) 
    {
        int BYTE_IN = getchar_timeout_us(400000); 
            
        if (BYTE_IN == PICO_ERROR_TIMEOUT) 
        {
            // Serial went quiet. Python finished its data stream
            return false; 
        }
        
        PACKET_ARRARY[BYTES_COLLECTED++] = (uint8_t)BYTE_IN;
    }

    // Unpack Address
    OUT_PACKET->ADDRESS = (((uint32_t)PACKET_ARRARY[0]) << 24) |
                          (((uint32_t)PACKET_ARRARY[1]) << 16) |
                          (((uint32_t)PACKET_ARRARY[2]) << 8)  |
                          (((uint32_t)PACKET_ARRARY[3]));
    memcpy(OUT_PACKET->PAYLOAD, &PACKET_ARRARY[4], HEX_PAYLOAD_SIZE_BYTES);

    return true;
}


/* -------------------------------------------------------------------------- */
/*                                    MAIN                                    */
/* -------------------------------------------------------------------------- */
int main()
{
    /* -------------------------------------------------------------------------- */
    /*                     (1) Initialization                                     */
    /* -------------------------------------------------------------------------- */
    stdio_init_all();
    S08_PIO_INIT();

    /* -------------------------------------------------------------------------- */
    /*                (2a) Wait for Valid MCU from USB_RP_COM.py                  */
    /* -------------------------------------------------------------------------- */
    while (true)
    {
        uint8_t DEVICE_FAMILY = IDENTIFY_MCU();
        size_t  PACKET_COUNTER = 0;

        if (DEVICE_FAMILY == FAMILY_MC9S08PA4)
        {
            S19Packet_t S19_ACTIVE_PACKET;

            memset(S19_STAGING_BUFFER, 0, sizeof(S19_STAGING_BUFFER));

            size_t MAX_S19_ELEMENTS = sizeof(S19_STAGING_BUFFER) / sizeof(S19_STAGING_BUFFER[0]);

            while (LOAD_S19_DATA(&S19_ACTIVE_PACKET))
            {
                if (PACKET_COUNTER < MAX_S19_ELEMENTS)
                {
                    memcpy(&S19_STAGING_BUFFER[PACKET_COUNTER], &S19_ACTIVE_PACKET, sizeof(S19Packet_t));
                    PACKET_COUNTER++;
                } 
            }
        
            PROGRAM_STATUS = PROGRAM_MC9S08PA4(S19_STAGING_BUFFER, PACKET_COUNTER);
        }
        
        if((2 <= DEVICE_FAMILY) && (DEVICE_FAMILY <= FAMILY_MCU_LAST))
        {
            HEXPacket_t HEX_ACTIVE_PACKET;                  
                     
            memset(HEX_STAGING_BUFFER, 0, sizeof(HEX_STAGING_BUFFER));  

            size_t MAX_PIC_ELEMENTS = sizeof(HEX_STAGING_BUFFER) / sizeof(HEX_STAGING_BUFFER[0]);
            
            while (LOAD_HEX_DATA(&HEX_ACTIVE_PACKET))  
            {
                if (PACKET_COUNTER < MAX_PIC_ELEMENTS)
                {
                    memcpy(&HEX_STAGING_BUFFER[PACKET_COUNTER], &HEX_ACTIVE_PACKET, sizeof(HEXPacket_t));   
                    PACKET_COUNTER++;
                }
            }

            // Determine MCU and execute programming routine
            if (DEVICE_FAMILY == FAMILY_PIC12F157X)
            {
                PROGRAM_STATUS = PROGRAM_PIC12F157X(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_PIC16F183XX)
            {
                PROGRAM_STATUS = PROGRAM_PIC16F183XX(HEX_STAGING_BUFFER, PACKET_COUNTER);   
            }
            else if (DEVICE_FAMILY == FAMILY_PIC18FXXK80)
            {
                PROGRAM_STATUS = PROGRAM_PIC18FXXK80(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_PIC18F2XK83)
            {
                PROGRAM_STATUS = PROGRAM_PIC18F2XK83(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_PIC18FXXQ8X)
            {
                PROGRAM_STATUS = PROGRAM_PIC18FXXQ8X(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA192A3U)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA192A3U(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA32C3)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA32C3(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA32E5)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA32E5(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA64AU)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA64AU(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA128A3U)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA128A3U(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
            else if (DEVICE_FAMILY == FAMILY_ATXMEGA128A4U)
            {
                PROGRAM_STATUS = PROGRAM_ATXMEGA128A4U(HEX_STAGING_BUFFER, PACKET_COUNTER);
            }
        }

        /* -------------------------------------------------------------------------- */
        /*                (2b) PASS or FAIL?                                          */
        /* -------------------------------------------------------------------------- */
        if (PROGRAM_STATUS == true)
        {
            printf("PASS\n");
            PROGRAM_STATUS = false;
        }
        else
        {
            printf("FAIL\n");
            PROGRAM_STATUS = false;
        }
    }
    return 0;
}
/* -------------------------------------------------------------------------- */
/*                                  END MAIN                                  */
/* -------------------------------------------------------------------------- */
