#include "xmega_nvm.h"
#include <string.h>

#define CMD 0x010001CAu
#define CTRLA 0x010001CBu
#define EEPROM_BASE 0x008C0000u
#define USERSIG_BASE 0x008E0400u
#define FUSE_BASE 0x008F0020u

static const uint8_t fuse_mask[6] = {0xFF, 0xFF, 0x63, 0, 0x1F, 0x3F};

bool xmega_valid_request(xmega_region region, uint32_t offset,
                         const uint8_t *data, size_t length)
{
    if (!data || !length) return false;
    if (region == XMEGA_FUSE) {
        if (offset >= 6 || offset == 3 || length != 1) return false;
        /* Reserved bits must be supplied as one. Reject, never silently alter. */
        if ((uint8_t)(data[0] | fuse_mask[offset]) != 0xFF) return false;
        if (offset == 2 && (data[0] & 3) == 0) return false;
        if (offset == 4 && (data[0] & 0x0C) == 0x08) return false;
        if (offset == 5 && (data[0] & 0x30) == 0) return false;
        return true;
    }
    uint32_t size = region == XMEGA_EEPROM ? 2048 :
                    region == XMEGA_USERSIG ? 512 :
                    region == XMEGA_FLASH ? 0x32000 : 0;
    return offset < size && length <= size - offset;
}

static bool read_bytes(const xmega_bus *bus, uint32_t addr, uint8_t *out, size_t n)
{
    if (!bus->wait()) return false;
    bus->write(CMD, 0x43); /* READ_NVM */
    for (size_t i = 0; i < n; ++i)
        if (!bus->read(addr + (uint32_t)i, out + i)) return false;
    return true;
}

static bool command(const xmega_bus *bus, uint8_t cmd, uint32_t trigger, uint8_t value)
{
    if (!bus->wait()) return false;
    bus->write(CMD, cmd);
    bus->write(trigger, value);
    return bus->wait();
}

xmega_result xmega_write(const xmega_bus *bus, xmega_region region,
                         uint32_t offset, const uint8_t *data, size_t length)
{
    if (!bus || !bus->read || !bus->write || !bus->wait || !bus->reset ||
        !xmega_valid_request(region, offset, data, length)) return XMEGA_INVALID;

    xmega_result result = XMEGA_IO;
    if (region == XMEGA_FUSE) {
        uint8_t current;
        if (!read_bytes(bus, FUSE_BASE + offset, &current, 1)) goto done;
        if (((current ^ data[0]) & fuse_mask[offset]) != 0) {
            if (!command(bus, 0x4C, FUSE_BASE + offset, data[0])) goto done;
            bus->write(CMD, 0);
            /* Several FUSEBYTE4 bits only read correctly after reset. */
            if (!bus->reset()) return XMEGA_IO;
            if (!read_bytes(bus, FUSE_BASE + offset, &current, 1)) goto done;
        }
        result = ((current ^ data[0]) & fuse_mask[offset]) ? XMEGA_VERIFY : XMEGA_OK;
        goto done;
    }

    uint32_t base = region == XMEGA_EEPROM ? EEPROM_BASE :
                    region == XMEGA_FLASH ? 0x00800000u : USERSIG_BASE;
    uint32_t page_size = region == XMEGA_EEPROM ? 32 : 512;
    uint8_t page[512], verify[512];
    while (length) {
        uint32_t start = offset - offset % page_size;
        uint32_t within = offset % page_size;
        size_t count = page_size - within;
        if (count > length) count = length;
        if (!read_bytes(bus, base + start, page, page_size)) goto done;
        if (memcmp(page + within, data, count) != 0) {
            memcpy(page + within, data, count);
            if (!command(bus, region == XMEGA_EEPROM ? 0x36 : 0x26, CTRLA, 1)) goto done;
            if (region == XMEGA_USERSIG &&
                !command(bus, 0x18, USERSIG_BASE, 0x55)) goto done;
            bus->write(CMD, region == XMEGA_EEPROM ? 0x33 : 0x23);
            /* Ascending bytes preserve the low-byte/high-byte flash buffer order. */
            for (uint32_t i = 0; i < page_size; ++i)
                bus->write(base + start + i, page[i]);
            if (!command(bus, region == XMEGA_EEPROM ? 0x35 :
                              region == XMEGA_FLASH ? 0x2F : 0x1A,
                         base + start, 0x55)) goto done;
            if (!read_bytes(bus, base + start, verify, page_size)) goto done;
            if (memcmp(page, verify, page_size)) {
                result = XMEGA_VERIFY;
                goto done;
            }
        }
        offset += (uint32_t)count;
        data += count;
        length -= count;
    }
    result = XMEGA_OK;
done:
    bus->write(CMD, 0);
    return result;
}
