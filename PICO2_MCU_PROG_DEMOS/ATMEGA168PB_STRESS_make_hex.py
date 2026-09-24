"""Builds ATMEGA168PB_STRESS.hex: compiled program + full-flash pattern fill + EEPROM + fuses + lock."""
import sys

FLASH_SIZE, EEPROM_SIZE = 16384, 512
FUSES = [0xE2, 0xDE, 0xFD]          # LOW, HIGH, EXT  -> 0x820000..0x820002
LOCK = 0xEF                         # -> 0x830000


def read_ihex(path):
    mem, upper = {}, 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(':'):
            continue
        n, addr, typ = int(line[1:3], 16), int(line[3:7], 16), int(line[7:9], 16)
        data = bytes.fromhex(line[9:9 + 2 * n])
        if typ == 0:
            for i, b in enumerate(data):
                mem[upper + addr + i] = b
        elif typ == 4:
            upper = int.from_bytes(data, 'big') << 16
        elif typ == 1:
            break
    return mem


def records(base, data):
    out, upper = [], None
    for off in range(0, len(data), 16):
        addr = base + off
        if addr >> 16 != upper:
            upper = addr >> 16
            rec = bytes([2, 0, 0, 4, upper >> 8, upper & 0xFF])
            out.append(':' + rec.hex().upper() + f'{(-sum(rec)) & 0xFF:02X}')
        chunk = data[off:off + 16]
        rec = bytes([len(chunk), (addr >> 8) & 0xFF, addr & 0xFF, 0]) + chunk
        out.append(':' + rec.hex().upper() + f'{(-sum(rec)) & 0xFF:02X}')
    return out


prog = read_ihex(sys.argv[1])
x = 0x168BA5A5
flash = bytearray(FLASH_SIZE)
for a in range(FLASH_SIZE):
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= x >> 17
    x ^= (x << 5) & 0xFFFFFFFF
    flash[a] = prog[a] if a in prog else (x & 0xFF) % 0xFF

eeprom = bytes((n * 7 + 0x21) % 255 for n in range(EEPROM_SIZE))

lines = []
lines += records(0x000000, bytes(flash))
lines += records(0x810000, eeprom)
lines += records(0x820000, bytes(FUSES))
lines += records(0x830000, bytes([LOCK]))
lines.append(':00000001FF')
open(sys.argv[2], 'w', newline='\r\n').write('\n'.join(lines) + '\n')

print(f'program {len(prog)} B, flash 0xFF bytes: {flash.count(0xFF)} (all inside the program), '
      f'eeprom 0xFF bytes: {eeprom.count(0xFF)}')
