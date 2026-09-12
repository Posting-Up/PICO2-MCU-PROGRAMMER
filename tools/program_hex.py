"""Program the pinned XMEGA combined Intel HEX image through the dedicated firmware."""
import argparse
import hashlib
from pathlib import Path
import sys
import time

DEFAULT_IMAGE = Path(__file__).resolve().parents[1] / "PICO2_MCU_PROG_DEMOS/[AVR] ATXMEGA192A3U - PDI/XMEGA192A3U_STRESS.hex"
REGIONS = (
    ("FLASH", 0, 0x32000),
    ("EEPROM", 0x810000, 2048),
    ("FUSE", 0x820000, 6),
    ("USERSIG", 0x850000, 512),
)
FUSE_MASKS = (0xFF, 0xFF, 0x63, 0, 0x1F, 0x3F)


def parse_hex(path):
    memory = {}
    upper = 0
    eof = False
    for lineno, line in enumerate(Path(path).read_text(encoding="ascii").splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if eof:
            raise ValueError(f"line {lineno}: data after EOF")
        if not line.startswith(":"):
            raise ValueError(f"line {lineno}: missing colon")
        try:
            rec = bytes.fromhex(line[1:])
        except ValueError as exc:
            raise ValueError(f"line {lineno}: malformed hex") from exc
        if len(rec) < 5 or len(rec) != rec[0] + 5 or sum(rec) & 255:
            raise ValueError(f"line {lineno}: length/checksum error")
        count, addr, kind = rec[0], int.from_bytes(rec[1:3], "big"), rec[3]
        data = rec[4:-1]
        if kind == 0:
            if addr + count > 0x10000:
                raise ValueError(f"line {lineno}: record crosses 64 KiB boundary")
            for i, value in enumerate(data):
                absolute = upper + addr + i
                if absolute in memory:
                    raise ValueError(f"line {lineno}: overlapping address {absolute:#x}")
                memory[absolute] = value
        elif kind == 1 and count == 0 and addr == 0:
            eof = True
        elif kind in (2, 4) and count == 2 and addr == 0:
            upper = int.from_bytes(data, "big") << (4 if kind == 2 else 16)
        elif kind in (3, 5) and count == 4 and addr == 0:
            pass  # Entry-point metadata; does not program a memory location.
        else:
            raise ValueError(f"line {lineno}: unsupported/malformed record type {kind}")
    if not eof or not memory:
        raise ValueError("missing EOF or empty image")
    return memory


def plan_image(memory):
    sections = {name: {} for name, _, _ in REGIONS}
    for address, value in memory.items():
        for name, base, size in REGIONS:
            if base <= address < base + size:
                sections[name][address - base] = value
                break
        else:
            raise ValueError(f"unsupported memory address {address:#08x}")
    fuses = sections["FUSE"]
    if 3 in fuses:
        if fuses[3] != 0xFF:
            raise ValueError("reserved FUSEBYTE3 must be FF when present")
        del fuses[3]
    for index, value in fuses.items():
        if (value | FUSE_MASKS[index]) != 255:
            raise ValueError(f"FUSEBYTE{index}: reserved bits must be one")
        if index == 2 and value & 3 == 0:
            raise ValueError("reserved BODPD configuration")
        if index == 4 and value & 12 == 8:
            raise ValueError("reserved STARTUPTIME configuration")
        if index == 5 and value & 48 == 0:
            raise ValueError("reserved BODACT configuration")
    commands = []
    # Each command stays within one physical page; all fuses follow all memories.
    for name in ("FLASH", "EEPROM", "USERSIG", "FUSE"):
        items = sorted(sections[name].items())
        page_size = 32 if name == "EEPROM" else 1 if name == "FUSE" else 512
        start = previous = None
        data = bytearray()
        for offset, value in items:
            if start is not None and (offset != previous + 1 or offset // page_size != start // page_size):
                commands.append((name, start, bytes(data)))
                data.clear()
                start = None
            if start is None:
                start = offset
            data.append(value)
            previous = offset
        if data:
            commands.append((name, start, bytes(data)))
    return sections, commands


def exchange(port, command, timeout=120):
    port.write((command + "\n").encode("ascii"))
    port.flush()
    deadline = time.monotonic() + timeout
    lines = []
    while time.monotonic() < deadline:
        line = port.readline().decode("ascii", errors="replace").strip()
        if line == "PASS":
            return lines
        if line == "FAIL":
            raise RuntimeError(f"firmware rejected {command.split()[0]}: {'; '.join(lines)}")
        if line:
            lines.append(line)
    raise TimeoutError(f"no completion for {command.split()[0]}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--hex", type=Path, default=DEFAULT_IMAGE)
    parser.add_argument("--port", help="Pico 2 USB serial port, e.g. COM7")
    parser.add_argument("--write", action="store_true", help="program and verify; otherwise inspect only")
    args = parser.parse_args(argv)
    sections, commands = plan_image(parse_hex(args.hex))
    print(f"Image: {args.hex}")
    print(f"SHA256: {hashlib.sha256(args.hex.read_bytes()).hexdigest()}")
    for name, base, size in REGIONS:
        section = sections[name]
        if section:
            print(f"{name}: {len(section)} bytes, HEX {base + min(section):#08x}..{base + max(section):#08x}")
    print("Fuses: " + " ".join(f"{i}={v:02X}" for i, v in sorted(sections["FUSE"].items())))
    print(f"{len(commands)} verified write commands; reserved fuse byte 3 is skipped.")
    if not args.write:
        print("Inspection only. Add --port COMx --write to program.")
        return 0
    if not args.port:
        parser.error("--write requires --port")
    import serial
    with serial.Serial(args.port, 115200, timeout=1, write_timeout=10) as port:
        time.sleep(1)
        port.reset_input_buffer()
        if "ATXMEGA192A3U-PDI/1" not in exchange(port, "HELLO"):
            raise RuntimeError("wrong programmer firmware")
        try:
            exchange(port, "BEGIN")
            for number, (name, offset, data) in enumerate(commands, 1):
                exchange(port, f"{name} {offset} {data.hex()}")
                print(f"[{number}/{len(commands)}] {name} {offset:#x}: {len(data)} bytes verified")
            exchange(port, "END")
        except Exception:
            try:
                exchange(port, "END", timeout=5)
            except Exception:
                pass
            raise
    print("PASS: all supplied writable bytes verified; PDI released.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, RuntimeError, TimeoutError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
