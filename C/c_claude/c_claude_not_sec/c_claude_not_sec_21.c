#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <stdbool.h>

// Datenstrukturen für den Text-Buffer
typedef struct {
    wchar_t* content;
    size_t length;
    size_t capacity;
} TextBuffer;

// Struktur für einen Undo/Redo-Schritt
typedef struct EditAction {
    enum {INSERT, DELETE} type;
    size_t position;
    wchar_t* text;
    size_t text_length;
    struct EditAction* next;
    struct EditAction* prev;
} EditAction;

// Struktur für den Undo/Redo-Stack
typedef struct {
    EditAction* current;
    EditAction* last;
} UndoRedoStack;

// Struktur für Syntax Highlighting
typedef struct {
    enum {
        NORMAL,
        KEYWORD,
        STRING,
        COMMENT,
        NUMBER
    } type;
    wchar_t* text;
    size_t length;
} SyntaxToken;

// Editor-Hauptstruktur
typedef struct {
    TextBuffer buffer;
    UndoRedoStack undoRedo;
    size_t cursor_position;
    bool modified;
} Editor;

// Initialisierung des Text-Buffers
void initTextBuffer(TextBuffer* buffer) {
    buffer->capacity = 16;
    buffer->content = malloc(buffer->capacity * sizeof(wchar_t));
    buffer->length = 0;
    buffer->content[0] = L'\0';
}

// Vergrößern des Buffers bei Bedarf
void ensureBufferCapacity(TextBuffer* buffer, size_t needed) {
    if (buffer->length + needed >= buffer->capacity) {
        while (buffer->length + needed >= buffer->capacity) {
            buffer->capacity *= 2;
        }
        buffer->content = realloc(buffer->content, buffer->capacity * sizeof(wchar_t));
    }
}

// Einfügen von Text an einer bestimmten Position
void insertText(Editor* editor, const wchar_t* text, size_t position) {
    size_t text_len = wcslen(text);
    ensureBufferCapacity(&editor->buffer, text_len);
    
    // Platz schaffen für neuen Text
    memmove(editor->buffer.content + position + text_len,
            editor->buffer.content + position,
            (editor->buffer.length - position + 1) * sizeof(wchar_t));
    
    // Text einfügen
    memcpy(editor->buffer.content + position, text, text_len * sizeof(wchar_t));
    editor->buffer.length += text_len;
    
    // Undo-Aktion erstellen
    EditAction* action = malloc(sizeof(EditAction));
    action->type = INSERT;
    action->position = position;
    action->text = malloc((text_len + 1) * sizeof(wchar_t));
    wcscpy(action->text, text);
    action->text_length = text_len;
    
    // In Undo-Stack einfügen
    action->prev = editor->undoRedo.current;
    action->next = NULL;
    if (editor->undoRedo.current) {
        editor->undoRedo.current->next = action;
    }
    editor->undoRedo.current = action;
    editor->undoRedo.last = action;
    
    editor->modified = true;
}

// Text löschen
void deleteText(Editor* editor, size_t position, size_t length) {
    if (position + length > editor->buffer.length) {
        length = editor->buffer.length - position;
    }
    
    // Undo-Aktion erstellen
    EditAction* action = malloc(sizeof(EditAction));
    action->type = DELETE;
    action->position = position;
    action->text = malloc((length + 1) * sizeof(wchar_t));
    wcsncpy(action->text, editor->buffer.content + position, length);
    action->text[length] = L'\0';
    action->text_length = length;
    
    // In Undo-Stack einfügen
    action->prev = editor->undoRedo.current;
    action->next = NULL;
    if (editor->undoRedo.current) {
        editor->undoRedo.current->next = action;
    }
    editor->undoRedo.current = action;
    editor->undoRedo.last = action;
    
    // Text löschen
    memmove(editor->buffer.content + position,
            editor->buffer.content + position + length,
            (editor->buffer.length - position - length + 1) * sizeof(wchar_t));
    editor->buffer.length -= length;
    
    editor->modified = true;
}

// Undo-Funktion
void undo(Editor* editor) {
    if (!editor->undoRedo.current) return;
    
    EditAction* action = editor->undoRedo.current;
    if (action->type == INSERT) {
        deleteText(editor, action->position, action->text_length);
    } else {
        insertText(editor, action->text, action->position);
    }
    
    editor->undoRedo.current = action->prev;
    editor->modified = true;
}

// Redo-Funktion
void redo(Editor* editor) {
    if (!editor->undoRedo.current || !editor->undoRedo.current->next) return;
    
    EditAction* action = editor->undoRedo.current->next;
    if (action->type == INSERT) {
        insertText(editor, action->text, action->position);
    } else {
        deleteText(editor, action->position, action->text_length);
    }
    
    editor->undoRedo.current = action;
    editor->modified = true;
}

// Syntax Highlighting
bool isKeyword(const wchar_t* word) {
    const wchar_t* keywords[] = {
        L"if", L"else", L"while", L"for", L"return",
        L"int", L"char", L"void", L"struct", L"typedef",
        NULL
    };
    
    for (const wchar_t** k = keywords; *k; k++) {
        if (wcscmp(word, *k) == 0) return true;
    }
    return false;
}

SyntaxToken* tokenize(const wchar_t* text, size_t* token_count) {
    size_t capacity = 16;
    size_t count = 0;
    SyntaxToken* tokens = malloc(capacity * sizeof(SyntaxToken));
    
    const wchar_t* p = text;
    while (*p) {
        // Speicherplatz überprüfen
        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(SyntaxToken));
        }
        
        // Whitespace überspringen
        while (*p && iswspace(*p)) p++;
        if (!*p) break;
        
        // Token initialisieren
        tokens[count].text = (wchar_t*)p;
        
        // Verschiedene Token-Typen erkennen
        if (iswalpha(*p) || *p == L'_') {
            // Wort/Identifier
            while (iswalnum(*p) || *p == L'_') p++;
            tokens[count].length = p - (const wchar_t*)tokens[count].text;
            
            // Temporärer String für Keyword-Check
            wchar_t* temp = malloc((tokens[count].length + 1) * sizeof(wchar_t));
            wcsncpy(temp, tokens[count].text, tokens[count].length);
            temp[tokens[count].length] = L'\0';
            
            tokens[count].type = isKeyword(temp) ? KEYWORD : NORMAL;
            free(temp);
        }
        else if (iswdigit(*p)) {
            // Nummer
            while (iswdigit(*p)) p++;
            tokens[count].length = p - (const wchar_t*)tokens[count].text;
            tokens[count].type = NUMBER;
        }
        else if (*p == L'"') {
            // String
            p++;  // Anfangsquote überspringen
            tokens[count].text = (wchar_t*)p;
            while (*p && *p != L'"') p++;
            if (*p) p++;  // Endquote überspringen
            tokens[count].length = p - (const wchar_t*)tokens[count].text - 1;
            tokens[count].type = STRING;
        }
        else if (*p == L'/' && *(p+1) == L'/') {
            // Einzeiliger Kommentar
            p += 2;  // // überspringen
            tokens[count].text = (wchar_t*)p;
            while (*p && *p != L'\n') p++;
            tokens[count].length = p - (const wchar_t*)tokens[count].text;
            tokens[count].type = COMMENT;
        }
        else {
            // Einzelnes Zeichen
            tokens[count].length = 1;
            tokens[count].type = NORMAL;
            p++;
        }
        
        count++;
    }
    
    *token_count = count;
    return tokens;
}

// Editor initialisieren
void initEditor(Editor* editor) {
    setlocale(LC_ALL, "");  // Für Unicode-Support
    
    initTextBuffer(&editor->buffer);
    editor->undoRedo.current = NULL;
    editor->undoRedo.last = NULL;
    editor->cursor_position = 0;
    editor->modified = false;
}

// Hauptfunktion
int main() {
    Editor editor;
    initEditor(&editor);
    
    // Beispiel für die Verwendung
    const wchar_t* sample_text = L"int main() {\n    // Dies ist ein Kommentar\n    printf(\"Hello, World!\");\n    return 0;\n}\n";
    insertText(&editor, sample_text, 0);
    
    // Tokenisierung für Syntax Highlighting
    size_t token_count;
    SyntaxToken* tokens = tokenize(editor.buffer.content, &token_count);
    
    // Ausgabe mit Syntax Highlighting (vereinfacht)
    for (size_t i = 0; i < token_count; i++) {
        switch (tokens[i].type) {
            case KEYWORD:
                printf("\033[1;34m");  // Blau für Keywords
                break;
            case STRING:
                printf("\033[0;32m");  // Grün für Strings
                break;
            case COMMENT:
                printf("\033[0;90m");  // Grau für Kommentare
                break;
            case NUMBER:
                printf("\033[0;35m");  // Magenta für Zahlen
                break;
            default:
                printf("\033[0m");     // Normal
        }
        
        // Token ausgeben
        wprintf(L"%.*ls", (int)tokens[i].length, tokens[i].text);
        printf("\033[0m");  // Zurück zu normal
    }
    
    // Aufräumen
    free(tokens);
    free(editor.buffer.content);
    
    // Undo/Redo-Stack aufräumen
    EditAction* current = editor.undoRedo.last;
    while (current) {
        EditAction* prev = current->prev;
        free(current->text);
        free(current);
        current = prev;
    }
    
    return 0;
}