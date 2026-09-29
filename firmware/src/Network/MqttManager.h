/*

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include "Elevator/Elevator.h"

class MqttManager {
public:
    MqttManager();
    void init(Elevator* elevatorPtr);
    void update();
    
    // Публикация системного лога в облако
    void logToCloud(const char* level, const char* message);

private:
    Elevator* m_elevator = nullptr;

    // --- Локальный клиент (Home Assistant / Local MQTT) ---
    AsyncMqttClient m_localClient;
    Ticker m_localReconnectTimer;
    bool m_localConnected = false;
    
    void initLocalClient();
    void connectToLocal();
    void onLocalConnect(bool sessionPresent);
    void onLocalDisconnect(AsyncMqttClientDisconnectReason reason);
    void onLocalMessage(char* topic, char* payload, size_t len);
    void sendHAHomeDiscovery();
    void publishLocalStatus();

    // --- Облачный сервисный клиент (VPS Telemetry) ---
    AsyncMqttClient m_cloudClient;
    Ticker m_cloudReconnectTimer;
    bool m_cloudConnected = false;
    
    void initCloudClient();
    void connectToCloud();
    void onCloudConnect(bool sessionPresent);
    void onCloudDisconnect(AsyncMqttClientDisconnectReason reason);
    void publishCloudTelemetry();

    uint32_t m_lastLocalPubMs = 0;
    uint32_t m_lastCloudPubMs = 0;
};

#endif // MQTT_MANAGER_H

*/