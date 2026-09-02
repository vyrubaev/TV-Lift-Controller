import os
import shutil
import json
import re
from datetime import datetime

# Вход в окружение SCons
try:
    from SCons.Script import Import # type: ignore
    Import("env")
except Exception:
    env = None

def extract_firmware_version(project_dir):
    """Ищет OtaUpdater.h и извлекает номер версии"""
    possible_paths = [
        os.path.join(project_dir, "src", "OTA", "OtaUpdater.h"),
        os.path.join(project_dir, "src", "OtaUpdater.h"),
        os.path.join(project_dir, "OtaUpdater.h")
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            print(f"\n[OTA-SCRIPT] Читаем файл: {path}")
            try:
                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()
                    match = re.search(r'#define\s+CURRENT_FIRMWARE_VERSION\s+"([^"]+)"', content)
                    if match:
                        ver = match.group(1)
                        print(f"[OTA-SCRIPT] Прочитана версия: {ver}")
                        return ver
            except Exception as e:
                print(f"[OTA-SCRIPT] Ошибка файла: {e}")
                
    print("[OTA-SCRIPT] OtaUpdater.h не найден, используем дефолт 1.0.0")
    return "1.0.0"

def after_build(source, target, env):
    # Определяем пути
    project_dir = env.subst("$PROJECT_DIR") if hasattr(env, "subst") else os.getcwd()
    server_dir = os.path.abspath(os.path.join(project_dir, "../backend/public/firmware"))
    
    firmware_path = str(target[0])
    version = extract_firmware_version(project_dir)
    
    # Создаем папку бэкенда и копируем файлы
    os.makedirs(server_dir, exist_ok=True)
    dest_bin = os.path.join(server_dir, "firmware.bin")
    shutil.copy(firmware_path, dest_bin)
    
    manifest = {
        "version": version,
        "url": "http://192.168.88.33:3000/firmware/firmware.bin",
        "build_date": datetime.now().isoformat()
    }
    
    manifest_path = os.path.join(server_dir, "version.json")
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print("\n==========================================")
    print("[OTA-SCRIPT] Файлы обновлены в backend:")
    print(f"  Версия: {version}")
    print(f"  BIN:  {dest_bin}")
    print(f"  JSON: {manifest_path}")
    print("==========================================\n")

    print(f"[DEBUG] Путь к бэкенду: {server_dir}")

# Привязываем пост-акцию к окончанию сборки
if env:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)