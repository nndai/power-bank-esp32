Import("env")
import os

# Path to the Arduino ESP32 SDK
ARDUINO_SDK = os.path.join(
    os.path.expanduser("~"),
    ".platformio", "packages", "framework-arduinoespressif32",
    "tools", "sdk", "esp32"
)

# Add include path for esp_littlefs.h
env.Append(
    CPPPATH=[
        os.path.join(ARDUINO_SDK, "include", "esp_littlefs", "include")
    ]
)

# Link the esp_littlefs static library
# We use the full path as a File node so SCons links it correctly
littlefs_lib_path = os.path.join(ARDUINO_SDK, "lib", "libesp_littlefs.a")
env.Append(LIBS=[env.File(littlefs_lib_path)])
