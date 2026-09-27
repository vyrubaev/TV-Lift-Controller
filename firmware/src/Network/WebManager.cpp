#include "WebManager.h"
#include "Core/Core.h"
#include "Config/DeviceConfig.h"

static const char* MDNS_HOSTNAME = "tv-lift";
static const char* AP_SSID      = "TV-Lift-Setup";
static const byte  DNS_PORT     = 53;

WebManager::WebManager() {}

// Вспомогательный метод для безопасной перезагрузки без блокировки сетевого стека
static void scheduleReboot(uint32_t delayMs = 500) {
    xTaskCreate([](void* arg) {
        uint32_t ms = (uint32_t)(uintptr_t)arg;
        vTaskDelay(pdMS_TO_TICKS(ms));
        Core::reboot();
        vTaskDelete(NULL);
    }, "deferred_reboot", 2048, (void*)(uintptr_t)delayMs, 1, NULL);
}

void WebManager::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;

    // 1. Инициализация LittleFS
    if (!LittleFS.begin(true)) {
        Logger::info("[LittleFS] Ошибка монтирования файловой системы!");
    } else {
        Logger::info("[LittleFS] Успешно смонтирована.");
    }

    // 2. Обработка 3-кратного сброса питания (Сброс Wi-Fi)
    m_prefs.begin("system-cfg", false);
    int bootCount = m_prefs.getInt("boot_cnt", 0) + 1;
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

    // 👈 ВМЕСТО xTaskCreate: Запоминаем старт и взводим флаг
    m_bootCounterActive = true;
    m_bootTimerMs = millis();

    // 3. Подключение к сети
    loadCredentials();
    if (m_ssid.length() > 0) {
        startSTAMode();
    } else {
        startAPMode();
    }
}

void WebManager::update() {

    // 👈 Безопасный сброс счетчика через 3 секунды стабильной работы в основном потоке
    if (m_bootCounterActive && (millis() - m_bootTimerMs >= 3000)) {
        m_bootCounterActive = false;
        
        m_prefs.begin("system-cfg", false);
        m_prefs.putInt("boot_cnt", 0);
        m_prefs.end();
        
        Logger::info("[System] Счетчик перезагрузок успешно сброшен");
    }
    // Логика состояния Wi-Fi
    if (m_wifiState == WifiState::CONNECTING_STA) {
        // 1. Успешно подключились
        if (WiFi.status() == WL_CONNECTED) {
            m_wifiState = WifiState::STA_MODE;

            char logBuf[96];
            snprintf(logBuf, sizeof(logBuf), "[WiFi] Успешно подключено! IP: %s", WiFi.localIP().toString().c_str());
            Logger::info(logBuf);

            if (MDNS.begin(MDNS_HOSTNAME)) {
                snprintf(logBuf, sizeof(logBuf), "[mDNS] Адрес: http://%s.local", MDNS_HOSTNAME);
                Logger::info(logBuf);
                MDNS.addService("http", "tcp", 80);
            }

            setupWebSocket();
            setupRoutes();
            m_server.begin();
        } 
        // 2. Превышен таймаут (10 секунд)
        else if (millis() - m_wifiConnectStartMs >= WIFI_TIMEOUT_MS) {
            Logger::info("[WiFi] Превышено время ожидания! Переход в AP режим...");
            startAPMode();
        }
    }
    else if (m_wifiState == WifiState::AP_MODE) {
        m_dnsServer.processNextRequest();
    } 
    else if (m_wifiState == WifiState::STA_MODE) {
        m_ws.cleanupClients();
        broadcastStatus();
    }
}

void WebManager::setupWebSocket() {
    m_ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            char logBuf[96];
            snprintf(logBuf, sizeof(logBuf), "[WebSocket] Клиент подключен ID: %u", client->id());
            Logger::info(logBuf);
        } else if (type == WS_EVT_DISCONNECT) {
            char logBuf[96];
            snprintf(logBuf, sizeof(logBuf), "[WebSocket] Клиент отключен ID: %u", client->id());
            Logger::info(logBuf);
        } else if (type == WS_EVT_DATA) {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                this->handleWsCommand(data, len);
            }
        }
    });

    m_server.addHandler(&m_ws);
}

void WebManager::handleWsCommand(uint8_t* data, size_t len) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        Logger::error("[WS] Ошибка парсинга JSON");
        return;
    }

    if (doc.containsKey("action") && m_elevator) {
        const char* action = doc["action"];
        if (strcmp(action, "UP") == 0) {
            m_elevator->postWebCommand(Elevator::PendingCommand::Type::UP);
        } else if (strcmp(action, "DOWN") == 0) {
            m_elevator->postWebCommand(Elevator::PendingCommand::Type::DOWN);
        } else if (strcmp(action, "STOP") == 0) {
            m_elevator->postWebCommand(Elevator::PendingCommand::Type::STOP);
        }
    } 
    else if (doc.containsKey("cmd")) {
        const char* cmd = doc["cmd"];
        if (strcmp(cmd, "reboot") == 0) {
            Logger::info("[WebSocket] Запрошена перезагрузка");
            scheduleReboot(200);
        }
    }
}

void WebManager::broadcastStatus() {
    if (!m_elevator || m_ws.count() == 0) return;

    uint32_t now = millis();
    bool isMoving = (m_elevator->getState() != ElevatorState::STOPPED);
    uint32_t interval = isMoving ? 100 : 1000; // 10 Гц в движении, 1 Гц в покое

    if (now - m_lastBroadcastMs < interval) return;
    m_lastBroadcastMs = now;

    // Формируем JSON с данными
    StaticJsonDocument<128> doc;
    doc["st"]  = static_cast<int>(m_elevator->getState());
    doc["cur"] = m_elevator->getCurrentAmps();
    
    // Если есть свежий ИК-код — добавляем его
    if (DeviceConfig::LAST_IR_CODE != 0) {
        char hexBuffer[12];
        snprintf(hexBuffer, sizeof(hexBuffer), "0x%08X", (unsigned int)DeviceConfig::LAST_IR_CODE);
        doc["ir"] = hexBuffer;
    }

    String jsonString;
    serializeJson(doc, jsonString);

    // Безопасный обход клиентов по ссылке
    for (auto& client : m_ws.getClients()) {
        if (client.status() == WS_CONNECTED && !client.queueIsFull()) {
            client.text(jsonString);
        }
    }
}

void WebManager::startSTAMode() {
    m_wifiState = WifiState::CONNECTING_STA;
    m_wifiConnectStartMs = millis(); // Запоминаем время старта

    WiFi.mode(WIFI_STA);
    WiFi.begin(m_ssid.c_str(), m_password.c_str());

    char logBuf[96];
    snprintf(logBuf, sizeof(logBuf), "[WiFi] Инициализация подключения к: %s", m_ssid.c_str());
    Logger::info(logBuf);

    // Больше никакого while() и delay()! Выходим мгновенно.
}

void WebManager::startAPMode() {
    m_wifiState = WifiState::AP_MODE;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    IPAddress apIP = WiFi.softAPIP();

    char logBuf[96];
    snprintf(logBuf, sizeof(logBuf), "[AP] Режим точки доступа: %s", AP_SSID);
    Logger::info(logBuf);

    m_dnsServer.start(DNS_PORT, "*", apIP);
    
    setupCaptivePortalRoutes();
    setupWebSocket();
    setupRoutes(); 
    m_server.begin();
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

            request->send(200, "text/html", "<html><body><h2>Congratulation! Settings save!</h2><p>System rebooting...</p></body></html>");
            scheduleReboot(1000); // Отложенная перезагрузка
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });

    m_server.onNotFound([](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"><title>Setup TV-Lift</title></head>
<body style="background:#121212;color:#fff;font-family:sans-serif;padding:20px;text-align:center;">
    <h2>Settings Wi-Fi TV-Lift</h2>
    <form action="/save" method="POST" onsubmit="this.querySelector('button').innerText='Save...'">
        <select id="networks" name="ssid" style="width:280px;padding:10px;margin:10px 0;background:#222;color:#fff;border:1px solid #444;"><option>Scaning networks...</option></select><br>
        <div style="display:inline-block;width:280px;position:relative;margin:10px 0;">
            <input type="password" id="p" name="pass" placeholder="Wi-Fi password" required style="width:100%;padding:10px;box-sizing:border-box;background:#222;color:#fff;border:1px solid #444;padding-right:35px;">
            <span onclick="let t=document.getElementById('p');t.type=t.type==='password'?'text':'password';this.innerText=t.type==='password'?'👁':'👁‍🗨';" style="position:absolute;right:8px;top:10px;cursor:pointer;color:#888;">👁</span>
        </div><br>
        <button type="submit" style="width:280px;padding:10px;background:#4CAF50;color:#fff;border:none;cursor:pointer;margin-top:10px;">Save</button>
    </form>
    <script>
        fetch('/scan').then(r => r.json()).then(data => {
            let select = document.getElementById('networks');
            select.innerHTML = '';
            data.networks.forEach(net => {
                let opt = document.createElement('option');
                opt.value = net.ssid;
                opt.innerHTML = `${net.ssid} (${net.rssi} dBm)`;
                select.appendChild(opt);
            });
        });
    </script>
</body>
</html>)rawliteral";
        request->send(200, "text/html", html);
    });
}

void WebManager::setupRoutes() {
    m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "Error: index.html not found");
        }
    });

    m_server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/config.html")) {
            request->send(LittleFS, "/config.html", "text/html");
        } else {
            request->send(404, "text/plain", "Error: config.html not found");
        }
    });

    m_server.on("/get-ir", HTTP_GET, [](AsyncWebServerRequest *request){
        char jsonBuffer[32];
        snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"code\":\"0x%08X\"}", (unsigned int)DeviceConfig::LAST_IR_CODE);
        request->send(200, "application/json", jsonBuffer);
    });

    m_server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"rebooting\"}");
        scheduleReboot(500);
    });

    m_server.on("/api/config/reset", HTTP_POST, [](AsyncWebServerRequest *request){
        DeviceConfig::loadDefaults(); 
        DeviceConfig::save();
        
        request->send(200, "application/json", "{\"status\":\"reset_ok\"}");
        scheduleReboot(500);
    });

    m_server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);

        doc["VERSION"]                    = DeviceConfig::VERSION;
        doc["MOUNT_TYPE"]                 = DeviceConfig::MOUNT_TYPE;
        doc["IS_MASTER"]                  = DeviceConfig::IS_MASTER;
        doc["NODE_ID"]                    = DeviceConfig::NODE_ID;
        doc["DEBUG_ENABLED"]              = DeviceConfig::DEBUG_ENABLED;
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
        doc["otaUpdateIntervalMs"]        = DeviceConfig::otaUpdateIntervalMs;

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

    if (m_handleSaveConfig == nullptr) {
        m_handleSaveConfig = new AsyncCallbackJsonWebHandler(
            "/api/config", 
            [](AsyncWebServerRequest *request, JsonVariant &json) {
                JsonObject jsonObj = json.as<JsonObject>();
                if (jsonObj.isNull()) {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
                    return;
                }

                if (jsonObj.containsKey("MOUNT_TYPE"))                 DeviceConfig::MOUNT_TYPE                 = jsonObj["MOUNT_TYPE"];
                if (jsonObj.containsKey("IS_MASTER"))                  DeviceConfig::IS_MASTER                  = jsonObj["IS_MASTER"];
                if (jsonObj.containsKey("NODE_ID"))                    DeviceConfig::NODE_ID                    = jsonObj["NODE_ID"];
                if (jsonObj.containsKey("DEBUG_ENABLED"))              DeviceConfig::DEBUG_ENABLED              = jsonObj["DEBUG_ENABLED"];
                if (jsonObj.containsKey("MOTOR_SPEED"))                DeviceConfig::MOTOR_SPEED                = jsonObj["MOTOR_SPEED"];
                if (jsonObj.containsKey("SOFT_START_MIN_PWM"))         DeviceConfig::SOFT_START_MIN_PWM         = jsonObj["SOFT_START_MIN_PWM"];
                if (jsonObj.containsKey("SOFT_START_STEP_MS"))         DeviceConfig::SOFT_START_STEP_MS         = jsonObj["SOFT_START_STEP_MS"];
                if (jsonObj.containsKey("SOFT_START_STEP_PWM"))        DeviceConfig::SOFT_START_STEP_PWM        = jsonObj["SOFT_START_STEP_PWM"];
                if (jsonObj.containsKey("CURRENT_SENSOR_SENSITIVITY")) DeviceConfig::CURRENT_SENSOR_SENSITIVITY = jsonObj["CURRENT_SENSOR_SENSITIVITY"];
                if (jsonObj.containsKey("CURRENT_SENSOR_OFFSET_V"))    DeviceConfig::CURRENT_SENSOR_OFFSET_V    = jsonObj["CURRENT_SENSOR_OFFSET_V"];
                if (jsonObj.containsKey("startCurrentTimeoutMs"))      DeviceConfig::startCurrentTimeoutMs      = jsonObj["startCurrentTimeoutMs"];
                if (jsonObj.containsKey("maxMotorCurrentAmps"))        DeviceConfig::maxMotorCurrentAmps        = jsonObj["maxMotorCurrentAmps"];
                if (jsonObj.containsKey("overcurrentTimeoutMs"))       DeviceConfig::overcurrentTimeoutMs       = jsonObj["overcurrentTimeoutMs"];
                if (jsonObj.containsKey("MAX_FORWARD_TIME_MS"))        DeviceConfig::MAX_FORWARD_TIME_MS        = jsonObj["MAX_FORWARD_TIME_MS"];
                if (jsonObj.containsKey("MAX_REVERSE_TIME_MS"))        DeviceConfig::MAX_REVERSE_TIME_MS        = jsonObj["MAX_REVERSE_TIME_MS"];
                if (jsonObj.containsKey("FORWARD_LIMIT_RUN_ON_MS"))    DeviceConfig::FORWARD_LIMIT_RUN_ON_MS     = jsonObj["FORWARD_LIMIT_RUN_ON_MS"];
                if (jsonObj.containsKey("REVERSE_LIMIT_RUN_ON_MS"))    DeviceConfig::REVERSE_LIMIT_RUN_ON_MS     = jsonObj["REVERSE_LIMIT_RUN_ON_MS"];
                if (jsonObj.containsKey("MAX_LIFT_ENCODER_TICKS"))     DeviceConfig::MAX_LIFT_ENCODER_TICKS     = jsonObj["MAX_LIFT_ENCODER_TICKS"];
                if (jsonObj.containsKey("otaUpdateIntervalMs"))        DeviceConfig::otaUpdateIntervalMs        = jsonObj["otaUpdateIntervalMs"];

                auto parseIrCode = [](JsonVariant v) -> uint32_t {
                    if (v.is<const char*>()) {
                        const char* str = v.as<const char*>();
                        return str ? strtoul(str, nullptr, 0) : 0;
                    }
                    return v.as<uint32_t>();
                };

                if (jsonObj.containsKey("IR_CODE_UP"))     DeviceConfig::IR_CODE_UP     = parseIrCode(jsonObj["IR_CODE_UP"]);
                if (jsonObj.containsKey("IR_CODE_DOWN"))   DeviceConfig::IR_CODE_DOWN   = parseIrCode(jsonObj["IR_CODE_DOWN"]);
                if (jsonObj.containsKey("IR_CODE_STOP"))   DeviceConfig::IR_CODE_STOP   = parseIrCode(jsonObj["IR_CODE_STOP"]);
                if (jsonObj.containsKey("IR_CODE_REPEAT")) DeviceConfig::IR_CODE_REPEAT = parseIrCode(jsonObj["IR_CODE_REPEAT"]);

                if (jsonObj.containsKey("otaUrl") && jsonObj["otaUrl"].is<const char*>()) {
                    snprintf(DeviceConfig::otaUrl, sizeof(DeviceConfig::otaUrl), "%s", jsonObj["otaUrl"].as<const char*>());
                }

                DeviceConfig::save();
                request->send(200, "application/json", "{\"status\":\"ok\"}");

                scheduleReboot(500);
            }
        );

        m_server.addHandler(m_handleSaveConfig);
    }
}