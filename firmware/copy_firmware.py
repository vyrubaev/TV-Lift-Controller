import os
import shutil
import json
import re
from datetime import datetime

print("\n>>> [ЧЕКЕР] ПИТОН-СКРИПТ ИНИЦИАЛИЗИРОВАН ПЛАТФОРМИО! <<<\n")

# 1. Пути к файлам
project_dir = os.getcwd()
header_file = os.path.join(project_dir, "src", "OTA", "OtaUpdater.h")
target_dir = os.path.abspath(os.path.join(project_dir, "../backend/public/firmware"))
json_file = os.path.join(target_dir, "version.json")
bin_source = os.path.join(project_dir, ".pio", "build", "esp32dev", "firmware.bin")
bin_target = os.path.join(target_dir, "firmware.bin")

# 2. Ищем версию в OtaUpdater.h
version = "1.0.0"
if os.path.exists(header_file):
    with open(header_file, "r", encoding="utf-8") as f:
        match = re.search(r'#define\s+CURRENT_FIRMWARE_VERSION\s+"([^"]+)"', f.read())
        if match:
            version = match.group(1)

print(f"[OTA] Прочитана версия из файла: {version}")

# 3. Создаем папку в бэкенде, если её нет
os.makedirs(target_dir, exist_ok=True)

# 4. Перезаписываем version.json
manifest = {
    "version": version,
    "url": "http://192.168.88.33:3000/firmware/firmware.bin",
    "build_date": datetime.now().isoformat()
}

with open(json_file, "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2)

print(f"[OTA] Записан JSON: {json_file}")

# 5. Копируем бинарник (если он скомпилирован)
if os.path.exists(bin_source):
    shutil.copy(bin_source, bin_target)
    print(f"[OTA] Скопирован бинарник -> {bin_target}")

print(">>> [ЧЕКЕР] СКРИПТ УСПЕШНО ЗАВЕРШИЛ РАБОТУ! <<<\n")