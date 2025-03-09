#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <errno.h>

#define MAX_PATH_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 256
#define ENCRYPTION_KEY "your-secure-key-here"

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int encrypted;
} ConfigEntry;

// Sichere String-Operationen
void secure_strncpy(char *dest, const char *src, size_t n) {
    if (dest && src && n > 0) {
        size_t i;
        for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }
}

// Überprüfung des Dateipfads gegen Directory Traversal
int is_safe_path(const char *path) {
    if (!path || strstr(path, "..") || strstr(path, "~") || path[0] == '/') {
        return 0;
    }
    
    char resolved_path[MAX_PATH_LENGTH];
    if (realpath(path, resolved_path) == NULL) {
        return 0;
    }
    
    // Überprüfen Sie, ob der Pfad innerhalb des erlaubten Verzeichnisses liegt
    char allowed_dir[MAX_PATH_LENGTH];
    if (getcwd(allowed_dir, sizeof(allowed_dir)) == NULL) {
        return 0;
    }
    
    return strncmp(resolved_path, allowed_dir, strlen(allowed_dir)) == 0;
}

// Verschlüsselungsfunktion für sensitive Daten
int encrypt_value(const char *plain_text, char *cipher_text, size_t max_length) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    unsigned char key[32];
    unsigned char iv[16] = {0}; // In der Praxis sollte ein sicherer IV verwendet werden
    
    // Key-Derivation (in der Praxis sollte PBKDF2 oder ähnliches verwendet werden)
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL,
                   (unsigned char *)ENCRYPTION_KEY, strlen(ENCRYPTION_KEY),
                   1, key, iv);

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int cipher_length = 0;
    int final_length = 0;

    if (EVP_EncryptUpdate(ctx, (unsigned char *)cipher_text, &cipher_length,
                         (unsigned char *)plain_text, strlen(plain_text)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptFinal_ex(ctx, (unsigned char *)cipher_text + cipher_length,
                           &final_length) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);
    return cipher_length + final_length;
}

// Entschlüsselungsfunktion
int decrypt_value(const char *cipher_text, char *plain_text, size_t max_length) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    unsigned char key[32];
    unsigned char iv[16] = {0};
    
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), NULL,
                   (unsigned char *)ENCRYPTION_KEY, strlen(ENCRYPTION_KEY),
                   1, key, iv);

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int plain_length = 0;
    int final_length = 0;

    if (EVP_DecryptUpdate(ctx, (unsigned char *)plain_text, &plain_length,
                         (unsigned char *)cipher_text, strlen(cipher_text)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptFinal_ex(ctx, (unsigned char *)plain_text + plain_length,
                           &final_length) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);
    plain_text[plain_length + final_length] = '\0';
    return plain_length + final_length;
}

// Sicheres Öffnen der Konfigurationsdatei
FILE* secure_fopen(const char *filename, const char *mode) {
    if (!is_safe_path(filename)) {
        return NULL;
    }
    
    // Exclusive Creation für Schreibzugriffe um Race Conditions zu vermeiden
    if (strchr(mode, 'w') != NULL) {
        int fd = open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            return NULL;
        }
        return fdopen(fd, mode);
    }
    
    return fopen(filename, mode);
}

// Parser für die Konfigurationsdatei
int parse_config_file(const char *filename, ConfigEntry *entries, size_t max_entries) {
    FILE *file = secure_fopen(filename, "r");
    if (!file) {
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    size_t entry_count = 0;
    
    while (fgets(line, sizeof(line), file) && entry_count < max_entries) {
        // Entferne Newline
        char *newline = strchr(line, '\n');
        if (newline) {
            *newline = '\0';
        }
        
        // Überspringe Kommentare und leere Zeilen
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        
        // Parse Key-Value Pair
        char *delimiter = strchr(line, '=');
        if (!delimiter) {
            continue;
        }
        
        *delimiter = '\0';
        char *key = line;
        char *value = delimiter + 1;
        
        // Entferne führende und nachfolgende Leerzeichen
        while (*key && (*key == ' ' || *key == '\t')) key++;
        while (*value && (*value == ' ' || *value == '\t')) value++;
        
        char *key_end = key + strlen(key) - 1;
        char *value_end = value + strlen(value) - 1;
        while (key_end > key && (*key_end == ' ' || *key_end == '\t')) *key_end-- = '\0';
        while (value_end > value && (*value_end == ' ' || *value_end == '\t')) *value_end-- = '\0';
        
        // Sichere Kopie in die Struktur
        secure_strncpy(entries[entry_count].key, key, MAX_KEY_LENGTH);
        secure_strncpy(entries[entry_count].value, value, MAX_VALUE_LENGTH);
        
        // Prüfe ob der Wert verschlüsselt werden soll (z.B. wenn er mit "ENC:" beginnt)
        entries[entry_count].encrypted = (strncmp(value, "ENC:", 4) == 0);
        if (entries[entry_count].encrypted) {
            char temp_value[MAX_VALUE_LENGTH];
            secure_strncpy(temp_value, value + 4, MAX_VALUE_LENGTH);
            
            char encrypted_value[MAX_VALUE_LENGTH];
            if (encrypt_value(temp_value, encrypted_value, MAX_VALUE_LENGTH) > 0) {
                secure_strncpy(entries[entry_count].value, encrypted_value, MAX_VALUE_LENGTH);
            }
        }
        
        entry_count++;
    }
    
    fclose(file);
    return entry_count;
}

// Beispiel für die Verwendung
int main() {
    ConfigEntry entries[100];
    int count = parse_config_file("config.txt", entries, 100);
    
    if (count < 0) {
        fprintf(stderr, "Fehler beim Parsen der Konfigurationsdatei\n");
        return 1;
    }
    
    printf("Gelesene Konfigurationseinträge:\n");
    for (int i = 0; i < count; i++) {
        printf("Key: %s, ", entries[i].key);
        
        if (entries[i].encrypted) {
            char decrypted_value[MAX_VALUE_LENGTH];
            if (decrypt_value(entries[i].value, decrypted_value, MAX_VALUE_LENGTH) > 0) {
                printf("Value (encrypted): %s\n", decrypted_value);
            } else {
                printf("Value: [Entschlüsselungsfehler]\n");
            }
        } else {
            printf("Value: %s\n", entries[i].value);
        }
    }
    
    return 0;
}