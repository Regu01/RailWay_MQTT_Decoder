#include "SignalBlocs.h"





SignalBlocs::SignalBlocs(int baseBoardNo, MqttClient* mqttClient) 
    : BaseBoardNo(baseBoardNo), BaseAdd(baseBoardNo * 100), _mqttClient(mqttClient) {}

void SignalBlocs::initBlocks() {
    Serial.println("Initializing blocks to INACTIVE...");

    for (int i = 0; i < NumberOfBlocks; i++) {
        LastBlockState[i] = LOW;  // INACTIVE au départ
        const int sensorAddress = BaseAdd + 1 + i;

        // Vérification si le client MQTT est connecté avant d'envoyer l'état
        if (_mqttClient->isConnected()) {
            sendBlockStateToMQTT(sensorAddress, "INACTIVE");  // Envoyer l'état INACTIVE via MQTT
            Serial.print("Block ");
            Serial.print(i);
            Serial.println(" set to INACTIVE and published to MQTT");
        } else {
            Serial.println("MQTT client not connected. Unable to send initial state.");
        }
    }
}

void SignalBlocs::displayBlockStates() {
    for (int i = 0; i < NumberOfBlocks; i++) {
        int pinState = digitalRead(BlockPins[i]);
        const int sensorAddress = BaseAdd + 1 + i;

        if (LastBlockState[i] != pinState) {  // Detect state change
            const char* state = pinState ? "INACTIVE" : "ACTIVE";
            sendBlockStateToMQTT(sensorAddress, state);  // Send the new state to MQTT
            LastBlockState[i] = pinState;  // Update the last known state
        }
    }
}

void SignalBlocs::sendBlockStateToMQTT(int blockIndex, const char* state) {
    char topic[50];
    snprintf(topic, sizeof(topic), "trains/track/sensor/%d", blockIndex);
    _mqttClient->publish(topic, state);
    Log.notice("MQTT: Published %d to %d" CR, state, topic);
}

// void SignalBlocs::handleDebounce(int blockIndex, int pinState) {
// }