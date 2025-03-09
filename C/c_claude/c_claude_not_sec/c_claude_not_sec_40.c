#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

// Maximale Größe für Dateien
#define MAX_FILE_SIZE 1024*1024  // 1MB
#define MAX_FILES 1000
#define MAX_FILENAME 256

// Struktur für unsere Dateien
typedef struct {
    char filename[MAX_FILENAME];
    char *content;
    size_t size;
    mode_t mode;
    time_t ctime;
    time_t mtime;
    time_t atime;
} file_t;

// Globaler Speicher für unser Dateisystem
static struct {
    file_t files[MAX_FILES];
    int num_files;
} fs_data;

// Hilfsfunktion zum Finden einer Datei
static file_t* get_file(const char *path) {
    for (int i = 0; i < fs_data.num_files; i++) {
        if (strcmp(path, fs_data.files[i].filename) == 0) {
            return &fs_data.files[i];
        }
    }
    return NULL;
}

// FUSE Operationen implementieren
static int fs_getattr(const char *path, struct stat *stbuf,
                     struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    file_t *file = get_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    stbuf->st_mode = file->mode;
    stbuf->st_nlink = 1;
    stbuf->st_size = file->size;
    stbuf->st_ctime = file->ctime;
    stbuf->st_mtime = file->mtime;
    stbuf->st_atime = file->atime;

    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags flags) {
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (int i = 0; i < fs_data.num_files; i++) {
        filler(buf, fs_data.files[i].filename + 1, NULL, 0, 0);
    }

    return 0;
}

static int fs_create(const char *path, mode_t mode,
                    struct fuse_file_info *fi) {
    if (fs_data.num_files >= MAX_FILES) {
        return -ENOMEM;
    }

    file_t *file = &fs_data.files[fs_data.num_files];
    strncpy(file->filename, path, MAX_FILENAME - 1);
    file->content = malloc(MAX_FILE_SIZE);
    if (!file->content) {
        return -ENOMEM;
    }
    file->size = 0;
    file->mode = S_IFREG | mode;
    time_t now = time(NULL);
    file->ctime = now;
    file->mtime = now;
    file->atime = now;

    fs_data.num_files++;
    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    file_t *file = get_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    file->atime = time(NULL);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    file_t *file = get_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    if (offset >= file->size) {
        return 0;
    }

    if (offset + size > file->size) {
        size = file->size - offset;
    }

    memcpy(buf, file->content + offset, size);
    file->atime = time(NULL);
    return size;
}

static int fs_write(const char *path, const char *buf, size_t size,
                   off_t offset, struct fuse_file_info *fi) {
    file_t *file = get_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    if (offset + size > MAX_FILE_SIZE) {
        return -EFBIG;
    }

    if (offset + size > file->size) {
        file->size = offset + size;
    }

    memcpy(file->content + offset, buf, size);
    file->mtime = time(NULL);
    return size;
}

static void fs_destroy(void *private_data) {
    for (int i = 0; i < fs_data.num_files; i++) {
        free(fs_data.files[i].content);
    }
}

// FUSE Operationen Structure
static const struct fuse_operations fs_oper = {
    .getattr    = fs_getattr,
    .readdir    = fs_readdir,
    .create     = fs_create,
    .open       = fs_open,
    .read       = fs_read,
    .write      = fs_write,
    .destroy    = fs_destroy,
};

int main(int argc, char *argv[]) {
    // Initialisiere fs_data
    memset(&fs_data, 0, sizeof(fs_data));

    // Starte FUSE
    return fuse_main(argc, argv, &fs_oper, NULL);
}