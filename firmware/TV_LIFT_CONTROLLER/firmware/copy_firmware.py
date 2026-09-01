import os
import json
import shutil
from datetime import datetime
import ExtraScript

Import("env")

# Укажите абсолютный или относительный путь к папке сервера
SERVER_FIRMWARE_DIR = "../iot-backend/public/firmware"
VERSION = "1.0.1"

def after_build(source, target, env):
    firmware_path = str(target[0])
    
    if not os.path.exists(SERVER_FIRMWARE_DIR):
        os.makedirs(SERVER_FIRMWARE_DIR)
        
    # Копируем .bin файл
    dest_bin = os.path.join(SERVER_FIRMWARE_DIR, "firmware.bin")
    shutil.copy(firmware_path, dest_bin)
    
    # Обновляем version.json
    manifest = {
        "version": VERSION,
        "build_date": datetime.now().isoformat()
    }
    
    with open(os.path.join(SERVER_FIRMWARE_DIR, "version.json"), "w") as f:
        json.dump(manifest, f, indent=2)
        
    print(f"\n[OTA-BUILD] Firmware and version.json copied to {SERVER_FIRMWARE_DIR}\n")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)