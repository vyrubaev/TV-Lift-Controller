import os
import json
import shutil
from datetime import datetime
from SCons.Script import Import

Import("env")

# Путь к папке прошивок сервера относительно папки firmware/
PROJECT_DIR = env.get("PROJECT_DIR")
SERVER_FIRMWARE_DIR = os.path.abspath(os.path.join(PROJECT_DIR, "../backend/public/firmware"))

# Версия прошивки (должна совпадать с CURRENT_FIRMWARE_VERSION в OtaUpdater.h)
VERSION = "1.0.1"

def after_build(source, target, env):
    firmware_path = str(target[0])
    
    # Создаем папку бэкенда, если её еще нет
    if not os.path.exists(SERVER_FIRMWARE_DIR):
        os.makedirs(SERVER_FIRMWARE_DIR)
        
    # 1. Копируем скомпилированный бинарник firmware.bin
    dest_bin = os.path.join(SERVER_FIRMWARE_DIR, "firmware.bin")
    shutil.copy(firmware_path, dest_bin)
    
    # 2. Генерируем манифест version.json для ESP32
    manifest = {
        "version": VERSION,
        "url": "http://192.168.88.33:3000/firmware/firmware.bin",
        "build_date": datetime.now().isoformat()
    }
    
    manifest_path = os.path.join(SERVER_FIRMWARE_DIR, "version.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print(f"\n==========================================")
    print(f"[OTA-BUILD] Успешно скопировано в backend!")
    print(f"  BIN:  {dest_bin}")
    print(f"  JSON: {manifest_path}")
    print(f"==========================================\n")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)