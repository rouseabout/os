#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t boot;
    uint8_t starting_head;
    uint16_t starting_cylinder;
    uint8_t system_id;
    uint8_t ending_head;
    uint16_t ending_cylinder;
    uint32_t relative_sector;
    uint32_t total_sectors;
} GPT;

int main()
{
    int fd = open("disk_image", O_RDONLY);
    if (fd < 0)
        return -1;

    uint8_t buf[512];
    read(fd, buf, sizeof(buf));
    close(fd);

    GPT * g = (GPT *)(buf + 0x1BE);
    printf("boot: 0x%x, 0x%x, 0x%x\n", g->boot, g->relative_sector * 512, g->total_sectors * 512);

    return 0;
}
