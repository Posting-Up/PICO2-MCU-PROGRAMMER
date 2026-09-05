######################################################################################################
##                                         Headers                                                  ##
######################################################################################################
import serial
import serial.tools.list_ports
import time
import argparse
import os 


######################################################################################################
##                                         Device Tables                                            ##
######################################################################################################
NXP_TARGETS = {
    "MC9S08PA4"
}


######################################################################################################
##                                         Functions                                                ##
######################################################################################################       
def AUTO_DETECT_PICO2():
    """Attempting to establish connection with RP PICO 2... Scanning COM PORTS"""

    PORTS = serial.tools.list_ports.comports()

    for port in PORTS:
        if port.vid == 0x2E8A:  
            print(f"[+] Automatically detected Raspberry Pi Pico on {port.device}")
            return port.device

    return None


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
                        BYTE_SUM            = sum(int(FULL_HEX_LINE    [i:i+2], 16) for i in range(0, len(FULL_HEX_LINE    ), 2))
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


######################################################################################################
##                                        MAIN                                                      ##
######################################################################################################
def main():
    # WAIT FOR USER INPUT
    PARSER = argparse.ArgumentParser(description="PICO2 Programmer Link Utility")   # PARSE INPUT
    
    # DEFINE USER ARGUMENTS
    PARSER.add_argument('--MCU', type=str, required=True, help="Target MCU (e.g., MC9S08PA4)")
    PARSER.add_argument('--FW_FILE_PATH', type=str, required=True, help="Absolute path to production firmware")
    
    # PROCESS USER ARGUEMENTS
    ARGS = PARSER.parse_args()

    print("\n")
    print("======== PICO2 MCU PROG =========")
    print("=================================")
    print("=================================\n")

    # ========================================================================================
    ## DETERMINE MCU BASED ON USER INPUT                                                    ##
    # ========================================================================================
    TARGET_MCU = ARGS.MCU.upper()

    # VERIFY FW_FILE_PATH
    ABS_FW = os.path.abspath(ARGS.FW_FILE_PATH)

    if not os.path.exists(ABS_FW):
        print(f"[+] Firmware file not found {ABS_FW}")
        print("\n>>> RESULT: FAIL <<<")
        return

    if TARGET_MCU in NXP_TARGETS:
        S19_PAYLOADS = PARSE_S19_FILE(ARGS.FW_FILE_PATH, WIDTH=64)
    else:
        return

    # ========================================================================================
    ## BIT BANG THE MCU                                                                     ##
    # ========================================================================================
    # OPEN CONNECTION
    PICO_PORT = AUTO_DETECT_PICO2()      # RETURN COMX

    if PICO_PORT is not None:
        try:
            print(f"[+] Opening serial link to Pico on {PICO_PORT}...")
            
            PICO_CONNECTION = serial.Serial(PICO_PORT, baudrate=115200, timeout=5.0)
            time.sleep(2) 

            # ========================================================================================
            # 1. SEND DEVICE FAMILY AND CONFIRM ACK FROM RP PICO 2                                   #
            # ========================================================================================
            print(f"[+] Configuring target device family: {ARGS.MCU}")

            INIT_CMD = f"INIT_FAMILY:{ARGS.MCU}\n"
            PICO_CONNECTION.write(INIT_CMD.encode('utf-8'))
            PICO_CONNECTION.flush()
                    
            # WAIT FOR ACK
            INIT_ACK = False
            time.sleep(0.100)
            while PICO_CONNECTION.in_waiting > 0:
                line = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()
                if line == "MCU_FAMILY_IDENTIFIED":
                    INIT_ACK = True
                elif line:
                    print(f"    [PICO DEV LOG] {line}")
                    
            if not INIT_ACK:
                print("[-] Error: Pico failed to acknowledge device initialization. Aborting.")
                PICO_CONNECTION.close()
                return

            # ========================================================================================
            # 2. HIGH-SPEED BINARY DATA STREAM (w/ MCU_FILTER)                                       #
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
                        
                    # print(f"[-] [{index}/{len(S19_PAYLOADS)}] Transmitting 66 binary bytes for address {hex(block['address'])}")
                    # print(f"    {PACKET.hex()}")
    
                    # SEND DATA
                    PICO_CONNECTION.write(PACKET)
                    PICO_CONNECTION.flush()
    
                    # COOLDOWN
                    time.sleep(0.010) 
                    ack_received = False
    
                    # ========================================================================================
                    # 3. ...Line Transfer Completed (WAIT FOR RP PICO ACK=S19_LINE_SUCCESS)                  #
                    # ======================================================================================== 
                    while not ack_received:
                        DEBUG_LINE = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()
                            
                        if not DEBUG_LINE:  # TIMEOUT
                            break
                                
                        if DEBUG_LINE == "S19_LINE_SUCCESS":
                            ack_received = True
                            break
                        else:
                            print(f"    [PICO DEV LOG] {DEBUG_LINE}")
    
                    if not ack_received:
                        print(f"[-] Fault or timeout encountered at address {hex(block['address'])}. Terminating link.")
                        print("    Raw response received from Pico TIMEOUT (Missing S19_LINE_SUCCESS)")
                        PICO_CONNECTION.close()
                        return                    
                            
                END_TIME    = time.perf_counter()
                DURATION_MS = (END_TIME - START_TIME)  * 1000
                print(f"[+] Data Transfer stream completed in {DURATION_MS:.2f} ms.")

            # ========================================================================================
            # 4. Wait for PASS or FAIL                                                               #
            # ======================================================================================== 
            print("[+] Stream complete. Waiting for target flash verification...")
    
            PROGRAM_FINISHED = False

            while not PROGRAM_FINISHED:
                DEBUG_LINE = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()
                    
                if not DEBUG_LINE:  # TIMEOUT
                    print("[-] Error: Hardware operational check timed out or stopped responding.")
                    PICO_CONNECTION.close()
                    return
                if DEBUG_LINE == "PASS":
                    print("\n=======================================================")
                    print("[SUCCESS] FLASH SUCCESS: Target memory maps fully verified!")
                    print("=======================================================\n")
                    PROGRAM_FINISHED = True
                elif DEBUG_LINE == "FAIL":
                    print("[-] Error: Core hardware flashing matrix validation verification failed.")
                    PICO_CONNECTION.close()
                    return
                else:
                    print(f"    [PICO DEV LOG] {DEBUG_LINE}")
    
        # ========================================================================================
        # 5. Exceptions                                                                          #
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
    ## (ERROR) NO PICO CONNECTED                                                            ##
    # ========================================================================================
    else:
        print("[-] Error: No Raspberry Pi Pico detected. Check USB connections.")
                  
if __name__ == '__main__':
    main()
######################################################################################################
##                                      END MAIN                                                    ##
######################################################################################################