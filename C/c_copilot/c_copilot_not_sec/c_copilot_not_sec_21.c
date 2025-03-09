#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* text;
    int length;
} TextBuffer;

TextBuffer* createTextBuffer() {
    TextBuffer* buffer = (TextBuffer*)malloc(sizeof(TextBuffer));
    buffer->text = NULL;
    buffer->length = 0;
    return buffer;
}

void insertText(TextBuffer* buffer, const char* newText, int position) {
    int newTextLength = strlen(newText);
    int newLength = buffer->length + newTextLength;
    buffer->text = (char*)realloc(buffer->text, (newLength + 1) * sizeof(char));
    memmove(buffer->text + position + newTextLength, buffer->text + position, (buffer->length - position + 1) * sizeof(char));
    memcpy(buffer->text + position, newText, newTextLength * sizeof(char));
    buffer->length = newLength;
}

void deleteText(TextBuffer* buffer, int position, int length) {
    memmove(buffer->text + position, buffer->text + position + length, (buffer->length - position - length + 1) * sizeof(char));
    buffer->length -= length;
    buffer->text = (char*)realloc(buffer->text, (buffer->length + 1) * sizeof(char));
}

void printText(TextBuffer* buffer) {
    printf("%s\n", buffer->text);
}

void freeTextBuffer(TextBuffer* buffer) {
    free(buffer->text);
    free(buffer);
}

int main() {
    TextBuffer* buffer = createTextBuffer();
    insertText(buffer, "Hello, world!", 0);
    printText(buffer);
    deleteText(buffer, 7, 6);
    printText(buffer);
    freeTextBuffer(buffer);
    return 0;
}
