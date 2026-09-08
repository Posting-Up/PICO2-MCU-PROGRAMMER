######################################################################################################
##                                         Headers                                                  ##
######################################################################################################
import serial
import serial.tools.list_ports
import time
import argparse
import os 


######################################################################################################
##                                         Targets                                                  ##
######################################################################################################
NXP_TARGETS = {
    "MC9S08PA4"
}

PIC_TARGETS = {
    "PIC12F157X",
    "PIC16F183XX",
    "PIC18FXXK80",
    "PIC18F2XK83",
    "PIC18FXXQ8X"
}

AVR_PDI_TARGETS = {
    "ATXMEGA192A3U",
    "ATXMEGA32C3",
    "ATXMEGA32E5",
    "ATXMEGA64AU",
    "ATXMEGA128A3U",
    "ATXMEGA128A4U"
}


######################################################################################################
##                                          Defines                                                 ##
######################################################################################################
SERIAL_READ_TIMEOUT_S    = 15.0
SERIAL_WRITE_TIMEOUT_S   = 30.0     
INIT_ACK_TIMEOUT_S       = 3.0      
PROGRAM_WAIT_TIMEOUT_S   = 600.0
STREAM_CHUNK_BYTES       = 4096
HEX_MAX_PACKETS          = 8192


######################################################################################################
##                                         Functions                                                ##
######################################################################################################       
# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Scans COM ports for RP Pico 2 Vendor ID  |
# │                                                        |
# │ INPUT       : ---                                      │
# │ RETURNS     : COM_PORT_# (int)                         |
# └────────────────────────────────────────────────────────┘
def AUTO_DETECT_PICO2():
    PORTS = serial.tools.list_ports.comports()

    for port in PORTS:
        if port.vid == 0x2E8A:
            print(f"[+] Auto-detected PICO 2 on COM {port.device}")
            return port.device

    return None

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Parses S19 file (Motorola SREC)          │
# │                                                        │
# │ INPUT       : FILE_PATH (str)  - Path to .s19 file     │
# │               WIDTH     (int)  - Target byte length    │
# │                                                        │
# │ RETURNS     : list[dict] - Array of parsed records     │
# │                            [{'address', 'data_bytes'}] │
# │               None       - If FileNotFoundError        │
# └────────────────────────────────────────────────────────┘
def PARSE_S19_FILE(FILE_PATH, WIDTH):
    PARSED_RECORDS = []
    
    try:
        with open(FILE_PATH, 'r', encoding='utf-8', errors='ignore') as file:
            for line_num, line in enumerate(file, 1):
                line = line.strip()

                if not line:
                    continue
                
                if not line.startswith('S'):
                    print(f"[-] Line {line_num}: Invalid start character. Skipping.")
                    continue
                    
                RECORD_TYPE = line[0:2]
                
                if RECORD_TYPE == 'S1':
                    try:
                        BYTE_COUNT          = int(line[2:4], 16)
                        ADDR_STR            = line[4:8]
                        ADDR_INT            = int(ADDR_STR, 16)
                        
                        DATA_CHAR_LENGTH    = (BYTE_COUNT - 2 - 1) * 2
                        DATA_HEX            = line[8:8 + DATA_CHAR_LENGTH]
                        
                        CHECKSUM_STR        = line[8 + DATA_CHAR_LENGTH : 8 + DATA_CHAR_LENGTH + 2]
                        CHECKSUM_           = int(CHECKSUM_STR    , 16)
                        
                        FULL_HEX_LINE       = line[2 : 8 + DATA_CHAR_LENGTH]
                        BYTE_SUM            = sum(int(FULL_HEX_LINE[i:i+2], 16) for i in range(0, len(FULL_HEX_LINE), 2))
                        CALC_CHECKSUM       = (~BYTE_SUM  ) & 0xFF

                        if CALC_CHECKSUM != CHECKSUM_:
                            print(f"""
                            [-] S19 CHECKSUM ERROR
                            Line:        {line_num}
                            Record:      {line}
                            Byte Count:  0x{BYTE_COUNT:02X}
                            Address:     0x{ADDR_INT:04X}
                            Data Bytes:  {len(DATA_HEX)//2}
                            Expected:    0x{CHECKSUM_:02X}
                            Calculated:  0x{CALC_CHECKSUM:02X}
                        """)
                            continue
                        
                        RAW_BYTES = bytearray.fromhex(DATA_HEX)

                        if len(RAW_BYTES) < WIDTH:
                            RAW_BYTES.extend([0xFF] * (WIDTH - len(RAW_BYTES)))

                        PARSED_RECORDS.append({
                            'address': ADDR_INT,
                            'data_bytes': bytes(RAW_BYTES)
                        })
                    except ValueError:
                        print(f"[-] Line {line_num}: Error decoding hex string.")     

        return PARSED_RECORDS    

    except FileNotFoundError:
        print(f"[-] Error: The file '{FILE_PATH}' was not found.")
        return None

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Parses HEX file (Intel HEX)              │
# │                                                        │
# │ INPUT       : FILE_PATH (str)  - Path to .hex file     │
# │               WIDTH     (int)  - Target byte length    │
# │                                                        │
# │ RETURNS     : list[dict] - Array of parsed records     │
# │                            [{'address', 'data_bytes'}] │
# │               None       - If FileNotFoundError        │
# └────────────────────────────────────────────────────────┘
def PARSE_HEX_FILE(FILE_PATH, WIDTH):
    PARSED_RECORDS = []
    
    UPPER_ADDR_BITS = 0 
        
    try:
        with open(FILE_PATH, 'r', encoding='utf-8', errors='ignore') as file:
            for line_num, line in enumerate(file, 1):
                line = line.strip()

                if not line:
                    continue
                
                if not line.startswith(':'):
                    if ':' in line:
                        line = line[line.index(':'):]
                    else:
                        print(f"[-] Line {line_num}: Invalid start character. Skipping.")
                        continue

                try:
                    BYTE_COUNT  = int(line[1:3], 16)
                    LINE_OFFSET = int(line[3:7], 16)
                    RECORD_TYPE = int(line[7:9], 16) 
                    DATA_HEX    = line[9:9 + (BYTE_COUNT * 2)]
                except ValueError:
                    print(f"[-] Line {line_num}: Malformed hex values. Skipping.")
                    continue

                try:
                    RAW_LINE_TEXT = line.lstrip(':')
                    ALL_LINE_BYTES = bytes.fromhex(RAW_LINE_TEXT)
                    
                    if (sum(ALL_LINE_BYTES) & 0xFF) != 0:
                        ACTUAL_LINE_CHECKSUM  = ALL_LINE_BYTES[-1]
                        HEADER_AND_DATA_BYTES = ALL_LINE_BYTES[:-1]
                        CORRECT_CALC          = (256 - (sum(HEADER_AND_DATA_BYTES) & 0xFF)) & 0xFF
                        
                        print(f"[-] Line {line_num}: Checksum mismatch! (Expected: {hex(ACTUAL_LINE_CHECKSUM)}, Calc: {hex(CORRECT_CALC)}). Skipping.")
                        continue   
                except ValueError:
                    print(f"[-] Line {line_num}: Invalid hex characters in checksum validation string.")
                    continue
                
                if RECORD_TYPE == 2:    # EXTENDED SEGMENT ADDRESS RECORD
                    SEGMENT_BASE    = int(DATA_HEX, 16)
                    UPPER_ADDR_BITS = SEGMENT_BASE << 4
                elif RECORD_TYPE == 4:  # EXTENDED LINEAR ADDRESS RECORD
                    LINEAR_BASE     = int(DATA_HEX, 16)
                    UPPER_ADDR_BITS = LINEAR_BASE << 16  
                elif RECORD_TYPE == 0:  # DATA RECORD
                    ABSOLUTE_ADDR = UPPER_ADDR_BITS + LINE_OFFSET

                    RAW_BYTES = bytearray.fromhex(DATA_HEX)

                    CHUNK_START = 0

                    while CHUNK_START < len(RAW_BYTES):
                        chunk = RAW_BYTES[CHUNK_START:CHUNK_START + WIDTH]

                        if len(chunk) < WIDTH:
                            chunk.extend([0xFF] * (WIDTH - len(chunk)))
                        PARSED_RECORDS.append({
                            'address': ABSOLUTE_ADDR + CHUNK_START,
                            'data_bytes': bytes(chunk)
                        })

                        CHUNK_START += WIDTH
                elif RECORD_TYPE == 1:  # END OF FILE RECORD
                    print(f"[+] Reached Intel HEX End-Of-File marker at line {line_num}.")
                    break
                else:
                    continue     

        return PARSED_RECORDS

    except FileNotFoundError:
        print(f"[-] Error: The file '{FILE_PATH}' was not found.")
        return None

    
######################################################################################################
##                                            MAIN                                                  ##
######################################################################################################
def main():
    print("\n\n")
    print(" ╔═════════════════════════════════════════════╗ ")
    print(" ║                                             ║ ")
    print(" ║               PICO2 MCU PROG                ║ ")
    print(" ║                                             ║ ")
    print(" ╚═════════════════════════════════════════════╝ ")
    print("\n\n")

    # ========================================================================================
    #                     (1) Wait for valid parsed input                                    #
    # ========================================================================================
    PARSER = argparse.ArgumentParser()
    
    PARSER.add_argument('--MCU', type=str, required=True, help="Target MCU")
    PARSER.add_argument('--FW_FILE_PATH', type=str, required=True, help="Absolute FW Path")

    ARGS = PARSER.parse_args()

    # ========================================================================================
    #                           (2) Determe MCU                                              #
    # ========================================================================================
    TARGET_MCU = ARGS.MCU.upper()

    # ========================================================================================
    #                        (3) Verify FW_FILE_PATH                                         #
    # ========================================================================================
    ABS_FW = os.path.abspath(ARGS.FW_FILE_PATH)

    if not os.path.exists(ABS_FW):
        print(f"[+] Firmware file not found {ABS_FW}")
        print("\n>>> RESULT: FAIL <<<")
        return

    # ========================================================================================
    #                        (4) Parse S19 or HEX                                            #
    # ========================================================================================
    if TARGET_MCU in NXP_TARGETS:
        S19_PAYLOADS = PARSE_S19_FILE(ARGS.FW_FILE_PATH, WIDTH=64)
    elif TARGET_MCU in PIC_TARGETS or TARGET_MCU in AVR_PDI_TARGETS:
        HEX_PAYLOADS = PARSE_HEX_FILE(ARGS.FW_FILE_PATH, WIDTH=32)
    else:
        return

    # ========================================================================================
    #                         (5) Bit-Bang the MCU                                           #
    # ========================================================================================
    # OPEN CONNECTION
    PICO_PORT = AUTO_DETECT_PICO2()

    if PICO_PORT is not None:
        try:
            print(f"[+] Opening serial link to Pico2 on {PICO_PORT}...")
            
            PICO_CONNECTION = serial.Serial(PICO_PORT, baudrate=115200,
                                           timeout=SERIAL_READ_TIMEOUT_S,
                                           write_timeout=SERIAL_WRITE_TIMEOUT_S)
            time.sleep(2)

            PICO_CONNECTION.reset_input_buffer()
            PICO_CONNECTION.reset_output_buffer()

            # ========================================================================================
            #          (6) SEND DEVICE FAMILY AND CONFIRM ACK FROM RP PICO 2                         #
            # ========================================================================================
            print(f"[+] Configuring target device family: {ARGS.MCU}")

            INIT_CMD = f"INIT_FAMILY:{ARGS.MCU}\n"
            PICO_CONNECTION.write(INIT_CMD.encode('utf-8'))
            PICO_CONNECTION.flush()

            # WAIT FOR ACK - poll
            INIT_ACK  = False
            ACK_LIMIT = time.perf_counter() + INIT_ACK_TIMEOUT_S

            while time.perf_counter() < ACK_LIMIT:
                if PICO_CONNECTION.in_waiting == 0:
                    time.sleep(0.010)
                    continue

                line = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()

                if line == "MCU_FAMILY_IDENTIFIED":
                    INIT_ACK = True
                    break
                elif line:
                    print(f"    [PICO DEV LOG] {line}")

            if not INIT_ACK:
                print(f"[-] Error: Pico failed to acknowledge device initialization "
                      f"within {INIT_ACK_TIMEOUT_S:.0f}s. Aborting.")
                PICO_CONNECTION.close()
                return

            # ========================================================================================
            #            (7) HIGH-SPEED BINARY DATA STREAM (w/ MCU_FILTER)                           #
            # ========================================================================================
            START_TIME = time.perf_counter()

            # TARGET MCU == NXP
            if TARGET_MCU in NXP_TARGETS:
                print("[+] Starting high-speed binary stream ...")
    
                # STREAM DATA
                for index, block in enumerate(S19_PAYLOADS, 1):
                    BINARY_ADDR = block['address'].to_bytes(2, byteorder='big')
                    BINARY_DATA = block['data_bytes']
                        
                    # CONSTRUCT 66 BYTE PACKET
                    PACKET = BINARY_ADDR + BINARY_DATA
    
                    # SEND DATA
                    PICO_CONNECTION.write(PACKET)
                    PICO_CONNECTION.flush()
    
                # ========================================================================================
                #                      (8A) ...Line Transfer Completed                                   #
                # ========================================================================================           
                END_TIME    = time.perf_counter()
                DURATION_MS = (END_TIME - START_TIME)  * 1000
                print(f"[+] Data Transfer stream completed in {DURATION_MS:.2f} ms.")

            # TARGET MCU == PIC or AVR PDI
            if TARGET_MCU in PIC_TARGETS or TARGET_MCU in AVR_PDI_TARGETS:
                print("[+] Starting high-speed binary stream ...")

                # STREAM DATA
                if len(HEX_PAYLOADS) > HEX_MAX_PACKETS:
                    print(f"[-] Error: {len(HEX_PAYLOADS)} packets exceeds the Pico's "
                          f"HEX_STAGING_BUFFER capacity of {HEX_MAX_PACKETS}. "
                          f"The image would be silently truncated. Aborting.")
                    PICO_CONNECTION.close()
                    return

                # CONSTRUCT 36 BYTE PACKET
                STREAM = bytearray()
                for block in HEX_PAYLOADS:
                    STREAM += block['address'].to_bytes(4, byteorder='big')
                    STREAM += block['data_bytes']

                TOTAL_BYTES = len(STREAM)

                # SEND DATA
                for offset in range(0, TOTAL_BYTES, STREAM_CHUNK_BYTES):
                    PICO_CONNECTION.write(STREAM[offset:offset + STREAM_CHUNK_BYTES])

                PICO_CONNECTION.flush()

                # ========================================================================================
                #                     (8B) ...Line Transfer Completed                                    #
                # ======================================================================================== 
                END_TIME = time.perf_counter()
                DURATION_MS = (END_TIME - START_TIME) * 1000
                print(f"[+] Data Transfer stream completed in {DURATION_MS:.2f} ms "
                      f"({len(HEX_PAYLOADS)} packets, {TOTAL_BYTES} bytes).")

            # ========================================================================================
            #                          (9) Wait for PASS or FAIL                                     #
            # ======================================================================================== 
            print("[+] Stream complete. Waiting for target flash verification...")

            if TARGET_MCU in AVR_PDI_TARGETS:
                print("    (AVR PDI: no progress output is expected during programming")

            PREV_TIMEOUT             = PICO_CONNECTION.timeout
            PICO_CONNECTION.timeout  = PROGRAM_WAIT_TIMEOUT_S

            PROGRAM_FINISHED = False
            PROGRAM_START    = time.perf_counter()

            try:
                while not PROGRAM_FINISHED:
                    DEBUG_LINE = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()

                    if not DEBUG_LINE:  # TIMEOUT
                        print(f"[-] Error: Hardware operational check timed out after "
                              f"{PROGRAM_WAIT_TIMEOUT_S:.0f}s or stopped responding.")
                        PICO_CONNECTION.close()
                        return

                    if DEBUG_LINE == "PASS":
                        PROGRAM_MS = (time.perf_counter() - PROGRAM_START) * 1000
                        print(f"[+] On-chip programming phase completed in {PROGRAM_MS:.2f} ms "
                              f"({PROGRAM_MS / 1000.0:.1f} s).")
                        print("\n=======================================================")
                        print("[SUCCESS] FLASH SUCCESS: Target memory maps fully verified!")
                        print("=======================================================\n")
                        PROGRAM_FINISHED = True
                    elif DEBUG_LINE == "FAIL":
                        PROGRAM_MS = (time.perf_counter() - PROGRAM_START) * 1000
                        print(f"[+] On-chip programming phase ran for {PROGRAM_MS:.2f} ms "
                              f"({PROGRAM_MS / 1000.0:.1f} s) before reporting failure.")
                        print("[-] Error: Core hardware flashing matrix validation verification failed.")
                        PICO_CONNECTION.close()
                        return
                    else:
                        print(f"    [PICO DEV LOG] {DEBUG_LINE}")
            finally:
                if PICO_CONNECTION.is_open:
                    PICO_CONNECTION.timeout = PREV_TIMEOUT
    
        # ========================================================================================
        #                                EXCEPTIONS                                              #
        # ========================================================================================
        # SERIAL PORT COM ERROR
        except serial.SerialException as e:
            print(f"[-] Serial Port Error: {e}")
        # CLOSE USB CONNECTION
        finally:
            if 'PICO_CONNECTION' in locals() and PICO_CONNECTION.is_open:
                PICO_CONNECTION.close()
                print("[+] Connection closed safely.")        

    # ========================================================================================
    #                            PICO2 CONECTION ERROR                                       #
    # ========================================================================================
    else:
        print("[-] Error: No Raspberry Pi Pico detected. Check USB connections.")
                  
if __name__ == '__main__':
    main()
######################################################################################################
##                                          END MAIN                                                ##
######################################################################################################