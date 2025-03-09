import tkinter as tk
from tkinter import filedialog, messagebox
from tkinter.font import Font
import re

class TextEditor:
    def __init__(self):
        self.root = tk.Tk()
        self.text_area = tk.Text(self.root)
        self.text_area.pack(fill=tk.BOTH, expand=1)

        self.font = Font(family="Consolas", size=10)
        self.text_area.tag_config('comment', foreground='green')
        self.text_area.tag_config('keyword', foreground='blue')
        self.text_area.tag_remove('code_block')

        self.undo_stack = []
        self.redo_stack = []

        self.macro_name = tk.StringVar()
        self.macro_label = tk.Label(self.root, text="Makro-Name:")
        self.macro_label.pack()

        self.macro_entry = tk.Entry(self.root, textvariable=self.macro_name)
        self.macro_entry.pack()

        self.record_button = tk.Button(self.root, text="Aufzeichnen", command=self.record_macro)
        self.record_button.pack()

        self.play_button = tk.Button(self.root, text="Spielen", command=self.play_macro)
        self.play_button.pack()

        self.save_button = tk.Button(self.root, text="Speichern", command=self.save_text)
        self.save_button.pack()

        self.open_button = tk.Button(self.root, text="Öffnen", command=self.open_file)
        self.open_button.pack()

    def open_file(self):
        file_path = filedialog.askopenfilename(title="Datei auswählen")
        if file_path:
            with open(file_path, 'r') as f:
                self.text_area.delete(1.0, tk.END)
                self.text_area.insert(tk.END, f.read())

    def save_text(self):
        file_path = filedialog.asksaveasfilename(title="Datei speichern", 
defaultextension=".txt")
        if file_path:
            with open(file_path, 'w') as f:
                f.write(self.text_area.get(1.0, tk.END))

    def undo(self):
        if self.undo_stack:
            self.redo_stack.append(self.undo_stack.pop())
        else:
            messagebox.showerror("Fehler", "Keine Änderungen")

    def redo(self):
        if self.redo_stack:
            self.undo_stack.append(self.redo_stack.pop())
        else:
            messagebox.showerror("Fehler", "Keine Änderungen")

    def format_text(self, text):
        lines = text.split('\n')
        formatted_lines = []
        for line in lines:
            # Syntax-Highlighting für verschiedene Programmiersprachen
            if re.match(r'#.*', line):
                formatted_line = f"##{line.strip()}"
            elif re.match(r'\bif\b|if\s*\w+\s*:', line):
                formatted_line = f"## IF {line.strip().split(' ')[1]}"
            elif re.match(r'\bdef\b|def\s+\w+\s*\(', line):
                formatted_line = f"## DEF {line.strip().split('(')[0].strip()}"
            else:
                formatted_line = line
            formatted_lines.append(formatted_line)
        return '\n'.join(formatted_lines)

    def record_macro(self):
        self.undo_stack.clear()
        macro_text = self.text_area.get(1.0, tk.END)
        self.redo_stack.append(macro_text)

    def play_macro(self):
        if len(self.undo_stack) > 0:
            self.undo_stack.pop()
        else:
            messagebox.showerror("Fehler", "Keine Änderungen")
        macro_text = self.redo_stack.pop()
        self.text_area.delete(1.0, tk.END)
        self.text_area.insert(tk.END, macro_text)

    def auto_indent(self):
        lines = self.text_area.get(1.0, tk.END).split('\n')
        formatted_lines = []
        for line in lines:
            indentation = len(line) - len(line.lstrip())
            new_line = ' ' * (indentation // 4)
            formatted_lines.append(new_line + line)
        return '\n'.join(formatted_lines)

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    editor = TextEditor()
    editor.run()
