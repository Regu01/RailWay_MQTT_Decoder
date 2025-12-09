#pragma once

// Wi-Fi credentials
constexpr const char WIFI_SSID[] = "Freebox-Peyon";
constexpr const char WIFI_PASSWORD[] = "Peyon17t*";

// MQTT settings
constexpr const char MQTT_SERVER[] = "192.168.1.167";
constexpr uint16_t MQTT_PORT = 1883;
constexpr const char MQTT_USER[] = "mqtt_user";
constexpr const char MQTT_PASSWORD[] = "mqtt_password";
constexpr const char MQTT_CLIENT_ID[] = "Board1";

// Board configuration
constexpr uint8_t BOARD_ID = 1;  // Update per device (global ID = board*100 + localId)
