/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "src/HCS08_BDM.h"
#include "PIC_ISP.h"
#include "AVR_PDI.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
#define FAMILY_MCHP_FIRST  2u
#define FAMILY_MCHP_LAST   7u

static bool        PROGRAM_STATUS;   
static uint8_t     DEVICE_FAMILY;                       
static size_t      PACKET_COUNTER;

static S19Packet_t S19_ACTIVE_PACKET;
static HEXPacket_t HEX_ACTIVE_PACKET; 

static S19Packet_t S19_STAGING_BUFFER[S19_MAX_PACKETS];    
static HEXPacket_t HEX_STAGING_BUFFER[HEX_MAX_PACKETS];    

static size_t      MAX_S19_ELEMENTS = sizeof(S19_STAGING_BUFFER) / sizeof(S19_STAGING_BUFFER[0]);
static size_t      MAX_HEX_ELEMENTS = sizeof(HEX_STAGING_BUFFER) / sizeof(HEX_STAGING_BUFFER[0]);


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Identifies MCU from USB_RP_COM.py
 * INPUT:           ---
 * RETURN:      [INT] Valid MCU #
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

                if (strcmp(INIT_BUF, "INIT_FAMILY:HCS08") == 0)        // MCU == HCS08?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_HCS08;
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
 * RETURN:      [TRUE]=PASS, [FALSE]=FAIL
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

    return true;
}

/**
 * DESCRIPTION: Loads HEX packets into a staging buffer
 * INPUT:       HEX Packets
 * RETURN:      [TRUE]=PASS, [FALSE]=FAIL
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
            // TIMEOUT
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
    /*                (2) Wait for Valid MCU from USB_RP_COM.py                   */
    /* -------------------------------------------------------------------------- */
    while (true)
    {
        DEVICE_FAMILY = IDENTIFY_MCU();
        PACKET_COUNTER = 0;

        // NXP
        if (DEVICE_FAMILY == FAMILY_HCS08)
        {
            memset(S19_STAGING_BUFFER, 0, sizeof(S19_STAGING_BUFFER));

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
        
        // MICROCHIP
        if((DEVICE_FAMILY >= FAMILY_MCHP_FIRST) && (DEVICE_FAMILY <= FAMILY_MCHP_LAST))
        {    
            memset(HEX_STAGING_BUFFER, 0, sizeof(HEX_STAGING_BUFFER));  

            while (LOAD_HEX_DATA(&HEX_ACTIVE_PACKET))  
            {
                if (PACKET_COUNTER < MAX_HEX_ELEMENTS)
                {
                    memcpy(&HEX_STAGING_BUFFER[PACKET_COUNTER], &HEX_ACTIVE_PACKET, sizeof(HEXPacket_t));   
                    PACKET_COUNTER++;
                }
            }

            // PROGRAM MCU
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
        }

        /* -------------------------------------------------------------------------- */
        /*                (3) PASS or FAIL?                                           */
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
