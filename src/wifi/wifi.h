#ifndef WIFI_H
#define WIFI_H

#include "credentials.h"
#include <WiFi.h>
#include <ArduinoLog.h>

class WifiManager {
public:
    WifiManager(const char* ssid, const char* password);
    void connect();
    void maintain();
    bool isConnected();
    void disconnect();
    void printStatus();

private:
    const char* _ssid;
    const char* _password;

    const int WIFI_LED_PIN = 23;   // Active LOW
    const int BLINK_DELAY = 200;
    const unsigned long RECONNECT_BASE_DELAY = 2000;
    const unsigned long RECONNECT_MAX_DELAY = 20000;
    const int MAX_RETRY_ATTEMPTS = 10;

    unsigned long _lastReconnectAttempt = 0;
    unsigned long _currentReconnectDelay = RECONNECT_BASE_DELAY;
    int _retryAttempts = 0;
    void logConnectionDetails();
    void updateLed();
    void blinkDuringConnection();
};

#endif // WIFI_H
