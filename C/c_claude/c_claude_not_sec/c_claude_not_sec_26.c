#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 8192  // Größe des Ringpuffers in Samples
#define MAX_CALLBACKS 8   // Maximale Anzahl der Callback-Funktionen

typedef float sample_t;   // Audiodatentyp (32-bit Float)

// Callback-Funktion Typ für Audio-Prozessierung
typedef void (*audio_callback_t)(sample_t* buffer, size_t length);

// Ringpuffer-Struktur
typedef struct {
    sample_t* buffer;           // Datenpuffer
    size_t write_pos;          // Schreibposition
    size_t read_pos;           // Leseposition
    size_t size;              // Puffergröße
    pthread_mutex_t mutex;     // Mutex für Thread-Sicherheit
    pthread_cond_t not_full;   // Condition Variable für nicht vollen Puffer
    pthread_cond_t not_empty;  // Condition Variable für nicht leeren Puffer
    
    // Echtzeit-Prozessierung
    audio_callback_t callbacks[MAX_CALLBACKS];
    size_t num_callbacks;
} AudioBuffer;

// Initialisierung des Audio-Buffers
AudioBuffer* audio_buffer_create() {
    AudioBuffer* ab = (AudioBuffer*)malloc(sizeof(AudioBuffer));
    if (!ab) return NULL;

    ab->buffer = (sample_t*)malloc(BUFFER_SIZE * sizeof(sample_t));
    if (!ab->buffer) {
        free(ab);
        return NULL;
    }

    ab->size = BUFFER_SIZE;
    ab->write_pos = 0;
    ab->read_pos = 0;
    ab->num_callbacks = 0;

    pthread_mutex_init(&ab->mutex, NULL);
    pthread_cond_init(&ab->not_full, NULL);
    pthread_cond_init(&ab->not_empty, NULL);

    return ab;
}

// Registrierung einer Callback-Funktion für Audio-Prozessierung
int audio_buffer_register_callback(AudioBuffer* ab, audio_callback_t callback) {
    if (ab->num_callbacks >= MAX_CALLBACKS) {
        return -1;
    }
    ab->callbacks[ab->num_callbacks++] = callback;
    return 0;
}

// Schreiben von Audiodaten in den Puffer
size_t audio_buffer_write(AudioBuffer* ab, const sample_t* data, size_t length) {
    size_t written = 0;
    
    pthread_mutex_lock(&ab->mutex);
    
    while (written < length) {
        // Warten, wenn der Puffer voll ist
        while ((ab->write_pos + 1) % ab->size == ab->read_pos) {
            pthread_cond_wait(&ab->not_full, &ab->mutex);
        }
        
        // Daten in den Puffer schreiben
        ab->buffer[ab->write_pos] = data[written];
        ab->write_pos = (ab->write_pos + 1) % ab->size;
        written++;
        
        pthread_cond_signal(&ab->not_empty);
    }
    
    pthread_mutex_unlock(&ab->mutex);
    return written;
}

// Lesen von Audiodaten aus dem Puffer mit Echtzeit-Prozessierung
size_t audio_buffer_read(AudioBuffer* ab, sample_t* data, size_t length) {
    size_t read = 0;
    
    pthread_mutex_lock(&ab->mutex);
    
    while (read < length) {
        // Warten, wenn der Puffer leer ist
        while (ab->read_pos == ab->write_pos) {
            pthread_cond_wait(&ab->not_empty, &ab->mutex);
        }
        
        // Daten aus dem Puffer lesen
        data[read] = ab->buffer[ab->read_pos];
        ab->read_pos = (ab->read_pos + 1) % ab->size;
        read++;
        
        pthread_cond_signal(&ab->not_full);
    }
    
    pthread_mutex_unlock(&ab->mutex);

    // Anwenden der Callback-Funktionen auf die gelesenen Daten
    for (size_t i = 0; i < ab->num_callbacks; i++) {
        if (ab->callbacks[i]) {
            ab->callbacks[i](data, read);
        }
    }
    
    return read;
}

// Freigeben des Audio-Buffers
void audio_buffer_destroy(AudioBuffer* ab) {
    if (ab) {
        pthread_mutex_destroy(&ab->mutex);
        pthread_cond_destroy(&ab->not_full);
        pthread_cond_destroy(&ab->not_empty);
        free(ab->buffer);
        free(ab);
    }
}

// Beispiel für eine Audio-Prozessierungs-Callback-Funktion (Verstärkung)
void amplify_callback(sample_t* buffer, size_t length) {
    const float gain = 1.5f;
    for (size_t i = 0; i < length; i++) {
        buffer[i] *= gain;
    }
}

// Beispiel-Nutzung
int main() {
    // Audio-Buffer erstellen
    AudioBuffer* ab = audio_buffer_create();
    if (!ab) {
        fprintf(stderr, "Fehler beim Erstellen des Audio-Buffers\n");
        return 1;
    }

    // Verstärkungs-Callback registrieren
    audio_buffer_register_callback(ab, amplify_callback);

    // Beispieldaten
    sample_t test_data[64];
    for (int i = 0; i < 64; i++) {
        test_data[i] = (float)i / 64.0f;  // Testsignal generieren
    }

    // Daten schreiben
    size_t written = audio_buffer_write(ab, test_data, 64);
    printf("Geschriebene Samples: %zu\n", written);

    // Daten lesen und verarbeiten
    sample_t output_data[64];
    size_t read = audio_buffer_read(ab, output_data, 64);
    printf("Gelesene Samples: %zu\n", read);

    // Ergebnisse anzeigen
    for (int i = 0; i < 5; i++) {
        printf("Sample %d: %.3f\n", i, output_data[i]);
    }

    // Aufräumen
    audio_buffer_destroy(ab);
    return 0;
}