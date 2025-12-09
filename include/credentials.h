#pragma once

// Wi-Fi credentials
constexpr const char WIFI_SSID[] = "Freebox-Peyon";
constexpr const char WIFI_PASSWORD[] = "Peyon17t*";

// MQTT settings
constexpr const char MQTT_SERVER[] = "192.168.1.167";
constexpr uint16_t MQTT_PORT = 1883;
constexpr const char MQTT_USER[] = "mqtt_user";
constexpr const char MQTT_PASSWORD[] = "mqtt_password";

// Board configuration (set once per device)
#define BOARD_ID_NUM 1



constexpr uint8_t BOARD_ID = BOARD_ID_NUM;

// Build client ID from BOARD_ID (e.g., Board1, Board2, ...)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
constexpr const char MQTT_CLIENT_ID[] = "Board" STR(BOARD_ID_NUM);
