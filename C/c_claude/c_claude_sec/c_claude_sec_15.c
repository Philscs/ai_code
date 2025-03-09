#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <pthread.h>
#include <errno.h>
#include <zlib.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#define BUFFER_SIZE 4096
#define MAX_LOG_SIZE (10 * 1024 * 1024)  // 10MB
#define MAX_ROTATION_FILES 5
#define COMPRESSION_LEVEL 6

typedef struct {
    char *filename;
    size_t max_size;
    pthread_mutex_t mutex;
    EVP_CIPHER_CTX *cipher_ctx;
    unsigned char key[32];
    unsigned char iv[16];
    int compression_enabled;
    FILE *current_file;
    size_t current_size;
} secure_logger_t;

// Buffer for atomic writes
typedef struct {
    char data[BUFFER_SIZE];
    size_t used;
} write_buffer_t;

// Initialize the logger
secure_logger_t *logger_init(const char *filename, const unsigned char *encryption_key, 
                           int enable_compression) {
    secure_logger_t *logger = calloc(1, sizeof(secure_logger_t));
    if (!logger) return NULL;

    logger->filename = strdup(filename);
    logger->max_size = MAX_LOG_SIZE;
    logger->compression_enabled = enable_compression;

    // Initialize mutex for thread safety
    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        free(logger->filename);
        free(logger);
        return NULL;
    }

    // Initialize encryption
    memcpy(logger->key, encryption_key, 32);
    RAND_bytes(logger->iv, 16);  // Generate random IV
    
    logger->cipher_ctx = EVP_CIPHER_CTX_new();
    if (!logger->cipher_ctx) {
        pthread_mutex_destroy(&logger->mutex);
        free(logger->filename);
        free(logger);
        return NULL;
    }

    // Set secure file permissions
    umask(0077);  // Only owner can read/write

    return logger;
}

// Rotate log files
static int rotate_logs(secure_logger_t *logger) {
    char old_name[256], new_name[256];
    
    // Remove oldest log file
    snprintf(old_name, sizeof(old_name), "%s.%d", logger->filename, MAX_ROTATION_FILES);
    unlink(old_name);

    // Rotate existing log files
    for (int i = MAX_ROTATION_FILES - 1; i >= 0; i--) {
        snprintf(old_name, sizeof(old_name), "%s.%d", logger->filename, i);
        snprintf(new_name, sizeof(new_name), "%s.%d", logger->filename, i + 1);
        rename(old_name, new_name);
    }

    // Rename current log file
    rename(logger->filename, old_name);

    return 0;
}

// Encrypt data
static int encrypt_data(secure_logger_t *logger, const unsigned char *plaintext, 
                       int plaintext_len, unsigned char *ciphertext) {
    int len, ciphertext_len;

    EVP_EncryptInit_ex(logger->cipher_ctx, EVP_aes_256_gcm(), NULL, logger->key, logger->iv);
    
    EVP_EncryptUpdate(logger->cipher_ctx, ciphertext, &len, plaintext, plaintext_len);
    ciphertext_len = len;

    EVP_EncryptFinal_ex(logger->cipher_ctx, ciphertext + len, &len);
    ciphertext_len += len;

    return ciphertext_len;
}

// Compress data if enabled
static int compress_data(const char *input, size_t input_len, char *output, 
                        size_t output_size) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit(&strm, COMPRESSION_LEVEL) != Z_OK)
        return -1;

    strm.avail_in = input_len;
    strm.next_in = (unsigned char *)input;
    strm.avail_out = output_size;
    strm.next_out = (unsigned char *)output;

    deflate(&strm, Z_FINISH);
    deflateEnd(&strm);

    return output_size - strm.avail_out;
}

// Write to log file with atomic operations
int logger_write(secure_logger_t *logger, const char *message) {
    pthread_mutex_lock(&logger->mutex);

    // Check if rotation is needed
    if (logger->current_size >= logger->max_size) {
        rotate_logs(logger);
        logger->current_size = 0;
    }

    // Prepare buffer
    write_buffer_t buffer;
    size_t msg_len = strlen(message);
    
    // Add timestamp
    time_t now = time(NULL);
    int header_len = strftime(buffer.data, BUFFER_SIZE, "[%Y-%m-%d %H:%M:%S] ", localtime(&now));
    
    memcpy(buffer.data + header_len, message, msg_len);
    buffer.data[header_len + msg_len] = '\n';
    buffer.used = header_len + msg_len + 1;

    // Compress if enabled
    char compressed_data[BUFFER_SIZE];
    if (logger->compression_enabled) {
        int compressed_size = compress_data(buffer.data, buffer.used, 
                                         compressed_data, BUFFER_SIZE);
        if (compressed_size > 0) {
            memcpy(buffer.data, compressed_data, compressed_size);
            buffer.used = compressed_size;
        }
    }

    // Encrypt
    unsigned char encrypted_data[BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    int encrypted_len = encrypt_data(logger, (unsigned char *)buffer.data, 
                                   buffer.used, encrypted_data);

    // Open file with exclusive lock
    int fd = open(logger->filename, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    // Get exclusive lock
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        pthread_mutex_unlock(&logger->mutex);
        return -1;
    }

    // Write atomically
    ssize_t written = write(fd, encrypted_data, encrypted_len);
    
    // Release lock and close
    flock(fd, LOCK_UN);
    close(fd);

    if (written > 0) {
        logger->current_size += written;
    }

    pthread_mutex_unlock(&logger->mutex);
    return (written == encrypted_len) ? 0 : -1;
}

// Clean up
void logger_cleanup(secure_logger_t *logger) {
    if (!logger) return;

    pthread_mutex_destroy(&logger->mutex);
    EVP_CIPHER_CTX_free(logger->cipher_ctx);
    free(logger->filename);
    free(logger);
}