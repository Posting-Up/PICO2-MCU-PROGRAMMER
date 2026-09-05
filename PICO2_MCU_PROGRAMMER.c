/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "MC9S08PA4.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// Device family code
#define FAMILY_MC9S08PA4            1u
// S19 Payload Sizing
#define S19_PACKET_SIZE_BYTES       66u          
#define S19_PAYLOAD_SIZE_BYES       64u 


/* -------------------------------------------------------------------------- */
/*                                  Statics                                   */
/* -------------------------------------------------------------------------- */
static bool        PROGRAM_STATUS;                         // Programming State
static S19Packet_t S19_STAGING_BUFFER[S19_MAX_PACKETS];    // S19 packet staging buffer


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
        int c = getchar_timeout_us(1000);
        if (c != PICO_ERROR_TIMEOUT) 
        {
            if (c == '\n' || c == '\r') 
            {
                INIT_BUF[IDX] = '\0';

                if (strcmp(INIT_BUF, "INIT_FAMILY:MC9S08PA4") == 0)    // MCU == MC9S08PA4?
                {
                    printf("[PICO DEV LOG] Target signature matched hardware profile.\n");
                    printf("MCU_FAMILY_IDENTIFIED\n");
                    return FAMILY_MC9S08PA4;
                }

                IDX = 0; // Clear index if a stray/malformed line is captured
            } 
            else if (IDX < sizeof(INIT_BUF) - 1) 
            {
                INIT_BUF[IDX++] = (char)c;
            }
        }
    }
}

/**
 * DESCRIPTION: Loads S19 packets into a staging buffer
 * INPUT:       S19 Packets
 * RETURN:      True=PASS, False=FAIL
 */
static bool LOAD_S19_DATA(S19Packet_t* out_packet)
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
    out_packet->address = (PACKET_ARRARY[0] << 8) | PACKET_ARRARY[1];
    memcpy(out_packet->payload, &PACKET_ARRARY[2], S19_PAYLOAD_SIZE_BYES);

    // ACK
    printf("S19_LINE_SUCCESS\n");
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
