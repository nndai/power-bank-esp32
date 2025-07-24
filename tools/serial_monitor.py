import websocket
import os
import time
import json

def on_message(ws, message):
    print(message, end='')

def on_error(ws, error):
    print("Error:", error)

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

def on_open(ws):
    print("Connected to ESP32 WebSocket")
    json_data = {
        "key": "initSerialMonitor",
    }
    ws.send(json.dumps(json_data))

if __name__ == "__main__":
    while True:
        try:
            ws = websocket.WebSocketApp(
                "ws://192.168.137.194:82",
                on_open=on_open,
                on_message=on_message,
                on_error=on_error,
                on_close=on_close
            )
            ws.run_forever()
        except websocket.WebSocketConnectionClosedException:
            time.sleep(1)
            print("Connection closed. Please check the server status.")