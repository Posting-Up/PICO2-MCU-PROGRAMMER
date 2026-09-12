#include "xmega_nvm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CMD 0x010001CAu
#define CTRLA 0x010001CBu
static uint8_t flash[0x32000], eeprom[2048], usersig[512], fuses[6], buffer[512];
static uint8_t command_byte;
static unsigned commits, erases, resets;
static bool fail_read, fail_wait, corrupt, fail_reset;
static uint8_t *map(uint32_t address)
{
    if (address >= 0x800000 && address < 0x832000) return &flash[address - 0x800000];
    if (address >= 0x8C0000 && address < 0x8C0800) return &eeprom[address - 0x8C0000];
    if (address >= 0x8E0400 && address < 0x8E0600) return &usersig[address - 0x8E0400];
    if (address >= 0x8F0020 && address < 0x8F0026) return &fuses[address - 0x8F0020];
    assert(!"unexpected address"); return NULL;
}
static bool read_byte(uint32_t a, uint8_t *value)
{
    assert(command_byte == 0x43);
    if (fail_read) return false;
    *value = *map(a);
    return true;
}
static bool wait_ready(void) { return !fail_wait; }
static bool reset_target(void) { ++resets; return !fail_reset; }
static void write_byte(uint32_t a, uint8_t value)
{
    if (a == CMD) {
        /* A chip erase or lock write must never be emitted. */
        assert(value != 0x40 && value != 0x08);
        command_byte = value; return;
    }
    if (a == CTRLA) {
        assert(value == 1 && (command_byte == 0x26 || command_byte == 0x36));
        memset(buffer, 255, sizeof(buffer)); return;
    }
    if (command_byte == 0x33 || command_byte == 0x23) {
        buffer[a % (command_byte == 0x33 ? 32 : 512)] = value; return;
    }
    if (command_byte == 0x18) {
        assert(a == 0x8E0400); memset(usersig, 255, sizeof(usersig)); ++erases; return;
    }
    if (command_byte == 0x35 || command_byte == 0x2F || command_byte == 0x1A) {
        unsigned size = command_byte == 0x35 ? 32 : 512;
        assert(a % size == 0);
        memcpy(map(a), buffer, size);
        if (corrupt) *map(a) ^= 1;
        ++commits; return;
    }
    if (command_byte == 0x4C) {
        *map(a) = corrupt ? value ^ 1 : value; ++commits; return;
    }
    assert(!"unexpected command");
}
static const xmega_bus bus = {read_byte, write_byte, wait_ready, reset_target};
static void init(void)
{
    memset(flash, 0xA5, sizeof(flash)); memset(eeprom, 0xA5, sizeof(eeprom));
    memset(usersig, 0xA5, sizeof(usersig)); memset(fuses, 255, sizeof(fuses));
    command_byte = 0; commits = erases = resets = 0;
    fail_read = fail_wait = corrupt = fail_reset = false;
}
int main(void)
{
    uint8_t data[512]; memset(data, 255, sizeof(data));
    init();
    assert(xmega_write(&bus, XMEGA_EEPROM, 31, data, 2) == XMEGA_OK);
    assert(commits == 2 && eeprom[30] == 0xA5 && eeprom[31] == 255 &&
           eeprom[32] == 255 && eeprom[33] == 0xA5);
    assert(xmega_write(&bus, XMEGA_EEPROM, 31, data, 2) == XMEGA_OK && commits == 2);
    init();
    assert(xmega_write(&bus, XMEGA_USERSIG, 511, data, 1) == XMEGA_OK);
    assert(commits == 1 && erases == 1 && usersig[0] == 0xA5 && usersig[511] == 255);
    init();
    assert(xmega_write(&bus, XMEGA_FLASH, 0x31E00, data, 512) == XMEGA_OK);
    assert(commits == 1 && flash[0x31DFF] == 0xA5 && flash[0x31FFF] == 255);
    init();
    data[0] = 0xF2;
    assert(xmega_write(&bus, XMEGA_FUSE, 4, data, 1) == XMEGA_OK);
    assert(fuses[4] == 0xF2 && resets == 1 && commits == 1);
    assert(xmega_write(&bus, XMEGA_FUSE, 3, data, 1) == XMEGA_INVALID);
    assert(xmega_write(&bus, XMEGA_EEPROM, 2048, data, 1) == XMEGA_INVALID);
    assert(xmega_write(&bus, XMEGA_USERSIG, 511, data, 2) == XMEGA_INVALID);
    assert(xmega_write(&bus, XMEGA_FLASH, 0xFFFFFFFF, data, 1) == XMEGA_INVALID);
    assert(xmega_write(&bus, XMEGA_EEPROM, 0, NULL, 1) == XMEGA_INVALID);
    init(); fail_read = true;
    assert(xmega_write(&bus, XMEGA_EEPROM, 0, data, 1) == XMEGA_IO && !commits);
    init(); fail_wait = true;
    assert(xmega_write(&bus, XMEGA_EEPROM, 0, data, 1) == XMEGA_IO && !commits);
    init(); corrupt = true;
    assert(xmega_write(&bus, XMEGA_USERSIG, 50, data, 1) == XMEGA_VERIFY);
    init(); fail_reset = true;
    assert(xmega_write(&bus, XMEGA_FUSE, 4, data, 1) == XMEGA_IO);
    puts("PASS: NVM page preservation, all-FF writes, bounds, failures, verification and fuse reset");
    return 0;
}
