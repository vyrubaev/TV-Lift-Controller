#pragma once

#include <AsyncMqttClient.h>
#include <Ticker.h>
#include "Elevator/Elevator.h"

class MqttManager {
public:
    MqttManager();

    void init(Elevator* elevatorPtr);
    void update();
    void logToCloud(const char* level, const char* message);

private:
    Elevator* m_elevator = nullptr;

    // Локальный клиент (Home Assistant)
    AsyncMqttClient m_localClient;
    bool m_localConnected = false;
    Ticker m_localReconnectTimer;
    uint32_t m_lastLocalPubMs = 0;

    void initLocalClient();
    void connectToLocal();
    void onLocalConnect(bool sessionPresent);
    void onLocalDisconnect(AsyncMqttClientDisconnectReason reason);
    void onLocalMessage(char* topic, char* payload, size_t len);
    void sendHAHomeDiscovery();

    // Облачный клиент (VPS Telemetry)
    AsyncMqttClient m_cloudClient;
    bool m_cloudConnected = false;
    Ticker m_cloudReconnectTimer;
    uint32_t m_lastCloudPubMs = 0;

    void initCloudClient();
    void connectToCloud();
    void onCloudConnect(bool sessionPresent);
    void onCloudDisconnect(AsyncMqttClientDisconnectReason reason);
    void publishCloudTelemetry();
};