import customtkinter
import websocket
import threading
import json
import time
import queue
import os
import re


class WebSocketApp(customtkinter.CTk):
    def __init__(self):
        super().__init__()
        self.title("ESP32 WebSocket Monitor & Uploader")
        self.geometry("800x600")

        #self.websocket_url = "ws://192.168.1.184:81"
        self.websocket_url = "ws://192.168.137.194:82"
        self.ws = None
        self.message_queue = queue.Queue()
        self.is_connected = False
        self.is_uploading = False
        self.is_display_error = False

        self.upload_failed_event = threading.Event()

        self.grid_columnconfigure(0, weight=1)
        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=0)
        self.grid_rowconfigure(2, weight=0)
        self.grid_rowconfigure(3, weight=0)

        self.message_display = customtkinter.CTkTextbox(self, wrap="word", font=("JetbrainMono", 14))
        self.message_display.grid(row=0, column=0, padx=20, pady=20, sticky="nsew")
        self.message_display.insert("end", "Initializing application...\n")
        self.message_display.configure(state="disabled")

        self.message_display._textbox.tag_configure("error", foreground="red")
        self.message_display._textbox.tag_configure("warning", foreground="orange")
        self.message_display._textbox.tag_configure("info", foreground="cyan")
        self.message_display._textbox.tag_configure("debug", foreground="gray")
        self.message_display._textbox.tag_configure("connection_status", foreground="green")
        self.message_display._textbox.tag_configure("upload", foreground="magenta")
        self.message_display._textbox.tag_configure("success", foreground="green")

        self.input_frame = customtkinter.CTkFrame(self)
        self.input_frame.grid(row=1, column=0, padx=20, pady=(0, 20), sticky="ew")
        self.input_frame.grid_columnconfigure(0, weight=1)

        self.input_frame.grid_columnconfigure(1, weight=0)
        self.input_frame.grid_columnconfigure(2, weight=0)

        self.input_entry = customtkinter.CTkEntry(self.input_frame, placeholder_text="Enter message to send")
        self.input_entry.grid(row=0, column=0, padx=(8, 9), pady=8, sticky="ew")
        self.input_entry.bind("<Return>", self.send_message_on_enter)

        self.send_button = customtkinter.CTkButton(self.input_frame, text="Send", font=("JetbrainMono", 12, "bold"), command=self.send_message)
        self.send_button.grid(row=0, column=1, padx=(0, 0), pady=8, sticky="e")
        self.send_button.configure(state="disabled")

        self.upload_button = customtkinter.CTkButton(self.input_frame, text="Upload Firmware", font=("JetbrainMono", 12, "bold"), command=self.start_firmware_upload)
        self.upload_button.grid(row=0, column=2, padx=(10, 9), pady=8, sticky="e")
        self.upload_button.configure(state="disabled")

        self.progress_frame = customtkinter.CTkFrame(self)
        self.progress_frame.grid(row=2, column=0, padx=20, pady=(0, 20), sticky="ew")
        self.progress_frame.grid_columnconfigure(0, weight=1)
        self.progress_frame.grid_columnconfigure(1, weight=0)
        self.progress_frame.grid_remove()

        self.progress_bar = customtkinter.CTkProgressBar(self.progress_frame, orientation="horizontal", height=13)
        self.progress_bar.grid(row=0, column=0, padx=(15, 10), pady=5, sticky="ew")
        self.progress_bar.set(0)
        self.progress_bar.configure(mode="determinate")

        self.upload_progress_label = customtkinter.CTkLabel(self.progress_frame, text="0% (0B/0B) 0B/s", font=("JetbrainMono", 12, "bold") )
        self.upload_progress_label.grid(row=0, column=1, padx=(10, 15), pady=5, sticky="e")

        self.status_label = customtkinter.CTkLabel(self, text="Status: Disconnected", text_color="red")
        self.status_label.grid(row=3, column=0, padx=10, pady=(0, 10), sticky="w")

        self.websocket_thread = threading.Thread(target=self.run_websocket, daemon=True)
        self.websocket_thread.start()
        self.after(50, self.check_message_queue)

    def on_ws_message(self, ws, message):
        """Callback for when a message is received from the main WebSocket.
        Special handling for upload failure messages that might come asynchronously."""

        if self.is_uploading:
            try:
                json_message = json.loads(message)
                if json_message.get("key") == "writeFirmwareFailed":
                    error_detail = json_message.get("error", "Unknown error during firmware write")
                    self.message_queue.put(f"[ERROR] Firmware write failed: {error_detail}\n")
                    self.upload_failed_event.set()
                    return

            except json.JSONDecodeError:
                pass
        self.message_queue.put(message)

    def on_ws_error(self, ws, error):
        """Callback for when a WebSocket error occurs on the main monitoring connection."""
        if self.is_display_error == False:
            self.message_queue.put(f"[ERROR] WebSocket connection error: {error}")
        self.is_connected = False
        if self.is_uploading:
            self.upload_failed_event.set()
            self.message_queue.put("[ERROR] Upload aborted due to connection error.\n")

        self.after(0, lambda: self.update_status("Disconnected", "red"))
        self.after(0, lambda: self.send_button.configure(state="disabled"))
        self.after(0, lambda: self.upload_button.configure(state="disabled"))

    def on_ws_close(self, ws, close_status_code, close_msg):
        """Callback for when the main WebSocket connection is closed."""
        if self.is_display_error == False:
            self.message_queue.put("[INFO] Connection closed. Attempting to reconnect...\n")
            self.is_display_error = True
        self.is_connected = False
        if self.is_uploading:
            self.upload_failed_event.set()
            self.message_queue.put("[ERROR] Upload aborted due to connection closed.\n")

        self.after(0, lambda: self.update_status("Disconnected", "red"))
        self.after(0, lambda: self.send_button.configure(state="disabled"))
        self.after(0, lambda: self.upload_button.configure(state="disabled"))

    def on_ws_open(self, ws):
        """Callback for when the main WebSocket connection is successfully opened."""
        self.message_queue.put("[INFO] Connected to ESP32 WebSocket\n")
        self.is_connected = True
        self.is_display_error = False

        self.after(0, lambda: self.update_status("Connected", "green"))
        self.after(0, lambda: self.send_button.configure(state="normal"))
        self.after(0, lambda: self.upload_button.configure(state="normal"))

        json_data = {
            "key": "initSerialMonitor",
        }
        try:
            ws.send(json.dumps(json_data))
            self.message_queue.put("[INFO] Sent 'initSerialMonitor' command.\n")
        except Exception as e:
            self.message_queue.put(f"[ERROR] Failed to send initSerialMonitor: {e}\n")

    def run_websocket(self):
        """Function to run the main WebSocket connection in a separate thread for monitoring."""
        while True:
            try:
                self.ws = websocket.WebSocketApp(
                    self.websocket_url,
                    on_open=self.on_ws_open,
                    on_message=self.on_ws_message,
                    on_error=self.on_ws_error,
                    on_close=self.on_ws_close
                )
                self.ws.run_forever(ping_interval=10, ping_timeout=5)
            except Exception as e:

                self.message_queue.put(f"[ERROR] WebSocket connection error: {e}. Retrying in 5 seconds...\n")
                self.is_connected = False
                self.after(0, lambda: self.update_status("Disconnected", "red"))
                self.after(0, lambda: self.send_button.configure(state="disabled"))
                self.after(0, lambda: self.upload_button.configure(state="disabled"))
                time.sleep(5)
            finally:
                if self.ws:
                    self.ws.close()

    def check_message_queue(self):
        """Periodically checks the message queue and updates the GUI textbox.
        Handles '\r' for overwriting current line and '\n' for new lines.
        Applies color tags based on message prefix (e.g., [ERROR], [WARNING])."""
        while not self.message_queue.empty():
            message = self.message_queue.get()
            self.message_display.configure(state="normal")
            is_at_bottom = self.message_display._textbox.yview()[1] >= 0.99 
            
            tags_to_apply = []
            if message.startswith("[ERROR]"):
                tags_to_apply.append("error")
            elif message.startswith("[WARNING]"):
                tags_to_apply.append("warning")
            elif message.startswith("[INFO]"):
                tags_to_apply.append("info")
            elif message.startswith("[DEBUG]"):
                tags_to_apply.append("debug")
            elif message.startswith("[SUCCESS]"):
                tags_to_apply.append("success")
            elif "Sent:" in message or "Connected to ESP32" in message or "Connection closed." in message:
                 tags_to_apply.append("connection_status")
            elif "Firmware upload" in message or "Sent 'uploadFirmwareEnd'" in message:
                 tags_to_apply.append("upload")

            if '\r' in message:
                try:
                    last_line_start_index = self.message_display.index("end-1c linestart")
                    self.message_display.delete(last_line_start_index, "end")
                except Exception:
                    pass
                self.message_display._textbox.insert("end", message.replace('\r', ''), *tags_to_apply)
                
            else:
                self.message_display._textbox.insert("end", message, *tags_to_apply)
                if not message.endswith('\n'):
                    self.message_display._textbox.insert("end", "\n")
                    
            if is_at_bottom:
                self.message_display.see("end") 
            self.message_display.configure(state="disabled")
        self.after(100, self.check_message_queue)
        
    def replace_hex_with_int(self, text: str) -> str:
        pattern = r"0x[0-9A-Fa-f]{1,2}"
        def hex_to_int(match):
            return str(int(match.group(), 16))
        return re.sub(pattern, hex_to_int, text)
    
    def send_message_on_enter(self, event=None):
        """Wrapper method to call send_message when Enter key is pressed."""
        self.send_message()

    def send_message(self):
        """Function to send a message via WebSocket when the 'Send' button is clicked."""
        if self.is_connected and self.ws and not self.is_uploading:
            message = self.input_entry.get()
            if message:
                try:
                    self.ws.send(self.replace_hex_with_int(message))
                    self.message_display.configure(state="normal")
                    self.message_display._textbox.insert("end", f"Sent: {message}\n", "connection_status")
                    self.message_display.see("end")
                    self.message_display.configure(state="disabled")

                except Exception as e:
                    self.message_display.configure(state="normal")
                    self.message_display._textbox.insert("end", f"[ERROR] Failed to send message: {e}\n", "error")
                    self.message_display.see("end")
                    self.message_display.configure(state="disabled")
        else:
            self.message_display.configure(state="normal")
            if self.is_uploading:
                self.message_display._textbox.insert("end", "[WARNING] Cannot send: Upload in progress. Please wait.\n", "warning")
            else:
                self.message_display._textbox.insert("end", "[ERROR] Cannot send: Not connected to WebSocket.\n", "error")
            self.message_display.see("end")
            self.message_display.configure(state="disabled")

    def start_firmware_upload(self):
        """Initiates the firmware upload process in a new thread."""
        if not self.is_connected:
            self.message_queue.put("[WARNING] Cannot start upload: Not connected to ESP32 WebSocket.\n")
            return
        if self.is_uploading:
            self.message_queue.put("[WARNING] Cannot start upload: Another upload is already in progress.\n")
            return

        FILE_PATH = ".pio/build/esp32dev/firmware.bin"
        if not os.path.exists(FILE_PATH):
            self.message_queue.put(f"[ERROR] Firmware file not found: {FILE_PATH}\n")
            return

        self.is_uploading = True
        self.upload_failed_event.clear()
        self.send_button.configure(state="disabled")
        self.upload_button.configure(text="Stop Upload", command=self.stop_firmware_upload, state="normal")
        self.progress_bar.set(0)
        self.upload_progress_label.configure(text="0% (0B/0B) 0B/s")
        self.progress_frame.grid()

        upload_thread = threading.Thread(target=self.run_firmware_upload, args=(FILE_PATH,))
        upload_thread.daemon = True
        upload_thread.start()

    def stop_firmware_upload(self):
        """Manually stops the firmware upload process."""
        if self.is_uploading:
            self.message_queue.put("[WARNING] Firmware upload manually aborted.\n")
            self.upload_failed_event.set()
            self.is_uploading = False
            self.after(0, lambda: self.reset_upload_ui())

    def format_bytes(self, num_bytes):
        """Formats bytes into human-readable units (B, KB, MB)."""
        if num_bytes < 1024:
            return f"{num_bytes}B"
        elif num_bytes < 1024 * 1024:
            return f"{num_bytes / 1024:.1f}KB"
        else:
            return f"{num_bytes / (1024 * 1024):.1f}MB"

    def run_firmware_upload(self, file_path):
        """Handles the actual firmware upload logic in a separate thread.
        This function uses a new, dedicated WebSocket connection for the upload."""
        ws_upload = None
        try:
            self.message_queue.put("[INFO] Establishing dedicated WebSocket connection for upload...\n")
            ws_upload = websocket.create_connection(self.websocket_url, timeout=10)
            self.message_queue.put("[INFO] Dedicated upload WebSocket connected.\n")

            file_size = os.path.getsize(file_path)
            self.message_queue.put(f"[INFO] Starting firmware upload for file: {file_path} (Size: {self.format_bytes(file_size)})\n")

            json_data_start = {
                "key": "otaUploadFirmware",
                "cmd": "uploadFirmwareStart",
                "size": file_size
            }
            ws_upload.send(json.dumps(json_data_start))
            self.message_queue.put("[INFO] Sent 'uploadFirmwareStart' command. Waiting for ESP32's response...\n")

            response_str = ws_upload.recv()
            if not response_str:
                self.message_queue.put("[ERROR] No response from ESP32 after 'uploadFirmwareStart'. Aborting upload.\n")
                self.upload_failed_event.set()
                return
            
            response_json = json.loads(response_str)
            if response_json.get("cmd") == "beginUploadFirmwareSuccess":
                self.message_queue.put("[INFO] ESP32 ready for firmware transfer. Starting data send...\n")
            elif response_json.get("cmd") == "beginUploadFirmwareFailed":
                error_msg = response_json.get("error", "Unknown error")
                self.message_queue.put(f"[ERROR] ESP32 failed to begin upload: {error_msg}. Aborting upload.\n")
                self.upload_failed_event.set()
                return
            else:
                self.message_queue.put(f"[WARNING] Unexpected JSON response after 'uploadFirmwareStart': {response_str}. Aborting upload.\n")
                self.upload_failed_event.set()
                return

            uploaded_bytes = 0
            chunk_size = 1024
            start_time = time.time()
            
            with open(file_path, "rb") as f:
                while True:
                    if self.upload_failed_event.is_set():
                        break

                    data = f.read(chunk_size)
                    if not data:
                        break

                    ws_upload.send_binary(data)
                    uploaded_bytes += len(data)
                    current_time = time.time()
                    elapsed_time = current_time - start_time
                    
                    progress = uploaded_bytes / file_size
                    progress_percent = int(progress * 100)

                    speed_text = "0B/s"
                    if elapsed_time > 0.1:
                        avg_speed_bps = uploaded_bytes / elapsed_time
                        speed_text = f"{self.format_bytes(avg_speed_bps)}/s"

                    self.after(0, lambda p=progress: self.progress_bar.set(p))
                    self.after(0, lambda pp=progress_percent, ub=uploaded_bytes, fs=file_size, spd=speed_text:
                               self.upload_progress_label.configure(text=f"{pp}% [{self.format_bytes(ub)}/{self.format_bytes(fs)}] {spd}"))
                    time.sleep(0.015)

            time.sleep(0.2)

            if not self.upload_failed_event.is_set():
                json_data_end = {
                    "key": "otaUploadFirmware",
                    "cmd": "uploadFirmwareEnd"
                }
                ws_upload.send(json.dumps(json_data_end))
                self.message_queue.put("[INFO] Sent 'uploadFirmwareEnd' command. Waiting for final response...\n")
                
                response_str = ws_upload.recv()
                if not response_str:
                    self.message_queue.put("[ERROR] No response from ESP32 after 'uploadFirmwareEnd'.\n")
                else:
                    response_json = json.loads(response_str)
                    if response_json.get("cmd") == "uploadFirmwareSuccess":
                        self.message_queue.put("[SUCCESS] Upload firmware successful\n")
                        self.is_uploading = False
                    elif response_json.get("cmd") == "uploadFirmwareFailed":
                        error_msg = response_json.get("error", "Unknown error")
                        self.message_queue.put(f"[ERROR] ESP32 upload firmware failed: {error_msg}.\n")
                    else:
                        self.message_queue.put(f"[WARNING] Unexpected JSON response after 'uploadFirmwareEnd': {response_str}.\n")
                        self.upload_failed_event.set()
                    
            else:
                self.message_queue.put("[ERROR] Firmware upload aborted (internal or manual trigger).\n")

        except FileNotFoundError:
            self.message_queue.put(f"[ERROR] Firmware file not found at: {file_path}\n")
            self.upload_failed_event.set()
        except websocket._exceptions.WebSocketConnectionClosedException:
            self.message_queue.put("[ERROR] WebSocket connection lost during upload.\n")
            self.upload_failed_event.set()
        except websocket._exceptions.WebSocketTimeoutException:
            self.message_queue.put("[ERROR] WebSocket timeout during upload. Check ESP32 response/stability.\n")
            self.upload_failed_event.set()
        except json.JSONDecodeError as e:
            self.message_queue.put(f"[ERROR] Invalid JSON received during upload handshake: {e}. Aborting upload.\n")
            self.upload_failed_event.set()
        except Exception as e:
            self.message_queue.put(f"[ERROR] An unexpected error occurred during firmware upload: {e}\n")
            self.upload_failed_event.set()
        finally:
            if ws_upload:
                ws_upload.close()
                self.message_queue.put("[INFO] Dedicated upload WebSocket closed.\n")
            self.is_uploading = False
            self.after(0, lambda: self.reset_upload_ui())

    def reset_upload_ui(self):
        """Resets the UI elements related to upload after upload finishes or is aborted."""
        self.progress_frame.grid_remove()
        self.progress_bar.set(0)
        self.upload_progress_label.configure(text="0% (0B/0B) 0B/s")
        self.send_button.configure(state="normal")
        self.upload_button.configure(text="Upload Firmware", command=self.start_firmware_upload, state="normal")

    def update_status(self, status_text, color):
        """Helper function to update the connection status label."""
        self.status_label.configure(text=f"Status: {status_text}", text_color=color)


if __name__ == "__main__":
    customtkinter.set_appearance_mode("System")
    customtkinter.set_default_color_theme("blue")

    app = WebSocketApp()
    app.mainloop()
