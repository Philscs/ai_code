import tkinter as tk
from tkinter import messagebox

class TextEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Text Editor")
        self.text_area = tk.Text(self.root, undo=True)
        self.text_area.pack(fill=tk.BOTH, expand=True)
        self.text_area.bind("<Control-y>", self.redo)
        self.text_area.bind("<Control-Y>", self.redo)
        self.text_area.bind("<Control-z>", self.undo)
        self.text_area.bind("<Control-Z>", self.undo)
        self.text_area.bind("<Tab>", self.auto_indent)
        self.text_area.bind("<Key>", self.record_macro)
        self.macro = ""

    def undo(self, event=None):
        try:
            self.text_area.edit_undo()
        except tk.TclError:
            messagebox.showinfo("Info", "Nothing to undo")

    def redo(self, event=None):
        try:
            self.text_area.edit_redo()
        except tk.TclError:
            messagebox.showinfo("Info", "Nothing to redo")

    def auto_indent(self, event=None):
        current_line = self.text_area.get("insert linestart", "insert lineend")
        indent = len(current_line) - len(current_line.lstrip())
        self.text_area.insert("insert", "\n" + " " * indent)

    def record_macro(self, event=None):
        if event.char.isprintable():
            self.macro += event.char

    def play_macro(self):
        self.text_area.insert("insert", self.macro)

if __name__ == "__main__":
    root = tk.Tk()
    editor = TextEditor(root)
    root.mainloop()
