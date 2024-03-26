#include <errno.h>
#include <string.h>
#include <bsd/string.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"
#include "ext2.h"
#include "ext2_fs.h"
#include "libgen.h"
#include <limits.h>

#define EXT2_S_IFMT     0xF000  /* format mask */
#define EXT2_S_IFSOCK	0xC000	/* socket */
#define EXT2_S_IFLNK	0xA000	/* symbolic link */
#define EXT2_S_IFREG	0x8000	/* regular file */
#define EXT2_S_IFBLK	0x6000	/* block device */
#define EXT2_S_IFDIR	0x4000	/* directory */
#define EXT2_S_IFCHR	0x2000	/* character device */
#define EXT2_S_IFIFO	0x1000	/* fifo */

#define EXT2_S_ISUID	0x0800	/* Set process User ID */
#define EXT2_S_ISGID	0x0400	/* Set process Group ID */
#define EXT2_S_ISVTX	0x0200	/* sticky bit */

#define EXT2_S_IRUSR	0x0100	/* user read */
#define EXT2_S_IWUSR	0x0080	/* user write */
#define EXT2_S_IXUSR	0x0040	/* user execute */
#define EXT2_S_IRGRP	0x0020	/* group read */
#define EXT2_S_IWGRP	0x0010	/* group write */
#define EXT2_S_IXGRP	0x0008	/* group execute */
#define EXT2_S_IROTH	0x0004	/* others read */
#define EXT2_S_IWOTH	0x0002	/* others write */
#define EXT2_S_IXOTH	0x0001  /* others execute */

#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLINK 7

typedef struct {
    FileDescriptor *fd;

    struct ext2_super_block super;
    int block_size;
    int nb_groups;
    int inode_size;

    struct ext2_group_desc * bg;
    char * block_bitmap;
    char * inode_bitmap;
} Ext2Context;

#define BLOCK_OFFSET(x) ((x) * s->block_size)

void * ext2_init(const char * path)
{
    Ext2Context * s = kmalloc(sizeof(Ext2Context), "ext2-cntx");
    if (!s)
        return NULL;

    s->fd = vfs_open(path, O_RDONLY, 0);
    if (!s->fd) {
        kprintf("ext2: vfs_open %s failed\n", path);
        kfree(s);
        return NULL;
    }

    vfs_lseek(s->fd, 1024, SEEK_SET);
    vfs_read(s->fd, &s->super, sizeof(struct ext2_super_block));

    if (s->super.s_magic != EXT2_SUPER_MAGIC)
       panic("ext2: !EXT2_SUPER_MAGIC");

    s->block_size = 1024 << s->super.s_log_block_size;
    kprintf("ext2: block_size=%d\n", s->block_size);

    s->nb_groups = 1 + (s->super.s_blocks_count - 1) / s->super.s_blocks_per_group;
    kprintf("ext2: nb_groups: %d\n", s->nb_groups);

    if (s->super.s_rev_level == EXT2_DYNAMIC_REV)
        s->inode_size = s->super.s_inode_size;
    else
        s->inode_size = sizeof(struct ext2_inode);

    /* block group(s) */

    s->bg = kmalloc(s->nb_groups * sizeof(struct ext2_group_desc), "ext2-block-bg");
    if (!s->bg)
        return NULL;
    vfs_lseek(s->fd, (s->block_size == 1024 ? 1024 : 0) + BLOCK_OFFSET(1), SEEK_SET);
    vfs_read(s->fd, s->bg, s->nb_groups * sizeof(struct ext2_group_desc));

    /* bitmaps */

    s->block_bitmap = kmalloc(s->nb_groups * s->block_size, "ext2-block-bm");
    if (!s->block_bitmap)
        return NULL;

    s->inode_bitmap = kmalloc(s->nb_groups * s->block_size, "ext2-inode-bm");
    if (!s->inode_bitmap)
        return NULL;

    for (int i = 0; i < s->nb_groups; i++) {
        vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[i].bg_block_bitmap), SEEK_SET);
        vfs_read(s->fd, s->block_bitmap + i * s->block_size, s->block_size);

        vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[i].bg_inode_bitmap), SEEK_SET);
        vfs_read(s->fd, s->inode_bitmap + i * s->block_size, s->block_size);
    }

    return s;
}

static int read_inode(Ext2Context * s, int inode, struct ext2_inode * i)
{
    int bg_number = (inode - 1) / s->super.s_inodes_per_group;
    int rel_inode = (inode - 1) % s->super.s_inodes_per_group;

    if (vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[bg_number].bg_inode_table) + rel_inode * s->inode_size, SEEK_SET) < 0)
        return -EIO;

    if (vfs_read(s->fd, i, sizeof(struct ext2_inode)) != sizeof(struct ext2_inode))
        return -EIO;

    return 0;
}

static int write_inode(Ext2Context * s, int inode, const struct ext2_inode * i)
{
    int bg_number = (inode - 1) / s->super.s_inodes_per_group;
    int rel_inode = (inode - 1) % s->super.s_inodes_per_group;

    if (vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[bg_number].bg_inode_table) + rel_inode * s->inode_size, SEEK_SET) < 0)
        return -EIO;

    if (vfs_write(s->fd, i, sizeof(struct ext2_inode)) != sizeof(struct ext2_inode))
        return -EIO;

    return 0;
}

#if 0
static void dump_inode(int inode, const struct ext2_inode * i)
{
    kprintf("inode: %d mode=0x%x, blocks=0x%x (%d bytes), size=%d",
        inode, i->i_mode, i->i_blocks, i->i_blocks * 512, i->i_size);
    if (i->i_mode & EXT2_S_IFDIR) kprintf(" DIR");
    if (i->i_mode & EXT2_S_IFREG) kprintf(" REG");
    kprintf("\n");
    for (int j = 0; j < EXT2_IND_BLOCK; j++) {
        kprintf("block[%d] = %d\n", j, i->i_block[j]);
    }
    kprintf("\n");
}
#endif

static void read_dir_block(Ext2Context * s, int dir_block, uint8_t * data, int * pos, int * size)
{
    vfs_lseek(s->fd, BLOCK_OFFSET(dir_block), SEEK_SET);
    int sz = MIN(*size, s->block_size);
    vfs_read(s->fd, data + *pos, sz);
    *pos += sz;
    *size -= sz;
}

static void read_ind_block(Ext2Context * s, int ind_block, uint8_t * data, int * pos, int * size)
{
    uint32_t * tab = kmalloc(s->block_size, "ext2-ind-tab");
    if (!tab)
        return; //FIXME: error

    vfs_lseek(s->fd, BLOCK_OFFSET(ind_block), SEEK_SET);
    vfs_read(s->fd, tab, s->block_size);

    for (int k = 0; k < s->block_size / sizeof(uint32_t) && tab[k] && *size > 0; k++)
        read_dir_block(s, tab[k], data, pos, size);

    kfree(tab);
}

static void read_dind_block(Ext2Context * s, int dind_block, uint8_t * data, int * pos, int * size)
{
    uint32_t * tab = kmalloc(s->block_size, "ext2-dind-tab");
    if (!tab)
        return; //FIXME: error

    vfs_lseek(s->fd, BLOCK_OFFSET(dind_block), SEEK_SET);
    vfs_read(s->fd, tab, s->block_size);

    for (int k = 0; k < s->block_size / sizeof(uint32_t) && tab[k] && *size > 0; k++)
        read_ind_block(s, tab[k], data, pos, size);

    kfree(tab);
}

static void read_inode_data(Ext2Context *s, int inode, const struct ext2_inode * i, void *data_, int size)
{
    KASSERT(i->i_size == size);

    uint8_t * data = data_;
    int pos = 0;

    for (int j = 0; j < EXT2_IND_BLOCK && i->i_block[j] && size > 0; j++)
        read_dir_block(s, i->i_block[j], data, &pos, &size);

    if (i->i_block[EXT2_IND_BLOCK])
        read_ind_block(s, i->i_block[EXT2_IND_BLOCK], data, &pos, &size);

    if (i->i_block[EXT2_DIND_BLOCK])
        read_dind_block(s, i->i_block[EXT2_DIND_BLOCK], data, &pos, &size);

    KASSERT(i->i_block[EXT2_TIND_BLOCK] == 0);
}

//FIXME: this works because we are not varying size of the file
static void write_inode_data(Ext2Context * s, int inode, const struct ext2_inode * i, void * data_, int size)
{
    KASSERT(i->i_size == size);
    KASSERT((i->i_mode & EXT2_S_IFMT) != EXT2_S_IFLNK);
    uint8_t * data = data_;
    int pos = 0;

    for (int j = 0; j < EXT2_IND_BLOCK && i->i_block[j] && size > 0; j++) {
        vfs_lseek(s->fd, BLOCK_OFFSET(i->i_block[j]), SEEK_SET);
        int sz = MIN(size, s->block_size);
        vfs_write(s->fd, data + pos, sz);
        pos += sz;
        size -= sz;
    }

    if (i->i_block[EXT2_IND_BLOCK]) {
        uint32_t * tab = kmalloc(s->block_size, "ext2-write-tmp");
        if (!tab)
            return; //FIXME: error
        vfs_lseek(s->fd, BLOCK_OFFSET(i->i_block[EXT2_IND_BLOCK]), SEEK_SET);
        vfs_read(s->fd, tab, s->block_size);
        for (int k = 0; k < s->block_size / sizeof(uint32_t) && tab[k] && size > 0; k++) {
            //kprintf("\tinode ind data block[%d] = %d\n", k, tab[k]);
            vfs_lseek(s->fd, BLOCK_OFFSET(tab[k]), SEEK_SET);
            int sz = MIN(size, s->block_size);
            vfs_write(s->fd, data + pos, sz);
            pos += sz;
            size -= sz;
        }
        kfree(tab);
    }

    KASSERT(i->i_block[EXT2_DIND_BLOCK] == 0);
    KASSERT(i->i_block[EXT2_TIND_BLOCK] == 0);
}

#define PAD(x) (((x) + 3) & ~3)

static int append_dir(Ext2Context * s, int dinode, const char * new_name, int new_inode, int is_directory)
{
    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, dinode, &i)) < 0)
        return ret;

    KASSERT((i.i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR);

    uint8_t * dir = kmalloc(i.i_size, "ext2-dir-tmp");
    if (!dir)
        return -ENOMEM;
    read_inode_data(s, dinode, &i, dir, i.i_size);

    int pos = 0;
    while (pos < i.i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(dir + pos);

        if (entry->rec_len < 8)
            break;

#if 0
        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        kprintf("  entry len %d, name len %d, inode %d, name='%s'\n", entry->rec_len, entry->name_len, entry->inode, file_name);
#endif

        int existing_size = 8 + PAD(entry->name_len);
        int slack = entry->rec_len - existing_size;

        if (slack >= 8 + PAD(strlen(new_name))) {
            entry->rec_len = existing_size;

            struct ext2_dir_entry_2 * entry2 = (struct ext2_dir_entry_2 *)(dir + pos + existing_size);
            entry2->rec_len = slack;
            entry2->name_len = strlen(new_name);
            entry2->inode = new_inode;
            entry2->file_type = is_directory ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
            memset(entry2->name, 0, entry->name_len);
            memcpy(entry2->name, new_name, strlen(new_name));

            write_inode_data(s, dinode, &i, dir, i.i_size);
            kfree(dir);
            return 0;
        }

        pos += entry->rec_len;
    }

    kfree(dir);
    return -ENOSPC;
}

// for given directory, return inode of the target filename
static int find_inode_dir(Ext2Context * s, int dinode, const char * target, int delete)
{
    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, dinode, &i)) < 0)
        return ret;

    //dump_inode(dinode, &i);
    if ((i.i_mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
        return -EINVAL; /* not a directory */

    uint8_t * dir = kmalloc(i.i_size, "ext2-dir-tmp");
    if (!dir)
        return -ENOMEM;
    read_inode_data(s, dinode, &i, dir, i.i_size);

    int pos = 0;
    while (pos < i.i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(dir + pos);

        if(entry->rec_len < 8)
            break;

        if (entry->inode == 0) {
            pos += entry->rec_len;
            continue;
        }

        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;

        //kprintf("dir inode=%d, name=%s\n", entry->inode, file_name);

        if (!strcmp(file_name, target)) {
            int inode = entry->inode; //preserve because we may blow it away

            if (delete) {
                entry->inode = 0;
                memset(entry->name, 0, entry->name_len);
                write_inode_data(s, dinode, &i, dir, i.i_size);
            }

            kfree(dir);
            return inode;
        }

        pos += entry->rec_len;
    }

    kfree(dir);

    return -ENOENT;
}

static int allocate_inode(Ext2Context * s)
{
    for (int g = 0; g < s->nb_groups; g++) {
    for (int i = 0; i < s->super.s_inodes_per_group / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (!(s->inode_bitmap[g * s->block_size + i] & (1 << j))) {
                s->inode_bitmap[g * s->block_size + i] |= 1 << j;
                s->super.s_free_inodes_count--;
                s->bg[g].bg_free_inodes_count--;
                return 1 + g * s->super.s_inodes_per_group + i * 8 + j;
            }
        }
    }
    }
    return 0;
}

static void flush(Ext2Context * s)
{
    vfs_lseek(s->fd, 1024, SEEK_SET);
    vfs_write(s->fd, &s->super, sizeof(s->super));

    vfs_lseek(s->fd, (s->block_size == 1024 ? 1024 : 0) + BLOCK_OFFSET(1), SEEK_SET);
    vfs_write(s->fd, s->bg, s->nb_groups * sizeof(struct ext2_group_desc));

    for (int i = 0; i < s->nb_groups; i++) {
        vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[i].bg_block_bitmap), SEEK_SET);
        vfs_write(s->fd, s->block_bitmap + i * s->block_size, s->block_size);

        vfs_lseek(s->fd, BLOCK_OFFSET(s->bg[i].bg_inode_bitmap), SEEK_SET);
        vfs_write(s->fd, s->inode_bitmap + i * s->block_size, s->block_size);
    }
}

static int allocate_block(Ext2Context * s)
{
    for (int g = 0; g < s->nb_groups; g++) {
    for (int i = 0; i < s->super.s_blocks_per_group / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (!(s->block_bitmap[g * s->block_size + i] & (1 << j))) {
                s->block_bitmap[g * s->block_size + i] |= 1 << j;
                s->super.s_free_blocks_count--;
                s->bg[g].bg_free_blocks_count--;
                return 1 + g * s->super.s_blocks_per_group +  i * 8 + j;
            }
        }
    }
    }
    return 0;
}

static void allocate_blocks(Ext2Context * s, struct ext2_inode * i, size_t size)
{
    size_t pos = 0;
    for (int j = 0; j < EXT2_IND_BLOCK && pos < size; j++) {
        if (!i->i_block[j]) {
            i->i_block[j] = allocate_block(s);
            if (!i->i_block[j])
                return; //FIXME: error
            kprintf(" allocated block %d\n", i->i_block[j]);
        }
        pos += s->block_size;
    }

    if (pos < size) {
        uint32_t * tab = kmalloc(s->block_size, "ext2-alloc-blks");
        if (!tab)
            return; //FIXME: error

        if (i->i_block[EXT2_IND_BLOCK]) {
            vfs_lseek(s->fd, BLOCK_OFFSET(i->i_block[EXT2_IND_BLOCK]), SEEK_SET);
            vfs_read(s->fd, tab, s->block_size);
        } else {
            i->i_block[EXT2_IND_BLOCK] = allocate_block(s);
            if (!i->i_block[EXT2_IND_BLOCK])
                return; //FIXME: error
            kprintf(" allocated indirect block %d\n", i->i_block[EXT2_IND_BLOCK]);
            memset(tab, 0, s->block_size);
        }

        for (int j = 0; j < s->block_size / 4 && pos < size; j++) {
            if (!tab[j]) {
                tab[j] = allocate_block(s);
                if (!tab[j])
                    return; //FIXME: error
                kprintf(" allocated level2 block %d\n", tab[j]);
            }
            pos += s->block_size;
        }

        vfs_lseek(s->fd, BLOCK_OFFSET(i->i_block[EXT2_IND_BLOCK]), SEEK_SET);
        vfs_write(s->fd, tab, s->block_size);
        kfree(tab);
    }

    KASSERT(pos >= size);

    i->i_size = size;
    i->i_blocks = (size + 511) / 512;
}

static void ext2_close(FileDescriptor * fd)
{
    Ext2Context * s = fd->priv_data;
    //kprintf("ext2[%p]: close inode=%d\n", s, fd->inode);
    if (fd->buf) {
        if (fd->dirty) {
            struct ext2_inode i;
            if (read_inode(s, fd->inode, &i) < 0)
                return;

            allocate_blocks(s, &i, fd->buf_size);

            write_inode_data(s, fd->inode, &i, fd->buf, fd->buf_size);

            i.i_mtime = tnow.tv_sec;

            if (write_inode(s, fd->inode, &i) < 0)
                return;

            flush(s);
        }
        kfree(fd->buf);
        fd->buf = NULL;
    }
}

static int ext2_write(FileDescriptor * fd, const void * buf, int size)
{
    if (!fd->buf || fd->pos + size > fd->buf_size) {
        char * tmp = krealloc(fd->buf, fd->pos + size);
        if (!tmp)
            return -ENOMEM;
        fd->buf = tmp;
        fd->buf_size = fd->pos + size;
    }
    memcpy(fd->buf + fd->pos, buf, size);
    fd->pos += size;
    fd->dirty = 1;
    return size;
}

static int ext2_read(FileDescriptor * fd, void * buf, int size)
{
    Ext2Context * s = fd->priv_data;
    // kprintf("ext2[%p] read inode=%d, fd->pos=%d, size=%d\n", s, fd->inode, fd->pos, size);
    if (!fd->buf) {
        int ret;
        struct ext2_inode i;
        if ((ret = read_inode(s, fd->inode, &i)) < 0)
            return ret;

        if (!i.i_size)
            return 0;

        //FIXME: improve read_inode_data so we don't need to read in the whole file for first read()
        fd->buf = kmalloc(i.i_size, "ext2-read-tmp");
        if (!fd->buf)
            return -ENOMEM;

        read_inode_data(s, fd->inode, &i, fd->buf, i.i_size);

        fd->buf_size = i.i_size;
    }

    int sz = MIN(size, fd->buf_size - fd->pos);
    memcpy(buf, fd->buf + fd->pos, sz);
    fd->pos += sz;
    return sz;
}

static off_t ext2_lseek(FileDescriptor * fd, off_t offset, int whence)
{
    Ext2Context * s = fd->priv_data;

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, fd->inode, &i)) < 0)
        return ret;

    // kprintf("ext2[%p] seek %d/%d, %d\n", s, offset, i.i_size, whence);
    if (fd->flags & (O_WRONLY|O_RDWR)) {
        if (!fd->buf && i.i_size > 0) {
            fd->buf = kmalloc(i.i_size, "ext2-fd-cache");
            if (!fd->buf)
                return -ENOMEM;
            read_inode_data(s, fd->inode, &i, fd->buf, i.i_size);
            fd->buf_size = i.i_size;
        }
        if (whence == SEEK_END)
            fd->pos = MAX(0, fd->buf_size + offset);
        else if (whence == SEEK_CUR)
            fd->pos = MAX(0, fd->pos + offset);
        else if (whence == SEEK_SET)
            fd->pos = MAX(0, offset);
        else
            return -EINVAL;

        return fd->pos;
    }

    if (whence == SEEK_END)
        fd->pos = i.i_size;
    else if (whence == SEEK_CUR)
        fd->pos = MIN(i.i_size, MAX(0, fd->pos + offset));
    else if (whence == SEEK_SET)
        fd->pos = MIN(i.i_size, MAX(0, offset));
    else
        return -EINVAL;

    return fd->pos;
}

static int ext2_getdents(FileDescriptor * fd, struct dirent * dent, size_t count)
{
    Ext2Context * s = fd->priv_data;

    //kprintf("ext2[%p] getdents %d\n", s, count);

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, fd->inode, &i)) < 0)
        return ret;

    //FIXME: improve read_inode_data so we don't need to read in the whole file for each getdents()
    uint8_t * data = kmalloc(i.i_size, "ext2-dents-tmp");
    if (!data)
        return -ENOMEM;

    read_inode_data(s, fd->inode, &i, data, i.i_size);

    int pos = 0;
    int index = 0;
    int nread = 0;
    while (pos < i.i_size && nread < count) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(data + pos);

        if (entry->rec_len < 8)
            break;

        if (entry->inode == 0) {
            pos += entry->rec_len;
            continue;
        }

        if (index < fd->pos)
            goto next;

        KASSERT(sizeof(dent[nread].d_name) > sizeof(entry->name));

        memcpy(dent[nread].d_name, entry->name, entry->name_len);
        dent[nread].d_name[entry->name_len] = 0;
        dent[nread].d_ino = entry->inode;

        nread++;
next:
        pos += entry->rec_len;
        index++;
    }

    kfree(data);

    fd->pos += nread;
    return nread;
}

static int count_files_in_dir(Ext2Context * s, int inode, const struct ext2_inode *i)
{
    uint8_t * data = kmalloc(i->i_size, "ext2-count-tmp");
    if (!data)
        return -ENOMEM;

    read_inode_data(s, inode, i, data, i->i_size);

    int count = 0;
    int pos = 0;
    while (pos < i->i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(data + pos);

        if (entry->rec_len < 8)
            break;

        if (entry->inode == 0)
            goto next;

        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;

        if (!strcmp(file_name, ".") || !strcmp(file_name, ".."))
            goto next;

        count++;

next:
        pos += entry->rec_len;
    }

    kfree(data);

    return count;
}

static int inode_stat(Ext2Context * s, int inode, struct stat * st)
{
    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;

    st->st_dev = 0;
    st->st_ino = inode;
    st->st_mode = i.i_mode;
    st->st_nlink = i.i_links_count;
    st->st_uid = i.i_uid;
    st->st_gid = i.i_gid;
    st->st_rdev = 0;
    st->st_size = i.i_size;
    st->st_atim = (struct timespec){.tv_sec=i.i_atime};
    st->st_mtim = (struct timespec){.tv_sec=i.i_mtime};
    st->st_ctim = (struct timespec){.tv_sec=i.i_ctime};
    st->st_blksize = s->block_size;
    st->st_blocks = i.i_blocks;
    return 0;
}

static int ext2_inode_utime(void * priv_data, int inode, const struct utimbuf * times)
{
    Ext2Context * s = priv_data;
    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;

    i.i_atime = times ? times->actime : tnow.tv_sec;
    i.i_mtime = times ? times->modtime : tnow.tv_sec;
    if ((ret = write_inode(s, inode, &i)) < 0)
        return ret;

    return 0;
}

static void clear_block(Ext2Context * s, int block)
{
    block--;
    int idx = block / 8;
    int bit = block % 8;
    s->block_bitmap[idx] &= ~(1 << bit);
    s->super.s_free_blocks_count ++;
    s->bg[block / s->super.s_blocks_per_group].bg_free_blocks_count++;
}

static void clear_inode(Ext2Context * s, int inode)
{
    inode--;
    int idx = inode / 8;
    int bit = inode % 8;
    s->inode_bitmap[idx] &= ~(1 << bit);
    s->super.s_free_inodes_count++;
    s->bg[inode / s->super.s_inodes_per_group].bg_free_inodes_count++;
}

static int clear_blocks(Ext2Context * s, struct ext2_inode * i)
{
    for (int j = 0; j < EXT2_IND_BLOCK && i->i_block[j]; j++) {
        clear_block(s, i->i_block[j]);
        i->i_block[j] = 0;
    }

    if (i->i_block[EXT2_IND_BLOCK]) {
        uint32_t * tab = kmalloc(s->block_size, "ext2-tab-tmp");
        if (!tab)
            return -ENOMEM;
        vfs_lseek(s->fd, BLOCK_OFFSET(i->i_block[EXT2_IND_BLOCK]), SEEK_SET);
        vfs_read(s->fd, tab, s->block_size);
        for (int j = 0; j < s->block_size / sizeof(uint32_t) && tab[j]; j++)
            clear_block(s, tab[j]);
        kfree(tab);
        clear_block(s, i->i_block[EXT2_IND_BLOCK]);
        i->i_block[EXT2_IND_BLOCK] = 0;
    }

    KASSERT(i->i_block[EXT2_DIND_BLOCK] == 0);
    KASSERT(i->i_block[EXT2_TIND_BLOCK] == 0);
    return 0;
}

static void populate_directory(uint8_t * buf, int size, int dinode, int ddinode)
{
    const char * dot = ".";
    struct ext2_dir_entry_2 * d = (struct ext2_dir_entry_2 *)buf;
    d->rec_len = 8 + PAD(strlen(dot));
    d->inode = dinode;
    d->name_len = strlen(dot);
    memset(d->name, 0, PAD(strlen(dot)));
    memcpy(d->name, dot, strlen(dot));

    const char * dotdot = "..";
    struct ext2_dir_entry_2 * e = (struct ext2_dir_entry_2 *)(buf + d->rec_len);
    e->rec_len = size - d->rec_len; /* remainder */
    e->inode = ddinode;
    e->name_len = strlen(dotdot);
    memset(e->name, 0, PAD(strlen(dotdot)));
    memcpy(e->name, dotdot, strlen(dotdot));
}

static int ext2_truncate(FileDescriptor * fd, off_t length)
{
    Ext2Context * s = fd->priv_data;

    if (!(fd->flags & (O_WRONLY|O_RDWR)))
        return -EINVAL;

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, fd->inode, &i)) < 0)
        return ret;

    //read file
    if (!fd->buf && i.i_size) {
        fd->buf = kmalloc(i.i_size, "ext2-read-tmp");
        if (!fd->buf)
            return -ENOMEM;

        read_inode_data(s, fd->inode, &i, fd->buf, i.i_size);

        fd->buf_size = i.i_size;
    }

    //realloc buffer
    char * tmp = krealloc(fd->buf, length);
    if (!tmp && length)
        return -ENOMEM;
    if (length > fd->buf_size)
        memset(tmp + fd->buf_size, 0, length - fd->buf_size);
    fd->buf = tmp;
    fd->buf_size = length;
    fd->dirty = 1;

    //clear blocks
    if ((ret = clear_blocks(s, &i)) < 0)
        return ret;

    //update size on disk
    i.i_size = 0;
    write_inode(s, fd->inode, &i);

    return 0;
}

static int ext2_inode_range(void * priv_data, int * min_inode, int * max_inode)
{
    Ext2Context * s = priv_data;
    *min_inode = 2;
    *max_inode = s->nb_groups * s->super.s_inodes_per_group;
    return 0;
}

static int ext2_inode_resolve(void * priv_data, int dinode, const char * target)
{
    Ext2Context * s = priv_data;
    return find_inode_dir(s, dinode, target, 0 /* delete */);
}

static int ext2_inode_symlink(void * priv_data, int inode, char * buf, size_t size)
{
    Ext2Context * s = priv_data;
    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;
    if ((i.i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK) {
        if (i.i_size > 60)
            panic("ext2: symlink size > 60\n");
        if (i.i_size + 1 > size)
            i.i_size = size - 1;
        memcpy(buf, i.i_block, i.i_size);
        buf[i.i_size] = 0;
        return i.i_size;
    }
    return -EINVAL;
}

static int ext2_inode_creat(void * priv_data, int dinode, const char * base, int mode, const char * symbolic_link)
{
    Ext2Context * s = priv_data;

    if ((mode & S_IFMT) == S_IFLNK && strlen(symbolic_link) > 60) //FIXME: support > 60 char symbolic links
        return -EINVAL;

    int inode = allocate_inode(s);
    if (inode == 0)
        return -ENOSPC;

    kprintf("allocate inode %d\n", inode);

    /* update inode entry */
    int ret;
    struct ext2_inode i;
    memset(&i, 0, sizeof(i));
    i.i_mode = mode;
    i.i_links_count = 1;
    i.i_atime = i.i_mtime = i.i_ctime = tnow.tv_sec;
    i.i_uid = 1;
    i.i_gid = 1;
    if ((mode & S_IFMT) == S_IFLNK) {
        i.i_size = strlen(symbolic_link);
        memcpy(i.i_block, symbolic_link, i.i_size);
    }
    if ((ret = write_inode(s, inode, &i)) < 0)
        return ret;

    /* update directory entry */
    append_dir(s, dinode, base, inode, (mode & S_IFMT) == S_IFDIR);

    flush(s);

    return inode;
}

static int ext2_open2(FileDescriptor * fd)
{
    fd->dirty = 0;
    fd->buf = NULL;
    fd->buf_size = 0;
    return 0;
}

static int ext2_inode_stat(void * priv_data, int inode, struct stat * st)
{
    Ext2Context * s = priv_data;
    return inode_stat(s, inode, st);
}

static int ext2_inode_populate_dir(void * priv_data, int dinode, int inode)
{
    Ext2Context * s = priv_data;

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;

    allocate_blocks(s, &i, s->block_size); // 1 block

    void * dir = kmalloc(s->block_size, "ext2-mkdir");
    if (!dir)
        return -ENOMEM;
    populate_directory(dir, s->block_size, inode, dinode);
    write_inode_data(s, inode, &i, dir, s->block_size);
    kfree(dir);

    write_inode(s, inode, &i);

    flush(s);

    return 0;
}

static int ext2_inode_unpopulate_dir(void * priv_data, int dinode, int inode, int is_dir, int links_adjustment)
{
    Ext2Context * s = priv_data;
    struct ext2_inode i;
    int ret;

    //sanity check
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;

    if (((i.i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR) ^ !!is_dir)
        return -EINVAL;

    if (((i.i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR) && count_files_in_dir(s, inode, &i))
        return -EINVAL;

    i.i_links_count += links_adjustment;

    if (i.i_links_count) {
        write_inode(s, inode, &i);
    } else {
        if ((i.i_mode & EXT2_S_IFMT) != EXT2_S_IFLNK) {
            if ((ret = clear_blocks(s, &i)) < 0)
                return ret;
        }

        clear_inode(s, inode);

        flush(s); /* flush headers, bitmaps */
    }

    //okay, now open directory and unpopulate
    if ((ret = read_inode(s, dinode, &i)) < 0)
        return ret;

    if ((i.i_mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
        return -EINVAL; /* not a directory */

    uint8_t * dir = kmalloc(i.i_size, "ext2-dir-tmp");
    if (!dir)
        return -ENOMEM;
    read_inode_data(s, dinode, &i, dir, i.i_size);

    int pos = 0;
    while (pos < i.i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(dir + pos);

        if(entry->rec_len < 8)
            break;

        if (entry->inode == 0) {
            pos += entry->rec_len;
            continue;
        }

        if (entry->inode == inode) {
            entry->inode = 0;
            memset(entry->name, 0, entry->name_len);
            write_inode_data(s, dinode, &i, dir, i.i_size);
            kfree(dir);
            return 0;
        }

        pos += entry->rec_len;
    }

    kfree(dir);

    return -ENOENT;
}

static int ext2_inode_append_dir(void * priv_data, int dinode, int inode, const char * name, int is_dir)
{
    Ext2Context * s = priv_data;

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, dinode, &i)) < 0)
        return ret;

    KASSERT((i.i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR);

    uint8_t * dir = kmalloc(i.i_size, "ext2-dir-tmp");
    if (!dir)
        return -ENOMEM;
    read_inode_data(s, dinode, &i, dir, i.i_size);

    int pos = 0;
    while (pos < i.i_size) {
        struct ext2_dir_entry_2 * entry = (struct ext2_dir_entry_2 *)(dir + pos);

        if (entry->rec_len < 8)
            break;

#if 0
        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        kprintf("  entry len %d, name len %d, inode %d, name='%s'\n", entry->rec_len, entry->name_len, entry->inode, file_name);
#endif

        int existing_size = 8 + PAD(entry->name_len);
        int slack = entry->rec_len - existing_size;

        if (slack >= 8 + PAD(strlen(name))) {
            entry->rec_len = existing_size;

            struct ext2_dir_entry_2 * entry2 = (struct ext2_dir_entry_2 *)(dir + pos + existing_size);
            entry2->rec_len = slack;
            entry2->name_len = strlen(name);
            entry2->inode = inode;
            entry2->file_type = is_dir ? EXT2_FT_DIR : EXT2_FT_REG_FILE;
            memset(entry2->name, 0, entry->name_len);
            memcpy(entry2->name, name, strlen(name));

            write_inode_data(s, dinode, &i, dir, i.i_size);
            kfree(dir);
            return 0;
        }

        pos += entry->rec_len;
    }

    kfree(dir);
    return -ENOSPC;
}

static int ext2_inode_increment_links_count(void * priv_data, int inode)
{
    Ext2Context * s = priv_data;

    int ret;
    struct ext2_inode i;
    if ((ret = read_inode(s, inode, &i)) < 0)
        return ret;

    i.i_links_count++;

    return write_inode(s, inode, &i);
}

const FileOperations ext2_io = {
    .name = "ext2",
    .inode_range = ext2_inode_range,
    .inode_resolve = ext2_inode_resolve,
    .inode_symlink = ext2_inode_symlink,
    .inode_creat = ext2_inode_creat,
    .inode_stat = ext2_inode_stat,
    .inode_populate_dir = ext2_inode_populate_dir,
    .inode_unpopulate_dir = ext2_inode_unpopulate_dir,
    .inode_append_dir = ext2_inode_append_dir,
    .inode_increment_links_count = ext2_inode_increment_links_count,
    .inode_utime = ext2_inode_utime,
    .open2 = ext2_open2,
    .file = {
        .close = ext2_close,
        .write = ext2_write,
        .read  = ext2_read,
        .lseek = ext2_lseek,
        .truncate = ext2_truncate,
    },
    .getdents = ext2_getdents,
};
