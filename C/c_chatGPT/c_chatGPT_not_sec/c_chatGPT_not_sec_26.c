#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024 // Größe des Audio-Buffers
typedef struct {
    float buffer[BUFFER_SIZE]; // Ringpuffer für Audio-Daten
    int read_index;           // Lesezeiger
    int write_index;          // Schreibzeiger
    int count;                // Anzahl der im Puffer gespeicherten Samples
    pthread_mutex_t mutex;    // Mutex zur Synchronisierung
    pthread_cond_t can_read;  // Bedingungsvariable für Lesen
    pthread_cond_t can_write; // Bedingungsvariable für Schreiben
} AudioBuffer;

void init_audio_buffer(AudioBuffer* audio_buffer) {
    memset(audio_buffer->buffer, 0, sizeof(audio_buffer->buffer));
    audio_buffer->read_index = 0;
    audio_buffer->write_index = 0;
    audio_buffer->count = 0;
    pthread_mutex_init(&audio_buffer->mutex, NULL);
    pthread_cond_init(&audio_buffer->can_read, NULL);
    pthread_cond_init(&audio_buffer->can_write, NULL);
}

void destroy_audio_buffer(AudioBuffer* audio_buffer) {
    pthread_mutex_destroy(&audio_buffer->mutex);
    pthread_cond_destroy(&audio_buffer->can_read);
    pthread_cond_destroy(&audio_buffer->can_write);
}

void write_to_buffer(AudioBuffer* audio_buffer, float* data, int size) {
    pthread_mutex_lock(&audio_buffer->mutex);

    for (int i = 0; i < size; i++) {
        while (audio_buffer->count == BUFFER_SIZE) {
            pthread_cond_wait(&audio_buffer->can_write, &audio_buffer->mutex);
        }

        audio_buffer->buffer[audio_buffer->write_index] = data[i];
        audio_buffer->write_index = (audio_buffer->write_index + 1) % BUFFER_SIZE;
        audio_buffer->count++;

        pthread_cond_signal(&audio_buffer->can_read);
    }

    pthread_mutex_unlock(&audio_buffer->mutex);
}

void read_from_buffer(AudioBuffer* audio_buffer, float* output, int size) {
    pthread_mutex_lock(&audio_buffer->mutex);

    for (int i = 0; i < size; i++) {
        while (audio_buffer->count == 0) {
            pthread_cond_wait(&audio_buffer->can_read, &audio_buffer->mutex);
        }

        output[i] = audio_buffer->buffer[audio_buffer->read_index];
        audio_buffer->read_index = (audio_buffer->read_index + 1) % BUFFER_SIZE;
        audio_buffer->count--;

        pthread_cond_signal(&audio_buffer->can_write);
    }

    pthread_mutex_unlock(&audio_buffer->mutex);
}

void* audio_producer(void* arg) {
    AudioBuffer* audio_buffer = (AudioBuffer*)arg;
    float sample = 0.0f;

    while (1) {
        float data[128];
        for (int i = 0; i < 128; i++) {
            data[i] = sample; // Beispiel: Erzeugung eines einfachen Signals
            sample += 0.01f;
        }
        write_to_buffer(audio_buffer, data, 128);
        usleep(10000); // Simuliert Echtzeit-Produktion
    }
    return NULL;
}

void* audio_consumer(void* arg) {
    AudioBuffer* audio_buffer = (AudioBuffer*)arg;

    while (1) {
        float output[128];
        read_from_buffer(audio_buffer, output, 128);

        // Beispiel: Verarbeitung der Daten (hier: Ausgabe auf der Konsole)
        printf("Verarbeitete Daten: %f\n", output[0]);
    }
    return NULL;
}

int main() {
    AudioBuffer audio_buffer;
    init_audio_buffer(&audio_buffer);

    pthread_t producer_thread, consumer_thread;

    pthread_create(&producer_thread, NULL, audio_producer, &audio_buffer);
    pthread_create(&consumer_thread, NULL, audio_consumer, &audio_buffer);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    destroy_audio_buffer(&audio_buffer);

    return 0;
}
