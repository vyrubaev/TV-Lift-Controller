#pragma once

#include "../Logger/Logger.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <DNSServer.h>       // Подключаем DNS-сервер для Captive Portal
#include <ESPmDNS.h>         // Подключаем mDNS для обращения по имени tv-lift.local
#include <LittleFS.h> // Добавляем LittleFS
#include <Preferences.h>     // Подключаем работу с энергонезависимой памятью NVS
#include "Elevator/Elevator.h"

enum class WifiState {
    CONNECTING_STA, // Пытаемся подключиться к домашнему Wi-Fi
    AP_MODE,        // Работаем в режиме Точки Доступа (Portal)
    STA_MODE        // Успешно подключены к домашней сети
};

class WebManager {
public:
    WebManager();
    void init(Elevator* elevatorPtr); 
    void update(); // ВЫЗЫВАТЬ В main loop() ОБЯЗАТЕЛЬНО!

    // Метод для сброса настроек Wi-Fi (например, по длинному нажатию физической кнопки)
    void resetWifiSettings();

private:
    AsyncWebServer m_server{80};
    AsyncWebSocket m_ws{"/ws"};
    DNSServer      m_dnsServer;   // Объект DNS-сервера
    Preferences    m_prefs;       // Объект для работы с NVS памятью

    WifiState      m_wifiState{WifiState::CONNECTING_STA};
    uint32_t       m_lastBroadcastMs{0};
    String         m_ssid;
    String         m_password;

    Elevator* m_elevator{nullptr}; // Храним ссылку на лифт

    void broadcastStatus();
    void loadCredentials();
    void saveCredentials(const String& ssid, const String& pass);
    void startAPMode();
    void startSTAMode();
    void setupRoutes();
    void setupCaptivePortalRoutes();    
};