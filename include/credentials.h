#pragma once

// Logging control: set to 0 to keep only errors, 1 to enable normal logs
#ifndef ENABLE_LOG_INFO
#define ENABLE_LOG_INFO 0
#endif

#include <stdint.h>

#if !defined(LOG_LEVEL)
  #if ENABLE_LOG_INFO
    #define LOG_LEVEL LOG_LEVEL_NOTICE
  #else
    #define LOG_LEVEL LOG_LEVEL_ERROR
  #endif
#endif

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
