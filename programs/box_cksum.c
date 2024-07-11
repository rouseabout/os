#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static uint32_t crctab[256];

static void cksum_init()
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t v = i << 24;
        for (int j = 0; j < 8; j++) {
            int bit = v & (1 << 31);
            v <<= 1;
            if (bit)
                v ^= 0x04C11DB7;
        }
        crctab[i] = v;
    }
}

static uint32_t cksum(uint8_t *data, size_t len)
{
    uint32_t crc = 0;
    for (int i = 0; i < len; i++)
        crc = crctab[data[i] ^ ((crc >> 24) & 0xFF)] ^ (crc << 8);
    while (len > 0) {
        crc = crctab[(len & 0xFF) ^ ((crc >> 24) & 0xFF)] ^ (crc << 8);
        len >>= 8;
    }
    return ~crc;
}

static int cksum_main(int argc, char ** argv, char ** envp)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    READ_FILE(uint8_t, buf, size, argv[1])
    cksum_init();
    printf("%u %ld %s\n", cksum(buf, size), size, argv[1]);
    free(buf);
    return EXIT_SUCCESS;
}
