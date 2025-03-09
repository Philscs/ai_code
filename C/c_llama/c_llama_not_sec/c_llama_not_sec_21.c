#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Typen für Textzeilen
typedef struct {
    char *text;
    size_t length;
} Line;

// Typen für Textbuffer
typedef struct {
    Line *lines;
    size_t lineCount;
} Buffer;

void editorInit(Buffer *buffer);
void editorAddLine(Buffer *buffer, const char *line);
void editorSaveBuffer(Buffer *buffer, const char *filename);
void editorUndo(Buffer *buffer);
void editorRedo(Buffer *buffer);

int main() {
    Buffer buffer;

    editorInit(&buffer);

    while (1) {
        printf("1. Text hinzufügen\n");
        printf("2. Speichern\n");
        printf("3. Undo\n");
        printf("4. Redo\n");
        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                char line[1000];
                printf("Text einfügen: ");
                fgets(line, sizeof(line), stdin);
                editorAddLine(&buffer, line);
                break;

            case 2:
                char filename[100];
                printf("Fenstername: ");
                scanf("%s", filename);
                editorSaveBuffer(&buffer, filename);
                break;

            case 3:
                if (buffer.lineCount > 0) {
                    textContraction(&buffer.lines[--buffer.lineCount]);
                }
                break;

            case 4:
                if (buffer.lineCount < buffer.lineCount - 1) {
                    textExpand(&buffer.lines[buffer.lineCount++]);
                }
                break;

            default:
                printf("Falsche Auswahl\n");
        }

        // Syntax Highlighting
        for (size_t i = 0; i < buffer.lineCount; ++i) {
            highlightLine(&buffer.lines[i]);
            printf("\n");
        }
    }

    return 0;
}

void editorInit(Buffer *buffer) {
    buffer->lines = malloc(sizeof(Line));
    if (buffer->lines == NULL) {
        printf("Memory error\n");
        exit(1);
    }
    buffer->lineCount = 0;

    // Initialisierung der Textzeile
    buffer->lines[0].text = "Text";
    buffer->lines[0].length = strlen(buffer->lines[0].text);
}

void editorAddLine(Buffer *buffer, const char *line) {
    size_t newLength = strlen(line) + 2; // +2 für einen Leerraum und einen Backslash
    Line *newLine = malloc(sizeof(Line));
    if (newLine == NULL) {
        printf("Memory error\n");
        exit(1);
    }

    strcpy(newLine->text, line);
    newLine->length = newLength;

    // Füge die neue Zeile zur Buffer hinzu
    buffer->lines = realloc(buffer->lines, sizeof(Line) * (buffer->lineCount + 1));
    if (buffer->lines == NULL) {
        printf("Memory error\n");
        exit(1);
    }

    buffer->lines[buffer->lineCount] = *newLine;
    buffer->lineCount++;

}

void editorSaveBuffer(Buffer *buffer, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Fehler beim Öffnen des Dateisystems\n");
        exit(1);
    }

    for (size_t i = 0; i < buffer->lineCount; ++i) {
        fprintf(file, "%s", buffer->lines[i].text);
        if (i < buffer->lineCount - 1) {
            fprintf(file, "\n");
        }
    }

    fclose(file);

}

void editorUndo(Buffer *buffer) {
    if (buffer->lineCount > 0) {
        // Entferne den letzten Buchstaben der letzten Zeile
        buffer->lines[--buffer->lineCount].length--;
        strcpy(buffer->lines[buffer->lineCount].text + buffer->lines[buffer->lineCount].length, 
buffer->lines[buffer->lineCount].text);
    }
}

void editorRedo(Buffer *buffer) {
    if (buffer->lineCount < buffer->lineCount - 1) {
        // Füge den letzten Buchstaben der letzten Zeile zurück
        strcpy(buffer->lines[buffer->lineCount].text + buffer->lines[buffer->lineCount].length, 
buffer->lines[buffer->lineCount].text);
        buffer->lines[buffer->lineCount].length++;
    }
}

void highlightLine(Line *line) {
    // Syntax Highlighting für C
    for (size_t i = 0; i < strlen(line->text); ++i) {
        if (line->text[i] == '#') {
            printf("\033[31m#%c\033[0m", line->text[i]);
        } else if (line->text[i] == '}' || line->text[i] == '}') {
            printf("\033[32m}\033[0m");
        } else if (line->text[i] == '(' || line->text[i] == '[') {
            printf("\033[33m(\033[0m");
        } else if (line->text[i] == ')') {
            printf("\033[33m)\033[0m");
        } else if (line->text[i] == ']') {
            printf("\033[34m]\033[0m");
        } else if (line->text[i] == ',') {
            printf("\033[36m,\033[0m");
        } else if (line->text[i] == ';') {
            printf("\033[37m;\033[0m");
        } else {
            printf("%c", line->text[i]);
        }
    }
}
