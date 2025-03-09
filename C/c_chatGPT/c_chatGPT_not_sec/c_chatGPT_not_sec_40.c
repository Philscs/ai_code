#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// Struktur zur Speicherung einer Datei
struct file {
    char name[256];
    char *content;
    size_t size;
    struct file *next;
};

static struct file *root = NULL;

// Funktion zum Finden einer Datei
static struct file *find_file(const char *name) {
    struct file *current = root;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// FUSE-Callback: getattr
static int ramfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        struct file *file = find_file(path + 1); // Entferne den '/' am Anfang
        if (!file) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = file->size;
    }
    return 0;
}

// FUSE-Callback: readdir
static int ramfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    struct file *current = root;
    while (current) {
        filler(buf, current->name, NULL, 0);
        current = current->next;
    }

    return 0;
}

// FUSE-Callback: open
static int ramfs_open(const char *path, struct fuse_file_info *fi) {
    struct file *file = find_file(path + 1);
    if (!file) {
        return -ENOENT;
    }
    return 0;
}

// FUSE-Callback: read
static int ramfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    struct file *file = find_file(path + 1);
    if (!file) {
        return -ENOENT;
    }

    if (offset < file->size) {
        if (offset + size > file->size) {
            size = file->size - offset;
        }
        memcpy(buf, file->content + offset, size);
    } else {
        size = 0;
    }

    return size;
}

// FUSE-Callback: write
static int ramfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    struct file *file = find_file(path + 1);
    if (!file) {
        return -ENOENT;
    }

    if (offset + size > file->size) {
        char *new_content = realloc(file->content, offset + size);
        if (!new_content) {
            return -ENOMEM;
        }
        file->content = new_content;
        file->size = offset + size;
    }

    memcpy(file->content + offset, buf, size);
    return size;
}

// FUSE-Callback: create
static int ramfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    (void) fi;

    if (find_file(path + 1)) {
        return -EEXIST;
    }

    struct file *new_file = malloc(sizeof(struct file));
    if (!new_file) {
        return -ENOMEM;
    }

    strncpy(new_file->name, path + 1, sizeof(new_file->name) - 1);
    new_file->name[sizeof(new_file->name) - 1] = '\0';
    new_file->content = NULL;
    new_file->size = 0;
    new_file->next = root;
    root = new_file;

    return 0;
}

// FUSE-Callback: unlink
static int ramfs_unlink(const char *path) {
    struct file **current = &root;

    while (*current) {
        if (strcmp((*current)->name, path + 1) == 0) {
            struct file *to_delete = *current;
            *current = to_delete->next;
            free(to_delete->content);
            free(to_delete);
            return 0;
        }
        current = &((*current)->next);
    }

    return -ENOENT;
}

static struct fuse_operations ramfs_operations = {
    .getattr = ramfs_getattr,
    .readdir = ramfs_readdir,
    .open = ramfs_open,
    .read = ramfs_read,
    .write = ramfs_write,
    .create = ramfs_create,
    .unlink = ramfs_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &ramfs_operations, NULL);
}
