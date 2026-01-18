import tkinter as tk
from tkinter import scrolledtext
import traceback
import threading
import sys

class TracebackWindow:
    def __init__(self, title: str=None, stack_trace: str=None, critical: bool=False, e: Exception=None):
        if stack_trace:
            self.stack_trace = stack_trace
        else:
            self.stack_trace = traceback.format_exc()
        self.critical = critical
        self.exception = e
        self.create_window(title=title)

    def create_window(self, title: str):
        
        self.window = tk.Tk()
        if title:
            title = f"EXCEPTION: {title}"
        else:
            title = "EXCEPTION"

        self.window.title(title)

        self.window.resizable(True, True)
        text_area = scrolledtext.ScrolledText(self.window, wrap=tk.WORD, width=100, height=30)
        text_area.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)
        text_area.insert(tk.END, self.stack_trace)
        text_area.configure(state='disabled')

        print(f"{title}\n{self.stack_trace}")

        self.window.protocol("WM_DELETE_WINDOW", self.on_close)

        # Maker sure the window is displayed if exception isn't trigged in a thread.
        self.window.mainloop()

    def on_close(self):
        self.window.destroy()
        if self.critical and self.exception != SystemExit:
            sys.exit(0)

class Thread(threading.Thread):
    def __init__(self, *args, **kwargs):
        self.title = kwargs.pop('title')
        self.stack_trace = None
        self.exception = None
        super().__init__(*args, **kwargs)

    def run(self):
        try:
            if self._target:
                self._target(*self._args, **self._kwargs)
        except Exception as e:
            self.stack_trace = traceback.format_exc()
            self.exception = e
        finally:
            pass

    def check_for_exceptions(self):
        # print(f"Checking {self.title} : {self.stack_trace}")
        if self.stack_trace:
            TracebackWindow(title=self.title, stack_trace=self.stack_trace, critical=True, e=self.exception)
            self.stack_trace = None
            self.e = None
            return True
        else:
            return False
