import threading
from typing import List, Any

import sys
import time

MIN_PROGRESS_PRINT_DELAY = 0.03


class ThrottledProgressPrinter:
    def __init__(self, *, outs: List[Any], print_progress: bool):
        self.outs = outs
        self.print_progress = print_progress
        self.next_can_print_time = time.monotonic()
        self.latest_msg = ''
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
            self.next_can_print_time = t + MIN_PROGRESS_PRINT_DELAY
            self.is_worker_running = False
            print(self.latest_msg, file=sys.stderr, flush=True)
        return max(dt, 0)

    def _print_worker(self):
        while True:
            with self.lock:
                dt = self._try_print_else_delay()
            if dt == 0:
                break
            time.sleep(dt)
