#!/usr/bin/env python3
import socket
import threading

class LineClient:
    def __init__(self):
        self.sock = None
        self.lock = threading.Lock()

    def connect(self, host: str, port: int, timeout=3.0):
        self.close()
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((host, port))
        s.settimeout(None)
        self.sock = s
        # read greeting
        return self.read_line()

    def send_line(self, line: str):
        if not self.sock:
            raise RuntimeError("not connected")
        data = (line + "\n").encode("utf-8")
        with self.lock:
            self.sock.sendall(data)

    def read_line(self) -> str:
        if not self.sock:
            raise RuntimeError("not connected")
        buf = bytearray()
        while True:
            b = self.sock.recv(1)
            if not b:
                raise RuntimeError("disconnected")
            if b == b"\n":
                break
            if b != b"\r":
                buf.extend(b)
        return buf.decode("utf-8")

    def request_reply(self, line: str) -> str:
        self.send_line(line)
        return self.read_line()

    def close(self):
        if self.sock:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

