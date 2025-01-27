#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/fb.h>

static uint8_t * fb_addr;
static int fb_stride, fb_width, fb_height, fb_bpp;

static void xor_box(int xx, int yy, int width, int height)
{
    if (fb_bpp == 1) {
        for (int j = yy; j < yy + height && j < fb_height; j++)
            for (int i = xx; i < xx + width && i < fb_width; i++)
                fb_addr[j * fb_stride + i/8] ^= ~(1 << (7 - (i & 7)));
    } else {
        for (int j = yy; j < yy + height && j < fb_height; j++)
            for (int i = xx; i < xx + width && i < fb_width; i++)
                for (unsigned int k = 0; k < fb_bpp/8; k++)
                    fb_addr[j * fb_stride + i * fb_bpp/8 + k] ^= 0xFF;
    }
}

int main(int argc, char ** argv)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd == -1) {
        perror("/dev/fb0");
        return EXIT_FAILURE;
    }

    struct fb_fix_screeninfo fixinfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0)
        return EXIT_FAILURE;

    struct fb_var_screeninfo varinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0)
        return EXIT_FAILURE;

    fb_stride = fixinfo.line_length;
    fb_width = varinfo.xres;
    fb_height = varinfo.yres;
    fb_bpp = varinfo.bits_per_pixel;

    fb_addr = mmap(NULL, fixinfo.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
    if (fb_addr == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }

    int mouse_fd  = open("/dev/mouse0", O_RDONLY);
    if (mouse_fd == -1) {
        perror("/dev/mouse0");
        return EXIT_FAILURE;
    }

    int x = fb_width / 2;
    int y = fb_height / 2;
    int buttons = 0;
    xor_box(x, y, 8, 8);

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(mouse_fd, &rfds);

#define MIN(a, b) ((a)<(b)?(a):(b))
#define MAX(a, b) ((a)>(b)?(a):(b))
        int n = select(mouse_fd + 1, &rfds, NULL, NULL, NULL);
        if (n < 0) {
            perror("select");
            return EXIT_FAILURE;
        }

        if (FD_ISSET(mouse_fd, &rfds)) {
            char mouse[3];
            if (read(mouse_fd, mouse, 3) == 3) {
                xor_box(x, y, 8, 8);
                int b = mouse[0] & 7;
                int dx = mouse[1];
                int dy = mouse[2];
                x = MIN(MAX(x + dx, 0), fb_width - 1);
                y = MIN(MAX(y - dy, 0), fb_height - 1);
                if (buttons ^ b) {
                    fprintf(stdout, "[%d%d%d]", !!(b & 1), !!(b & 2), !!(b & 4));
                    fflush(stdout);
                }
                buttons = b & 7;
                xor_box(x, y, 8, 8);
            }
        }
    }

    munmap(fb_addr, fixinfo.smem_len);
    close(fd);
    close(mouse_fd);

    return EXIT_SUCCESS;
}
