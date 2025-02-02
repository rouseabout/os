#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

static uint8_t * fb_addr;
static int fb_stride, fb_width, fb_height, fb_bpp;

static void draw_box(int xx, int yy, int width, int height, int color)
{
    if (fb_bpp == 1) {
        for (int j = yy; j < yy + height && j < fb_height; j++)
            for (int i = xx; i < xx + width && i < fb_width; i++)
                if (color & 1)
                    fb_addr[j * fb_stride + i/8] &= ~(1 << (7 - (i & 7)));
                else
                    fb_addr[j * fb_stride + i/8] |= 1 << (7 - (i & 7));
    } else {
        for (int j = yy; j < yy + height && j < fb_height; j++)
            for (int i = xx; i < xx + width && i < fb_width; i++)
                for (unsigned int k = 0; k < fb_bpp/8; k++)
                    fb_addr[j * fb_stride + i * fb_bpp/8 + k] = color;
    }
}

int main(int argc, char ** argv)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        perror("/dev/fb0");
        return EXIT_FAILURE;
    }

    struct fb_fix_screeninfo fixinfo;
    struct fb_var_screeninfo varinfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0)
        return EXIT_FAILURE;

    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0)
        return EXIT_FAILURE;

    fb_stride = fixinfo.line_length;
    fb_width = varinfo.xres;
    fb_height = varinfo.yres;
    fb_bpp = varinfo.bits_per_pixel;

    fb_addr = mmap(NULL, fixinfo.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb_addr == MAP_FAILED) {
        perror("mmap\n");
        return EXIT_FAILURE;
    }

    int size = 64;
    if (argc == 2)
        size = atoi(argv[1]);

    while(1)
        draw_box(random() % fb_width, random() % fb_height, random() % size , random() % size, random());

    munmap(fb_addr, fixinfo.smem_len);
    close(fd);

    return EXIT_SUCCESS;
}
