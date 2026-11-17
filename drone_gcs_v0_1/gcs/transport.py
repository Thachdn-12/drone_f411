"""Serial I/O and heartbeat live in a dedicated worker, with a GUI lease."""
import queue
import threading
import time

import serial
from PySide6.QtCore import QThread, Signal

from .protocol import encode


class SerialWorker(QThread):
    opened = Signal()
    received = Signal(bytes)
    transmitted = Signal(bytes)
    failed = Signal(str)

    def __init__(self, name, parent=None):
        super().__init__(parent)
        self.name = name
        self.outgoing = queue.PriorityQueue(maxsize=64)
        self.stopping = threading.Event()
        self.lock = threading.Lock()
        self.sequence = 0
        self.lease = time.monotonic()
        self.lease_expired = False
        self.abort_epoch = 0
        self.connected = False

    def touch(self):
        self.lease = time.monotonic()

    def send(self, message, urgent=False):
        with self.lock:
            if self.stopping.is_set():
                raise RuntimeError('UART is closing')
            if urgent:
                self.abort_epoch += 1
                while True:
                    try:
                        self.outgoing.get_nowait()
                    except queue.Empty:
                        break
            self.sequence = (self.sequence + 1) & 0xffffffff
            seq = self.sequence
            self.outgoing.put_nowait((0 if urgent else 1, seq, self.abort_epoch, encode(message, seq)))
            return seq

    def stop(self):
        self.stopping.set()
        self.wait(500)

    def run(self):
        port = None
        heartbeat_at = 0.0
        try:
            port = serial.Serial(self.name, 115200, timeout=.01, write_timeout=.02)
            self.connected = True
            self.opened.emit()
            while not self.stopping.is_set():
                now = time.monotonic()
                if now - self.lease > 1:
                    if not self.lease_expired:
                        self.send({'type': 'disarm'}, urgent=True)
                        self.lease_expired = True
                elif now - heartbeat_at >= .2 and not self.lease_expired:
                    self.send({'type': 'heartbeat'})
                    heartbeat_at = now
                try:
                    _, _, epoch, packet = self.outgoing.get_nowait()
                    if epoch == self.abort_epoch:
                        if port.write(packet) != len(packet):
                            raise serial.SerialException('Incomplete serial write')
                        self.transmitted.emit(packet)
                except queue.Empty:
                    pass
                data = port.read(min(max(port.in_waiting, 1), 4096))
                if data:
                    self.received.emit(data)
        except (serial.SerialException, OSError, RuntimeError, queue.Full) as exc:
            self.failed.emit(str(exc))
        finally:
            self.connected = False
            if port:
                try:
                    # Stop outputs on an orderly close; the firmware watchdog also remains active.
                    self.sequence = (self.sequence + 1) & 0xffffffff
                    port.write(encode({'type': 'disarm'}, self.sequence))
                except (serial.SerialException, OSError):
                    pass
                try:
                    port.close()
                except (serial.SerialException, OSError):
                    pass
