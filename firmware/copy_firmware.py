import json
import os
import re
import shutil
from datetime import datetime
from typing import Any
from importlib import import_module

Import = import_module("SCons.Script").Import

env: Any
Import("env")

print("\n>>> [OTA] СКРИПТ ПОДГОТОВКИ ОБНОВЛЕНИЯ ИНИЦИАЛИЗИРОВАН <<<\n")

project_dir = env.subst("$PROJECT_DIR")
header_file = os.path.join(project_dir, "src", "Config/DeviceConfig.h") 
target_dir = os.path.abspath(os.path.join(project_dir, "../backend/public/firmware"))
json_file = os.path.join(target_dir, "version.json")
bin_source = os.path.join(project_dir, ".pio", "build", "esp32dev", "firmware.bin")
bin_target = os.path.join(target_dir, "firmware.bin")

def after_build(target, source, env):
    """Эта функция выполнится ТОЛЬКО ПОСЛЕ успешной компиляции прошивки"""
    print("\n>>> [OTA] Компиляция завершена. Копируем файлы... <<<")
    
    # 1. Читаем актуальную версию
    version = "1.0.0"
    if os.path.exists(header_file):
        with open(header_file, "r", encoding="utf-8") as f:
            content = f.read()
            match = re.search(r'VERSION\s*=\s*"([^"]+)"', content)
            if match:
                version = match.group(1)
    
    print(f"[OTA] Актуальная версия из кода: {version}")

    # 2. Создаем папку бэкенда
    os.makedirs(target_dir, exist_ok=True)

    # 3. Проверяем наличие свежего бинарника и копируем
    if os.path.exists(bin_source):
        shutil.copy(bin_source, bin_target)
        print(f"[OTA] Новый бинарник скопирован -> {bin_target}")
    else:
        print(f"[OTA ОШИБКА] Бинарник не найден: {bin_source}")
        return

    # 4. Записываем JSON-манифест строго ПОСЛЕ появления свежего бинарника
    manifest = {
        "version": version,
        "url": "http://192.168.88.33:3000/firmware/firmware.bin",
        "build_date": datetime.now().isoformat()
    }

    with open(json_file, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print(f"[OTA] Манифест успешно обновлен: {json_file}")
    print(">>> [OTA] ВСЁ ГОТОВО К ОБНОВЛЕНИЮ! <<<\n")

# Регистрируем хук, чтобы скрипт отрабатывал строго после сборки ELF/BIN файлов
env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)