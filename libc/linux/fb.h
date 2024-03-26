#ifndef LINUX_FB_H
#define LINUX_FB_H

#define FBIOGET_FSCREENINFO 1
#define FBIOGET_VSCREENINFO 2

struct fb_fix_screeninfo {
    char id[16];
    int line_length;
    int smem_len;
};

struct fb_bitfield {
    int offset;
    int length;
};

struct fb_var_screeninfo {
    int xres;
    int yres;
    int xres_virtual;
    int yres_virtual;
    int xoffset;
    int yoffset;
    int bits_per_pixel;
    int grayscale;
    struct fb_bitfield red, green, blue, transp;
};

#endif /* LINUX_FB_H */
