import threading
from typing import List, Any

import sys
import time


class ThrottledProgressPrinter:
    """Handles printing progress updates interspersed amongst output.

    Throttles the progress updates to not flood the screen when 100 show up
    at the same time, and instead only show the latest one.
    """
    def __init__(self, *, outs: List[Any], print_progress: bool, min_progress_delay: float):
        self.outs = outs
        self.print_progress = print_progress
        self.next_can_print_time = time.monotonic()
        self.latest_msg = ''
        self.min_progress_delay = min_progress_delay
        self.is_worker_running = False
        self.lock = threading.Lock()

    def print_out(self, msg: str) -> None:
        with self.lock:
            for out in self.outs:
                print(msg, file=out, flush=True)

    def show_latest_progress(self, msg: str) -> None:
        if not self.print_progress:
            return
        with self.lock:
            if msg == self.latest_msg:
                return
            self.latest_msg = msg
            if not self.is_worker_running:
                dt = self._try_print_else_delay()
                if dt > 0:
                    self.is_worker_running = True
                    threading.Thread(target=self._print_worker).start()

    def _try_print_else_delay(self) -> float:
        t = time.monotonic()
        dt = self.next_can_print_time - t
        if dt <= 0:
            self.next_can_print_time = t + self.min_progress_delay
            self.is_worker_running = False
            if self.latest_msg != "":
                print('\033[31m' + self.latest_msg + '\033[0m', file=sys.stderr, flush=True)
        return max(dt, 0)

    def _print_worker(self):
        while True:
            with self.lock:
                dt = self._try_print_else_delay()
            if dt == 0:
                break
            time.sleep(dt)
