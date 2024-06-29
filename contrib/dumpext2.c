#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ext2_fs.h"

#define BLOCK_OFFSET(x) ((x) * block_size)

void hexdump(const void * ptr, unsigned int size)
{
    const uint8_t * buf = ptr;
    for (unsigned i = 0 ; i < size; i += 16) {
        printf("%5x:", i);
        for (unsigned int j = i; j < size && j < i + 16; j++) {
            printf(" %02x", buf[j]);
        }
        printf(" | ");
        for (unsigned int j = i; j < size && j < i + 16; j++) {
            printf("%c", buf[j] >= 32 && buf[j] <= 127 ? buf[j] : '.');
        }
        printf("\n");
    }
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))

void dump_block(int fd, int block, int block_size, int size)
{
    uint8_t * data = malloc(block_size);
    lseek(fd, BLOCK_OFFSET(block), SEEK_SET);
    read(fd, data, block_size);

    printf("block[%d]\n", block);
    hexdump(data, MIN(block_size, size));
    printf("\n");
}

void dump_dir(int fd, int block, int block_size, int i_size)
{
    uint8_t * data = malloc(block_size);
    lseek(fd, BLOCK_OFFSET(block), SEEK_SET);
    read(fd, data, block_size);

    // hexdump(data, block_size);

    int pos = 0;
    while (pos < i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(data + pos);
        if(entry->rec_len <= 8)
            break;

        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;

        printf("dir rec_len=%d, inode=%d, name=%s\n", entry->rec_len, entry->inode, file_name);
next:
        pos += entry->rec_len;
    }
    printf("\n");
}

int main(int argc, char ** argv)
{
    int fd;
    struct ext2_super_block s;

    if (argc != 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("open failed\n");
        return 1;
    }

    lseek(fd, 1024, SEEK_SET);
    read(fd, &s, sizeof(s));

    if (s.s_magic != EXT2_SUPER_MAGIC) {
        printf("magic fail\n");
        return 1;
    }

    printf("--- Super Block ---\n");
    printf("s_inodes_count: %d\n", s.s_inodes_count);
    printf("s_blocks_count: %d\n", s.s_blocks_count);
    printf("s_r_blocks_count: %d\n", s.s_r_blocks_count);
    printf("s_free_blocks_count: %d\n", s.s_free_blocks_count);
    printf("s_free_inodes_count: %d\n", s.s_free_inodes_count);
    printf("s_first_data_block: %d\n", s.s_first_data_block);
    int block_size = 1024 << s.s_log_block_size;
    int block_offset = block_size == 1024 ? 1024 : 0;
    printf("s_log_block_size: %d (%d bytes)\n", s.s_log_block_size, block_size);
    printf("s_log_frag_size: %d\n", s.s_log_frag_size);
    printf("s_blocks_per_group: %d\n", s.s_blocks_per_group);
    printf("s_frags_per_group: %d\n", s.s_frags_per_group);
    printf("s_inodes_per_group: %d\n", s.s_inodes_per_group);
    printf("s_mtime: %d\n", s.s_mtime);
    printf("s_wtime: %d\n", s.s_wtime);
    printf("s_mnt_count: %d\n", s.s_mnt_count);
    printf("s_max_mnt_count: %d\n", s.s_max_mnt_count);
    printf("s_magic: 0x%x\n", s.s_magic);
    printf("s_state: %d\n", s.s_state);
    printf("s_errors: %d\n", s.s_errors);
    printf("s_minor_rev_level: %d\n", s.s_minor_rev_level);
    printf("s_lastcheck: %d\n", s.s_lastcheck);
    printf("s_checkinterval: %d\n", s.s_checkinterval);
    printf("s_creator_os: %d\n", s.s_creator_os);
    printf("s_rev_level: %d\n", s.s_rev_level);
    printf("s_def_resuid: %d\n", s.s_def_resuid);
    printf("s_def_resgid: %d\n", s.s_def_resgid);
    int inode_size;
    if (s.s_rev_level == EXT2_DYNAMIC_REV) {
        printf("--- EXT2_DYNAMIC_REV ---\n");
        printf("s_first_ino: %d\n", s.s_first_ino);
        printf("s_inode_size: %d\n", s.s_inode_size);
        inode_size = s.s_inode_size;
    } else
        inode_size = sizeof(struct ext2_inode);

    printf("\n");

    int nb_groups =  1 + (s.s_blocks_count - 1) / s.s_blocks_per_group;
    printf(">> nb_groups: %d\n", nb_groups);


    for (int i = 0; i < nb_groups; i++) {
        printf("--- Group %d ---\n", i);

        if (lseek(fd, (block_size==1024 ? 1024 : 0) + BLOCK_OFFSET(1) + i*sizeof(struct ext2_group_desc), SEEK_SET) < 0) {
            printf("lseek error\n");
            break;
        }

        struct ext2_group_desc bg;
        if (read(fd, &bg, sizeof(bg)) != sizeof(bg)) {
            printf("read error\n");
            break;
        }

        printf("bg[%d].bg_block_bitmap: 0x%x\n", i, bg.bg_block_bitmap);
        printf("bg[%d].bg_inode_bitmap: 0x%x\n", i, bg.bg_inode_bitmap);
        printf("bg[%d].bg_inode_table: 0x%x\n", i, bg.bg_inode_table);
        printf("bg[%d].bg_free_blocks_count: %d\n", i, bg.bg_free_blocks_count);
        printf("bg[%d].bg_free_inodes_count: %d\n", i, bg.bg_free_inodes_count);
        printf("bg[%d].bg_used_dirs_count: %d\n", i, bg.bg_used_dirs_count);
        //bg_reserved[3];
        printf("\n");

        unsigned char * bitmap;

        bitmap = malloc(block_size);
        lseek(fd, BLOCK_OFFSET(bg.bg_block_bitmap), SEEK_SET);
        read(fd, bitmap, block_size);
        printf("block_bitmap:\n");
        //hexdump(bitmap, block_size);
#if 1
        for (int j = 0; j < s.s_blocks_per_group / 8 && j < block_size; j++) {
            for (int k = 0; k < 8; k++) {
                printf("bg[%d].block_bitmap[%d]=%d\n", i,  1 + i * s.s_blocks_per_group + j*8 + k, !!(bitmap[j] & (1 << k)));
            }
        }
#endif
        printf("\n\n");
        free(bitmap);

        bitmap = malloc(block_size);
        lseek(fd, BLOCK_OFFSET(bg.bg_inode_bitmap), SEEK_SET);
        read(fd, bitmap, block_size);
        printf("inode_bitmap:\n");
        //hexdump(bitmap, block_size);
#if 1
        for (int j = 0; j < s.s_inodes_per_group / 8 && j < block_size; j++) {
            for (int k = 0; k < 8; k++) {
                printf("bg[%d].inode_bitmap[%d]=%d\n", i, 1 + i * s.s_inodes_per_group + j*8 + k, !!(bitmap[j] & (1 << k)));
            }
        }
#endif
        printf("\n\n");
        free(bitmap);

        int inodes_per_block = block_size / sizeof(struct ext2_inode);
        int itable_blocks = s.s_inodes_per_group / inodes_per_block;

        printf("itable blocks %d\n", itable_blocks);

        for (int j = 0; j < s.s_inodes_per_group; j++) {
            struct ext2_inode inode;
            lseek(fd, BLOCK_OFFSET(bg.bg_inode_table) + j*inode_size, SEEK_SET);
            read(fd, &inode, sizeof(inode));

            printf("inode: %d mode=0x%x, blocks=0x%x (%d bytes), size=%d", i*s.s_inodes_per_group + j + 1, inode.i_mode, inode.i_blocks, inode.i_blocks * 512, inode.i_size);
            if (inode.i_mode & S_IFDIR) printf(" DIR");
            if (inode.i_mode & S_IFREG) printf(" REG");
            printf("\n");

            for (int k = 0; k < EXT2_N_BLOCKS; k++) {
                if (inode.i_block[k]) {
                    printf("    block[%d]=%d\n", k, inode.i_block[k]);
                    if (inode.i_mode & S_IFDIR)
                        dump_dir(fd, inode.i_block[k], block_size, inode.i_size);
                    else
                        dump_block(fd, inode.i_block[k], block_size, inode.i_size);
                }
            }
        }
    }

    return 0;
}
