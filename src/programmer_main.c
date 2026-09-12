#include "pico/stdlib.h"
#include "pdi_programmer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool dispatch(char *line)
{
    if (!strcmp(line, "INFO")) return pdi_identify();
    if (!strcmp(line, "HELLO")) { puts("ATXMEGA192A3U-PDI/1"); return true; }
    if (!strcmp(line, "BEGIN")) return pdi_begin();
    if (!strcmp(line, "END")) return pdi_end();
    char *name = strtok(line, " ");
    char *offset_text = strtok(NULL, " ");
    char *hex = strtok(NULL, " ");
    if (!name || !offset_text || !hex || strtok(NULL, " ")) return false;
    xmega_region region;
    if (!strcmp(name, "EEPROM")) region = XMEGA_EEPROM;
    else if (!strcmp(name, "USERSIG")) region = XMEGA_USERSIG;
    else if (!strcmp(name, "FUSE")) region = XMEGA_FUSE;
    else if (!strcmp(name, "FLASH")) region = XMEGA_FLASH;
    else return false;
    /* Offsets are unsigned decimal to keep the serial protocol unambiguous. */
    if (strspn(offset_text, "0123456789") != strlen(offset_text)) return false;
    char *end;
    unsigned long offset = strtoul(offset_text, &end, 10);
    if (*end || offset > 0x32000) return false;
    size_t chars = strlen(hex);
    if (!chars || chars % 2 || chars > 1024) return false;
    uint8_t data[512];
    for (size_t i = 0; i < chars / 2; ++i) {
        int hi = nibble(hex[2*i]), lo = nibble(hex[2*i+1]);
        if (hi < 0 || lo < 0) return false;
        data[i] = (uint8_t)((hi << 4) | lo);
    }
    return pdi_program(region, (uint32_t)offset, data, chars / 2);
}

int main(void)
{
    stdio_init_all();
    static char line[1060];
    size_t used = 0;
    bool overflow = false;
    absolute_time_t deadline = make_timeout_time_ms(30000);
    while (true) {
        int c = getchar_timeout_us(100000);
        if (c == PICO_ERROR_TIMEOUT) {
            if (!stdio_usb_connected() || time_reached(deadline)) { pdi_end(); used = 0; overflow = false; }
            continue;
        }
        if (c == '\r') continue;
        if (c == '\n') {
            line[used] = 0;
            if (overflow || used) {
                bool ok = !overflow && dispatch(line);
                if (!ok) pdi_end();
                printf("%s\n", ok ? "PASS" : "FAIL");
            }
            fflush(stdout);
            used = 0;
            overflow = false;
            deadline = make_timeout_time_ms(30000);
        } else if (c < 32 || c > 126 || used == sizeof(line) - 1) {
            overflow = true;
        } else if (!overflow) line[used++] = (char)c;
    }
}
