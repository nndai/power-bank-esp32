import websocket
import json

# Địa chỉ WebSocket của ESP32
WS_URL = "ws://192.168.137.194:82"   # chỉnh IP/port theo ESP32 của bạn

def on_message(ws, message):
    # Nếu là binary thì websocket-client tự đưa dạng bytes
    if isinstance(message, bytes):
        # Ghi chunk vào file
        with open("received_file.bin", "ab") as f:
            f.write(message)
    else:
        # Xử lý tin nhắn text (JSON)
        try:
            data = json.loads(message)
            key = data.get("key")
            if key == "openFileSuccess":
                print("[+] File opened, start receiving...")
                # Xóa file cũ
                open("received_file.bin", "wb").close()
            elif key == "openFileFailed":
                print("[-] Failed to open file on ESP32")
            elif key == "readFileSuccess":
                print("[+] File fully received!")
        except Exception as e:
            print("[!] Error parsing message:", e, message)

def on_open(ws):
    # Gửi yêu cầu đọc file
    request = {
        "key": "readFileFromLittleFS",
        "name": "/ip5356_write.log"  # Tên file muốn đọc
    }
    ws.send(json.dumps(request))
    print("[*] Sent request to read file")

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

if __name__ == "__main__":
    ws = websocket.WebSocketApp(
        WS_URL,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close
    )
    ws.run_forever()
