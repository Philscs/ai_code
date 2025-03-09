#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#define CONFIG_FILE_PATH "./config.cfg"
#define MAX_LINE_LENGTH 256
#define ENCRYPTION_KEY "example_key_1234" // Replace with a secure key

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void encrypt_sensitive_data(const char *plaintext, char *ciphertext, size_t *ciphertext_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handle_error("EVP_CIPHER_CTX_new");

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)ENCRYPTION_KEY, (unsigned char *)ENCRYPTION_KEY)) {
        handle_error("EVP_EncryptInit_ex");
    }

    int len;
    if (1 != EVP_EncryptUpdate(ctx, (unsigned char *)ciphertext, &len, (unsigned char *)plaintext, strlen(plaintext))) {
        handle_error("EVP_EncryptUpdate");
    }
    *ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, (unsigned char *)ciphertext + len, &len)) {
        handle_error("EVP_EncryptFinal_ex");
    }
    *ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
}

void decrypt_sensitive_data(const char *ciphertext, size_t ciphertext_len, char *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handle_error("EVP_CIPHER_CTX_new");

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)ENCRYPTION_KEY, (unsigned char *)ENCRYPTION_KEY)) {
        handle_error("EVP_DecryptInit_ex");
    }

    int len;
    if (1 != EVP_DecryptUpdate(ctx, (unsigned char *)plaintext, &len, (unsigned char *)ciphertext, ciphertext_len)) {
        handle_error("EVP_DecryptUpdate");
    }

    int plaintext_len = len;
    if (1 != EVP_DecryptFinal_ex(ctx, (unsigned char *)plaintext + len, &len)) {
        handle_error("EVP_DecryptFinal_ex");
    }
    plaintext_len += len;
    plaintext[plaintext_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);
}

void parse_config_file(const char *filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) handle_error("open");

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) handle_error("fstat");

    if (!S_ISREG(file_stat.st_mode)) {
        fprintf(stderr, "Error: Not a regular file\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    FILE *file = fdopen(fd, "r");
    if (!file) handle_error("fdopen");

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        size_t len = strnlen(line, sizeof(line));
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (strncmp(line, "sensitive=", 10) == 0) {
            char plaintext[MAX_LINE_LENGTH];
            char ciphertext[MAX_LINE_LENGTH];
            size_t ciphertext_len;

            encrypt_sensitive_data(line + 10, ciphertext, &ciphertext_len);
            printf("Encrypted sensitive data: %s\n", ciphertext);

            decrypt_sensitive_data(ciphertext, ciphertext_len, plaintext);
            printf("Decrypted sensitive data: %s\n", plaintext);
        } else {
            printf("Read config line: %s\n", line);
        }
    }

    fclose(file);
}

int main() {
    parse_config_file(CONFIG_FILE_PATH);
    return 0;
}
