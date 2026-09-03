#include "WebManager.h"

static const char* MDNS_HOSTNAME = "tv-lift";
static const char* AP_SSID      = "TV-Lift-Setup";
static const byte  DNS_PORT     = 53;

WebManager::WebManager() {}

void WebManager::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;

    // Инициализация LittleFS
    if (!LittleFS.begin(true)) {
        Logger::info("[LittleFS] Ошибка монтирования файловой системы!");
    } else {
        Logger::info("[LittleFS] Успешно смонтирована.");
    }

    // 1. Проверка 3-кратного сброса питания
    m_prefs.begin("system-cfg", false);
    int bootCount = m_prefs.getInt("boot_cnt", 0);
    bootCount++;
    m_prefs.putInt("boot_cnt", bootCount);
    m_prefs.end();

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[SYSTEM] Счетчик быстрых перезагрузок: %d", bootCount);
    Logger::info(buffer);

    if (bootCount >= 3) {
        Logger::info("[SYSTEM] !!! ОБНАРУЖЕН СБРОС СЕТИ (3 перезагрузки) !!!");
        
        m_prefs.begin("system-cfg", false);
        m_prefs.putInt("boot_cnt", 0);
        m_prefs.end();

        resetWifiSettings();
        return;
    }

    xTaskCreate([](void* arg) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        Preferences prefs;
        prefs.begin("system-cfg", false);
        prefs.putInt("boot_cnt", 0);
        prefs.end();
        Logger::info("[System] Счетчик перезагрузок сброшен");
        vTaskDelete(NULL);
    }, "reset_boot_counter", 2048, NULL, 1, NULL);

    // 2. Стандартный запуск сети
    loadCredentials();

    if (m_ssid.length() > 0) {
        startSTAMode();
    } else {
        startAPMode();
    }
}

void WebManager::loadCredentials() {
    m_prefs.begin("wifi-config", true);
    m_ssid = m_prefs.getString("ssid", "");
    m_password = m_prefs.getString("pass", "");
    m_prefs.end();
}

void WebManager::saveCredentials(const String& ssid, const String& pass) {
    m_prefs.begin("wifi-config", false);
    m_prefs.putString("ssid", ssid);
    m_prefs.putString("pass", pass);
    m_prefs.end();
}

void WebManager::resetWifiSettings() {
    m_prefs.begin("wifi-config", false);
    m_prefs.clear();
    m_prefs.end();
    
    startAPMode();
}

void WebManager::startSTAMode() {
    m_wifiState = WifiState::CONNECTING_STA;
    WiFi.mode(WIFI_STA);
    WiFi.begin(m_ssid.c_str(), m_password.c_str());

    char connLog[64];
    snprintf(connLog, sizeof(connLog), "[WiFi] Подключение к: %s", m_ssid.c_str());
    Logger::info(connLog);

    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        m_wifiState = WifiState::STA_MODE;
        Logger::info("[WiFi] Успешно подключено!");
        
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[WiFi] IP-адрес: %s", WiFi.localIP().toString().c_str());
        Logger::info(buffer);

        if (MDNS.begin(MDNS_HOSTNAME)) {
            snprintf(buffer, sizeof(buffer), "[mDNS] Запущен! Адрес: http://%s.local", MDNS_HOSTNAME);
            Logger::info(buffer);
            MDNS.addService("http", "tcp", 80);
        }

        // Регистрируем основные роутеры и API
        setupRoutes();
        m_server.begin();
    } else {
        Logger::info("[WiFi] Не удалось подключиться! Переход в AP режим...");
        startAPMode();
    }
}

void WebManager::startAPMode() {
    m_wifiState = WifiState::AP_MODE;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    IPAddress apIP = WiFi.softAPIP();

    Logger::info("[AP] Режим точки доступа активен!");
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[AP] Подключайтесь к сети: %s", AP_SSID);
    Logger::info(buffer);   

    m_dnsServer.start(DNS_PORT, "*", apIP);
    
    // ВАЖНО: Регистрируем и маршруты captive portal, и основные роутеры с API
    setupCaptivePortalRoutes();
    setupRoutes(); 
    m_server.begin();
}

void WebManager::setupCaptivePortalRoutes() {
    m_server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.createNestedArray("networks");

        for (int i = 0; i < n; ++i) {
            JsonObject net = array.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    m_server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
            String newSsid = request->getParam("ssid", true)->value();
            String newPass = request->getParam("pass", true)->value();

            this->saveCredentials(newSsid, newPass);

            request->send(200, "text/html", "<html><body><h2>Сохранено!</h2><p>Перезагрузка...</p></body></html>");
            delay(1000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });

    m_server.onNotFound([](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Настройка TV-Lift</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: #fff; text-align: center; padding: 20px; }
        .card { background: #1e1e1e; padding: 20px; border-radius: 10px; max-width: 400px; margin: 0 auto; }
        select, input, button { width: 100%; padding: 12px; margin: 8px 0; box-sizing: border-box; border-radius: 5px; border: 1px solid #333; font-size: 16px; background: #2a2a2a; color: #fff; }
        button { background: #4CAF50; font-weight: bold; cursor: pointer; border: none; }
    </style>
</head>
<body>
    <div class="card">
        <h2>Настройка Wi-Fi TV-Lift</h2>
        <form action="/save" method="POST">
            <select id="networks" name="ssid"><option>Поиск сетей...</option></select>
            <input type="password" name="pass" placeholder="Пароль от Wi-Fi" required>
            <button type="submit">Сохранить</button>
        </form>
    </div>
    <script>
        fetch('/scan').then(r => r.json()).then(data => {
            let select = document.getElementById('networks');
            select.innerHTML = '';
            data.networks.forEach(net => {
                let opt = document.createElement('option');
                opt.value = net.ssid;
                opt.innerHTML = `${net.ssid} (${net.rssi} dBm) ${net.open ? '' : '🔒'}`;
                select.appendChild(opt);
            });
        });
    </script>
</body>
</html>
)rawliteral";
        request->send(200, "text/html", html);
    });
}

void WebManager::setupRoutes() {
    // 1. Отдача index.html из LittleFS
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "Error: index.html not found in LittleFS!");
        }
    });

    // 2. Отдача config.html для технического специалиста
    m_server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/config.html")) {
            request->send(LittleFS, "/config.html", "text/html");
        } else {
            request->send(404, "text/plain", "Error: config.html not found in LittleFS!");
        }
    });

    // 3. GET: Чтение текущих настроек в JSON
    m_server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        
        doc["MOUNT_TYPE"]                 = DeviceConfig::MOUNT_TYPE;
        doc["IS_MASTER"]                  = DeviceConfig::IS_MASTER;
        doc["NODE_ID"]                    = DeviceConfig::NODE_ID;
        doc["MOTOR_SPEED"]                = DeviceConfig::MOTOR_SPEED;
        doc["SOFT_START_MIN_PWM"]         = DeviceConfig::SOFT_START_MIN_PWM;
        doc["SOFT_START_STEP_MS"]         = DeviceConfig::SOFT_START_STEP_MS;
        doc["SOFT_START_STEP_PWM"]        = DeviceConfig::SOFT_START_STEP_PWM;
        doc["CURRENT_SENSOR_SENSITIVITY"] = DeviceConfig::CURRENT_SENSOR_SENSITIVITY;
        doc["CURRENT_SENSOR_OFFSET_V"]    = DeviceConfig::CURRENT_SENSOR_OFFSET_V;
        doc["startCurrentTimeoutMs"]      = DeviceConfig::startCurrentTimeoutMs;
        doc["maxMotorCurrentAmps"]        = DeviceConfig::maxMotorCurrentAmps;
        doc["overcurrentTimeoutMs"]       = DeviceConfig::overcurrentTimeoutMs;
        doc["MAX_FORWARD_TIME_MS"]        = DeviceConfig::MAX_FORWARD_TIME_MS;
        doc["MAX_REVERSE_TIME_MS"]        = DeviceConfig::MAX_REVERSE_TIME_MS;
        doc["FORWARD_LIMIT_RUN_ON_MS"]    = DeviceConfig::FORWARD_LIMIT_RUN_ON_MS;
        doc["REVERSE_LIMIT_RUN_ON_MS"]    = DeviceConfig::REVERSE_LIMIT_RUN_ON_MS;
        doc["MAX_LIFT_ENCODER_TICKS"]     = DeviceConfig::MAX_LIFT_ENCODER_TICKS;
        doc["OTA_INTERVAL_MS"]            = DeviceConfig::otaUpdateIntervalMs;

        char hexBuffer[11];
        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", DeviceConfig::IR_CODE_UP);
        doc["IR_CODE_UP"]   = hexBuffer;
        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", DeviceConfig::IR_CODE_DOWN);
        doc["IR_CODE_DOWN"] = hexBuffer;
        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", DeviceConfig::IR_CODE_STOP);
        doc["IR_CODE_STOP"] = hexBuffer;
        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", DeviceConfig::IR_CODE_REPEAT);
        doc["IR_CODE_REPEAT"] = hexBuffer;

        doc["otaUrl"] = DeviceConfig::otaUrl;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // 4. POST: Сохранение настроек
    AsyncCallbackJsonWebHandler* handleSaveConfig = new AsyncCallbackJsonWebHandler(
        "/api/config", 
        [](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            if (jsonObj.isNull()) {
                request->send(400, "application/json", "{\"status\":\"error\"}");
                return;
            }

            if (jsonObj.containsKey("MOUNT_TYPE"))                 DeviceConfig::MOUNT_TYPE                 = jsonObj["MOUNT_TYPE"];
            if (jsonObj.containsKey("IS_MASTER"))                  DeviceConfig::IS_MASTER                  = jsonObj["IS_MASTER"];
            if (jsonObj.containsKey("NODE_ID"))                    DeviceConfig::NODE_ID                    = jsonObj["NODE_ID"];
            if (jsonObj.containsKey("MOTOR_SPEED"))                DeviceConfig::MOTOR_SPEED                = jsonObj["MOTOR_SPEED"];
            if (jsonObj.containsKey("SOFT_START_MIN_PWM"))         DeviceConfig::SOFT_START_MIN_PWM         = jsonObj["SOFT_START_MIN_PWM"];
            if (jsonObj.containsKey("SOFT_START_STEP_MS"))         DeviceConfig::SOFT_START_STEP_MS         = jsonObj["SOFT_START_STEP_MS"];
            if (jsonObj.containsKey("SOFT_START_STEP_PWM"))        DeviceConfig::SOFT_START_STEP_PWM        = jsonObj["SOFT_START_STEP_PWM"];
            if (jsonObj.containsKey("CURRENT_SENSOR_SENSITIVITY")) DeviceConfig::CURRENT_SENSOR_SENSITIVITY = jsonObj["CURRENT_SENSOR_SENSITIVITY"];
            if (jsonObj.containsKey("CURRENT_SENSOR_OFFSET_V"))    DeviceConfig::CURRENT_SENSOR_OFFSET_V    = jsonObj["CURRENT_SENSOR_OFFSET_V"];
            if (jsonObj.containsKey("startCurrentTimeoutMs"))     DeviceConfig::startCurrentTimeoutMs     = jsonObj["startCurrentTimeoutMs"];
            if (jsonObj.containsKey("maxMotorCurrentAmps"))       DeviceConfig::maxMotorCurrentAmps       = jsonObj["maxMotorCurrentAmps"];
            if (jsonObj.containsKey("overcurrentTimeoutMs"))      DeviceConfig::overcurrentTimeoutMs      = jsonObj["overcurrentTimeoutMs"];
            if (jsonObj.containsKey("MAX_FORWARD_TIME_MS"))        DeviceConfig::MAX_FORWARD_TIME_MS        = jsonObj["MAX_FORWARD_TIME_MS"];
            if (jsonObj.containsKey("MAX_REVERSE_TIME_MS"))        DeviceConfig::MAX_REVERSE_TIME_MS        = jsonObj["MAX_REVERSE_TIME_MS"];
            if (jsonObj.containsKey("FORWARD_LIMIT_RUN_ON_MS"))     DeviceConfig::FORWARD_LIMIT_RUN_ON_MS     = jsonObj["FORWARD_LIMIT_RUN_ON_MS"];
            if (jsonObj.containsKey("REVERSE_LIMIT_RUN_ON_MS"))     DeviceConfig::REVERSE_LIMIT_RUN_ON_MS     = jsonObj["REVERSE_LIMIT_RUN_ON_MS"];
            if (jsonObj.containsKey("MAX_LIFT_ENCODER_TICKS"))     DeviceConfig::MAX_LIFT_ENCODER_TICKS     = jsonObj["MAX_LIFT_ENCODER_TICKS"];
            if (jsonObj.containsKey("OTA_INTERVAL_MS"))            DeviceConfig::otaUpdateIntervalMs        = jsonObj["OTA_INTERVAL_MS"];

            // Безопасный парсинг HEX-строк с явным приведением через .as<const char*>()
            if (jsonObj.containsKey("IR_CODE_UP"))     DeviceConfig::IR_CODE_UP     = strtoul(jsonObj["IR_CODE_UP"].as<const char*>(), nullptr, 0);
            if (jsonObj.containsKey("IR_CODE_DOWN"))   DeviceConfig::IR_CODE_DOWN   = strtoul(jsonObj["IR_CODE_DOWN"].as<const char*>(), nullptr, 0);
            if (jsonObj.containsKey("IR_CODE_STOP"))   DeviceConfig::IR_CODE_STOP   = strtoul(jsonObj["IR_CODE_STOP"].as<const char*>(), nullptr, 0);
            if (jsonObj.containsKey("IR_CODE_REPEAT")) DeviceConfig::IR_CODE_REPEAT = strtoul(jsonObj["IR_CODE_REPEAT"].as<const char*>(), nullptr, 0);

            if (jsonObj.containsKey("otaUrl")) {
                snprintf(DeviceConfig::otaUrl, sizeof(DeviceConfig::otaUrl), "%s", jsonObj["otaUrl"].as<const char*>());
            }

            // Вызываем сохранение в энергонезависимую память NVS
            DeviceConfig::save();

            // Отправляем ответ клиенту и уходим на перезагрузку
            request->send(200, "application/json", "{\"status\":\"ok\"}");
            delay(500); 
          
            ESP.restart();
        }
    );
    m_server.addHandler(handleSaveConfig);

    // 5. Настройка WebSocket и регистрация
    m_ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_DATA) {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                
                StaticJsonDocument<128> doc;
                DeserializationError error = deserializeJson(doc, data, len);

                if (!error && doc.containsKey("action")) {
                    const char* action = doc["action"];

                    if (m_elevator) {
                        if (strcmp(action, "UP") == 0) {
                            m_elevator->postWebCommand(Elevator::PendingCommand::Type::UP);
                        } else if (strcmp(action, "DOWN") == 0) {
                            m_elevator->postWebCommand(Elevator::PendingCommand::Type::DOWN);
                        } else if (strcmp(action, "STOP") == 0) {
                            m_elevator->postWebCommand(Elevator::PendingCommand::Type::STOP);
                        }
                    }
                }
            }
        }
    });

    m_server.addHandler(&m_ws);
}

void WebManager::update() {
    if (m_wifiState == WifiState::AP_MODE) {
        m_dnsServer.processNextRequest();
    } else if (m_wifiState == WifiState::STA_MODE) {
        m_ws.cleanupClients();
        broadcastStatus();
    }
}

void WebManager::broadcastStatus() {
    if (!m_elevator || m_ws.count() == 0) return;

    uint32_t now = millis();
    if (now - m_lastBroadcastMs < 100) return; 

    // Используем ссылку auto& и обращение через точку .
    for (auto& client : m_ws.getClients()) {
        if (client.status() == WS_CONNECTED && client.queueIsFull()) {
            return; // Пропускаем кадр, если очередь переполнена
        }
    }

    m_lastBroadcastMs = now;

    StaticJsonDocument<128> doc;
    doc["st"] = static_cast<int>(m_elevator->getState());

    String jsonString;
    serializeJson(doc, jsonString);
    m_ws.textAll(jsonString);
}