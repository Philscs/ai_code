#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Strukt für die Log-Einträge
typedef struct {
    char timestamp[20];
    char message[1024];
} LogEntry;

// Strukt für den Logger
typedef struct {
    FILE* logFile;
    int compressionMode;
} Logger;

// Funktion zum Erstellen eines neuen Loggers
Logger* createLogger(const char* logFilename, const char* logPath) {
    Logger* logger = malloc(sizeof(Logger));
    
    // Atomische Dateioperationen mit flock()
    if (flock(logger->logFile, LOCK_EX | LOCK_NB) == -1) {
        perror("flock");
        exit(1);
    }
    
    // Sichere Dateienachrichtigkeiten setzen
    chmod(logPath, 0600);
    
    return logger;
}

// Funktion zum Schreiben in den Logger
void writeLog(Logger* logger, const char* message) {
    char timestamp[20];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    LogEntry entry;
    strcpy(entry.timestamp, timestamp);
    strcpy(entry.message, message);
    
    // Buffermanagement und Komprimierung
    if (logger->compressionMode == 1) {
        char compressed[1024];
        compressMessage(entry.message, 1024, compressed);
        
        // Schreiben des komprimierten Inhalts
        fwrite(compressed, 1, strlen(compressed), logger->logFile);
    } else {
        // Schreiben des nichtkomprimierten Inhalts
        fprintf(logger->logFile, "%s %s\n", entry.timestamp, entry.message);
    }
}

// Funktion zum Rotation der Log-Datei
void rotateLogger(Logger* logger) {
    int fd = logger->logFile;
    
    // Atomische Dateioperationen mit flock()
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        perror("flock");
        exit(1);
    }
    
    // Lese die gesamte Log-Datei
    fseek(logger->logFile, 0, SEEK_END);
    rewind(logger->logFile);
    size_t logSize = ftell(logger->logFile);
    
    // Schneide die Datei auf eine bestimmte Größe
    if (logSize > 1024 * 1024) {
        truncate(logger->logFile, 1024 * 1024);
    }
    
    fclose(logger->logFile);
    
    // Neue Log-Datei erzeugen und schreiben
    logger->logFile = fopen(logFilename + ".1", "w");
    
    // Atomische Dateioperationen mit flock()
    if (flock(logger->logFile, LOCK_EX | LOCK_NB) == -1) {
        perror("flock");
        exit(1);
    }
}

int main() {
    const char logFilename[] = "/var/log/myapp.log";
    const char logPath[] = "/var/log";
    
    Logger* logger = createLogger(logFilename, logPath);
    
    writeLog(logger, "Ein Log-Entry.");
    writeLog(logger, "Ein weiterer Log-Entry.");
    rotateLogger(logger);
    
    return 0;
}