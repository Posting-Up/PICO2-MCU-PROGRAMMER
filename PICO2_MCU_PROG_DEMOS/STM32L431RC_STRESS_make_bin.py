"""Builds STM32L431RC_STRESS.bin: the compiled program followed by a full-flash pattern fill.

    python STM32L431RC_STRESS_make_bin.py prog.bin STM32L431RC_STRESS.bin
"""
import sys

FLASH_SIZE = 256 * 1024

prog = open(sys.argv[1], 'rb').read()
assert len(prog) < FLASH_SIZE

x = 0x431CA5A5
image = bytearray(FLASH_SIZE)
for a in range(FLASH_SIZE):
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= x >> 17
    x ^= (x << 5) & 0xFFFFFFFF
    image[a] = prog[a] if a < len(prog) else (x & 0xFF) % 0xFF

open(sys.argv[2], 'wb').write(image)
print(f'program {len(prog)} B + pattern -> {len(image)} B, 0xFF bytes outside the program: '
      f'{image[len(prog):].count(0xFF)}')
