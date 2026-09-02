import os
import shutil
import json
import re
from datetime import datetime

# ----------------------------------------------------------------------
# 1. ОПРЕДЕЛЕНИЕ ПУТЕЙ БЕЗ ИСПОЛЬЗОВАНИЯ __file__ И SCons-ПЕРЕМЕННЫХ
# ----------------------------------------------------------------------
# Получаем текущую рабочую директорию, откуда запущен PlatformIO (firmware_ESP32)
CURRENT_DIR = os.path.abspath(os.getcwd())

# Если pio run был запущен из внутренней папки, поднимаемся до firmware_ESP32
if "firmware_ESP32" in CURRENT_DIR:
    PROJECT_DIR = CURRENT_DIR.split("firmware_ESP32")[0] + "firmware_ESP32"
else:
    PROJECT_DIR = CURRENT_DIR

# Прямой путь к OtaUpdater.h
OTA_HEADER_PATH = os.path.join(PROJECT_DIR, "src", "OTA", "OtaUpdater.h")

# Прямой путь к целевой папке сервера
SERVER_FIRMWARE_DIR = os.path.abspath(os.path.join(PROJECT_DIR, "../backend/public/firmware"))

# ----------------------------------------------------------------------
# 2. ИЗВЛЕЧЕНИЕ ВЕРСИИ ИЗ C++ ЗАГОЛОВКА
# ----------------------------------------------------------------------
def extract_firmware_version():
    """Считывает CURRENT_FIRMWARE_VERSION прямо из файла OtaUpdater.h"""
    print(f"\n[OTA-SCRIPT] Ищем файл версии по пути: {OTA_HEADER_PATH}")
    
    if os.path.exists(OTA_HEADER_PATH):
        try:
            with open(OTA_HEADER_PATH, "r", encoding="utf-8") as f:
                content = f.read()
                match = re.search(r'#define\s+CURRENT_FIRMWARE_VERSION\s+"([^"]+)"', content)
                if match:
                    version = match.group(1)
                    print(f"[OTA-SCRIPT] УСПЕХ! Прочитана версия из C++: {version}")
                    return version
                else:
                    print("[OTA-SCRIPT] ОШИБКА: Макрос CURRENT_FIRMWARE_VERSION не найден в OtaUpdater.h")
        except Exception as e:
            print(f"[OTA-SCRIPT] ОШИБКА чтения файла: {e}")
    else:
        print(f"[OTA-SCRIPT] ОШИБКА: Файл OtaUpdater.h НЕ НАЙДЕН по указанному пути!")

    return "1.0.0_FALLBACK"

# ----------------------------------------------------------------------
# 3. ПОСТ-АКЦИЯ СБОРКИ (POST BUILD ACTION)
# ----------------------------------------------------------------------
def after_build(source, target, env):
    firmware_path = str(target[0])
    version = extract_firmware_version()
    
    # Создаем целевую директорию на бэкенде, если её нет
    os.makedirs(SERVER_FIRMWARE_DIR, exist_ok=True)
        
    # Копируем скомпилированный бинарник firmware.bin
    dest_bin = os.path.join(SERVER_FIRMWARE_DIR, "firmware.bin")
    shutil.copy(firmware_path, dest_bin)
    
    # Формируем манифест version.json для ESP32
    manifest = {
        "version": version,
        "url": "http://192.168.88.33:3000/firmware/firmware.bin",
        "build_date": datetime.now().isoformat()
    }
    
    manifest_path = os.path.join(SERVER_FIRMWARE_DIR, "version.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print("==================================================")
    print("[OTA-SCRIPT] ФАЙЛЫ УСПЕШНО СКОПИРОВАНЫ В BACKEND!")
    print(f"  Версия прошивки: {version}")
    print(f"  Файл бинарника:  {dest_bin}")
    print(f"  Файл манифеста: {manifest_path}")
    print("==================================================\n")

# ----------------------------------------------------------------------
# 4. ИНИЦИАЛИЗАЦИЯ B SCONS
# ----------------------------------------------------------------------
try:
    from SCons.Script import Import  # type: ignore
    env = None
    Import("env")
    if env is not None:
        env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
except Exception as e:
    pass