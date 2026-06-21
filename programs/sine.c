#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SZ 2048

int main(int argc, char **argv)
{
    float freq = argc == 2 ? atof(argv[1]) : 440.0;

    int fd = open("/dev/dsp", O_WRONLY);
    if (fd == -1) {
        perror("/dev/dsp");
        return EXIT_FAILURE;
    }
    double base = 0.0;

    while (1) {
       int16_t buf[SZ];
       for (int i = 0; i < SZ; i++)
           buf[i] = sin(M_PI * (base + i) * freq / 48000) * 32767;
       base += SZ;
       write(fd, buf, sizeof(buf));
    }

    return EXIT_SUCCESS;
}
