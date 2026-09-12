import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("program_hex", ROOT / "tools/program_hex.py")
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)


def record(address, kind, data=b""):
    raw = bytes([len(data)]) + address.to_bytes(2, "big") + bytes([kind]) + data
    return ":" + (raw + bytes([-sum(raw) & 255])).hex()


class HexTests(unittest.TestCase):
    def parse(self, lines):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.hex"
            path.write_text("\n".join(lines))
            return host.parse_hex(path)

    def test_real_image(self):
        memory = host.parse_hex(host.DEFAULT_IMAGE)
        sections, commands = host.plan_image(memory)
        self.assertEqual(bytes(sections["EEPROM"].values()), bytes(range(256)) * 8)
        self.assertEqual(bytes(sections["USERSIG"].values()), bytes(range(256)) * 2)
        self.assertEqual(sections["FUSE"], {0: 0x35, 1: 0xBE, 2: 0xFF, 4: 0xF2, 5: 0xEF})
        rebuilt = {}
        bases = {name: base for name, base, _ in host.REGIONS}
        for name, offset, data in commands:
            for i, value in enumerate(data):
                address = bases[name] + offset + i
                self.assertNotIn(address, rebuilt)
                rebuilt[address] = value
        expected = dict(memory)
        del expected[0x820003]
        self.assertEqual(rebuilt, expected)
        self.assertTrue(all(name == "FUSE" for name, _, _ in commands[-5:]))

    def test_short_records_no_padding(self):
        memory = self.parse([record(0, 4, b"\x00\x82"), record(0, 0, b"\x35"),
                             record(0, 1)])
        _, commands = host.plan_image(memory)
        self.assertEqual(commands, [("FUSE", 0, b"\x35")])

    def test_page_crossing_and_unordered_records(self):
        memory = self.parse([record(0, 4, b"\x00\x81"),
                             record(33, 0, b"\x33"), record(31, 0, b"\x11\x22"), record(0, 1)])
        _, commands = host.plan_image(memory)
        self.assertEqual(commands, [("EEPROM", 31, b"\x11"), ("EEPROM", 32, b"\x22\x33")])

    def test_bad_checksum(self):
        with self.assertRaises(ValueError):
            self.parse([":010000003500", record(0, 1)])

    def test_missing_eof(self):
        with self.assertRaises(ValueError):
            self.parse([record(0, 0, b"x")])

    def test_overlapping(self):
        with self.assertRaises(ValueError):
            self.parse([record(0, 0, b"x"), record(0, 0, b"x"), record(0, 1)])

    def test_unknown_region(self):
        with self.assertRaises(ValueError):
            host.plan_image({0x830000: 0})

    def test_reserved_fuse(self):
        for address, value in [(0x820003, 0), (0x820004, 0), (0x820005, 0xCF)]:
            with self.assertRaises(ValueError):
                host.plan_image({address: value})

    def test_trailing_records(self):
        with self.assertRaises(ValueError):
            self.parse([record(0, 1), record(0, 0, b"x")])


if __name__ == "__main__":
    unittest.main()
