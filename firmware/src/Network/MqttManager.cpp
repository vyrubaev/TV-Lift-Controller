#include "MqttManager.h"
#include "Config/DeviceConfig.h"
#include <WiFi.h>
#include <ArduinoJson.h>

MqttManager::MqttManager() {}

void MqttManager::init(Elevator* elevatorPtr) {
    m_elevator = elevatorPtr;

    if (DeviceConfig::localMqttEnabled) {
        initLocalClient();
        connectToLocal();
    }
    
    if (DeviceConfig::cloudMqttEnabled) {
        initCloudClient();
        connectToCloud();
    }
}

// ============================================================================
// 1. ЛОКАЛЬНЫЙ MQTT (Home Assistant)
// ============================================================================
void MqttManager::initLocalClient() {
    m_localClient.setServer(DeviceConfig::localMqttServer, DeviceConfig::localMqttPort);
    if (strlen(DeviceConfig::localMqttUser) > 0) {
        m_localClient.setCredentials(DeviceConfig::localMqttUser, DeviceConfig::localMqttPass);
    }
    m_localClient.setClientId("TV_Lift_Local");
    m_localClient.setWill("tv_lift/status", 1, true, "offline");

    m_localClient.onConnect([this](bool session) { this->onLocalConnect(session); });
    m_localClient.onDisconnect([this](AsyncMqttClientDisconnectReason r) { this->onLocalDisconnect(r); });
    m_localClient.onMessage([this](char* t, char* p, AsyncMqttClientMessageProperties, size_t len, size_t, size_t) {
        this->onLocalMessage(t, p, len);
    });
}

void MqttManager::connectToLocal() {
    if (WiFi.status() == WL_CONNECTED && !m_localConnected) {
        m_localClient.connect();
    }
}

void MqttManager::onLocalConnect(bool sessionPresent) {
    m_localConnected = true;
    m_localClient.publish("tv_lift/status", 1, true, "online");
    m_localClient.subscribe("tv_lift/cover/set", 1);

    if (DeviceConfig::localMqttHaDiscovery) {
        sendHAHomeDiscovery();
    }
}

void MqttManager::onLocalDisconnect(AsyncMqttClientDisconnectReason reason) {
    m_localConnected = false;
    m_localReconnectTimer.once(5, [this]() { this->connectToLocal(); });
}

void MqttManager::onLocalMessage(char* topic, char* payload, size_t len) {
    char cmd[16] = {0};
    size_t copyLen = (len < sizeof(cmd) - 1) ? len : (sizeof(cmd) - 1);
    memcpy(cmd, payload, copyLen);
    cmd[copyLen] = '\0';

    if (strcmp(topic, "tv_lift/cover/set") == 0 && m_elevator) {
        if (strcmp(cmd, "OPEN") == 0)        m_elevator->postWebCommand(Elevator::PendingCommand::Type::UP);
        else if (strcmp(cmd, "CLOSE") == 0)   m_elevator->postWebCommand(Elevator::PendingCommand::Type::DOWN);
        else if (strcmp(cmd, "STOP") == 0)    m_elevator->postWebCommand(Elevator::PendingCommand::Type::STOP);
    }
}

void MqttManager::sendHAHomeDiscovery() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(1024);
#endif

    doc["name"]               = "TV Lift";
    doc["unique_id"]          = "tv_lift_esp32_c6";
    doc["device_class"]       = "tv";
    doc["command_topic"]      = "tv_lift/cover/set";
    doc["state_topic"]        = "tv_lift/cover/state";
    doc["availability_topic"] = "tv_lift/status";
    doc["payload_open"]       = "OPEN";
    doc["payload_close"]      = "CLOSE";
    doc["payload_stop"]       = "STOP";
    doc["state_open"]         = "open";
    doc["state_closed"]       = "closed";
    doc["state_opening"]      = "opening";
    doc["state_closing"]      = "closing";

    String output;
    serializeJson(doc, output);
    m_localClient.publish("homeassistant/cover/tv_lift/config", 1, true, output.c_str(), output.length());
}

// ============================================================================
// 2. ОБЛАЧНЫЙ MQTT (VPS Telemetry)
// ============================================================================
void MqttManager::initCloudClient() {
    m_cloudClient.setServer(DeviceConfig::cloudMqttServer, DeviceConfig::cloudMqttPort);
    m_cloudClient.setCredentials(DeviceConfig::cloudMqttToken, "X-Device-Token");
    
    static String cloudClientId;
    cloudClientId = "TVLift_Cloud_" + WiFi.macAddress();
    cloudClientId.replace(":", "");
    m_cloudClient.setClientId(cloudClientId.c_str());

    m_cloudClient.onConnect([this](bool session) { this->onCloudConnect(session); });
    m_cloudClient.onDisconnect([this](AsyncMqttClientDisconnectReason r) { this->onCloudDisconnect(r); });
}

void MqttManager::connectToCloud() {
    if (WiFi.status() == WL_CONNECTED && !m_cloudConnected) {
        m_cloudClient.connect();
    }
}

void MqttManager::onCloudConnect(bool sessionPresent) {
    m_cloudConnected = true;
    logToCloud("INFO", "Device connected to Cloud Telemetry Server");
}

void MqttManager::onCloudDisconnect(AsyncMqttClientDisconnectReason reason) {
    m_cloudConnected = false;
    m_cloudReconnectTimer.once(15, [this]() { this->connectToCloud(); });
}

void MqttManager::logToCloud(const char* level, const char* message) {
    if (!m_cloudConnected) return;

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(256);
#endif

    doc["mac"]     = WiFi.macAddress();
    doc["level"]   = level;
    doc["msg"]     = message;
    doc["uptime"]  = millis() / 1000;

    String payload;
    serializeJson(doc, payload);
    m_cloudClient.publish("vps/telemetry/logs", 1, false, payload.c_str(), payload.length());
}

void MqttManager::publishCloudTelemetry() {
    if (!m_cloudConnected || !m_elevator) return;

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(512);
#endif

    doc["mac"]         = WiFi.macAddress();
    doc["state"]       = (int)m_elevator->getState();
    doc["current_a"]   = m_elevator->getCurrentAmps();
    //doc["position"]    = m_elevator->getEncoderTicks();
    doc["uptime_sec"]  = millis() / 1000;
    doc["wifi_rssi"]   = WiFi.RSSI();

    String payload;
    serializeJson(doc, payload);
    
    String topic = "vps/telemetry/data/" + WiFi.macAddress();
    m_cloudClient.publish(topic.c_str(), 0, false, payload.c_str(), payload.length());
}

// ============================================================================
// 3. ОСНОВНОЙ ЦИКЛ ОБРАБОТКИ
// ============================================================================
void MqttManager::update() {
    if (WiFi.status() != WL_CONNECTED) return;

    uint32_t now = millis();

    // Публикация состояния в Home Assistant (раз в 1 сек)
    if (m_localConnected && (now - m_lastLocalPubMs >= 1000)) {
        m_lastLocalPubMs = now;
        if (m_elevator) {
            ElevatorState state = m_elevator->getState();
            const char* st = "closed";

            if (state == ElevatorState::MOVING_UP) {
                st = "opening";
            } else if (state == ElevatorState::MOVING_DOWN) {
                st = "closing";
            } else if (state == ElevatorState::OPEN || state == ElevatorState::STOPPED) {
                st = "open";
            } else if (state == ElevatorState::CLOSED) {
                st = "closed";
            }

            m_localClient.publish("tv_lift/cover/state", 0, false, st);
        }
    }

    // Публикация метрик на VPS (раз в 5 секунд)
    if (m_cloudConnected && (now - m_lastCloudPubMs >= 5000)) {
        m_lastCloudPubMs = now;
        publishCloudTelemetry();
    }
}