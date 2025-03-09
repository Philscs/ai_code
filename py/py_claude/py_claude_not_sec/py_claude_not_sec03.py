import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from pygments import lex
from pygments.lexers import get_lexer_by_name, get_all_lexers
from pygments.styles import get_style_by_name
import re

class UndoRedoStack:
    def __init__(self):
        self.undo_stack = []
        self.redo_stack = []
    
    def push(self, text, cursor_position):
        self.undo_stack.append((text, cursor_position))
        self.redo_stack.clear()
    
    def undo(self):
        if len(self.undo_stack) > 1:
            current = self.undo_stack.pop()
            self.redo_stack.append(current)
            return self.undo_stack[-1]
        return None
    
    def redo(self):
        if self.redo_stack:
            current = self.redo_stack.pop()
            self.undo_stack.append(current)
            return current
        return None

class Macro:
    def __init__(self):
        self.actions = []
        self.is_recording = False
    
    def start_recording(self):
        self.actions.clear()
        self.is_recording = True
    
    def stop_recording(self):
        self.is_recording = False
    
    def record_action(self, action_type, content=None):
        if self.is_recording:
            self.actions.append((action_type, content))
    
    def play(self, editor):
        for action_type, content in self.actions:
            if action_type == "insert":
                editor.insert("insert", content)
            elif action_type == "delete":
                editor.delete("insert-1c", "insert")

class SyntaxHighlightingText(tk.Text):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.undo_redo_stack = UndoRedoStack()
        self.macro = Macro()
        self.current_lexer = get_lexer_by_name("python")
        self.current_style = get_style_by_name("monokai")
        
        # Konfiguriere Tags für Syntax-Highlighting
        self.configure_tags()
        
        # Binde Events
        self.bind("<Key>", self.on_key_press)
        self.bind("<Return>", self.handle_return)
        self.bind("<Tab>", self.handle_tab)
        
        # Erstelle Rechtsklick-Menü
        self.create_context_menu()
    
    def configure_tags(self):
        style = self.current_style
        for token, style_def in style:
            foreground = style_def["color"]
            background = style_def.get("bgcolor")
            font = "TkFixedFont"
            
            tag_config = {"font": font}
            if foreground:
                tag_config["foreground"] = f"#{foreground}"
            if background:
                tag_config["background"] = f"#{background}"
                
            self.tag_configure(str(token), **tag_config)
    
    def highlight_syntax(self):
        content = self.get("1.0", "end-1c")
        self.tag_remove("all", "1.0", "end")
        
        for token, value in lex(content, self.current_lexer):
            start_index = self.index("1.0 + %dc" % len("".join(value for _, value in lex(content, self.current_lexer))))
            end_index = self.index(f"{start_index} + {len(value)}c")
            self.tag_add(str(token), start_index, end_index)
    
    def on_key_press(self, event):
        if event.keysym in ("Left", "Right", "Up", "Down"):
            return
        
        # Speichere den aktuellen Zustand für Undo/Redo
        current_text = self.get("1.0", "end-1c")
        cursor_position = self.index("insert")
        self.undo_redo_stack.push(current_text, cursor_position)
        
        # Zeichne Makro auf
        if self.macro.is_recording:
            if event.char:
                self.macro.record_action("insert", event.char)
            elif event.keysym == "BackSpace":
                self.macro.record_action("delete")
        
        self.after(10, self.highlight_syntax)
    
    def handle_return(self, event):
        # Hole Einrückung der aktuellen Zeile
        current_line = self.get("insert linestart", "insert")
        match = re.match(r'^(\s+)', current_line)
        indent = match.group(1) if match else ""
        
        # Prüfe auf zusätzliche Einrückung (z.B. nach einem Doppelpunkt)
        if current_line.strip().endswith(":"):
            indent += "    "
        
        self.insert("insert", f"\n{indent}")
        return "break"
    
    def handle_tab(self, event):
        self.insert("insert", "    ")
        return "break"
    
    def create_context_menu(self):
        self.context_menu = tk.Menu(self, tearoff=0)
        self.context_menu.add_command(label="Ausschneiden", command=lambda: self.event_generate("<<Cut>>"))
        self.context_menu.add_command(label="Kopieren", command=lambda: self.event_generate("<<Copy>>"))
        self.context_menu.add_command(label="Einfügen", command=lambda: self.event_generate("<<Paste>>"))
        self.context_menu.add_separator()
        self.context_menu.add_command(label="Rückgängig", command=self.undo)
        self.context_menu.add_command(label="Wiederherstellen", command=self.redo)
        
        self.bind("<Button-3>", self.show_context_menu)
    
    def show_context_menu(self, event):
        self.context_menu.tk_popup(event.x_root, event.y_root)
    
    def undo(self):
        result = self.undo_redo_stack.undo()
        if result:
            text, cursor_position = result
            self.delete("1.0", "end")
            self.insert("1.0", text)
            self.mark_set("insert", cursor_position)
            self.highlight_syntax()
    
    def redo(self):
        result = self.undo_redo_stack.redo()
        if result:
            text, cursor_position = result
            self.delete("1.0", "end")
            self.insert("1.0", text)
            self.mark_set("insert", cursor_position)
            self.highlight_syntax()

class TextEditor(tk.Tk):
    def __init__(self):
        super().__init__()
        
        self.title("Python Text Editor")
        self.geometry("800x600")
        
        # Erstelle Menüleiste
        self.create_menu()
        
        # Erstelle Haupttextfeld
        self.text_area = SyntaxHighlightingText(self, wrap=tk.NONE)
        self.text_area.pack(expand=True, fill="both")
        
        # Erstelle Scrollbars
        self.create_scrollbars()
        
        # Erstelle Statusleiste
        self.create_status_bar()
        
        # Konfiguriere Sprachen-Auswahl
        self.configure_language_selection()
    
    def create_menu(self):
        menubar = tk.Menu(self)
        
        # Datei-Menü
        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Neu", command=self.new_file)
        file_menu.add_command(label="Öffnen", command=self.open_file)
        file_menu.add_command(label="Speichern", command=self.save_file)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.quit)
        menubar.add_cascade(label="Datei", menu=file_menu)
        
        # Bearbeiten-Menü
        edit_menu = tk.Menu(menubar, tearoff=0)
        edit_menu.add_command(label="Rückgängig", command=self.text_area.undo)
        edit_menu.add_command(label="Wiederherstellen", command=self.text_area.redo)
        edit_menu.add_separator()
        edit_menu.add_command(label="Ausschneiden", command=lambda: self.text_area.event_generate("<<Cut>>"))
        edit_menu.add_command(label="Kopieren", command=lambda: self.text_area.event_generate("<<Copy>>"))
        edit_menu.add_command(label="Einfügen", command=lambda: self.text_area.event_generate("<<Paste>>"))
        menubar.add_cascade(label="Bearbeiten", menu=edit_menu)
        
        # Makro-Menü
        macro_menu = tk.Menu(menubar, tearoff=0)
        macro_menu.add_command(label="Aufnahme starten", command=self.start_macro_recording)
        macro_menu.add_command(label="Aufnahme beenden", command=self.stop_macro_recording)
        macro_menu.add_command(label="Makro abspielen", command=self.play_macro)
        menubar.add_cascade(label="Makro", menu=macro_menu)
        
        self.config(menu=menubar)
    
    def create_scrollbars(self):
        # Vertikale Scrollbar
        vertical_scroll = ttk.Scrollbar(self, orient="vertical", command=self.text_area.yview)
        vertical_scroll.pack(side="right", fill="y")
        
        # Horizontale Scrollbar
        horizontal_scroll = ttk.Scrollbar(self, orient="horizontal", command=self.text_area.xview)
        horizontal_scroll.pack(side="bottom", fill="x")
        
        # Konfiguriere Text Widget für Scrollbars
        self.text_area.configure(
            yscrollcommand=vertical_scroll.set,
            xscrollcommand=horizontal_scroll.set
        )
    
    def create_status_bar(self):
        self.status_bar = ttk.Label(self, text="Bereit")
        self.status_bar.pack(side="bottom", fill="x")
    
    def configure_language_selection(self):
        # Erstelle Combobox für Sprachauswahl
        languages = sorted([lexer[0] for lexer in get_all_lexers()])
        self.language_var = tk.StringVar(value="Python")
        
        language_frame = ttk.Frame(self)
        language_frame.pack(side="top", fill="x")
        
        ttk.Label(language_frame, text="Sprache:").pack(side="left", padx=5)
        language_combo = ttk.Combobox(
            language_frame, 
            textvariable=self.language_var,
            values=languages,
            state="readonly"
        )
        language_combo.pack(side="left", padx=5)
        
        # Binde Event
        language_combo.bind("<<ComboboxSelected>>", self.change_language)
    
    def change_language(self, event):
        try:
            self.text_area.current_lexer = get_lexer_by_name(
                self.language_var.get().lower()
            )
            self.text_area.highlight_syntax()
        except:
            messagebox.showerror(
                "Fehler",
                f"Lexer für {self.language_var.get()} nicht gefunden."
            )
    
    def new_file(self):
        self.text_area.delete("1.0", "end")
        self.title("Python Text Editor - Neue Datei")
    
    def open_file(self):
        file_path = filedialog.askopenfilename(
            defaultextension=".txt",
            filetypes=[
                ("Alle Dateien", "*.*"),
                ("Text Dateien", "*.txt"),
                ("Python Dateien", "*.py")
            ]
        )
        
        if file_path:
            try:
                with open(file_path, 'r') as file:
                    content = file.read()
                    self.text_area.delete("1.0", "end")
                    self.text_area.insert("1.0", content)
                    self.title(f"Python Text Editor - {file_path}")
                    self.text_area.highlight_syntax()
            except Exception as e:
                messagebox.showerror("Fehler", f"Fehler beim Öffnen der Datei: {str(e)}")
    
    def save_file(self):
        file_path = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[
                ("Alle Dateien", "*.*"),
                ("Text Dateien", "*.txt"),
                ("Python Dateien", "*.py")
            ]
        )
        
        if file_path:
            try:
                content = self.text_area.get("1.0", "end-1c")
                with open(file_path, 'w') as file:
                    file.write(content)
                self.title(f"Python Text Editor - {file_path}")
            except Exception as e:
                messagebox.showerror("Fehler", f"Fehler beim Speichern der Datei: {str(e)}")
    
    def start_macro_recording(self):
        self.text_area.macro.start_recording()
        self.status_bar.config(text="Makro-Aufnahme läuft...")
    
    def stop_macro_recording(self):
        self.text_area.macro.stop_recording()
        self.status_bar.config(text="Makro-Aufnahme beendet")
    
    def play_macro(self):
        self.text_area.macro.play(self.text_area)
        self.status_bar.config(text="Makro abgespielt")

if __name__ == "__main__":
    editor = TextEditor()
    editor.mainloop()