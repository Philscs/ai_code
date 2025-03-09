#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/mount.h>

#define MAX_PATH 1024

// FUSE-Schleife
static int init_fuse(struct mount_info *m, char **argv, char *path) {
    printf("init_fuse: start\n");
    return 0;
}

// FUSE-Schleife
static void release_fuse(struct mount_info *m, void *arg) {
    printf("release_fuse: start\n");
    printf("release_fuse: end\n");
}

// Datei- und Verzeichnisdarstellung
static int getattr(const char *path, struct kstat *buf) {
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;

    buf->st_mode = S_IRUSR | S_IWUSR | S_IXUSR;
    buf->st_uid = getuid();
    buf->st_gid = getgid();
    buf->st_size = st.st_size;
    buf->st_atime = st.st_atime;
    buf->st_mtime = st.st_mtime;

    return 0;
}

static int readdir(const char *path, struct dirent **buf) {
    DIR *dirp;
    if ((dirp = opendir(path)) == NULL)
        return -1;

    while (1) {
        struct dirent *dp;
        dp = readdir(dirp);
        if (dp == NULL)
            break;

        buf[0] = *dp;
        buf[1] = NULL;
    }

    closedir(dirp);

    return 2; // Success
}

static int mkdir(const char *path, mode_t mode) {
    printf("mkdir: %s\n", path);
    return 0;
}

// FUSE-Main-Modul
int main(int argc, char **argv) {
    struct mount_info m = {NULL, init_fuse, release_fuse};

    if (mount(m.mnt, argv[1], NULL, MS_BIND | MS_RDONLY, NULL) == -1)
        perror("mount");

    return 0;
}