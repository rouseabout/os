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
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    size_t size = lseek(fd, 0, SEEK_END);
    uint8_t * buf = malloc(size + 1);
    if (!buf) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    lseek(fd, 0, SEEK_SET);
    size = read(fd, buf, size);
    if (size == -1) {
        perror("read");
        return EXIT_FAILURE;
    }
    close(fd);
    cksum_init();
    printf("%u %ld %s\n", cksum(buf, size), size, argv[1]);
    return EXIT_SUCCESS;
}
