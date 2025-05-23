#include <Arduino.h>

#include "wifi/wifi.h"
#include "mqtt/MqttClient.h"
#include "signal_blocs/SignalBlocs.h"

// Déclaration des informations Wi-Fi
const char *ssid = "Freebox-Peyon";
const char *password = "Peyon17t*";
const char *mqtt_server = "192.168.1.167";
const char *mqtt_user = "mqtt_user";
const char *mqtt_password = "mqtt_password";




// Initialisation du Wi-Fi et MQTT
WifiManager wifiManager(ssid, password);
MqttClient mqttClient(mqtt_server, 1883, mqtt_user, mqtt_password);

// Initialisation du module SignalBlocs
SignalBlocs signalBlocs(1, &mqttClient);

///// TEMP /////

#define pubbksr "trains/track/sensor/"

///// TEMP /////






void setup() {
    delay(1000);

    Serial.begin(115200);
    Log.begin(LOG_LEVEL_NOTICE, &Serial);

    Log.notice(F("-------------------------\n"));
    // Connexion au Wi-Fi
    wifiManager.connect();

    // Connexion au serveur MQTT
    mqttClient.connect();

    // Vérifier la connexion MQTT
    if (mqttClient.isConnected()) {
        Serial.println("MQTT client connected. Initializing blocks...");

        // Initialiser les blocs
        signalBlocs.initBlocks();  // Appel de la nouvelle fonction d'initialisation

        Serial.println("Blocks initialized.");
    } else {
        Serial.println("MQTT client not connected at setup.");
    }
    Log.notice(F("-------------------------\n"));
}

void loop() {
    mqttClient.loop();

    static unsigned long lastDisplayTime = 0;
    if (millis() - lastDisplayTime > 5000) {
        if (wifiManager.isConnected() && mqttClient.isConnected()) {
        }

        lastDisplayTime = millis();
    }
    signalBlocs.displayBlockStates();
    delay(100);
}











// void CheckBlocks() {
//     for (int i = 0; i < NumberOfBlocks; i++) {
//         updateBlockState(i);
//     }
// }

// void updateBlockState(int blockIndex) {
//     bool isBlockActive = !digitalRead(BlockPins[blockIndex]);
    
//     if (isBlockActive) {
//         handleBlockActive(blockIndex);
//     } else {
//         handleBlockInactive(blockIndex);
//     }
// }

// void handleBlockActive(int blockIndex) {
//     BlockDebounce[blockIndex] = BlockDebounceThreshold;
//     if (LastBlockState[blockIndex] != 1) {
//         LastBlockState[blockIndex] = 1;
//         sendBlock(blockIndex);
//     }
// }

// void handleBlockInactive(int blockIndex) {
//     if (BlockDebounce[blockIndex] > 0) {
//         BlockDebounce[blockIndex]--;
//     } else {
//         if (LastBlockState[blockIndex] != 0) {
//             LastBlockState[blockIndex] = 0;
//             sendBlock(blockIndex);
//         }
//     }
// }

// void sendBlock(int blockToSend) {
//     const int sensorAddress = BaseAdd + 1 + blockToSend;
//     const char* state = LastBlockState[blockToSend] ? "ACTIVE" : "INACTIVE";
    
//     Serial.print("SensorAddress: ");
//     Serial.println(sensorAddress);
//     Serial.println(state);
// }