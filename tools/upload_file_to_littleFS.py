import os
import json
import time
import websocket

class Uploader:
    def __init__(self, url, local_path, remote_path, key_open, key_end, chunk_size):
        self.url = url
        self.local_path = local_path
        self.remote_path = remote_path
        self.key_open = key_open
        self.key_end = key_end
        self.chunk_size = chunk_size
        self.ws = None
        self.total_size = 0
        self.sent = 0
        self.opened = False
        self.failed = False

    def on_message(self, ws, message):
        if isinstance(message, bytes):
            # Not expecting binary from device during upload; ignore or log
            return
        try:
            data = json.loads(message)
        except Exception:
            print("[!] Non-JSON text message:", message)
            return

        key = data.get("key") or data.get("cmd")
        if key in ("openFileSuccess", "writeOpenSuccess"):
            self.opened = True
            print("[+] Remote file opened for writing")
        elif key in ("openFileFailed", "writeOpenFailed"):
            self.failed = True
            print("[-] Failed to open remote file for writing")
        elif key in ("writeFileSuccess", "writeSuccess"):
            print("[+] Upload completed successfully")
        elif key in ("writeFileFailed", "writeFailed"):
            self.failed = True
            print("[-] Upload failed on device")
        # else: other informational messages ignored

    def on_open(self, ws):
        self.ws = ws
        self.total_size = os.path.getsize(self.local_path)
        req = {
            "key": self.key_open,
            "name": self.remote_path,
            "size": self.total_size,
        }
        ws.send(json.dumps(req))
        print(f"[*] Request open: {self.remote_path} ({self.total_size} bytes)")

    def on_error(self, ws, error):
        print("[!] WebSocket error:", error)
        self.failed = True

    def on_close(self, ws, code, reason):
        print("[*] WebSocket closed", code, reason)

    def run(self, timeout):
        ws = websocket.WebSocketApp(
            self.url,
            on_open=self.on_open,
            on_message=self.on_message,
            on_error=self.on_error,
            on_close=self.on_close,
        )

        # Run in thread; simple wait loop for open then stream
        import threading
        t = threading.Thread(target=ws.run_forever, kwargs={"ping_timeout": timeout})
        t.daemon = True
        t.start()

        # Wait for open or failure
        start = time.time()
        while not self.opened and not self.failed and (time.time() - start < timeout):
            time.sleep(0.05)

        if not self.opened or self.failed:
            ws.close()
            raise SystemExit(1)

        # Start streaming file
        with open(self.local_path, "rb") as f:
            self.sent = 0
            while True:
                chunk = f.read(self.chunk_size)
                if not chunk:
                    break
                try:
                    ws.send(chunk, opcode=websocket.ABNF.OPCODE_BINARY)
                except Exception as e:
                    print("[!] Send chunk failed:", e)
                    self.failed = True
                    break
                self.sent += len(chunk)
                pct = (self.sent * 100) // self.total_size if self.total_size else 100
                print(f"[=] {self.sent}/{self.total_size} ({pct}%)", end="\r")

        # Signal end
        try:
            ws.send(json.dumps({"key": self.key_end}))
        except Exception as e:
            print("[!] Send end failed:", e)
            self.failed = True

        # Wait a short time for device confirmation
        time.sleep(1.0)
        ws.close()
        t.join(timeout=2.0)


if __name__ == "__main__":
    uploader = Uploader(
        url="ws://192.168.137.194:82",
        local_path="tools/output_videos/turnOff.bin",
        remote_path="/videos/turnOff.vd",
        key_open="writeFileToLittleFS",
        key_end="writeFileToLittleFSEnd",
        chunk_size=1024,
    )
    uploader.run(30)
    if uploader.failed:
        raise SystemExit(1)
    print("\n[+] Done")


