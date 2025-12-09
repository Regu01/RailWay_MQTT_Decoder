#include "wifi.h"

WifiManager::WifiManager(const char* ssid, const char* password)
    : _ssid(ssid), _password(password)
{
    pinMode(WIFI_LED_PIN, OUTPUT);
    digitalWrite(WIFI_LED_PIN, HIGH); // LED OFF at startup (active LOW)
}

void WifiManager::connect() {
    Log.notice("Connecting to Wi-Fi: %s" CR, _ssid);

    _retryAttempts = 0;
    _currentReconnectDelay = RECONNECT_BASE_DELAY;
    WiFi.begin(_ssid, _password);

    for (int attempts = 0; attempts < MAX_RETRY_ATTEMPTS && WiFi.status() != WL_CONNECTED; attempts++) {
        blinkDuringConnection();  // short blocking blink during initial connect
    }

    if (WiFi.status() == WL_CONNECTED) {
        Log.notice("Connected to Wi-Fi successfully!" CR);
        logConnectionDetails();
    } else {
        Log.error("Failed to connect to Wi-Fi" CR);
    }

    updateLed();  // LED ON if connected, OFF otherwise
}

void WifiManager::maintain() {
    unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
        _retryAttempts = 0;
        _currentReconnectDelay = RECONNECT_BASE_DELAY;
        updateLed();
        return;
    }

    if (now - _lastReconnectAttempt < _currentReconnectDelay) {
        return;
    }

    _lastReconnectAttempt = now;
    if (_retryAttempts < MAX_RETRY_ATTEMPTS) {
        _retryAttempts++;
    }

    Log.warning("Wi-Fi lost. Reconnecting attempt %d" CR, _retryAttempts);
    WiFi.disconnect();
    WiFi.begin(_ssid, _password);

    blinkDuringConnection();
    updateLed();

    unsigned long nextDelay = _currentReconnectDelay * 2;
    _currentReconnectDelay = nextDelay > RECONNECT_MAX_DELAY ? RECONNECT_MAX_DELAY : nextDelay;
}

bool WifiManager::isConnected() {
    bool connected = WiFi.status() == WL_CONNECTED;

    if (!connected) {
        Log.warning("Wi-Fi is not connected." CR);
    }

    updateLed();
    return connected;
}

void WifiManager::disconnect() {
    if (isConnected()) {
        WiFi.disconnect();
        Log.notice("Disconnected from Wi-Fi." CR);
    } else {
        Log.warning("Wi-Fi was not connected." CR);
    }

    updateLed();
}

void WifiManager::printStatus() {
    if (isConnected()) {
        Log.notice("Connected to: %s with IP: %s" CR,
                    _ssid, WiFi.localIP().toString().c_str());
    } else {
        Log.warning("Not connected to any Wi-Fi network." CR);
    }
}

void WifiManager::logConnectionDetails() {
    Log.notice("SSID: %s" CR, WiFi.SSID().c_str());
    Log.notice("Signal Strength: %d dBm" CR, WiFi.RSSI());
    Log.notice("IP Address: %s" CR, WiFi.localIP().toString().c_str());
}

void WifiManager::updateLed() {
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(WIFI_LED_PIN, LOW);  // LED ON (active LOW)
    } else {
        digitalWrite(WIFI_LED_PIN, HIGH);   // LED OFF
    }
}

void WifiManager::blinkDuringConnection() {
    digitalWrite(WIFI_LED_PIN, LOW);
    delay(BLINK_DELAY);
    digitalWrite(WIFI_LED_PIN, HIGH);
    delay(BLINK_DELAY);
}
