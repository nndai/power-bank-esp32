import websocket
import os
import time
from tqdm import tqdm
import json

try:
    ws = websocket.create_connection("ws://192.168.137.194:82", timeout=3)
except websocket.WebSocketConnectionClosedException:
    print("Connection closed. Please check the server status.")
    exit(1)
except websocket.WebSocketTimeoutException:
    print("Connection timed out. Please check the server status.")
    exit(1)
except TimeoutError:
    print("Socket timeout. Please check the server IP/port and network.")
    exit(1)
except OSError as e:
    print(f"OS error: {e}")
    exit(1)
    

    
FILE_PATH = ".pio/build/esp32dev/firmware.bin"
file_size = os.path.getsize(FILE_PATH)

json_data = {
    "key": "uploadFirmwareStart",
    "size": file_size
}

ws.send_text(json.dumps(json_data))
time.sleep(1)

with open(FILE_PATH, "rb") as f, tqdm(total=file_size, unit='B', unit_scale=True, desc="Uploading") as pbar:
    while True:
        data = f.read(1024)
        if not data:
            break
        ws.send_binary(data)
        pbar.update(len(data))

time.sleep(1)

json_data = {
    "key": "uploadFirmwareEnd"
}
ws.send(json.dumps(json_data))

ws.settimeout(7)

try:
    json_respone_data = ws.recv()
    if json_respone_data:
        if json.loads(json_respone_data).get("key") == "uploadFirmwareSuccess":
            print("Firmware upload successful.")
            os.system("python C:\\Users\\daila\\Documents\\PlatformIO\\Projects\\PowerBankESP32\\test\\serial_monitor.py")
        else:
            print("Firmware upload failed.")
    else:
        print("No response from server after firmware upload.")
    
except websocket._exceptions.WebSocketTimeoutException:
    print("No response from server after firmware upload.")


