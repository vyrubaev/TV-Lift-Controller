import os
import shutil
import json
from datetime import datetime

# Отключаем предупреждения линтера VS Code
# type: ignore 
# pylint: disable=import-error,undefined-variable

try:
    from SCons.Script import Import  # type: ignore # pylint: disable=import-error
    Import("env")
except Exception:
    # Заглушка, если файл открыт отдельно от PlatformIO
    env = {}

# Путь к директории сборки (.pio/build/<env_name>)
# Если env — это словарь/объект SCons, безопасно получаем путь
build_dir = env.get("BUILD_DIR", ".") if isinstance(env, dict) else env.get("BUILD_DIR")

# Относительный путь к папке backend/public/firmware
SERVER_FIRMWARE_DIR = os.path.abspath(os.path.join(build_dir, "../../../backend/public/firmware"))

# Версия прошивки (должна совпадать с CURRENT_FIRMWARE_VERSION в C++)
VERSION = "1.0.1"

def after_build(source, target, env):
    firmware_path = str(target[0])
    
    # 1. Создаем папку бэкенда, если её нет
    if not os.path.exists(SERVER_FIRMWARE_DIR):
        os.makedirs(SERVER_FIRMWARE_DIR, exist_ok=True)
        
    # 2. Копируем скомпилированный бинарник firmware.bin
    dest_bin = os.path.join(SERVER_FIRMWARE_DIR, "firmware.bin")
    shutil.copy(firmware_path, dest_bin)
    
    # 3. Генерируем манифест version.json для ESP32
    manifest = {
        "version": VERSION,
        "url": "http://192.168.88.33:3000/firmware/firmware.bin",
        "build_date": datetime.now().isoformat()
    }
    
    manifest_path = os.path.join(SERVER_FIRMWARE_DIR, "version.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print("\n==========================================")
    print("[OTA-BUILD] Успешно скопировано в backend!")
    print(f"  BIN:  {dest_bin}")
    print(f"  JSON: {manifest_path}")
    print("==========================================\n")

# Привязываем пост-акцию к сборке бинарника
if hasattr(env, "AddPostAction"):
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)