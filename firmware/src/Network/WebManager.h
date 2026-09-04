#pragma once

#include "../Logger/Logger.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <DNSServer.h>       
#include <ESPmDNS.h>         
#include <LittleFS.h> 
#include <Preferences.h>     
#include "Elevator/Elevator.h"

enum class WifiState {
    CONNECTING_STA, // Попытка подключения к домашнему Wi-Fi
    AP_MODE,        // Режим точки доступа (Captive Portal)
    STA_MODE        // Успешно подключены к Wi-Fi
};

class WebManager {
public:
    WebManager();
    
    // Инициализация файловой системы, Wi-Fi и роутеров
    void init(Elevator* elevatorPtr); 

    // ЕДИНЫЙ метод обновления, вызываемый в main loop()
    void update(); 

    // Сброс настроек Wi-Fi и запуск AP-режима
    void resetWifiSettings();

    // Отправка строки во все подключенные WebSocket-клиенты (например, для логов)
    void broadcastWs(const String& payload);

private:
    AsyncWebServer m_server{80};
    AsyncWebSocket m_ws{"/ws"};
    DNSServer      m_dnsServer;   
    Preferences    m_prefs;       

    WifiState      m_wifiState{WifiState::CONNECTING_STA};
    uint32_t       m_lastBroadcastMs{0};
    String         m_ssid;
    String         m_password;

    Elevator*      m_elevator{nullptr}; 

    // Внутренние методы настройки
    void loadCredentials();
    void saveCredentials(const String& ssid, const String& pass);
    void startAPMode();
    void startSTAMode();
    
    void setupRoutes();
    void setupCaptivePortalRoutes();    
    void setupWebSocket();

    // Обработчики и рассылка
    void handleWsCommand(uint8_t* data, size_t len);
    void broadcastStatus();
};