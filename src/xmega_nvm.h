#ifndef XMEGA_NVM_H
#define XMEGA_NVM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { XMEGA_EEPROM, XMEGA_USERSIG, XMEGA_FUSE, XMEGA_FLASH } xmega_region;
typedef enum {
    XMEGA_OK, XMEGA_INVALID, XMEGA_IO, XMEGA_VERIFY
} xmega_result;

/* Operations return false on transport errors/timeouts. reset must release
 * reset, re-enter PDI programming and check the device identity again. */
typedef struct {
    bool (*read)(uint32_t address, uint8_t *value);
    void (*write)(uint32_t address, uint8_t value);
    bool (*wait)(void);
    bool (*reset)(void);
} xmega_bus;

bool xmega_valid_request(xmega_region region, uint32_t offset,
                         const uint8_t *data, size_t length);
xmega_result xmega_write(const xmega_bus *bus, xmega_region region,
                         uint32_t offset, const uint8_t *data, size_t length);
#endif
