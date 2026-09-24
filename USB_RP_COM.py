######################################################################################################
##                                         Headers                                                  ##
######################################################################################################
import serial
import serial.tools.list_ports
import time
import argparse
import os
import json
import subprocess
import sys
import sysconfig
import queue
import threading


######################################################################################################
##                                          Paths                                                   ##
######################################################################################################
PICOTOOL        = r"C:\pico2-universal-programmer-MAIN\2_VENDOR_TOOLS\picotool\2.3.0\picotool\picotool.exe"
OPENOCD         = r"C:\pico2-universal-programmer-MAIN\2_VENDOR_TOOLS\openocd\0.12.0+dev\openocd.exe"
OPENOCD_SCRIPTS = r"C:\pico2-universal-programmer-MAIN\2_VENDOR_TOOLS\openocd\0.12.0+dev\scripts"
PYMCUPROG       = r"C:\pico2-universal-programmer-MAIN\2_VENDOR_TOOLS\pymcuprog.exe"
STATE_FILE      = r"C:\pico2-universal-programmer-MAIN\PICO2_MCU_PROGRAMMER\firmware\UF2_STATE_PICO2.json"
PROJECT_DIR     = r"C:\pico2-universal-programmer-MAIN\PICO2_MCU_PROGRAMMER"

FIRMWARE_MAP = {
    "HCS08_BDM":        {"name": "PICO2_MCU_PROGRAMMER_V5",  "uf2": os.path.join(PROJECT_DIR, "firmware", "PICO2_MCU_PROGRAMMER.uf2")},
    "PIC_ICSP":         {"name": "PICO2_MCU_PROGRAMMER_V5",  "uf2": os.path.join(PROJECT_DIR, "firmware", "PICO2_MCU_PROGRAMMER.uf2")},
    "XMEGA_PDI":        {"name": "PICO2_MCU_PROGRAMMER_V5",  "uf2": os.path.join(PROJECT_DIR, "firmware", "PICO2_MCU_PROGRAMMER.uf2")},
    "CYPRESS_HCIUART":  {"name": "PICO2_MCU_PROGRAMMER_V5",  "uf2": os.path.join(PROJECT_DIR, "firmware", "PICO2_MCU_PROGRAMMER.uf2")},
    "OPENOCD_SWD":      {"name": "OPENOCD_SWD",              "uf2": os.path.join(PROJECT_DIR, "firmware", "OPENOCD_SWD.uf2")},
    "PYMCUPROG_UPDI":   {"name": "PYMCUPROG_UPDI",           "uf2": os.path.join(PROJECT_DIR, "firmware", "PYMCUPROG_UPDI.uf2")},
    "STK500V1_SPI":     {"name": "STK500V1_SPI",             "uf2": os.path.join(PROJECT_DIR, "firmware", "STK500V1_SPI.uf2")},
}


######################################################################################################
##                                       Target MCUs                                                ##
######################################################################################################
NXP_BDM_TARGETS = {
    "MC9S08PA4"
}

PIC_ICSP_TARGETS = {
    "PIC12F1571",
    "PIC16F18345",
    "PIC18F25K80",
    "PIC18F66K80",
    "PIC18F25K83",
    "PIC18F26Q84"
}

XMEGA_PDI_TARGETS = {
    "ATXMEGA32E5",
    "ATXMEGA64A4U",
    "ATXMEGA128A3U",
    "ATXMEGA128A4U",
    "ATXMEGA192A3U"
}

CYPRESS_HCIUART_TARGETS = {
    "CYBT213043"
}

CORTEX_M_SWD_TARGETS = {
    "STM32G030F":   {"cfg": "target/stm32g0x.cfg",      "erase_driver": "stm32g0x", "flash_base": "0x08000000"},
    "STM32L431RC":  {"cfg": "target/stm32l4x.cfg",      "erase_driver": "stm32l4x", "flash_base": "0x08000000"},
    "STM32L451CEU": {"cfg": "target/stm32l4x.cfg",      "erase_driver": "stm32l4x", "flash_base": "0x08000000"},
    "NRF52":        {"cfg": "target/nordic/nrf52.cfg",  "erase_driver": "nrf5",     "flash_base": "0x00000000", "recover_cmd": "nrf52_recover"},
}

AVR_UPDI_TARGETS = {
    "ATMEGA1608":   {"baud": 230400},   # 5/5 write+verify passes at 230400; 250000+ fails to initialise
    "ATMEGA3209":   {"baud": 230400}    # same megaAVR 0-series UPDI as the ATMEGA1608 (not yet bench-tested)
}

AVR_SPI_TARGETS = {
    "AT90CAN32": {
        "part": "c32",        "page_size": 256, "flash_size": 0x08000, "eeprom_size": 0x400, "eeprom_page": None, "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": None,     "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "AT90CAN32"
    },
    "AT90CAN64": {
        "part": "c64",        "page_size": 256, "flash_size": 0x10000, "eeprom_size": 0x800, "eeprom_page": None, "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": None,     "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "AT90CAN64"
    },
    "ATMEGA8": {
        "part": "m8",         "page_size": 64,  "flash_size": 0x02000, "eeprom_size": 0x200, "eeprom_page": None, "n_fuses": 2,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "ATmega8"
    },
    "ATMEGA48": {
        "part": "m48",        "page_size": 64,  "flash_size": 0x01000, "eeprom_size": 0x100, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": False,     "bootrst_fuse": None,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega48"
    },
    "ATMEGA88P": {
        "part": "m88p",       "page_size": 64,  "flash_size": 0x02000, "eeprom_size": 0x200, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 2,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega88P"
    },
    "ATMEGA88PA": {
        "part": "m88pa",      "page_size": 64,  "flash_size": 0x02000, "eeprom_size": 0x200, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 2,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega88PA"
    },
    "ATMEGA165": {
        "part": "m165",       "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": None, "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": (2, 0x01), "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "ATmega165A"
    },
    "ATMEGA165P": {
        "part": "m165p",      "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": None, "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": (2, 0x01), "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "ATmega165P"
    },
    "ATMEGA168": {
        "part": "m168",       "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 2,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega168"
    },
    "ATMEGA168P": {
        "part": "m168p",      "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 2,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega168P"
    },
    "ATMEGA168PB": {
        "part": "m168pb",     "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 2,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega168PB"
    },
    "ATMEGA169": {
        "part": "m169",       "page_size": 128, "flash_size": 0x04000, "eeprom_size": 0x200, "eeprom_page": None, "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": (2, 0x01), "dwen": None,     "slow_osc": None,
        "lock_write": "STD",  "atdf": "ATmega169A"
    },
    "ATMEGA328PB": {
        "part": "m328pb",     "page_size": 128, "flash_size": 0x08000, "eeprom_size": 0x400, "eeprom_page": 4,    "n_fuses": 3,
        "bootrst": True,      "bootrst_fuse": 1,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x03),
        "lock_write": "STD",  "atdf": "ATmega328PB"
    },
    "ATTINY13": {
        "part": "t13",        "page_size": 32,  "flash_size": 0x00400, "eeprom_size": 0x040, "eeprom_page": None, "n_fuses": 2,
        "bootrst": False,     "bootrst_fuse": None,
        "spien": (0, 0x80),   "rstdisbl": (1, 0x01), "dwen": (1, 0x08), "slow_osc": (0x03, 0x03),
        "lock_write": "STD",  "atdf": "ATtiny13"
    },
    "ATTINY26": {
        "part": "t26",        "page_size": 32,  "flash_size": 0x00800, "eeprom_size": 0x080, "eeprom_page": None, "n_fuses": 2,
        "bootrst": False,     "bootrst_fuse": None,
        "spien": (1, 0x08),   "rstdisbl": (1, 0x10), "dwen": None,     "slow_osc": None,
        "lock_write": "T26",  "atdf": "ATtiny26"
    },
    "ATTINY48": {
        "part": "t48",        "page_size": 64,  "flash_size": 0x01000, "eeprom_size": 0x040, "eeprom_page": None, "n_fuses": 3,
        "bootrst": False,     "bootrst_fuse": None,
        "spien": (1, 0x20),   "rstdisbl": (1, 0x80), "dwen": (1, 0x40), "slow_osc": (0x0F, 0x0F),
        "lock_write": "STD",  "atdf": "ATtiny48"
    },
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
HEX_PAYLOAD_BYTES        = 32
HEX_END_OF_STREAM_ADDR   = 0xFFFFFFFF   # packet address that ends the stream (see LOAD_HEX_DATA)
PORT_SETTLE_S            = 0.05         # the Pico's CDC port needs no reset delay, it is already listening

# CYBT / AIROC: the Pico's HCI UART follows the COM-port baud rate
AIROC_ROM_BAUDRATE       = 750000   # boot ROM autobaud limit (921600 fails)
AIROC_BOOT_BAUDRATE      = 115200   # minidriver always starts here
AIROC_DEFAULT_BAUDRATE   = 3000000
AIROC_EXIT_BAUD          = 2400     # tells the Pico to leave bridge mode
AIROC_HCI_RESPONSE_S     = 1.0
AIROC_ERASE_TIMEOUT_S    = 5.0
AIROC_WRITE_CHUNK        = 240

AVR_HEX_FLASH_BASE       = 0x000000
AVR_HEX_EEPROM_BASE      = 0x810000
AVR_HEX_FUSE_BASE        = 0x820000
AVR_HEX_LOCK_BASE        = 0x830000
AVR_HEX_SIGNATURE_BASE   = 0x840000
AVR_HEX_USERSIG_BASE     = 0x850000
AVR_HEX_REGION_SPAN      = 0x010000

STK_OK                   = 0x10
STK_INSYNC               = 0x14
STK_NOSYNC               = 0x15
STK_CRC_EOP              = 0x20
STK_GET_SYNC             = 0x30
STK_SET_DEVICE           = 0x42
STK_SET_DEVICE_EXT       = 0x45
STK_ENTER_PROGMODE       = 0x50
STK_LEAVE_PROGMODE       = 0x51
STK_CHIP_ERASE           = 0x52
STK_LOAD_ADDRESS         = 0x55
STK_UNIVERSAL            = 0x56
STK_PROG_PAGE            = 0x64
STK_READ_PAGE            = 0x74
STK_READ_SIGN            = 0x75
STK_MEMTYPE_FLASH        = 0x46
STK_MEMTYPE_EEPROM       = 0x45

ISP_SERIAL_BAUD          = 19200
ISP_READ_TIMEOUT_S       = 5.0
ISP_SYNC_TIMEOUT_S       = 0.2      # per GET_SYNC attempt; the bridge answers in ~2 ms
ISP_SYNC_ATTEMPTS        = 10
ISP_FUSE_WRITE_DELAY_S   = 0.02     # fixed wait only for parts without RDY/BSY polling
ISP_BUSY_POLL_LIMIT_S    = 0.1      # cap on RDY/BSY polling after one ISP write
ISP_EEPROM_CHUNK_BYTES   = 64
ISP_MAX_PAGE_BYTES       = 256

UPDI_PING_TIMEOUT_S      = 15.0     # a live target identifies in ~1-2 s
UPDI_WRITE_TIMEOUT_S     = 120.0
UPDI_PORT_RELEASE_S      = 90.0     # killed worker keeps the handle while its USB read
                                    # unwinds - measured at ~52 s on this bench
UPDI_PORT_PROBE_S        = 5.0

AVR_FUSE_OPS = {
    0: {"name": "LOW",      "read": (0x50, 0x00), "write": 0xA0},
    1: {"name": "HIGH",     "read": (0x58, 0x08), "write": 0xA8},
    2: {"name": "EXTENDED", "read": (0x50, 0x08), "write": 0xA4},
}


######################################################################################################
##                                         Functions                                                ##
######################################################################################################
# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Scans COM ports for RP Pico 2 Vendor ID  │
# │                                                        │
# │ INPUT       : None, unless current FW is uart_bridge   │
# │ RETURNS     : COM_PORT_# (int)                         │
# └────────────────────────────────────────────────────────┘
def AUTO_DETECT_PICO2(PREFFER_ITF=None):
    PORTS = serial.tools.list_ports.comports()

    PICO_PORTS = [p for p in PORTS if p.vid == 0x2E8A]

    if not PICO_PORTS:
        return None

    if PREFFER_ITF is not None:
        for p in PICO_PORTS:
            LOC = p.location or ""

            try:
                itf = int(LOC.rsplit('.', 1)[-1])
            except ValueError:
                itf = None

            if itf == PREFFER_ITF:
                print(f"[+] Detected Raspberry Pi Pico on {p.device}")
                return p.device

    print(f"[+] Detected Raspberry Pi Pico on {PICO_PORTS[0].device}")
    return PICO_PORTS[0].device

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Verifies correct PICO2 internal FW       │
# │                                                        │
# │ INPUT       : Required FW Name, Path to .uf2 file      │
# │ RETURNS     : T or F (PASS or FAIL)                    │
# └────────────────────────────────────────────────────────┘
def ENSURE_FW(REQUIRED_NAME, UF2_PATH):
    POLL_ATTEMPTS = 20
    POLL_INTERVAL = 0.25

    print(f"[*] Checking Pico firmware state...")

    CURRENT_NAME = None

    try:
        with open(STATE_FILE, 'r') as f:
            CURRENT_NAME = json.load(f).get("firmware")
    except (FileNotFoundError, json.JSONDecodeError):
        pass

    if CURRENT_NAME == REQUIRED_NAME:
        print(f"[+] Firmware verified: '{REQUIRED_NAME}' already active.")
        return True
    elif CURRENT_NAME:
        print(f"[*] Firmware mismatch. Active: '{CURRENT_NAME}' | Required: '{REQUIRED_NAME}'")
    else:
        print(f"[*] No firmware state on record. Proceeding with reflash.")

    print(f"[*] Triggering BOOTSEL...")
    if CURRENT_NAME in ("OPENOCD_SWD", "PYMCUPROG_UPDI"):
        pico_port = AUTO_DETECT_PICO2()

        if pico_port:
            try:
                with serial.Serial(pico_port, 1200):
                    pass
            except serial.SerialException:
                pass

        print(f"[*] Waiting for BOOTSEL...")

        bootsel_ready = False

        for _ in range(POLL_ATTEMPTS):
            time.sleep(POLL_INTERVAL)
            try:
                probe = subprocess.run(
                    [PICOTOOL, 'info'],
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=5
                )
                if probe.returncode == 0:
                    bootsel_ready = True
                    break
            except subprocess.TimeoutExpired:
                continue

        if not bootsel_ready:
            print("[-] Error: Pico did not enter BOOTSEL mode.")
            print("\n>>> RESULT: FAIL <<<")
            return False

        abs_uf2 = os.path.abspath(UF2_PATH)
        if not os.path.exists(abs_uf2):
            print(f"[-] UF2 not found: {abs_uf2}")
            print("\n>>> RESULT: FAIL <<<")
            return False

        print(f"[*] Loading: {abs_uf2}")
        try:
            load = subprocess.run(
                [PICOTOOL, 'load', abs_uf2, '--force'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=15
            )
        except subprocess.TimeoutExpired:
            print("[-] picotool load timed out.")
            print("\n>>> RESULT: FAIL <<<")
            return False

        if load.returncode != 0:
            print(f"[-] picotool load failed (code {load.returncode}): {load.stderr.strip()}")
            print("\n>>> RESULT: FAIL <<<")
            return False

        try:
            subprocess.run([PICOTOOL, 'reboot'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        except subprocess.TimeoutExpired:
            pass

        print(f"[*] Waiting for '{REQUIRED_NAME}' to come online...")
        for _ in range(POLL_ATTEMPTS):
            time.sleep(POLL_INTERVAL)
            if AUTO_DETECT_PICO2() is not None:
                time.sleep(0.5)
                with open(STATE_FILE, 'w') as f:
                    json.dump({"firmware": REQUIRED_NAME}, f)
                print(f"[+] Firmware swap complete. '{REQUIRED_NAME}' is now active.")
                return True

        print("[-] Pico did not re-enumerate after flash.")
        print("\n>>> RESULT: FAIL <<<")
        return False
    else:
        try:
            subprocess.run(
                [PICOTOOL, 'reboot', '-f', '-u'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10
            )
        except subprocess.TimeoutExpired:
            pass

        print(f"[*] Waiting for BOOTSEL...")
        bootsel_ready = False
        for _ in range(POLL_ATTEMPTS):
            time.sleep(POLL_INTERVAL)
            try:
                probe = subprocess.run(
                    [PICOTOOL, 'info'],
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=5
                )
                if probe.returncode == 0:
                    bootsel_ready = True
                    break
            except subprocess.TimeoutExpired:
                continue

        # The state file can be stale (e.g. the Pico was reflashed by hand), and the
        # third-party firmwares have no picotool reset interface - fall back to the
        # 1200-baud touch they do honour before giving up
        if not bootsel_ready:
            print(f"[*] picotool reboot had no effect - trying the 1200-baud BOOTSEL touch...")

            for PORT in [p.device for p in serial.tools.list_ports.comports() if p.vid == 0x2E8A]:
                try:
                    with serial.Serial(PORT, 1200):
                        pass
                except serial.SerialException:
                    pass

            for _ in range(POLL_ATTEMPTS):
                time.sleep(POLL_INTERVAL)
                try:
                    probe = subprocess.run(
                        [PICOTOOL, 'info'],
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=5
                    )
                    if probe.returncode == 0:
                        bootsel_ready = True
                        break
                except subprocess.TimeoutExpired:
                    continue

        if not bootsel_ready:
            print("[-] Error: Pico did not enter BOOTSEL mode.")
            print("\n>>> RESULT: FAIL <<<")
            return False

        abs_uf2 = os.path.abspath(UF2_PATH)
        if not os.path.exists(abs_uf2):
            print(f"[-] UF2 not found: {abs_uf2}")
            print("\n>>> RESULT: FAIL <<<")
            return False

        print(f"[*] Loading: {abs_uf2}")
        try:
            load = subprocess.run(
                [PICOTOOL, 'load', abs_uf2, '--force'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=15
            )
        except subprocess.TimeoutExpired:
            print("[-] picotool load timed out.")
            print("\n>>> RESULT: FAIL <<<")
            return False

        if load.returncode != 0:
            print(f"[-] picotool load failed (code {load.returncode}): {load.stderr.strip()}")
            print("\n>>> RESULT: FAIL <<<")
            return False

        try:
            subprocess.run([PICOTOOL, 'reboot'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
        except subprocess.TimeoutExpired:
            pass

        print(f"[*] Waiting for '{REQUIRED_NAME}' to come online...")
        for _ in range(POLL_ATTEMPTS):
            time.sleep(POLL_INTERVAL)
            if AUTO_DETECT_PICO2() is not None:
                time.sleep(0.5)
                with open(STATE_FILE, 'w') as f:
                    json.dump({"firmware": REQUIRED_NAME}, f)
                print(f"[+] Firmware swap complete. '{REQUIRED_NAME}' is now active.")
                return True

        print("[-] Pico did not re-enumerate after flash.")
        print("\n>>> RESULT: FAIL <<<")
        return False

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

                if RECORD_TYPE == 2:
                    SEGMENT_BASE    = int(DATA_HEX, 16)
                    UPPER_ADDR_BITS = SEGMENT_BASE << 4
                elif RECORD_TYPE == 4:
                    LINEAR_BASE     = int(DATA_HEX, 16)
                    UPPER_ADDR_BITS = LINEAR_BASE << 16
                elif RECORD_TYPE == 0:
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
                elif RECORD_TYPE == 1:
                    print(f"[+] Reached Intel HEX End-Of-File marker at line {line_num}.")
                    break
                else:
                    continue

        return PARSED_RECORDS

    except FileNotFoundError:
        print(f"[-] Error: The file '{FILE_PATH}' was not found.")
        return None

# ┌─────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Intel HEX -> contiguous areas split into  │
# │               CHUNK-byte (address, data) pieces, as the │
# │               AIROC Write_RAM/Read_RAM commands want it │
# │                                                         │
# │ INPUT       : FILE_PATH (str) - Path to .hex file       │
# │               CHUNK     (int) - Max bytes per piece     │
# │                                                         │
# │ RETURNS     : list[(address, bytes)]                    │
# │               None - on a malformed record / checksum   │
# └─────────────────────────────────────────────────────────┘
def PARSE_AIROC_HEX(FILE_PATH, CHUNK):
    AREAS = []
    BASE  = 0

    try:
        with open(FILE_PATH, 'r') as FILE:
            for LINE_NUM, LINE in enumerate(FILE, 1):
                if not LINE.startswith(':'):
                    continue
                try:
                    RECORD = bytes.fromhex(LINE.strip()[1:])
                except ValueError:
                    RECORD = b""
                if len(RECORD) < 5 or len(RECORD) != RECORD[0] + 5 or sum(RECORD) & 0xFF:
                    print(f"[-] Error: {FILE_PATH} line {LINE_NUM}: bad record or checksum.")
                    return None
                DATA = RECORD[4:-1]

                if RECORD[3] == 0x01:
                    break
                elif RECORD[3] == 0x02:
                    BASE = int.from_bytes(DATA, 'big') << 4
                elif RECORD[3] == 0x04:
                    BASE = int.from_bytes(DATA, 'big') << 16
                elif RECORD[3] == 0x00:
                    ADDRESS = BASE + int.from_bytes(RECORD[1:3], 'big')
                    if AREAS and AREAS[-1][0] + len(AREAS[-1][1]) == ADDRESS:
                        AREAS[-1][1].extend(DATA)
                    else:
                        AREAS.append([ADDRESS, bytearray(DATA)])
    except FileNotFoundError:
        print(f"[-] Error: The file '{FILE_PATH}' was not found.")
        return None

    return [(START + I, bytes(DATA[I:I + CHUNK]))
            for START, DATA in sorted(AREAS) for I in range(0, len(DATA), CHUNK)]

# ┌─────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Sends one HCI command over the Pico's     │
# │               USB <-> HCI UART bridge and waits for its │
# │               Command Complete event                    │
# │                                                         │
# │ INPUT       : CONN    (Serial) - Open Pico link         │
# │               OPCODE  (int)    - HCI opcode             │
# │               PARAMS  (bytes)  - Command parameters     │
# │               TIMEOUT (float)  - Seconds to wait        │
# │                                                         │
# │ RETURNS     : bytes - return parameters                 │
# │               None  - on timeout / error status         │
# └─────────────────────────────────────────────────────────┘
def AIROC_HCI_COMMAND(CONN, OPCODE, PARAMS=b"", TIMEOUT=AIROC_HCI_RESPONSE_S):
    OP = OPCODE.to_bytes(2, 'little')
    CONN.write(b"\x01" + OP + bytes([len(PARAMS)]) + PARAMS)

    RESPONSE = b""
    LIMIT    = time.perf_counter() + TIMEOUT
    while time.perf_counter() < LIMIT:
        RESPONSE += CONN.read(max(CONN.in_waiting, 1))
        if len(RESPONSE) >= 3 and len(RESPONSE) >= 3 + RESPONSE[2]:
            # 04 0E <len> <num_pkts> <opcode> <status> <params>
            if RESPONSE[:2] == b"\x04\x0E" and RESPONSE[4:7] == OP + b"\x00":
                return RESPONSE[7:3 + RESPONSE[2]]
            break

    print(f"[-] Error: HCI opcode 0x{OPCODE:04X} failed "
          f"(got {RESPONSE.hex(' ').upper() or 'nothing'}).")
    return None

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : WICED HCI download: minidriver load via  │
# │               the boot ROM, switch to BAUDRATE, erase, │
# │               write, read back every chunk to verify   │
# │               and launch                               │
# │                                                        │
# │ INPUT       : CONN       (Serial) - Open Pico link     │
# │               MINIDRIVER (list)   - PARSE_AIROC_HEX()  │
# │               FIRMWARE   (list)   - PARSE_AIROC_HEX()  │
# │               BAUDRATE   (int)    - Download baud      │
# │                                                        │
# │ RETURNS     : bool - True if every chunk verified      │
# └────────────────────────────────────────────────────────┘
def AIROC_DOWNLOAD(CONN, MINIDRIVER, FIRMWARE, BAUDRATE):
    CMD = lambda *A, **K: AIROC_HCI_COMMAND(CONN, *A, **K) is not None
    LE  = lambda N: N.to_bytes(4, 'little')

    def SET_BAUD(RATE, DELAY):
        time.sleep(DELAY)
        CONN.baudrate = RATE
        time.sleep(0.01)
        CONN.reset_input_buffer()

    CONN.timeout = 0.01
    SET_BAUD(AIROC_ROM_BAUDRATE, 0)

    print("[+] Loading minidriver...")
    if not (CMD(0x0C03) and CMD(0xFC2E)):                                   # Reset, Download_Minidriver
        return False
    time.sleep(0.05)
    if not all(CMD(0xFC4C, LE(A) + D) for A, D in MINIDRIVER):              # Write_RAM
        return False
    if not CMD(0xFC4E, LE(MINIDRIVER[0][0])):                               # Launch_RAM
        return False

    SET_BAUD(AIROC_BOOT_BAUDRATE, 0.2)
    if not CMD(0xFC18, bytes(2) + LE(BAUDRATE)):                            # Update_Baudrate
        return False
    SET_BAUD(BAUDRATE, 0.01)

    print(f"[+] Erasing, writing and verifying {sum(len(D) for _, D in FIRMWARE)} bytes...")
    if not CMD(0xFFCE, bytes.fromhex("EFEEBEFC"), TIMEOUT=AIROC_ERASE_TIMEOUT_S):  # Chip erase
        return False
    # All writes then all reads: a read right after each write stalls the minidriver.
    if not all(CMD(0xFC4C, LE(A) + D) for A, D in FIRMWARE):                # Write_RAM
        return False
    for A, D in FIRMWARE:
        if AIROC_HCI_COMMAND(CONN, 0xFC4D, LE(A) + bytes([len(D)])) != D:   # Read_RAM
            print(f"[-] Error: verify failed at 0x{A:08X}.")
            return False

    return CMD(0xFC4E, b"\xFF\xFF\xFF\xFF")                               # Launch firmware

# ┌──────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Checks the COM port actually accepts a     │
# │               connection, so a port held by a stalled    │
# │               pymcuprog - or a wedged Pico - is reported │
# │               as that, not as a target that failed to    │
# │               identify. The open runs in a child process │
# │               because opening a wedged CDC endpoint can  │
# │               block for minutes instead of erroring      │
# │                                                          │
# │ INPUT       : PORT (str) - COM port name                 │
# │ RETURNS     : bool - True when it can be opened          │
# └──────────────────────────────────────────────────────────┘
def PORT_IS_FREE(PORT):
    CHILD = [sys.executable, "-c",
             "import sys, serial; serial.Serial(sys.argv[1], timeout=0.1).close()", PORT]

    PROC = subprocess.Popen(CHILD, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        return PROC.wait(timeout=UPDI_PORT_PROBE_S) == 0
    except subprocess.TimeoutExpired:
        PROC.kill()
        return False

# ┌─────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Picks how to start pymcuprog.             │
# │               pymcuprog.exe is only a stub that re-runs │
# │               itself under python, so killing the stub  │
# │               strands that worker with the COM port     │
# │               still open and every later run then dies  │
# │               on PermissionError(13). Running it under  │
# │               this interpreter leaves one process that  │
# │               can actually be killed; the stub stays as │
# │               a fallback                                │
# │                                                         │
# │ INPUT       : None                                      │
# │ RETURNS     : list - argv prefix                        │
# └─────────────────────────────────────────────────────────┘
def PYMCUPROG_LAUNCHER():
    try:
        PROBE = subprocess.run([sys.executable, PYMCUPROG, "-V"],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               timeout=30)
        if PROBE.returncode == 0:
            return [sys.executable, PYMCUPROG]
    except (OSError, subprocess.TimeoutExpired):
        pass

    print("[!] Warning: falling back to the pymcuprog launcher stub; a stalled run may "
          "leave the port locked for a few seconds.")
    return [PYMCUPROG]

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Kills a stalled pymcuprog and waits for  │
# │               it to release the COM port               │
# │                                                        │
# │ INPUT       : PROC (Popen) - The pymcuprog process     │
# │               CMD  (list)  - Its argv (for the port)   │
# │                                                        │
# │ RETURNS     : None                                     │
# └────────────────────────────────────────────────────────┘
def KILL_PYMCUPROG(PROC, CMD):
    PORT = CMD[CMD.index("-u") + 1] if "-u" in CMD else "the port"

    # /T costs nothing for the single-process form and still reaches the worker
    # if this ever ran through the stub
    subprocess.run(["taskkill", "/PID", str(PROC.pid), "/T", "/F"],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # TerminateProcess does not free the handle: a pymcuprog stuck mid-transfer
    # only dies once its pending USB read unwinds, and until then it owns the COM
    # port. Waiting it out here is what keeps the next run from failing on
    # PermissionError(13) and being misread as a wrong target
    print(f"[*] Releasing {PORT} from the stalled pymcuprog (can take ~1 min)...")

    try:
        PROC.wait(timeout=UPDI_PORT_RELEASE_S)
    except subprocess.TimeoutExpired:
        print(f"[!] Warning: pymcuprog still holds {PORT}. Wait for it to exit before "
              f"the next run.")

# ┌─────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Runs one pymcuprog action in a single     │
# │               UPDI session, echoing its output live.    │
# │               pymcuprog pings the part itself before    │
# │               any action, so a READY_MARKER line that   │
# │               does not appear within READY_TIMEOUT_S    │
# │               means the handshake has stalled (wrong,   │
# │               unpowered or absent part) and the run is  │
# │               killed early. This replaces a separate    │
# │               ping run and its second handshake         │
# │                                                         │
# │ INPUT       : CMD             (list)  - Full argv       │
# │               READY_MARKER    (str)   - Handshake line  │
# │               READY_TIMEOUT_S (float) - Handshake limit │
# │               TIMEOUT_S       (float) - Wall-clock cap  │
# │                                                         │
# │ RETURNS     : True          - exited 0                  │
# │               False         - exited non-zero           │
# │               "NO_RESPONSE" - handshake never finished  │
# │               None          - hit TIMEOUT_S, killed     │
# └─────────────────────────────────────────────────────────┘
def RUN_PYMCUPROG_WATCHED(CMD, READY_MARKER, READY_TIMEOUT_S, TIMEOUT_S):
    LINES = queue.Queue()
    ENV   = dict(os.environ, PYTHONUNBUFFERED="1")
    PROC  = subprocess.Popen(CMD, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             text=True, errors="replace", env=ENV)

    def PUMP():
        for LINE in PROC.stdout:
            LINES.put(LINE)
        LINES.put(None)

    threading.Thread(target=PUMP, daemon=True).start()

    START = time.perf_counter()
    READY = False

    while True:
        LIMIT = READY_TIMEOUT_S if not READY else TIMEOUT_S

        try:
            LINE = LINES.get(timeout=max(0.0, START + LIMIT - time.perf_counter()))
        except queue.Empty:
            KILL_PYMCUPROG(PROC, CMD)
            return None if READY else "NO_RESPONSE"

        if LINE is None:
            return PROC.wait() == 0

        print(LINE, end="")

        if READY_MARKER in LINE:
            READY = True

# ┌────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Splits one combined AVR unified HEX into │
# │               FLASH / EEPROM / FUSES / LOCK buckets    │
# │                                                        │
# │ INPUT       : FILE_PATH (str) - Path to combined .hex  │
# │                                                        │
# │ RETURNS     : dict - {'FLASH' : {addr: byte},          │
# │                       'EEPROM': {addr: byte},          │
# │                       'FUSES' : {idx : byte},          │
# │                       'LOCK'  : {0   : byte},          │
# │                       'IGNORED': [str, ...]}           │
# │               None - on parse / file error             │
# └────────────────────────────────────────────────────────┘
def BUCKET_AVR_UNIFIED_HEX(FILE_PATH):
    BUCKETS = {'FLASH': {}, 'EEPROM': {}, 'FUSES': {}, 'LOCK': {}, 'IGNORED': []}

    REGIONS = (
        (AVR_HEX_EEPROM_BASE,    'EEPROM'),
        (AVR_HEX_FUSE_BASE,      'FUSES'),
        (AVR_HEX_LOCK_BASE,      'LOCK'),
        (AVR_HEX_SIGNATURE_BASE, 'SIGNATURE'),
        (AVR_HEX_USERSIG_BASE,   'USER_SIGNATURE'),
    )

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
                    EXPECTED_LEN   = 2 + 4 + 2 + (BYTE_COUNT * 2) + 2
                    ALL_LINE_BYTES = bytes.fromhex(line.lstrip(':')[:EXPECTED_LEN])
                    if (sum(ALL_LINE_BYTES) & 0xFF) != 0:
                        print(f"[-] Line {line_num}: Checksum mismatch. Skipping.")
                        continue
                except ValueError:
                    print(f"[-] Line {line_num}: Invalid hex characters. Skipping.")
                    continue

                if RECORD_TYPE == 2:
                    UPPER_ADDR_BITS = int(DATA_HEX, 16) << 4
                elif RECORD_TYPE == 4:
                    UPPER_ADDR_BITS = int(DATA_HEX, 16) << 16
                elif RECORD_TYPE == 0:
                    ABSOLUTE_ADDR = UPPER_ADDR_BITS + LINE_OFFSET
                    RAW_BYTES     = bytes.fromhex(DATA_HEX)

                    for i, b in enumerate(RAW_BYTES):
                        ADDR   = ABSOLUTE_ADDR + i
                        REGION = 'FLASH'
                        OFFSET = ADDR - AVR_HEX_FLASH_BASE

                        for BASE, NAME in REGIONS:
                            if BASE <= ADDR < BASE + AVR_HEX_REGION_SPAN:
                                REGION = NAME
                                OFFSET = ADDR - BASE
                                break
                        else:
                            if ADDR >= 0x800000:
                                REGION = 'UNKNOWN'
                                OFFSET = ADDR

                        if REGION in ('FLASH', 'EEPROM', 'FUSES', 'LOCK'):
                            BUCKETS[REGION][OFFSET] = b
                        else:
                            NOTE = f"0x{ADDR:06X} ({REGION})"
                            if NOTE not in BUCKETS['IGNORED']:
                                BUCKETS['IGNORED'].append(NOTE)
                elif RECORD_TYPE == 1:
                    break
                else:
                    continue

        return BUCKETS

    except FileNotFoundError:
        print(f"[-] Error: The file '{FILE_PATH}' was not found.")
        return None

# ┌─────────────────────────────────────────────────────────┐
# │ DESCRIPTION : Programs a classic AVR over SPI ISP using │
# │               a native STK500v1_SPI driver against the  │
# │               STK500V1_SPI.uf2 firmware.                │
# │                                                         │
# │ INPUT       : PORT    (str)  - Pico2 COM port           │
# │               META    (dict) - AVR_ISP_TARGETS entry    │
# │               BUCKETS (dict) - BUCKET_AVR_UNIFIED_HEX() │
# │                                                         │
# │ RETURNS     : None. Raises IOError / ValueError on any  │
# │               protocol, verify or safety failure.       │
# └─────────────────────────────────────────────────────────┘
def PROGRAM_AVR_ISP(PORT, META, BUCKETS):
    PAGE_SIZE   = META["page_size"]
    N_FUSES     = META["n_fuses"]
    FLASH       = dict(BUCKETS['FLASH'])
    EEPROM      = dict(BUCKETS['EEPROM'])
    HEX_FUSES   = dict(BUCKETS['FUSES'])
    HEX_LOCK    = dict(BUCKETS['LOCK'])

    if PAGE_SIZE > ISP_MAX_PAGE_BYTES:
        raise ValueError(f"page_size {PAGE_SIZE} exceeds bridge buffer ({ISP_MAX_PAGE_BYTES} B)")
    if not FLASH:
        raise ValueError("Combined hex contains no FLASH data (nothing below 0x800000)")

    MAX_FLASH = max(FLASH.keys())
    if MAX_FLASH >= META["flash_size"]:
        raise ValueError(f"FLASH record at 0x{MAX_FLASH:06X} is outside the device's "
                         f"{META['flash_size']} B flash")
    if EEPROM:
        MAX_EEPROM = max(EEPROM.keys())
        if MAX_EEPROM >= META["eeprom_size"]:
            raise ValueError(f"EEPROM record at 0x{MAX_EEPROM:04X} is outside the device's "
                             f"{META['eeprom_size']} B EEPROM")
    for IDX in sorted(HEX_FUSES):
        if IDX >= N_FUSES:
            raise ValueError(f"Hex supplies fuse index {IDX} (0x{AVR_HEX_FUSE_BASE + IDX:06X}) "
                             f"but this device only has {N_FUSES} fuse byte(s)")
    for IDX in sorted(HEX_LOCK):
        if IDX != 0:
            raise ValueError(f"Hex supplies lock byte index {IDX}; classic AVR has exactly one")

    CMD_NAMES = {
        STK_GET_SYNC: "GET_SYNC",             STK_SET_DEVICE: "SET_DEVICE",
        STK_SET_DEVICE_EXT: "SET_DEVICE_EXT", STK_ENTER_PROGMODE: "ENTER_PROGMODE",
        STK_LEAVE_PROGMODE: "LEAVE_PROGMODE", STK_CHIP_ERASE: "CHIP_ERASE",
        STK_LOAD_ADDRESS: "LOAD_ADDRESS",     STK_UNIVERSAL: "UNIVERSAL",
        STK_PROG_PAGE: "PROG_PAGE",           STK_READ_PAGE: "READ_PAGE",
        STK_READ_SIGN: "READ_SIGN",
    }

    def SEND_CMD(LINK, CMD, RX_EXTRA=0):
        LINK.write(bytes(list(CMD) + [STK_CRC_EOP]))
        N    = 2 + RX_EXTRA
        RESP = LINK.read(N)
        NAME = CMD_NAMES.get(CMD[0], f"0x{CMD[0]:02X}")
        if len(RESP) >= 1 and RESP[0] == STK_NOSYNC:
            raise IOError(f"Programmer NOSYNC on {NAME} - AVR not responding to SPI ISP.\n"
                          f"    Check: MISO level shifting, AVCC/VCC powered, "
                          f"RESET reaching ~0 V during programming, SPIEN still programmed.")
        if len(RESP) < N:
            raise IOError(f"Timeout on {NAME} (got {RESP.hex() if RESP else 'nothing'})")
        if RESP[0] != STK_INSYNC:
            raise IOError(f"Not in sync on {NAME}: 0x{RESP[0]:02X}")
        if RESP[-1] != STK_OK:
            raise IOError(f"Not OK on {NAME}: 0x{RESP[-1]:02X}")
        return RESP[1:-1]

    def UNIVERSAL(LINK, A, B, C, D):
        return SEND_CMD(LINK, [STK_UNIVERSAL, A, B, C, D], RX_EXTRA=1)[0]

    def READ_FUSE(LINK, IDX):
        OP_A, OP_B = AVR_FUSE_OPS[IDX]["read"]
        return UNIVERSAL(LINK, OP_A, OP_B, 0x00, 0x00)

    def READ_LOCK(LINK):
        return UNIVERSAL(LINK, 0x58, 0x00, 0x00, 0x00)

    # Waits for an ISP write to finish by polling RDY/BSY (0xF0) instead of sleeping
    # for the worst-case write time. Parts without polling keep the fixed delay
    def WAIT_READY(LINK):
        if not META.get("eeprom_page"):
            time.sleep(ISP_FUSE_WRITE_DELAY_S)
            return
        LIMIT = time.perf_counter() + ISP_BUSY_POLL_LIMIT_S
        while UNIVERSAL(LINK, 0xF0, 0x00, 0x00, 0x00) & 0x01:
            if time.perf_counter() > LIMIT:
                raise IOError("Target stayed busy after an ISP write (RDY/BSY never cleared)")

    with serial.Serial(PORT, ISP_SERIAL_BAUD, timeout=ISP_READ_TIMEOUT_S) as LINK:
        # The bridge is a USB CDC device that is already listening, so there is no
        # reset to wait out - retry GET_SYNC with a short timeout instead
        SYNCED       = False
        LINK.timeout = ISP_SYNC_TIMEOUT_S
        for _ in range(ISP_SYNC_ATTEMPTS):
            LINK.reset_input_buffer()
            LINK.write(bytes([STK_GET_SYNC, STK_CRC_EOP]))
            if LINK.read(2) == bytes([STK_INSYNC, STK_OK]):
                SYNCED = True
                break
        LINK.timeout = ISP_READ_TIMEOUT_S
        if not SYNCED:
            raise IOError("Could not sync with the STK500V1_SPI - check wiring and power")
        print("[+] STK500v1_SPI sync established.")

        # Page / EEPROM / flash sizes come from META: the bridge uses the page size to
        # decide where each flash page is committed, so a fixed value would corrupt
        # parts whose page size differs from it
        FLASH_SIZE  = META["flash_size"]
        EEPROM_SIZE = META["eeprom_size"]
        SEND_CMD(LINK, [STK_SET_DEVICE, 0x86, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x06,
                        0xFF, 0xFF, 0xFF, 0xFF,
                        (PAGE_SIZE >> 8) & 0xFF, PAGE_SIZE & 0xFF,
                        (EEPROM_SIZE >> 8) & 0xFF, EEPROM_SIZE & 0xFF,
                        (FLASH_SIZE >> 24) & 0xFF, (FLASH_SIZE >> 16) & 0xFF,
                        (FLASH_SIZE >> 8) & 0xFF, FLASH_SIZE & 0xFF])
        SEND_CMD(LINK, [STK_SET_DEVICE_EXT, 0x05, 0x04, 0xD7, 0xC2, 0x00])
        SEND_CMD(LINK, [STK_ENTER_PROGMODE])

        LINK.write(bytes([STK_READ_SIGN, STK_CRC_EOP]))
        SIG = LINK.read(5)
        if len(SIG) < 5 or SIG[0] != STK_INSYNC or SIG[4] != STK_OK:
            raise IOError(f"Read signature failed: {SIG.hex() if SIG else 'timeout'}")
        print(f"[+] Device signature: 0x{SIG[1]:02X}{SIG[2]:02X}{SIG[3]:02X}")

        CURRENT = {IDX: READ_FUSE(LINK, IDX) for IDX in range(N_FUSES)}
        CUR_LOCK = READ_LOCK(LINK)
        print("[+] Fuses on chip: " +
              "  ".join(f"{AVR_FUSE_OPS[i]['name']}=0x{CURRENT[i]:02X}" for i in range(N_FUSES)) +
              f"  LOCK=0x{CUR_LOCK:02X}")

        WANTED = dict(HEX_FUSES)

        BR_IDX = META["bootrst_fuse"]
        if BR_IDX is None:
            if not META["bootrst"]:
                print("[*] Device has no boot section - BOOTRST auto-fix not applicable.")
        elif BR_IDX in WANTED:
            print(f"[*] BOOTRST auto-fix skipped - hex file supplies "
                  f"{AVR_FUSE_OPS[BR_IDX]['name']} explicitly.")
        elif not (CURRENT[BR_IDX] & 0x01):
            WANTED[BR_IDX] = CURRENT[BR_IDX] | 0x01
            print(f"[!] BOOTRST programmed - auto-fixing {AVR_FUSE_OPS[BR_IDX]['name']} fuse "
                  f"0x{CURRENT[BR_IDX]:02X} -> 0x{WANTED[BR_IDX]:02X} so the part boots from 0x0000.")

        for IDX in sorted(WANTED):
            VAL   = WANTED[IDX] & 0xFF
            FNAME = AVR_FUSE_OPS[IDX]["name"]

            SP_IDX, SP_MASK = META["spien"]
            if IDX == SP_IDX and (VAL & SP_MASK):
                raise ValueError(f"REFUSED: {FNAME} fuse 0x{VAL:02X} leaves SPIEN unprogrammed "
                                 f"(bit 0x{SP_MASK:02X}) - that disables SPI ISP and the part "
                                 f"could only be recovered with a high-voltage programmer.")

            if META["rstdisbl"] is not None:
                RD_IDX, RD_MASK = META["rstdisbl"]
                if IDX == RD_IDX and not (VAL & RD_MASK):
                    raise ValueError(f"REFUSED: {FNAME} fuse 0x{VAL:02X} programs RSTDISBL "
                                     f"(bit 0x{RD_MASK:02X}) - RESET becomes an I/O pin and ISP "
                                     f"stops working permanently.")

            if META["dwen"] is not None:
                DW_IDX, DW_MASK = META["dwen"]
                if IDX == DW_IDX and not (VAL & DW_MASK):
                    raise ValueError(f"REFUSED: {FNAME} fuse 0x{VAL:02X} programs DWEN "
                                     f"(bit 0x{DW_MASK:02X}) - debugWIRE takes over RESET and "
                                     f"blocks SPI ISP until DWEN is cleared over debugWIRE.")

            if IDX == 0 and META["slow_osc"] is not None:
                OSC_MASK, OSC_VAL = META["slow_osc"]
                if (VAL & OSC_MASK) == OSC_VAL:
                    raise ValueError(f"REFUSED: LOW fuse 0x{VAL:02X} selects the internal 128 kHz "
                                     f"oscillator. The bridge clocks SPI at 200 kHz and the AVR "
                                     f"requires target_clk/4, so the ISP link would die mid-session.")

        def WRITE_FUSE(LINK, IDX):
            if IDX not in WANTED:
                return
            VAL   = WANTED[IDX] & 0xFF
            FNAME = AVR_FUSE_OPS[IDX]["name"]
            if VAL == CURRENT[IDX]:
                print(f"[+] {FNAME} fuse already 0x{VAL:02X} - no write needed.")
                return
            print(f"[*] Writing {FNAME} fuse: 0x{CURRENT[IDX]:02X} -> 0x{VAL:02X}")
            UNIVERSAL(LINK, 0xAC, AVR_FUSE_OPS[IDX]["write"], 0x00, VAL)
            WAIT_READY(LINK)
            VERIFY = READ_FUSE(LINK, IDX)
            if VERIFY != VAL:
                raise IOError(f"{FNAME} fuse write failed: wrote 0x{VAL:02X}, read 0x{VERIFY:02X}")
            CURRENT[IDX] = VERIFY
            print(f"[+] {FNAME} fuse verified: 0x{VERIFY:02X}")

        for IDX in (2, 1):
            if IDX < N_FUSES:
                WRITE_FUSE(LINK, IDX)

        SEND_CMD(LINK, [STK_CHIP_ERASE])
        print("[+] Chip erase OK.")

        END_BOUND = ((max(FLASH.keys()) + PAGE_SIZE) // PAGE_SIZE) * PAGE_SIZE
        PAGES = [(A, bytes([FLASH.get(A + i, 0xFF) for i in range(PAGE_SIZE)]))
                 for A in range(0, END_BOUND, PAGE_SIZE)]
        PAGES = [(A, D) for A, D in PAGES if any(b != 0xFF for b in D)]

        for INDEX, (PAGE_ADDR, PAGE_DATA) in enumerate(PAGES, 1):
            print(f"\r[*] Writing FLASH: {int(INDEX / len(PAGES) * 100):3d}%  "
                  f"({INDEX}/{len(PAGES)} pages)", end='', flush=True)
            WORD_ADDR = PAGE_ADDR >> 1
            N   = len(PAGE_DATA)
            PKT = bytes([STK_LOAD_ADDRESS, WORD_ADDR & 0xFF, (WORD_ADDR >> 8) & 0xFF, STK_CRC_EOP,
                         STK_PROG_PAGE, (N >> 8) & 0xFF, N & 0xFF, STK_MEMTYPE_FLASH] +
                        list(PAGE_DATA) + [STK_CRC_EOP])
            LINK.write(PKT)
            RESP = LINK.read(4)
            if RESP != bytes([STK_INSYNC, STK_OK, STK_INSYNC, STK_OK]):
                raise IOError(f"FLASH page write failed at 0x{PAGE_ADDR:04X}: "
                              f"{RESP.hex() if RESP else 'timeout'}")
        print()

        MISMATCHES = []
        for INDEX, (PAGE_ADDR, PAGE_DATA) in enumerate(PAGES, 1):
            print(f"\r[*] Verifying FLASH: {int(INDEX / len(PAGES) * 100):3d}%  "
                  f"({INDEX}/{len(PAGES)} pages)", end='', flush=True)
            WORD_ADDR = PAGE_ADDR >> 1
            N    = len(PAGE_DATA)
            LINK.write(bytes([STK_LOAD_ADDRESS, WORD_ADDR & 0xFF, (WORD_ADDR >> 8) & 0xFF,
                              STK_CRC_EOP]))
            BACK = SEND_CMD(LINK, [STK_READ_PAGE, (N >> 8) & 0xFF, N & 0xFF, STK_MEMTYPE_FLASH],
                            RX_EXTRA=N + 2)
            if BACK[:2] != bytes([STK_OK, STK_INSYNC]):
                raise IOError(f"LOAD_ADDRESS failed before FLASH read at 0x{PAGE_ADDR:04X}")
            BACK = BACK[2:]
            for i, (WR, RD) in enumerate(zip(PAGE_DATA, BACK)):
                if WR != RD:
                    MISMATCHES.append((PAGE_ADDR + i, WR, RD))
        print()
        if MISMATCHES:
            for ADDR, WR, RD in MISMATCHES[:5]:
                print(f"    [!] FLASH 0x{ADDR:04X}: wrote 0x{WR:02X}, read 0x{RD:02X}")
            raise IOError(f"FLASH verify FAILED: {len(MISMATCHES)} byte(s) mismatched")
        print(f"[+] FLASH verify OK ({len(PAGES)} pages, {len(PAGES) * PAGE_SIZE} bytes).")

        if EEPROM:
            EE_ADDRS = sorted(EEPROM.keys())
            START    = (EE_ADDRS[0]  // ISP_EEPROM_CHUNK_BYTES) * ISP_EEPROM_CHUNK_BYTES
            END      = ((EE_ADDRS[-1] // ISP_EEPROM_CHUNK_BYTES) + 1) * ISP_EEPROM_CHUNK_BYTES
            CHUNKS   = [(A, bytes([EEPROM.get(A + i, 0xFF) for i in range(ISP_EEPROM_CHUNK_BYTES)]))
                        for A in range(START, END, ISP_EEPROM_CHUNK_BYTES)]
            # EESAVE is HIGH fuse bit 3: when programmed (0) chip erase leaves EEPROM
            # untouched, so all-0xFF chunks still have to be written and verified
            EEPROM_ERASED = not (N_FUSES > 1 and not (CURRENT[1] & 0x08))
            if EEPROM_ERASED:
                CHUNKS = [(A, D) for A, D in CHUNKS if any(b != 0xFF for b in D)]

            EE_PAGE = META.get("eeprom_page")

            for INDEX, (EE_ADDR, DATA) in enumerate(CHUNKS, 1):
                print(f"\r[*] Writing EEPROM: {int(INDEX / len(CHUNKS) * 100):3d}%  "
                      f"({INDEX}/{len(CHUNKS)} chunks)", end='', flush=True)

                if EE_PAGE:
                    # Load one hardware EEPROM page (0xC1 per byte), commit it (0xC2) and
                    # poll RDY/BSY - about 3x faster than the bridge's byte-by-byte writes
                    # with a fixed worst-case delay after every byte
                    for PG in range(0, len(DATA), EE_PAGE):
                        PAGE_BYTES = DATA[PG:PG + EE_PAGE]
                        if EEPROM_ERASED and all(b == 0xFF for b in PAGE_BYTES):
                            continue
                        BASE = EE_ADDR + PG
                        for i, b in enumerate(PAGE_BYTES):
                            UNIVERSAL(LINK, 0xC1, 0x00, (BASE + i) & (EE_PAGE - 1), b)
                        UNIVERSAL(LINK, 0xC2, (BASE >> 8) & 0xFF, BASE & 0xFF & ~(EE_PAGE - 1), 0x00)
                        WAIT_READY(LINK)
                    continue

                SEND_CMD(LINK, [STK_LOAD_ADDRESS, EE_ADDR & 0xFF, (EE_ADDR >> 8) & 0xFF])
                N   = len(DATA)
                PKT = bytes([STK_PROG_PAGE, (N >> 8) & 0xFF, N & 0xFF, STK_MEMTYPE_EEPROM] +
                            list(DATA) + [STK_CRC_EOP])
                LINK.write(PKT)
                RESP = LINK.read(2)
                if RESP != bytes([STK_INSYNC, STK_OK]):
                    raise IOError(f"EEPROM write failed at 0x{EE_ADDR:04X}: "
                                  f"{RESP.hex() if RESP else 'timeout'}")
            print()

            EE_MISMATCHES = []
            for INDEX, (EE_ADDR, DATA) in enumerate(CHUNKS, 1):
                print(f"\r[*] Verifying EEPROM: {int(INDEX / len(CHUNKS) * 100):3d}%  "
                      f"({INDEX}/{len(CHUNKS)} chunks)", end='', flush=True)
                SEND_CMD(LINK, [STK_LOAD_ADDRESS, EE_ADDR & 0xFF, (EE_ADDR >> 8) & 0xFF])
                N    = len(DATA)
                BACK = SEND_CMD(LINK, [STK_READ_PAGE, (N >> 8) & 0xFF, N & 0xFF, STK_MEMTYPE_EEPROM],
                                RX_EXTRA=N)
                for i, (WR, RD) in enumerate(zip(DATA, BACK)):
                    if WR != RD:
                        EE_MISMATCHES.append((EE_ADDR + i, WR, RD))
            print()
            if EE_MISMATCHES:
                for ADDR, WR, RD in EE_MISMATCHES[:5]:
                    print(f"    [!] EEPROM 0x{ADDR:04X}: wrote 0x{WR:02X}, read 0x{RD:02X}")
                raise IOError(f"EEPROM verify FAILED: {len(EE_MISMATCHES)} byte(s) mismatched")
            print(f"[+] EEPROM verify OK ({len(CHUNKS)} chunks).")

        if HEX_LOCK:
            WANT_LOCK = HEX_LOCK[0] & 0xFF
            LIVE_LOCK = READ_LOCK(LINK)
            if WANT_LOCK == 0xFF:
                print(f"[+] Lock byte 0xFF requested (no locks) - chip erase already left "
                      f"the device unlocked (reads 0x{LIVE_LOCK:02X}).")
            elif (LIVE_LOCK & (~WANT_LOCK) & 0xFF) == 0:
                print(f"[+] Lock bits already satisfy 0x{WANT_LOCK:02X} (reads 0x{LIVE_LOCK:02X}).")
            else:
                print(f"[*] Writing lock bits: 0x{LIVE_LOCK:02X} -> 0x{WANT_LOCK:02X}")
                if META["lock_write"] == "T26":
                    UNIVERSAL(LINK, 0xAC, 0xFC | (WANT_LOCK & 0x03), 0x00, 0x00)
                else:
                    UNIVERSAL(LINK, 0xAC, 0xE0, 0x00, 0xC0 | (WANT_LOCK & 0x3F))
                WAIT_READY(LINK)
                BACK_LOCK = READ_LOCK(LINK)
                if (BACK_LOCK & (~WANT_LOCK) & 0xFF) != 0:
                    raise IOError(f"Lock bit write failed: wanted 0x{WANT_LOCK:02X}, "
                                  f"read 0x{BACK_LOCK:02X}")
                print(f"[+] Lock bits verified: 0x{BACK_LOCK:02X}")

        WRITE_FUSE(LINK, 0)

        SEND_CMD(LINK, [STK_LEAVE_PROGMODE])
        print("[+] Left programming mode; RESET released.")


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
    PARSER.add_argument('--FW_FILE_PATH', type=str, required=True, help="Absolute FW Path.")
    PARSER.add_argument('--MINIDRIVER_PATH', type=str, default=None,
                        help="CYBT/AIROC minidriver hex")
    PARSER.add_argument('--BAUDRATE', type=int, default=AIROC_DEFAULT_BAUDRATE,
                        help="CYBT/AIROC download baud")

    ARGS = PARSER.parse_args()

    # ========================================================================================
    #                           (2) Determe MCU                                              #
    # ========================================================================================
    TARGET_MCU = ARGS.MCU.upper()

    # ========================================================================================
    #                          (3) Verify Pico FW                                            #
    # ========================================================================================
    if TARGET_MCU in NXP_BDM_TARGETS:
        required_fw = FIRMWARE_MAP["HCS08_BDM"]
    elif TARGET_MCU in PIC_ICSP_TARGETS:
        required_fw = FIRMWARE_MAP["PIC_ICSP"]
    elif TARGET_MCU in XMEGA_PDI_TARGETS:
        required_fw = FIRMWARE_MAP["XMEGA_PDI"]
    elif TARGET_MCU in CYPRESS_HCIUART_TARGETS:
        required_fw = FIRMWARE_MAP["CYPRESS_HCIUART"]
    elif TARGET_MCU in CORTEX_M_SWD_TARGETS:
        required_fw = FIRMWARE_MAP["OPENOCD_SWD"]
    elif TARGET_MCU in AVR_UPDI_TARGETS:
        required_fw = FIRMWARE_MAP["PYMCUPROG_UPDI"]
    elif TARGET_MCU in AVR_SPI_TARGETS:
        required_fw = FIRMWARE_MAP["STK500V1_SPI"]
    else:
        print(f"[-] MCU '{TARGET_MCU}' not recognized. Check --MCU argument.")
        print("\n>>> RESULT: FAIL <<<")
        return

    if not ENSURE_FW(required_fw["name"], required_fw["uf2"]):
        return

    # ========================================================================================
    #                          (4) Verify FW_FILE_PATH                                       #
    # ========================================================================================
    ABS_FW = os.path.abspath(ARGS.FW_FILE_PATH)

    if not os.path.exists(ABS_FW):
        print(f"[+] Firmware file not found {ABS_FW}")
        print("\n>>> RESULT: FAIL <<<")
        return

    if TARGET_MCU in CYPRESS_HCIUART_TARGETS:
        ABS_MINIDRIVER = os.path.abspath(ARGS.MINIDRIVER_PATH or "")

        if not ARGS.MINIDRIVER_PATH or not os.path.exists(ABS_MINIDRIVER):
            print(f"[-] Error: --MINIDRIVER_PATH missing or not found: {ARGS.MINIDRIVER_PATH}")
            print("\n>>> RESULT: FAIL <<<")
            return

    # ========================================================================================
    #                          (5) MCU Handling                                              #
    # ========================================================================================
    if TARGET_MCU in NXP_BDM_TARGETS:
        S19_PAYLOADS = PARSE_S19_FILE(ARGS.FW_FILE_PATH, WIDTH=64)
    elif TARGET_MCU in PIC_ICSP_TARGETS or TARGET_MCU in XMEGA_PDI_TARGETS:
        HEX_PAYLOADS = PARSE_HEX_FILE(ARGS.FW_FILE_PATH, WIDTH=32)
    elif TARGET_MCU in CYPRESS_HCIUART_TARGETS:
        MINIDRIVER = PARSE_AIROC_HEX(ABS_MINIDRIVER, AIROC_WRITE_CHUNK)
        FIRMWARE   = PARSE_AIROC_HEX(ABS_FW, AIROC_WRITE_CHUNK)

        if not MINIDRIVER or not FIRMWARE:
            print("\n>>> RESULT: FAIL <<<")
            return
    elif TARGET_MCU in CORTEX_M_SWD_TARGETS:
        print(f"[+] Device: {TARGET_MCU} — routing to OpenOCD/CMSIS-DAP")

        meta = CORTEX_M_SWD_TARGETS[TARGET_MCU]
        tcl_fw = ABS_FW.replace('\\', '/')

        if ABS_FW.lower().endswith('.bin'):
            prog_cmd = f"program \"{tcl_fw}\" {meta['flash_base']} verify reset"
        else:
            prog_cmd = f"program \"{tcl_fw}\" verify reset"
        cmd = [
            OPENOCD,
            "-s", OPENOCD_SCRIPTS,
            "-f", "interface/cmsis-dap.cfg",
            "-c", "transport select swd",
            "-f", meta["cfg"],
            "-c", "init",
            "-c", "targets",
            "-c", "reset halt",
            "-c", f"{meta['erase_driver']} mass_erase 0",
            "-c", prog_cmd,
            "-c", "shutdown"
        ]

        t0 = time.perf_counter()

        try:
            subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
            print(f"[+] OpenOCD complete in {(time.perf_counter()-t0)*1000:.1f} ms.")
            print("\n>>> RESULT: PASS <<<")
        except subprocess.CalledProcessError as e:
            RECOVER_CMD = meta.get("recover_cmd")
            if RECOVER_CMD and "AP lock engaged" in (e.stderr or ""):
                print(f"[!] Debug AP locked (APPROTECT) — attempting '{RECOVER_CMD}'...")
                recover_cmd = [
                    OPENOCD,
                    "-s", OPENOCD_SCRIPTS,
                    "-f", "interface/cmsis-dap.cfg",
                    "-c", "transport select swd",
                    "-f", meta["cfg"],
                    "-c", "init",
                    "-c", RECOVER_CMD,
                    "-c", "shutdown"
                ]
                try:
                    subprocess.run(recover_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
                    print("[+] Recovery complete. Retrying flash...")
                    subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
                    print(f"[+] OpenOCD complete in {(time.perf_counter()-t0)*1000:.1f} ms.")
                    print("\n>>> RESULT: PASS <<<")
                except subprocess.CalledProcessError as e2:
                    print(f"[-] OpenOCD error after recovery attempt:\n{e2.stderr}", file=sys.stderr)
                    print("\n>>> RESULT: FAIL <<<")
            else:
                print(f"[-] OpenOCD error:\n{e.stderr}", file=sys.stderr)
                print("\n>>> RESULT: FAIL <<<")
        return
    elif TARGET_MCU in AVR_UPDI_TARGETS:
        print(f"[+] Device: {TARGET_MCU} — routing to pymcuprog / serialupdi")

        meta      = AVR_UPDI_TARGETS[TARGET_MCU]
        pico_port = AUTO_DETECT_PICO2(PREFFER_ITF=0)

        if not pico_port:
            print("[-] No Pico detected.")
            print("\n>>> RESULT: FAIL <<<")
            return
        if not os.path.exists(PYMCUPROG):
            print(f"[-] pymcuprog not found at {PYMCUPROG}. Run: pip install pymcuprog")
            print("\n>>> RESULT: FAIL <<<")
            return
        if not PORT_IS_FREE(pico_port):
            print(f"[-] Error: {pico_port} would not accept a connection. Either a "
                  f"pymcuprog stalled by an earlier run still owns it - it frees the port "
                  f"about a minute after being killed - or the Pico's USB is wedged and "
                  f"needs to be unplugged and back in.")
            print("\n>>> RESULT: FAIL <<<")
            return

        base_cmd = PYMCUPROG_LAUNCHER() + ["-t", "uart", "-d", TARGET_MCU.lower(),
                                           "-u", pico_port, "--clk", str(meta["baud"])]
        t0 = time.perf_counter()

        # A wrong, unpowered or absent part leaves pymcuprog stuck in the SerialUPDI
        # handshake with no output and no end. pymcuprog pings the part before it
        # writes, so the write runs as one UPDI session and is killed early if that
        # ping never answers - one handshake instead of a separate ping run plus the
        # write's own
        print(f"[*] Writing + verifying all memories from a single image...")
        WRITE = RUN_PYMCUPROG_WATCHED(base_cmd + ["write", "--erase", "-f", ABS_FW, "--verify"],
                                      "Ping response", UPDI_PING_TIMEOUT_S, UPDI_WRITE_TIMEOUT_S)

        if WRITE == "NO_RESPONSE":
            print(f"[-] Error: no UPDI response within {UPDI_PING_TIMEOUT_S:.0f}s. Check that "
                  f"the target is connected and powered, and that it really is a "
                  f"{TARGET_MCU}.")
            print("\n>>> RESULT: FAIL <<<")
            return
        if WRITE is None:
            print(f"[-] pymcuprog write timed out after {UPDI_WRITE_TIMEOUT_S:.0f}s.")
            print("\n>>> RESULT: FAIL <<<")
            return
        if not WRITE:
            print(f"[-] pymcuprog write failed. If the ping response above is not the "
                  f"{TARGET_MCU} signature, the connected part is not a {TARGET_MCU}.")
            print("\n>>> RESULT: FAIL <<<")
            return

        print(f"[+] UPDI complete in {(time.perf_counter()-t0)*1000:.1f} ms.")
        print("\n>>> RESULT: PASS <<<")
        return
    elif TARGET_MCU in AVR_SPI_TARGETS:
        print(f"[+] Device: {TARGET_MCU} — routing to native SPI ISP (STK500v1_SPI)")

        meta = AVR_SPI_TARGETS[TARGET_MCU]
        print(f"[*] Part '{meta['part']}' — flash {meta['flash_size']} B / page {meta['page_size']} B, "
              f"eeprom {meta['eeprom_size']} B, {meta['n_fuses']} fuse byte(s) "
              f"(ATDF: {meta['atdf']})")

        BUCKETS = BUCKET_AVR_UNIFIED_HEX(ABS_FW)

        if BUCKETS is None:
            print(f"[-] Failed to parse: {ABS_FW}")
            print("\n>>> RESULT: FAIL <<<")
            return

        for NOTE in BUCKETS['IGNORED']:
            print(f"[!] Ignoring record outside the classic-AVR memory map: {NOTE}")

        print(f"[+] Combined image: FLASH {len(BUCKETS['FLASH'])} B, "
              f"EEPROM {len(BUCKETS['EEPROM'])} B, "
              f"FUSES {sorted(BUCKETS['FUSES'])}, "
              f"LOCK {'yes' if BUCKETS['LOCK'] else 'no'}")

        pico_port = AUTO_DETECT_PICO2()

        if not pico_port:
            print("[-] No Pico detected.")
            print("\n>>> RESULT: FAIL <<<")
            return

        t0 = time.perf_counter()

        try:
            PROGRAM_AVR_ISP(pico_port, meta, BUCKETS)
        except (IOError, ValueError) as e:
            print(f"[-] ISP error: {e}")
            print("\n>>> RESULT: FAIL <<<")
            return
        except serial.SerialException as e:
            print(f"[-] Serial Port Error: {e}")
            print("\n>>> RESULT: FAIL <<<")
            return

        print(f"[+] ISP complete in {(time.perf_counter()-t0)*1000:.1f} ms.")
        print("\n=======================================================")
        print("[SUCCESS] FLASH SUCCESS: Target memory maps fully verified!")
        print("=======================================================\n")
        print("\n>>> RESULT: PASS <<<")
        return
    else:
        return

    # ========================================================================================
    #           (6) [NXP, PIC, XMEGA] Bit-Bang / [CYPRESS] HCI download                      #
    # ========================================================================================
    PICO_PORT = AUTO_DETECT_PICO2()

    if PICO_PORT is not None:
        try:
            print(f"[+] Opening serial link to Pico2 on {PICO_PORT}...")

            PICO_CONNECTION = serial.Serial(PICO_PORT, baudrate=115200,
                                           timeout=SERIAL_READ_TIMEOUT_S,
                                           write_timeout=SERIAL_WRITE_TIMEOUT_S)
            time.sleep(PORT_SETTLE_S)

            PICO_CONNECTION.reset_input_buffer()
            PICO_CONNECTION.reset_output_buffer()

            # ========================================================================================
            #          (7) SEND DEVICE FAMILY AND CONFIRM ACK FROM RP PICO 2                         #
            # ========================================================================================
            print(f"[+] Configuring target device family: {ARGS.MCU}")

            INIT_CMD = f"INIT_FAMILY:{ARGS.MCU}\n"
            PICO_CONNECTION.write(INIT_CMD.encode('utf-8'))
            PICO_CONNECTION.flush()

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
            #      (7B) [CYPRESS] HCI DOWNLOAD OVER THE PICO'S USB <-> HCI UART BRIDGE               #
            # ========================================================================================
            if TARGET_MCU in CYPRESS_HCIUART_TARGETS:
                START_TIME = time.perf_counter()

                try:
                    AIROC_OK = AIROC_DOWNLOAD(PICO_CONNECTION, MINIDRIVER, FIRMWARE,
                                              ARGS.BAUDRATE)
                finally:
                    try:
                        PICO_CONNECTION.baudrate = AIROC_EXIT_BAUD   # Pico reboots out of bridge mode
                    except serial.SerialException:
                        pass                                         # expected: port vanishes mid-call

                if AIROC_OK:
                    print(f"[+] Downloaded and verified in {time.perf_counter() - START_TIME:.2f} s.")
                    print("\n=======================================================")
                    print("[SUCCESS] FLASH SUCCESS: flash matches the hex file!")
                    print("=======================================================\n")
                    print("\n>>> RESULT: PASS <<<")
                else:
                    print("\n>>> RESULT: FAIL <<<")
                return

            # ========================================================================================
            #            (8) HIGH-SPEED BINARY DATA STREAM (w/ MCU_FILTER)                           #
            # ========================================================================================
            START_TIME = time.perf_counter()

            if TARGET_MCU in NXP_BDM_TARGETS:
                print("[+] Starting high-speed binary stream ...")

                for index, block in enumerate(S19_PAYLOADS, 1):
                    BINARY_ADDR = block['address'].to_bytes(2, byteorder='big')
                    BINARY_DATA = block['data_bytes']

                    PACKET = BINARY_ADDR + BINARY_DATA

                    PICO_CONNECTION.write(PACKET)
                    PICO_CONNECTION.flush()

                # ========================================================================================
                #                      (9A) ...Line Transfer Completed                                   #
                # ========================================================================================
                END_TIME    = time.perf_counter()
                DURATION_MS = (END_TIME - START_TIME)  * 1000
                print(f"[+] Data Transfer stream completed in {DURATION_MS:.2f} ms.")

            if TARGET_MCU in PIC_ICSP_TARGETS or TARGET_MCU in XMEGA_PDI_TARGETS:
                print("[+] Starting high-speed binary stream ...")

                if len(HEX_PAYLOADS) > HEX_MAX_PACKETS:
                    print(f"[-] Error: {len(HEX_PAYLOADS)} packets exceeds the Pico's "
                          f"HEX_STAGING_BUFFER capacity of {HEX_MAX_PACKETS}. "
                          f"The image would be silently truncated. Aborting.")
                    PICO_CONNECTION.close()
                    return

                STREAM = bytearray()
                for block in HEX_PAYLOADS:
                    STREAM += block['address'].to_bytes(4, byteorder='big')
                    STREAM += block['data_bytes']

                TOTAL_BYTES = len(STREAM)

                for offset in range(0, TOTAL_BYTES, STREAM_CHUNK_BYTES):
                    PICO_CONNECTION.write(STREAM[offset:offset + STREAM_CHUNK_BYTES])

                # End-of-stream packet: the Pico starts programming at once instead of
                # waiting out its 400 ms no-data timeout
                PICO_CONNECTION.write(HEX_END_OF_STREAM_ADDR.to_bytes(4, byteorder='big')
                                      + bytes([0xFF] * HEX_PAYLOAD_BYTES))
                PICO_CONNECTION.flush()

                # ========================================================================================
                #                     (9B) ...Line Transfer Completed                                    #
                # ========================================================================================
                END_TIME = time.perf_counter()
                DURATION_MS = (END_TIME - START_TIME) * 1000
                print(f"[+] Data Transfer stream completed in {DURATION_MS:.2f} ms "
                      f"({len(HEX_PAYLOADS)} packets, {TOTAL_BYTES} bytes).")

            # ========================================================================================
            #                         (10) Wait for PASS or FAIL                                     #
            # ========================================================================================
            print("[+] Stream complete. Waiting for target flash verification...")

            PREV_TIMEOUT             = PICO_CONNECTION.timeout
            PICO_CONNECTION.timeout  = PROGRAM_WAIT_TIMEOUT_S

            PROGRAM_FINISHED = False
            PROGRAM_START    = time.perf_counter()

            try:
                while not PROGRAM_FINISHED:
                    DEBUG_LINE = PICO_CONNECTION.readline().decode('utf-8', errors='ignore').strip()

                    if not DEBUG_LINE:
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
        except serial.SerialException as e:
            print(f"[-] Serial Port Error: {e}")
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
