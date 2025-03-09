#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <zlib.h>
#include <openssl/evp.h>

#define LOG_FILE "app.log"
#define ROTATED_LOG_FILE "app.log.old"
#define LOG_BUFFER_SIZE 4096
#define MAX_LOG_SIZE 1024 * 1024 // 1 MB
#define ENCRYPTION_KEY "my_secure_key"
#define ENCRYPTION_IV "my_secure_iv"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
char log_buffer[LOG_BUFFER_SIZE];
size_t buffer_offset = 0;

void encrypt_data(const char *plaintext, size_t plaintext_len, unsigned char *ciphertext, size_t *ciphertext_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)ENCRYPTION_KEY, (unsigned char *)ENCRYPTION_IV);

    int len;
    EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char *)plaintext, plaintext_len);
    *ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    *ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
}

void compress_data(const char *input, size_t input_len, char *output, size_t *output_len) {
    z_stream strm = {0};
    deflateInit(&strm, Z_BEST_COMPRESSION);

    strm.next_in = (unsigned char *)input;
    strm.avail_in = input_len;
    strm.next_out = (unsigned char *)output;
    strm.avail_out = *output_len;

    deflate(&strm, Z_FINISH);
    *output_len = strm.total_out;

    deflateEnd(&strm);
}

void rotate_log() {
    unlink(ROTATED_LOG_FILE);
    rename(LOG_FILE, ROTATED_LOG_FILE);
}

void write_to_file_atomic(const char *filename, const char *data, size_t len) {
    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);

    int fd = open(temp_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Failed to open temporary file");
        return;
    }

    if (write(fd, data, len) < 0) {
        perror("Failed to write to temporary file");
        close(fd);
        unlink(temp_filename);
        return;
    }

    close(fd);

    if (rename(temp_filename, filename) < 0) {
        perror("Failed to rename temporary file");
        unlink(temp_filename);
    }
}

void flush_log_buffer() {
    if (buffer_offset == 0) return;

    unsigned char encrypted_buffer[LOG_BUFFER_SIZE * 2];
    size_t encrypted_len;
    encrypt_data(log_buffer, buffer_offset, encrypted_buffer, &encrypted_len);

    write_to_file_atomic(LOG_FILE, (const char *)encrypted_buffer, encrypted_len);
    buffer_offset = 0;
}

void log_message(const char *message) {
    pthread_mutex_lock(&log_mutex);

    size_t msg_len = strlen(message);
    if (buffer_offset + msg_len >= LOG_BUFFER_SIZE) {
        flush_log_buffer();
    }

    memcpy(log_buffer + buffer_offset, message, msg_len);
    buffer_offset += msg_len;

    struct stat st;
    if (stat(LOG_FILE, &st) == 0 && st.st_size >= MAX_LOG_SIZE) {
        flush_log_buffer();
        rotate_log();
    }

    pthread_mutex_unlock(&log_mutex);
}

int main() {
    log_message("First log entry\n");
    log_message("Second log entry\n");
    log_message("Third log entry\n");

    flush_log_buffer();
    return 0;
}
