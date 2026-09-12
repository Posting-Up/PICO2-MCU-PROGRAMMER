# ATxmega192A3U programmer for Raspberry Pi Pico 2

This project programs the supplied **XMEGA192A3U_STRESS.hex** through PDI.
It derives its physical transport from Posting-Up/PICO2-MCU-PROGRAMMER,
commit `fe31fce17efb8c8ad0d565a938c3f82ab5517cd4`.

The dedicated firmware is built from `programmer/CMakeLists.txt`. The upstream
root CMake file, `src/AVR_PDI.c`, and `USB_RP_COM.py` remain as reference;
they are **not** the entry points for this implementation.

## Supplied image

Default input:
`PICO2_MCU_PROG_DEMOS/[AVR] ATXMEGA192A3U - PDI/XMEGA192A3U_STRESS.hex`

SHA256: `66b2057e548685cdf00559d8e527b88756c1f396eb4ecd51b071c0766b816dc3`

| Region | HEX addresses | PDI base | Supplied bytes |
|---|---|---|---:|
| Flash, application + boot | 0x000000?0x031FFF | 0x00800000 | 204800 |
| EEPROM | 0x810000?0x8107FF | 0x008C0000 | 2048 |
| Configuration | 0x820000?0x820005 | 0x008F0020 | 6; 5 writable |
| User signature / upstream USER_ID | 0x850000?0x8501FF | 0x008E0400 | 512 |

EEPROM is `00..FF` repeated eight times; user signature is `00..FF`
repeated twice. Fuse bytes are `35 BE FF FF F2 EF`. Byte 3 is reserved:
the uploader requires FF there and skips it. The writable fuses are
`0=35 1=BE 2=FF 4=F2 5=EF`. Fuse 0 also sets the **JTAG user ID to 0x35**;
this differs from the 512-byte user-signature row.

These are the supplied image's configuration values, not recommended defaults.
The stress image overwrites the full application and boot flash; its test
patterns are not validated application firmware.

## Wiring and programming

Use a powered 3.3 V target with a shared ground. Pico GPIO is not a target
power supply; use suitable level translation for a target at another voltage.

| Pico 2 | ATxmega192A3U signal |
|---|---|
| GP0, physical pin 1 | PDI_CLK / RESET |
| GP1, physical pin 2 | PDI_DATA |
| GND, physical pin 3 | GND |

Check target package pin numbers against its board schematic/datasheet.
The firmware uses PIO2 SM0 and requests a 125 kHz PDI bit clock; dispatcher
overhead stretches inter-frame timing. Scope-level timing has not been tested.

1. Hold BOOTSEL while connecting the Pico 2 over USB.
2. Copy `build/atxmega192a3u_programmer.uf2` to its BOOTSEL drive.
3. Connect the target and determine the Pico's USB serial port.
4. From this project directory run:

```powershell
python -m pip install -r requirements.txt
python tools/program_hex.py
python tools/program_hex.py --port COM7 --write
```

Replace COM7 with the actual port. Without `--write`, the command only
validates and summarizes the HEX file. Select another image with `--hex PATH`.

The entire file is validated before connecting. The supplied image produces
470 write commands: 400 flash pages, 64 EEPROM pages, one user-signature row,
and five fuse bytes. Each response is checked; failures produce a nonzero host
exit code. Programming is not atomic across the image. A failure or power loss
can leave an incomplete image; correct the cause before retrying.

## Implementation

- `src/pdi_transport.c`: derived PDI framing, entry/exit, NVM access and exact
  device-ID check (`1E 97 44`); wall-clock NVM polling deadlines.
- `src/xmega_nvm.c`: page read/modify/write and full-page verification.
  EEPROM uses 32-byte pages; flash/user signature use 512-byte pages.
  Untouched bytes are preserved and identical pages are skipped.
  All-FF writes still erase existing non-FF data.
- EEPROM explicitly clears its buffer and uses erase/write command 0x35.
  User signature clears the flash buffer, erases the row, loads bytes in
  low/high order, then writes with 0x1A. Flash uses erase/write command 0x2F.
- Fuses are written individually with 0x4C, followed by reset/re-entry and
  masked readback. Reserved fuse bits must be supplied as one.
- `src/programmer_main.c`: USB line protocol. HELLO identifies this firmware;
  BEGIN checks the target and holds programming mode; FLASH/EEPROM/USERSIG/FUSE
  take a decimal offset and 1?512 hex-encoded bytes; END releases PDI.
  Errors, disconnect, and 30 seconds without a command release the session.
- `tools/program_hex.py`: strict Intel HEX checksums, lengths, range and
  overlap validation; exact-length sparse records; page-sized transfers.
  Flash, EEPROM and user signature precede configuration.
- The target normally stays in reset between transfers. Fuse verification
  releases reset and re-enters programming, so target execution can occur then.
- No chip erase, lock-bit programming, or automatic factory-default reset.
  A protected target that rejects PDI/NVM access fails rather than being erased.
- NVM timeout, readback mismatch, device mismatch and reset-release errors
  prevent PASS. The inherited low-level PIO FIFO operations remain blocking:
  an internal PIO/firmware fault is outside the NVM timeout guarantee.

## Build and tests

Windows prerequisites: Python 3, Git, CMake, Ninja, and Visual Studio C++ build
tools. Portable SDK/compiler downloads stay in ignored `.deps/`.

```powershell
.\build.ps1 -FetchDependencies
python -m unittest discover -s tests -v
cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

The dependency script pins Pico SDK 2.2.0 and xPack Arm GCC 14.2.1-1.1,
checks the compiler archive against its published SHA, and initializes TinyUSB.
For an existing toolchain, configure `programmer/` with your own
`PICO_SDK_PATH` and `PICO_TOOLCHAIN_PATH`.

The parser tests use the actual supplied image and prove every writable byte
appears exactly once in the command plan. Native C tests simulate NVM to check
page preservation, all-FF writes, boundaries, timeouts/read errors, verification
failures and fuse reset behavior. These tests do not emulate PDI wire timing.

## Validation result

Release UF2 build passed with Pico SDK 2.2.0 and Arm GCC 14.2.1. All three
project C sources compile with -Wall -Wextra -Werror. Nine Python tests and
the native NVM CTest passed. The firmware reserves 4096 bytes of stack;
compiler-reported main/dispatch/NVM frames are 40/552/1088 bytes.
See `validation.json` for hashes and build details.

**No physical programmer/target was exercised.** UF2 boot, USB enumeration,
PDI electrical timing, fuse reset behavior and silicon readback remain unverified.

## Sources and provenance

- [Upstream AVR_PDI.c](https://github.com/Posting-Up/PICO2-MCU-PROGRAMMER/blob/fe31fce17efb8c8ad0d565a938c3f82ab5517cd4/src/AVR_PDI.c)
- [Supplied stress image](https://github.com/Posting-Up/PICO2-MCU-PROGRAMMER/blob/fe31fce17efb8c8ad0d565a938c3f82ab5517cd4/PICO2_MCU_PROG_DEMOS/%5BAVR%5D%20ATXMEGA192A3U%20-%20PDI/XMEGA192A3U_STRESS.hex)
- [Microchip device datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-8386-8-and-16-bit-AVR-Microcontroller-ATxmega64A3U-128A3U-192A3U-256A3U_datasheet.pdf)
- [XMEGA AU manual](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-8331-8-and-16-bit-AVR-Microcontroller-XMEGA-AU_Manual.pdf), sections 4.16 and 33.12.

The upstream PIO source is retained unchanged. Its introductory speed comment
describes a divider of 1; this firmware supplies a divider calculated for
125 kHz instead. Original upstream sources and attribution are retained;
no new license is asserted over that code.
