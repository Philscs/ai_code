#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static int myfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));

    // Set the attributes of the root directory
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    // Set the attributes of a file
    else if (strcmp(path, "/hello.txt") == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 12;
    }
    else {
        res = -ENOENT;
    }

    return res;
}

static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    // Add the entries for the root directory
    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "hello.txt", NULL, 0);
    }
    else {
        return -ENOENT;
    }

    return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi)
{
    // Check if the file exists
    if (strcmp(path, "/hello.txt") != 0) {
        return -ENOENT;
    }

    // Check if the file is being opened for reading
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;
    }

    return 0;
}

static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;

    // Check if the file exists
    if (strcmp(path, "/hello.txt") != 0) {
        return -ENOENT;
    }

    // Read the contents of the file
    const char *content = "Hello, World!\n";
    len = strlen(content);

    if (offset < len) {
        if (offset + size > len) {
            size = len - offset;
        }
        memcpy(buf, content + offset, size);
    } else {
        size = 0;
    }

    return size;
}

static struct fuse_operations myfs_oper = {
    .getattr    = myfs_getattr,
    .readdir    = myfs_readdir,
    .open       = myfs_open,
    .read       = myfs_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &myfs_oper, NULL);
}
