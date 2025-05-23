#ifndef SIGNAL_BLOCS_H
#define SIGNAL_BLOCS_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <mqtt/MqttClient.h>

// Configuration des blocs
constexpr int NumberOfBlocks = 12;
constexpr int BlockDebounceThreshold = 12;

class SignalBlocs {
public:
    // Constructeur pour initialiser avec le numéro de base de la carte
    SignalBlocs(int baseBoardNo, MqttClient* mqttClient);

    // Fonctions publiques
    void sendBlock(int blockToSend); // Fonction pour envoyer l'état d'un bloc
    void displayBlockStates();       // Fonction pour afficher l'état des pins dans un tableau
    void initBlocks(); 

private:
    // Attributs privés
    const int BaseBoardNo;  // Numéro de la carte de base
    const int BaseAdd;      // Adresse de base calculée

    // Pins associés aux blocs, et données associées pour chaque bloc
    const byte BlockPins[NumberOfBlocks] = {34, 35, 32, 33, 25, 26, 27, 14, 17, 13, 15, 4};  
    // byte BlockDebounce[NumberOfBlocks] = {};          // Débouncage des blocs
    bool LastBlockState[NumberOfBlocks] = {};         // État précédent des blocs

    MqttClient* _mqttClient;

    // Fonctions privées pour modulariser le code
    void printHeader();                               // Affiche l'en-tête du tableau
    void printFooter();                               // Affiche le pied du tableau
    int readPinState(int blockIndex);                 // Lit l'état de la broche
    bool handleDebounce(int blockIndex, int pinState);// Gère le debounce d'une broche
    int calculateSensorAddress(int blockIndex);       // Calcule l'adresse du capteur
    void printBlockState(int blockIndex, int pinState, int sensorAddress); // Affiche l'état du bloc
    void sendBlockStateToMQTT(int blockIndex, const char* state); // Send state via MQTT
};

#endif
