/* -------------------------------------------------------------------------- */
/*                                  Headers                                   */
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tx_byte.pio.h"
#include "rx_byte.pio.h"
#include "src/HCS08_BDM.h"


/* -------------------------------------------------------------------------- */
/*                                  Defines                                   */
/* -------------------------------------------------------------------------- */
// System clock (default: ~150MHz)
#define SM_MHZ                                  ((uint8_t)(clock_get_hz(clk_sys) / 1000000UL))

// S08 Command Opcodes
#define S08_CMD_READ_BYTE                       0xE0u        // READ_BYTE
#define S08_CMD_READ_PROGRAM_PROGRAM_STATUS     0xE4u        // READ_PROGRAM_PROGRAM_STATUS
#define S08_CMD_WRITE_BYTE                      0xC0u        // WRITE_BYTE

// Register Addresses
#define SYS_SDIDH                               0x3002u      // Device ID High register
#define SYS_SDIDL                               0x3003u      // Device ID Low  register
#define NVM_FCLKDIV                             0x3020u      // FDIVLD=bit[7], FDIVCK=bit[6], FDIV=bits[5:0]
#define NVM_FCCOBIX                             0x3022u       
#define NVM_FCCOBHI                             0x302Au
#define NVM_FCCOBLO                             0x302Bu
#define NVM_FSTAT                               0x3026u
#define NVM_FPROT                               0x3028u
#define NVM_EEPROT                              0x3029u
#define NVM_FSEC                                0x3021u

// Flash & EEPROM Command Opcodes
#define S08_PROGRAM_FLASH                       0x06u        // Program up to 2 longwords
#define S08_ERASE_ALL_BLOCK                     0x08u        
#define S08_UNSECURE_NVM                        0x0Bu        
#define S08_PROGRAM_EEPROM                      0x11u

// Protection Register unlock values
#define NVM_FPROT_UNPROTECT_ALL                 0xFFu        // FPOPEN=1, FPHDIS=1 -> no flash protection
#define NVM_EEPROT_UNPROTECT_ALL                0x80u        // DPOPEN=1           -> no EEPROM protection

// Flash Configuration Field 
#define NVM_CFG_RESERVED_BYTE                   0xFFu        // 0xFF78-0xFF7B reserved, must be programmed to 0xFF
#define NVM_CFG_FPROT_BYTE                      0xFFu        // 0xFF7C flash protection byte  -> unprotected
#define NVM_CFG_EEPROT_BYTE                     0xFFu        // 0xFF7D EEPROM protection byte -> unprotected
#define NVM_CFG_FOPT_BYTE                       0xFFu        // 0xFF7E flash nonvolatile byte
#define NVM_CFG_FSEC_UNSECURED                  0xFEu        // 0xFF7F KEYEN=11 (backdoor disabled), SEC=10 (unsecured)
#define NVM_FSEC_BYTE_ADDRESS                   0xFF7Fu      // Flash security byte, reloaded into FSEC on every reset
#define NVM_CFG_FIELD_START                     0xFF78u      // First byte of the configuration field phrase
#define NVM_CFG_FIELD_END                       0xFF7Fu      // Last  byte of the configuration field phrase
#define NVM_FSEC_SEC_MASK                       0x03u        // FSEC[SEC] bits [1:0]
#define NVM_FSEC_SEC_UNSECURED                  0x02u        // SEC = 10 -> Unsecured (00/01/11 are all Secured)

// Flag bits
#define NVM_FSTAT_CCIF                          0x80u        // Command Complete Interrupt Flag
#define NVM_FSTAT_ACCERR                        0x20u        // Access Error
#define NVM_FSTAT_FPVIOL                        0x10u        // Protection Violation
#define NVM_FSTAT_ERR_CLR                       0x30u        // Write 1s to clear ACCERR | FPVIOL before a command
#define NVM_FSTAT_ERR_MASK                      0x33u        // ACCERR | FPVIOL | MGSTAT1 | MGSTAT0

// Device ID & Device ID Mask
#define MC9S08PA4_DEV_ID                        0x0043u
#define MC9S08PA4_DEV_ID_MASK                   0x0FFFu      // Upper 4-bits are Revision

// Clock Divider
static uint8_t     FCLKDIV;

// Image Sizes
static uint8_t     FLASH_IMAGE[NVM_FLASH_SIZE_BYTES];   // 0xF000 - 0xFFFF
static uint8_t     EEPROM_IMAGE[NVM_EEPROM_SIZE_BYTES]; // 0x3100 - 0x317F

// Bit-Banging Timing Constraints (in RP2040 system clock cycles)
static uint16_t    BIT_TIME_SM_CYCLES;                  // Total duration of a single BDM bit window
static uint16_t    TX_1_LOW_TIME_SM_CYCLES;             // Low phase duration when transmitting a logical '1'
static uint16_t    TX_1_HIGH_TIME_SM_CYCLES;            // High phase duration when transmitting a logical '1'
static uint16_t    TX_0_LOW_TIME_SM_CYCLES;             // Low phase duration when transmitting a logical '0'
static uint16_t    TX_0_HIGH_TIME_SM_CYCLES;            // High phase duration when transmitting a logical '0'
static uint16_t    RX_LOW_TIME_SM_CYCLES;               // Host-driven low phase to signal the target to start transmitting a bit
static uint16_t    RX_WAIT_TO_SAMPLE_TIME_SM_CYCLES;    // Delay after host release before sampling the data line
static uint16_t    RX_FINISH_HIGH_TIME_SM_CYCLES;       // Required high/idle recovery time before the next bit can start
static uint16_t    RX_CMD_TO_DATA_DELAY_SM_CYCLES;      // Turnaround delay required between a BDM command byte and its data packet

// PIO Configuration for Transmitting (TX) Data
static const PIO   TX_BYTE_PIO = pio0;                  // PIO hardware block instance designated for BDM serial transmission
static const uint  TX_BYTE_SM  = 0;                     // State machine index within the TX PIO instance
static uint        TX_BYTE_OFFSET;                      // Program counter memory offset where the TX assembly code is loaded

// PIO Configuration for Receiving (RX) Data
static const PIO   RX_BYTE_PIO = pio1;                  // PIO hardware block instance designated for BDM serial reception
static const uint  RX_BYTE_SM  = 0;                     // State machine index within the RX PIO instance
static uint        RX_BYTE_OFFSET;                      // Program counter memory offset where the RX assembly code is loaded


/* -------------------------------------------------------------------------- */
/*                                  Handlers                                  */
/* -------------------------------------------------------------------------- */
/**
 * DESCRIPTION: Loads the TX/RX PIO programs and hands the BKGD pin to the TX state machine
 * INPUT:           ---
 * RETURN:          ---
 */
void S08_PIO_INIT(void)
{
    TX_BYTE_OFFSET = pio_add_program(TX_BYTE_PIO, &tx_byte_program);
    RX_BYTE_OFFSET = pio_add_program(RX_BYTE_PIO, &rx_byte_program);

    tx_byte_program_init(TX_BYTE_PIO, TX_BYTE_SM, TX_BYTE_OFFSET, PIN_BKGD, 32);
    rx_byte_program_init(RX_BYTE_PIO, RX_BYTE_SM, RX_BYTE_OFFSET, PIN_BKGD);

    tx_byte_claim_pin(TX_BYTE_PIO, TX_BYTE_SM, TX_BYTE_OFFSET, PIN_BKGD);
}

/**
 * DESCRIPTION: Power Cycle MCU
 * INPUT:           ---
 * RETURN:          ---
 */
static void S08_POWER_CYCLE(void)
{
    gpio_put(PIN_VDD_DISABLE, 1);
    sleep_ms(100);
    gpio_put(PIN_VDD_DISABLE, 0);
    sleep_ms(100);
}

/**
 * DESCRIPTION: (A) Enters Active Background (B) Sends SYNC (C) Determines BDC clock
 * INPUT:           ---
 * RETURN:      1 BDC Clock (nanoseconds)
 */
static uint64_t S08_CMD_SYNC(void)
{
    // (1) Initialize GPIO
    gpio_init(PIN_VDD_DISABLE);
    gpio_init(PIN_RESET);
    gpio_init(PIN_BKGD);

    gpio_set_dir(PIN_VDD_DISABLE, GPIO_OUT);
    gpio_set_dir(PIN_RESET, GPIO_OUT);
    gpio_set_dir(PIN_BKGD, GPIO_OUT);

    // (2) POWER - "OFF"
    // (3) Pull RESET & BKGD Pin LOW
    gpio_put(PIN_VDD_DISABLE, 1);
    gpio_put(PIN_RESET, 0);
    gpio_put(PIN_BKGD, 0);

    sleep_ms(100);

    // (4) POWER - "ON"
    gpio_put(PIN_VDD_DISABLE, 0);
    sleep_ms(10);

    // (5) RESET - "HIGH"
    gpio_put(PIN_RESET, 1);
    sleep_ms(10);

    // (6) BKGD - "HIGH" for 20 us
    gpio_put(PIN_BKGD, 1);
    sleep_us(20);

    // (6) BKGD - "LOW" for 32 us
    gpio_put(PIN_BKGD, 0);
    sleep_us(32);

    // (7) Speed-up pulse
    gpio_put(PIN_BKGD, 1);
    gpio_set_dir(PIN_BKGD, GPIO_IN);

    // (7b) Arm SysTick @ 6.67 ns
    systick_hw->csr = 0;
    systick_hw->rvr = 0x00FFFFFFu;
    systick_hw->cvr = 0;            // any write clears the counter and reloads it from RVR
    systick_hw->csr = 0x5u;         // ENABLE | CLKSOURCE=processor clock, interrupt off

    // (8) Wait for target to pull BKGD LOW
    uint64_t START_TIMEOUT = time_us_64();
    while (gpio_get(PIN_BKGD)) 
    {
        if (time_us_64() - START_TIMEOUT > 10000) { // 10ms timeout
            printf("SYNC Error: Target failed to assert pulse.\n");
            return 0;
        }
    }

    // (9) Capture the start of the pulse
    uint32_t PULSE_START_TICKS = systick_hw->cvr;

    // (10) Wait for BKGD to go back HIGH
    uint64_t END_TIMEOUT = time_us_64();
    while (!gpio_get(PIN_BKGD)) 
    {
        if (time_us_64() - END_TIMEOUT > 10000) { // 10ms timeout
            printf("SYNC Error: Target failed to release pulse.\n");
            return 0;
        }
    }

    // (10) Capture the exact end of the pulse
    uint32_t PULSE_END_TICKS = systick_hw->cvr;

    // (11) Measure and print Sync pulse
    uint32_t PULSE_TICKS   = (PULSE_START_TICKS - PULSE_END_TICKS) & 0x00FFFFFFu;
    uint64_t TICKS_PER_BDC = (uint64_t)SM_MHZ * 128ULL;
    uint64_t BDC_CLOCK_NS  = (((uint64_t)PULSE_TICKS * 1000ULL) + (TICKS_PER_BDC / 2ULL)) / TICKS_PER_BDC;

    printf("BDC clk=%llu ns/cycle\n", (unsigned long long)BDC_CLOCK_NS);

    // (11) Return 1 BDC Clock (in nanoseconds)
    return BDC_CLOCK_NS;
}

/**
 * DESCRIPTION: Writes 1 byte to MCU
 * INPUT:       [pio] Destination PIO block  [sm] Target state machine              [low/high_cycles] Timing limits
 *              [cmd] 8-bit command          [address] 16-bit target memory offset  [data] 8-bit write payload
 * RETURN:          ---
 */
static inline void S08_TX_BYTE(PIO pio, uint sm, uint32_t LOW_CYCLES, uint32_t HIGH_CYCLES, uint8_t CMD, uint16_t ADDRESS, uint8_t DATA)
{
    uint32_t PACKET = ((uint32_t)CMD << 24) | ((uint32_t)ADDRESS << 8) | (uint32_t)DATA;

    tx_byte_load_timing(pio, sm, LOW_CYCLES, HIGH_CYCLES);
    tx_byte_send(pio, sm, PACKET, 32);
}

/**
 * DESCRIPTION: Reads 1 byte from MCU
 * INPUT:       [pio] Destination PIO block  [sm] Target state machine              [low/high_cycles] Timing limits
 *              [cmd] 8-bit command          [address] 16-bit target memory offset
 * RETURN:      8-bit read payload
 */
static inline uint8_t S08_RX_BYTE(PIO pio, uint sm, uint32_t LOW_CYCLES, uint32_t HIGH_CYCLES, uint8_t CMD, uint16_t ADDRESS)
{
    // Construct 24-bit tx packet
    uint32_t PACKET = ((uint32_t)CMD << 16) | (uint32_t)ADDRESS;

    // Load transmission timing configurations and send
    tx_byte_load_timing(pio, sm, LOW_CYCLES, HIGH_CYCLES);
    tx_byte_send(pio, sm, PACKET, 24);

    // Safely disable TX state machine
    tx_byte_wait_idle(pio, sm);
    pio_sm_set_enabled(pio, sm, false);

    // Seize bus control and shift ownership over to the RX state machine
    rx_byte_claim_pin(RX_BYTE_PIO, RX_BYTE_SM, RX_BYTE_OFFSET, PIN_BKGD);

    // Stream out dynamic loop clocks per bit and capture the incoming data payload
    uint8_t DATA = rx_byte_receive(RX_BYTE_PIO, RX_BYTE_SM,
                                   RX_CMD_TO_DATA_DELAY_SM_CYCLES,
                                   RX_LOW_TIME_SM_CYCLES,
                                   RX_WAIT_TO_SAMPLE_TIME_SM_CYCLES,
                                   RX_FINISH_HIGH_TIME_SM_CYCLES);

    // Disable the RX engine and return line ownership back to the TX state machine
    pio_sm_set_enabled(RX_BYTE_PIO, RX_BYTE_SM, false);
    tx_byte_claim_pin(pio, sm, TX_BYTE_OFFSET, PIN_BKGD);

    return DATA;
}

/**
 * DESCRIPTION: Converts target BDC clock cycles into state machine execution cycles
 * INPUT:       [target_cycles] Target cycle count  [BDC_CLK_NS] Calibrated clock in ns  [sm_mhz] PIO speed in MHz
 * RETURN:      Calculated state machine cycles (rounded to nearest integer)
 */
static uint16_t S08_CONVERT_TO_SM_CYCLES(uint16_t TARGET_CYCLES, uint16_t BDC_CLK_NS, uint8_t sm_mhz)
{
    // Multiply target cycles by nanoseconds per cycle and PIO speed, then round to nearest integer via +500/1000
    return (uint16_t)((((uint64_t)TARGET_CYCLES * BDC_CLK_NS * sm_mhz) + 500ULL) / 1000ULL);
}

/**
 * DESCRIPTION: Launches a staged NVM command (write 1 to FSTAT[CCIF]) and waits for completion
 * INPUT:           ---
 * RETURN:      Final FSTAT value (test against NVM_FSTAT_ERR_MASK for ACCERR/FPVIOL/MGSTAT)
 */
static uint8_t S08_NVM_LAUNCH_COMMAND(void)
{
    // (1) Launch the command by writing 1 to FSTAT[CCIF]
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FSTAT, NVM_FSTAT_CCIF);

    // (2) Read FSTAT until the CCIF bit (0x80) is set back to 1 (Command complete)
    uint8_t FSTAT = 0;
    do {
        FSTAT = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, NVM_FSTAT);
    } while ((FSTAT & NVM_FSTAT_CCIF) == 0); // Wait until CCIF == 1

    // (3) Hand the raw status back so the caller can report ACCERR / FPVIOL / MGSTAT
    return FSTAT;
}

/**
 * DESCRIPTION: Programs one 8-byte flash phrase (two longwords) with the Program Flash command
 * INPUT:       [address] Phrase base address, must be longword aligned  [data] 8 source bytes
 * RETURN:      Final FSTAT value (test against NVM_FSTAT_ERR_MASK)
 *
 */
static uint8_t S08_PROGRAM_FLASH_PHRASE(uint16_t ADDRESS, const uint8_t* DATA)
{
    // (1) Clear any stale ACCERR / FPVIOL before starting the command write sequence (RM Figure 4-3)
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FSTAT, NVM_FSTAT_ERR_CLR);

    // (2) FCCOBIX = 0 : command opcode + global address [23:16]
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, 0);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, S08_PROGRAM_FLASH);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, NVM_FLASH_GLOBAL_HI);

    // (3) FCCOBIX = 1 : global address [15:0] of the phrase
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, 1);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, (uint8_t)(ADDRESS >> 8));
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, (uint8_t)(ADDRESS & 0xFFu));

    // (4) FCCOBIX = 2..5 : the four 16-bit program values, HI = lower address byte (big endian)
    for (uint8_t WORD = 0; WORD < 4u; WORD++)
    {
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, (uint8_t)(WORD + 2u));
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, DATA[(WORD * 2u) + 0u]);
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, DATA[(WORD * 2u) + 1u]);
    }

    // (5) Launch and wait for CCIF
    return S08_NVM_LAUNCH_COMMAND();
}

/**
 * DESCRIPTION: Programs 1 to 4 EEPROM bytes with the Program EEPROM command
 * INPUT:       [address] First EEPROM byte address  [data] source bytes  [count] 1..4 bytes
 * RETURN:      Final FSTAT value (test against NVM_FSTAT_ERR_MASK)
 */
static uint8_t S08_PROGRAM_EEPROM_BYTES(uint16_t ADDRESS, const uint8_t* DATA, uint8_t COUNT)
{
    // (1) Clear any stale ACCERR / FPVIOL before starting the command write sequence
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FSTAT, NVM_FSTAT_ERR_CLR);

    // (2) FCCOBIX = 0 : command opcode + global address [23:16]
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, 0);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, S08_PROGRAM_EEPROM);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, NVM_EEPROM_GLOBAL_HI);

    // (3) FCCOBIX = 1 : global address [15:0] of the first byte
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, 1);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, (uint8_t)(ADDRESS >> 8));
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, (uint8_t)(ADDRESS & 0xFFu));

    // (4) FCCOBIX = 2..(count+1) 
    for (uint8_t BYTE = 0; BYTE < COUNT; BYTE++)
    {
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, (uint8_t)(BYTE + 2u));
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, DATA[BYTE]);
        S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBLO, DATA[BYTE]);
    }

    // (5) Launch and wait for CCIF
    return S08_NVM_LAUNCH_COMMAND();
}

/**
 * DESCRIPTION: Programs MC9S08PA4 MCU
 * INPUT:           ---
 * RETURN:      True=SUCCESS, False=FAILURE
 */
bool PROGRAM_MC9S08PA4(const S19Packet_t* BUFFER, size_t TOTAL_PACKETS)
{
    /* -------------------------------------------------------------------------- */
    /*           (1) BDM Entry and determine 1 BDC Clk from SYNC pulse            */
    /* -------------------------------------------------------------------------- */
    uint16_t BDC_CLK_NS = S08_CMD_SYNC();


    /* -------------------------------------------------------------------------- */
    /*           (2) Calculate tx_bit and rx_bit pulsing metrics                  */
    /* -------------------------------------------------------------------------- */
    uint32_t IRQ_PROGRAM_STATUS         = save_and_disable_interrupts();

    uint16_t BDC_TIMING_NS = (uint16_t)(((uint32_t)BDC_CLK_NS * 108u + 50u) / 100u);           // +8% guard band

    // Map targets specified in BDC cycles to exact state machine cycle counts
    BIT_TIME_SM_CYCLES                  = S08_CONVERT_TO_SM_CYCLES(16, BDC_TIMING_NS, SM_MHZ); // 16 cycles
    TX_1_LOW_TIME_SM_CYCLES             = S08_CONVERT_TO_SM_CYCLES(3 , BDC_TIMING_NS, SM_MHZ); // 3  cycles
    TX_1_HIGH_TIME_SM_CYCLES            = S08_CONVERT_TO_SM_CYCLES(13, BDC_TIMING_NS, SM_MHZ); // 13 cycles
    TX_0_LOW_TIME_SM_CYCLES             = TX_1_HIGH_TIME_SM_CYCLES;                            // 13 cycles
    TX_0_HIGH_TIME_SM_CYCLES            = TX_1_LOW_TIME_SM_CYCLES;                             // 3  cycles
    RX_LOW_TIME_SM_CYCLES               = TX_1_LOW_TIME_SM_CYCLES - 1;                         // 3  cycles, less 1 SM cycle
    RX_WAIT_TO_SAMPLE_TIME_SM_CYCLES    = S08_CONVERT_TO_SM_CYCLES(7, BDC_TIMING_NS, SM_MHZ);  // 7  cycles
    RX_FINISH_HIGH_TIME_SM_CYCLES       = RX_WAIT_TO_SAMPLE_TIME_SM_CYCLES;                    // 7  cycles
    RX_CMD_TO_DATA_DELAY_SM_CYCLES      = BIT_TIME_SM_CYCLES;                                  // 16 cycles

    // Determine FDIV bits for FCLKDIV
    if (BDC_CLK_NS < 50 || BDC_CLK_NS > 1000)        // Out of safe range for Flash operations
    {
        restore_interrupts(IRQ_PROGRAM_STATUS);
        S08_POWER_CYCLE();
        return false;
    }
    else if (BDC_CLK_NS <= 51)   { FCLKDIV = 0x13; } // 19.6 MHz - 20.0 MHz
    else if (BDC_CLK_NS <= 53)   { FCLKDIV = 0x12; } // 18.6 MHz - 19.6 MHz
    else if (BDC_CLK_NS <= 56)   { FCLKDIV = 0x11; } // 17.6 MHz - 18.6 MHz
    else if (BDC_CLK_NS <= 60)   { FCLKDIV = 0x10; } // 16.6 MHz - 17.6 MHz
    else if (BDC_CLK_NS <= 64)   { FCLKDIV = 0x0F; } // 15.6 MHz - 16.6 MHz
    else if (BDC_CLK_NS <= 68)   { FCLKDIV = 0x0E; } // 14.6 MHz - 15.6 MHz
    else if (BDC_CLK_NS <= 73)   { FCLKDIV = 0x0D; } // 13.6 MHz - 14.6 MHz
    else if (BDC_CLK_NS <= 79)   { FCLKDIV = 0x0C; } // 12.6 MHz - 13.6 MHz
    else if (BDC_CLK_NS <= 86)   { FCLKDIV = 0x0B; } // 11.6 MHz - 12.6 MHz
    else if (BDC_CLK_NS <= 94)   { FCLKDIV = 0x0A; } // 10.6 MHz - 11.6 MHz
    else if (BDC_CLK_NS <= 104)  { FCLKDIV = 0x09; } //  9.6 MHz - 10.6 MHz
    else if (BDC_CLK_NS <= 116)  { FCLKDIV = 0x08; } //  8.6 MHz -  9.6 MHz
    else if (BDC_CLK_NS <= 131)  { FCLKDIV = 0x07; } //  7.6 MHz -  8.6 MHz
    else if (BDC_CLK_NS <= 151)  { FCLKDIV = 0x06; } //  6.6 MHz -  7.6 MHz
    else if (BDC_CLK_NS <= 178)  { FCLKDIV = 0x05; } //  5.6 MHz -  6.6 MHz
    else if (BDC_CLK_NS <= 217)  { FCLKDIV = 0x04; } //  4.6 MHz -  5.6 MHz
    else if (BDC_CLK_NS <= 277)  { FCLKDIV = 0x03; } //  3.6 MHz -  4.6 MHz
    else if (BDC_CLK_NS <= 384)  { FCLKDIV = 0x02; } //  2.6 MHz -  3.6 MHz
    else if (BDC_CLK_NS <= 625)  { FCLKDIV = 0x01; } //  1.6 MHz -  2.6 MHz
    else                         { FCLKDIV = 0x00; } //  1.0 MHz -  1.6 MHz

    restore_interrupts(IRQ_PROGRAM_STATUS);


    /* -------------------------------------------------------------------------- */
    /*                          (3) Write FCLKDIV                                 */
    /* -------------------------------------------------------------------------- */
    // Claim BKGD from SYNC
    tx_byte_claim_pin(TX_BYTE_PIO, TX_BYTE_SM, TX_BYTE_OFFSET, PIN_BKGD);
    // Write FDIV bits
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCLKDIV, FCLKDIV);


    /* -------------------------------------------------------------------------- */
    /*                          (4) Read Device ID                                */
    /* -------------------------------------------------------------------------- */
    uint8_t  SDIDH     = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, SYS_SDIDH);
    uint8_t  SDIDL     = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, SYS_SDIDL);
    uint16_t DEV_ID    = ((uint16_t)SDIDH << 8) | SDIDL;

    if ((DEV_ID & MC9S08PA4_DEV_ID_MASK) == MC9S08PA4_DEV_ID)
    {
        printf("Device ID = 0x%04X (part ID 0x%03X, silicon rev 0x%X)\n", DEV_ID, DEV_ID & MC9S08PA4_DEV_ID_MASK, DEV_ID >> 12);
    }
    else
    {
        printf("Device ID mismatch: expected 0x%03X\n", MC9S08PA4_DEV_ID);
        S08_POWER_CYCLE();
        return false;
    }

    /* -------------------------------------------------------------------------- */
    /*                          (5) Erase MCU                                     */
    /* -------------------------------------------------------------------------- */
    // (A) Reclaim BKGD pin for TX
    tx_byte_claim_pin(TX_BYTE_PIO, TX_BYTE_SM, TX_BYTE_OFFSET, PIN_BKGD);
    
    // (B) Disable FLASH & EEPROM protection
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FPROT,  NVM_FPROT_UNPROTECT_ALL);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_EEPROT, NVM_EEPROT_UNPROTECT_ALL);
    
    // (C) Clear any stale ACCERR / FPVIOL
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FSTAT, NVM_FSTAT_ERR_CLR);
    
    // (D) Erase ALL blocks
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBIX, 0);
    S08_TX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_WRITE_BYTE, NVM_FCCOBHI, S08_ERASE_ALL_BLOCK);

    // (E) Launch (write 1 to FSTAT[CCIF]) and poll FSTAT until CCIF == 1 (Command complete)
    uint8_t FSTAT = S08_NVM_LAUNCH_COMMAND();

    // (F) Check for Flash errors (ACCERR = 0x20, FPVIOL = 0x10, MGSTAT[1:0] = 0x03)
    if (FSTAT & NVM_FSTAT_ERR_MASK)
    {
        printf("ERASE FAILED (FSTAT=0x%02X)\n", FSTAT);
        S08_POWER_CYCLE();
        return false;
    }

    printf("ERASE OK (all flash and EEPROM blocks)\n");


    /* -------------------------------------------------------------------------- */
    /*                          (6) Program FLASH                                 */
    /* -------------------------------------------------------------------------- */
    memset(FLASH_IMAGE,  NVM_ERASED_BYTE, sizeof(FLASH_IMAGE));
    memset(EEPROM_IMAGE, NVM_ERASED_BYTE, sizeof(EEPROM_IMAGE));

    uint16_t LOWEST_FLASH  = NVM_FLASH_END;
    uint16_t HIGHEST_FLASH = NVM_FLASH_START;
    size_t   PACKETS_USED  = 0;

    for (size_t PACKET = 0; PACKET < TOTAL_PACKETS; PACKET++)
    {
        uint16_t ADDRESS = BUFFER[PACKET].ADDRESS;
        uint16_t MERGED  = 0;

        for (uint16_t OFFSET = 0; OFFSET < S19_PAYLOAD_SIZE_BYES; OFFSET++)
        {
            uint32_t TARGET = (uint32_t)ADDRESS + OFFSET;
            uint8_t  SOURCE = BUFFER[PACKET].PAYLOAD[OFFSET];

            if ((TARGET >= NVM_FLASH_START) && (TARGET <= NVM_FLASH_END))
            {
                FLASH_IMAGE[TARGET - NVM_FLASH_START] &= SOURCE;
                MERGED++;

                if (SOURCE != NVM_ERASED_BYTE)
                {
                    if ((uint16_t)TARGET < LOWEST_FLASH)  { LOWEST_FLASH  = (uint16_t)TARGET; }
                    if ((uint16_t)TARGET > HIGHEST_FLASH) { HIGHEST_FLASH = (uint16_t)TARGET; }
                }
            }
            else if ((TARGET >= NVM_EEPROM_START) && (TARGET <= NVM_EEPROM_END))
            {
                EEPROM_IMAGE[TARGET - NVM_EEPROM_START] &= SOURCE;
                MERGED++;
            }
        }

        if (MERGED == 0)
        {
            printf("SKIPPED packet @0x%04X (outside flash 0x%04X-0x%04X and EEPROM 0x%04X-0x%04X)\n",
                   ADDRESS, NVM_FLASH_START, NVM_FLASH_END, NVM_EEPROM_START, NVM_EEPROM_END);
        }
        else
        {
            PACKETS_USED++;
        }
    }

    printf("[PICO] Merged %zu of %zu packets; flash data spans 0x%04X-0x%04X\n",
           PACKETS_USED, TOTAL_PACKETS, LOWEST_FLASH, HIGHEST_FLASH);

    bool S19_COVERS_CONFIG_FIELD = false;

    for (uint16_t ADDRESS = NVM_CFG_FIELD_START; ADDRESS <= NVM_CFG_FIELD_END; ADDRESS++)
    {
        if (FLASH_IMAGE[ADDRESS - NVM_FLASH_START] != NVM_ERASED_BYTE)
        {
            S19_COVERS_CONFIG_FIELD = true;
        }
    }

    uint16_t PHRASES_PROGRAMMED = 0;

    for (uint16_t OFFSET = 0; OFFSET < NVM_FLASH_SIZE_BYTES; OFFSET += NVM_FLASH_PHRASE_BYTES)
    {
        bool PHRASE_IS_ERASED = true;

        for (uint8_t BYTE = 0; BYTE < NVM_FLASH_PHRASE_BYTES; BYTE++)
        {
            if (FLASH_IMAGE[OFFSET + BYTE] != NVM_ERASED_BYTE)
            {
                PHRASE_IS_ERASED = false;
                break;
            }
        }

        if (PHRASE_IS_ERASED)
        {
            continue;
        }

        uint16_t ADDRESS = (uint16_t)(NVM_FLASH_START + OFFSET);

        FSTAT = S08_PROGRAM_FLASH_PHRASE(ADDRESS, &FLASH_IMAGE[OFFSET]);

        if (FSTAT & NVM_FSTAT_ERR_MASK)
        {
            printf("PROGRAM FLASH FAILED @0x%04X (FSTAT=0x%02X)\n", ADDRESS, FSTAT);
            S08_POWER_CYCLE();
            return false;
        }

        PHRASES_PROGRAMMED++;
    }

    printf("PROGRAM FLASH OK (%u phrases)\n", PHRASES_PROGRAMMED);


    /* -------------------------------------------------------------------------- */
    /*                          (7) Verify FLASH                                  */
    /* -------------------------------------------------------------------------- */
    for (uint16_t OFFSET = 0; OFFSET < NVM_FLASH_SIZE_BYTES; OFFSET++)
    {
        uint16_t BYTE_ADDRESS = (uint16_t)(NVM_FLASH_START + OFFSET);
        uint8_t  EXPECTED     = FLASH_IMAGE[OFFSET];
        uint8_t  ACTUAL       = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, BYTE_ADDRESS);

        if (ACTUAL != EXPECTED)
        {
            uint8_t RETRY = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, BYTE_ADDRESS);

            printf("VERIFY FLASH FAILED @0x%04X (expected 0x%02X, read 0x%02X, re-read 0x%02X)\n",
                   BYTE_ADDRESS, EXPECTED, ACTUAL, RETRY);
            S08_POWER_CYCLE();
            return false;
        }
    }

    printf("VERIFY FLASH OK\n");


    /* -------------------------------------------------------------------------- */
    /*                          (8) Program EEPROM                                */
    /* -------------------------------------------------------------------------- */
    uint16_t BURSTS_PROGRAMMED = 0;

    for (uint16_t OFFSET = 0; OFFSET < NVM_EEPROM_SIZE_BYTES; OFFSET += NVM_EEPROM_BURST_BYTES)
    {
        bool BURST_IS_ERASED = true;

        for (uint8_t BYTE = 0; BYTE < NVM_EEPROM_BURST_BYTES; BYTE++)
        {
            if (EEPROM_IMAGE[OFFSET + BYTE] != NVM_ERASED_BYTE)
            {
                BURST_IS_ERASED = false;
                break;
            }
        }

        if (BURST_IS_ERASED)
        {
            continue;
        }

        uint16_t ADDRESS = (uint16_t)(NVM_EEPROM_START + OFFSET);

        FSTAT = S08_PROGRAM_EEPROM_BYTES(ADDRESS, &EEPROM_IMAGE[OFFSET], (uint8_t)NVM_EEPROM_BURST_BYTES);

        if (FSTAT & NVM_FSTAT_ERR_MASK)
        {
            printf("PROGRAM EEPROM FAILED @0x%04X (FSTAT=0x%02X)\n", ADDRESS, FSTAT);
            S08_POWER_CYCLE();
            return false;
        }

        BURSTS_PROGRAMMED++;
    }

    printf("PROGRAM EEPROM OK (%u bursts)\n", BURSTS_PROGRAMMED);


    /* -------------------------------------------------------------------------- */
    /*                          (9) Verify EEPROM                                 */
    /* -------------------------------------------------------------------------- */
    for (uint16_t OFFSET = 0; OFFSET < NVM_EEPROM_SIZE_BYTES; OFFSET++)
    {
        uint16_t BYTE_ADDRESS = (uint16_t)(NVM_EEPROM_START + OFFSET);
        uint8_t  EXPECTED     = EEPROM_IMAGE[OFFSET];
        uint8_t  ACTUAL       = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, BYTE_ADDRESS);

        if (ACTUAL != EXPECTED)
        {
            uint8_t RETRY = S08_RX_BYTE(TX_BYTE_PIO, TX_BYTE_SM, TX_1_LOW_TIME_SM_CYCLES, TX_1_HIGH_TIME_SM_CYCLES, S08_CMD_READ_BYTE, BYTE_ADDRESS);

            printf("VERIFY EEPROM FAILED @0x%04X (expected 0x%02X, read 0x%02X, re-read 0x%02X)\n",
                   BYTE_ADDRESS, EXPECTED, ACTUAL, RETRY);
            S08_POWER_CYCLE();
            return false;
        }
    }

    printf("VERIFY EEPROM OK\n");


    /* -------------------------------------------------------------------------- */
    /*        (10) Program the Flash Configuration Field to the unsecured state   */
    /* -------------------------------------------------------------------------- */
    const uint8_t CONFIG_FIELD[NVM_FLASH_PHRASE_BYTES] =
    {
        NVM_CFG_RESERVED_BYTE,      // 0xFF78 reserved
        NVM_CFG_RESERVED_BYTE,      // 0xFF79 reserved
        NVM_CFG_RESERVED_BYTE,      // 0xFF7A reserved
        NVM_CFG_RESERVED_BYTE,      // 0xFF7B reserved
        NVM_CFG_FPROT_BYTE,         // 0xFF7C flash protection byte
        NVM_CFG_EEPROT_BYTE,        // 0xFF7D EEPROM protection byte
        NVM_CFG_FOPT_BYTE,          // 0xFF7E flash nonvolatile byte
        NVM_CFG_FSEC_UNSECURED      // 0xFF7F flash SECURITY byte
    };

    S08_PROGRAM_FLASH_PHRASE(NVM_CFG_FIELD_START, CONFIG_FIELD);


    /* -------------------------------------------------------------------------- */
    /*                        (11) Power Cycle MCU                                */
    /* -------------------------------------------------------------------------- */
    S08_POWER_CYCLE();


    /* -------------------------------------------------------------------------- */
    /*                      (12) PROGRAMMED SUCCESSFULLY                          */
    /* -------------------------------------------------------------------------- */
    return true;
}
