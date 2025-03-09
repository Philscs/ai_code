import tkinter as tk
from tkinter import filedialog, messagebox
from tkinter.scrolledtext import ScrolledText
from pygments import highlight
from pygments.lexers import guess_lexer, get_lexer_by_name
from pygments.formatters import TkFormatter

class SyntaxHighlightingEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Texteditor mit Syntax-Highlighting")

        # Create ScrolledText widget
        self.text_widget = ScrolledText(root, wrap=tk.WORD, undo=True)
        self.text_widget.pack(fill=tk.BOTH, expand=True)
        self.text_widget.bind("<KeyRelease>", self.syntax_highlight)
        self.text_widget.bind("<Return>", self.auto_indent)

        # Menubar
        self.menu = tk.Menu(self.root)
        self.root.config(menu=self.menu)
        self.create_menus()

        # Variables for macros
        self.macro_recording = False
        self.macro_actions = []

    def create_menus(self):
        file_menu = tk.Menu(self.menu, tearoff=False)
        file_menu.add_command(label="Öffnen", command=self.open_file)
        file_menu.add_command(label="Speichern", command=self.save_file)
        file_menu.add_command(label="Speichern unter", command=self.save_file_as)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.root.quit)
        
        edit_menu = tk.Menu(self.menu, tearoff=False)
        edit_menu.add_command(label="Rückgängig", command=self.text_widget.edit_undo)
        edit_menu.add_command(label="Wiederholen", command=self.text_widget.edit_redo)
        
        macro_menu = tk.Menu(self.menu, tearoff=False)
        macro_menu.add_command(label="Makro aufzeichnen", command=self.record_macro)
        macro_menu.add_command(label="Makro abspielen", command=self.play_macro)

        self.menu.add_cascade(label="Datei", menu=file_menu)
        self.menu.add_cascade(label="Bearbeiten", menu=edit_menu)
        self.menu.add_cascade(label="Makros", menu=macro_menu)

    def open_file(self):
        file_path = filedialog.askopenfilename()
        if file_path:
            with open(file_path, "r") as file:
                content = file.read()
                self.text_widget.delete(1.0, tk.END)
                self.text_widget.insert(tk.END, content)
            self.syntax_highlight()

    def save_file(self):
        file_path = filedialog.asksaveasfilename(defaultextension=".txt")
        if file_path:
            with open(file_path, "w") as file:
                file.write(self.text_widget.get(1.0, tk.END))

    def save_file_as(self):
        self.save_file()

    def syntax_highlight(self, event=None):
        try:
            content = self.text_widget.get(1.0, tk.END)
            lexer = guess_lexer(content)
            formatted_code = highlight(content, lexer, TkFormatter(style="default", bg="default"))
            self.text_widget.mark_set("insert", tk.END)
            self.text_widget.insert(tk.END, formatted_code)
        except Exception as e:
            print("Error in syntax highlighting:", e)

    def auto_indent(self, event):
        current_line = self.text_widget.get("insert linestart", "insert")
        indentation = " " * (len(current_line) - len(current_line.lstrip()))
        self.text_widget.insert("insert", "\n" + indentation)
        return "break"

    def record_macro(self):
        if self.macro_recording:
            self.macro_recording = False
            messagebox.showinfo("Makro", "Aufzeichnung beendet.")
        else:
            self.macro_recording = True
            self.macro_actions = []
            messagebox.showinfo("Makro", "Aufzeichnung gestartet.")

    def play_macro(self):
        for action in self.macro_actions:
            self.text_widget.insert(tk.END, action)

if __name__ == "__main__":
    root = tk.Tk()
    editor = SyntaxHighlightingEditor(root)
    root.mainloop()
