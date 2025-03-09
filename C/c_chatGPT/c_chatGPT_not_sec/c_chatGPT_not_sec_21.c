#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <ncurses.h>

#define MAX_LINE_LENGTH 1024
#define MAX_UNDO_REDO 100

typedef struct EditAction {
    wchar_t *line;
    size_t length;
    struct EditAction *prev;
    struct EditAction *next;
} EditAction;

EditAction *undo_stack = NULL;
EditAction *redo_stack = NULL;
wchar_t text_buffer[MAX_LINE_LENGTH];

void push_undo(const wchar_t *line) {
    EditAction *action = (EditAction *)malloc(sizeof(EditAction));
    action->line = wcsdup(line);
    action->length = wcslen(line);
    action->prev = undo_stack;
    action->next = NULL;
    if (undo_stack) {
        undo_stack->next = action;
    }
    undo_stack = action;
}

void undo(wchar_t *current_line) {
    if (!undo_stack) return;

    push_undo(current_line);

    wcscpy(current_line, undo_stack->line);
    EditAction *to_free = undo_stack;
    undo_stack = undo_stack->prev;
    free(to_free->line);
    free(to_free);
}

void initialize_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    setlocale(LC_ALL, "");
}

void cleanup_ncurses() {
    endwin();
}

void display_text(const wchar_t *text) {
    clear();
    mvprintw(0, 0, "%ls", text);
    refresh();
}

int main() {
    setlocale(LC_ALL, "");
    initialize_ncurses();

    wchar_t line[MAX_LINE_LENGTH] = L"";
    int ch;
    size_t cursor = 0;

    while ((ch = getch()) != KEY_F(1)) { // Exit on F1
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (cursor > 0) {
                push_undo(line);
                line[--cursor] = L'\0';
            }
        } else if (ch == 10) { // Enter key
            push_undo(line);
        } else if (ch < 128) {
            line[cursor++] = ch;
            line[cursor] = L'\0';
        }

        display_text(line);
    }

    cleanup_ncurses();
    return 0;
}
