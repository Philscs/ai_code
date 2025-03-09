#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define false_positive_rate 0.1 // veränderbare Wahrscheinlichkeit von falschen positiven 
Ergebnissen

typedef struct {
    unsigned int size;
    unsigned int num_hashes;
    unsigned char* data;
} BloomFilter;

BloomFilter* new_bloom_filter(int size, int num_hashes) {
    BloomFilter* bf = (BloomFilter*)malloc(sizeof(BloomFilter));
    bf->size = size;
    bf->num_hashes = num_hashes;
    bf->data = (unsigned char*)malloc(size * sizeof(unsigned char));
    return bf;
}

void add(bloom_filter* bf, const unsigned char* key) {
    for (int i = 0; i < bf->num_hashes; i++) {
        int index = hash(key, i);
        if (bf->data[index] & (1 << (bf->size - 1))) {
            // wenn die Position bereits markiert ist, wird der Schlüssel nicht hinzugefügt
            return;
        }
        bf->data[index] |= (unsigned char)(1 << (bf->size - 1));
    }
}

int contains(bloom_filter* bf, const unsigned char* key) {
    for (int i = 0; i < bf->num_hashes; i++) {
        int index = hash(key, i);
        if (!(bf->data[index] & (1 << (bf->size - 1)))) {
            // wenn die Position nicht markiert ist, wird der Schlüssel nicht gefunden
            return 0;
        }
    }
    return 1; // der Schlüssel muss in der Filter vorhanden sein
}

unsigned int hash(const unsigned char* key, int index) {
    unsigned int h = 0;
    for (int i = 0; i < strlen(key); i++) {
        h ^= (key[i] + index);
    }
    return h % bf->size;
}

int main() {
    BloomFilter* bf = new_bloom_filter(1024, 3);

    add(bf, "abc");
    add(bf, "def");
    add(bf, "ghi");

    if (contains(bf, "abc")) printf("Key 'abc' gefunden!\n"); else printf("Key 'abc' nicht gefunden!\n");
    if (contains(bf, "xyz")) printf("Key 'xyz' gefunden!\n"); else printf("Key 'xyz' nicht gefunden!\n");

    return 0;
}
