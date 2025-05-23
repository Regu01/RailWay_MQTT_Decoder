#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "secrets.h"
#include <PCA95x5.h>
#include <EEPROM.h>
#include <settings.h>
#include <ArduinoLog.h>

const char* ssid = SECRET_SSID; //WiFI Name
const char* password = SECRET_PASS; //WiFi Password
const char* mqttUserName = SECRET_mqttUserName; // MQTT username
const char* mqttPwd = SECRET_mqttPwd; // MQTT password
const char* clientID = SECRET_mqttClientID; // client id
const int BaseBoardNo = S_BaseAdd;    
const int BaseAdd = BaseBoardNo * 100;   

//A2 A1 A0
//L L L 32 (decimal), 20 (hexadecimal)  U3
//L L H 33 (decimal), 21 (hexadecimal)  U4
//L H L 34 (decimal), 22 (hexadecimal)  U5
//L H H 35 (decimal), 23 (hexadecimal)  U6
//H L L 36 (decimal), 24 (hexadecimal)
//H L H 37 (decimal), 25 (hexadecimal)
//H H L 38 (decimal), 26 (hexadecimal)
//H H H 39 (decimal), 27 (hexadecimal)

PCA9555 ioexU0;
PCA9555 ioexU1;
PCA9555 ioexU2;
PCA9555 ioexU3;
PCA95x5::Port::Port PORTS[] = {
  PCA95x5::Port::P00,
  PCA95x5::Port::P01,
  PCA95x5::Port::P02,
  PCA95x5::Port::P03,
  PCA95x5::Port::P04,
  PCA95x5::Port::P05,
  PCA95x5::Port::P06,
  PCA95x5::Port::P07,
  PCA95x5::Port::P10,
  PCA95x5::Port::P11,
  PCA95x5::Port::P12,
  PCA95x5::Port::P13,
  PCA95x5::Port::P14,
  PCA95x5::Port::P15,
  PCA95x5::Port::P16,
  PCA95x5::Port::P17,
};

#define CAON      LOW
#define CAOFF     HIGH
#define CKON      HIGH
#define CKOFF     LOW

#define wifiLed     23
#define mqttLed     19
#define statusLed   2
#define configLed   5      
#define actLed      18  


byte testVariable = 0;
bool testI2CVariable = 0;

WiFiClient espClient;
//WiFiClientSecure espClient;
PubSubClient client(espClient);


#define subcfgen    "trains/config/boardcfg/cfgen"       //Enable Config Mode
#define subcfgbad   "trains/config/boardcfg/bad"         //Config Board Address to configure
#define subcfgbsf   "trains/config/boardcfg/bsf"         //Config Board Selected Feedback
#define subcfgrt    "trains/config/board/#"              //Config Root to Subscribe to
#define subcfgtno   "trains/config/board/turnoutno"      //Config Turnout Number 
#define subcfgtp0   "trains/config/board/pos0no"         //Config Turnout Position 0 Closed
#define subcfgtp1   "trains/config/board/pos1no"         //Config Turnout Position 1 Thrown 
#define subcfgled   "trains/config/board/led"            //Config LED State
#define subcfginfo  "trains/config/board/info"           //Config LED State

#define subourt     "trains/track/turnout/#"              //outputs root
#define suboual     "trains/track/turnout/"               //outputs all
#define pubbksr     "trains/track/sensor/"               //Publish Blocks root

char Convert2StringReturn[10];

byte TurnoutToConfigure;
byte TurnoutToConfigureP0;
byte TurnoutToConfigureP1;
const byte MinPosition = 20;
const byte MaxPosition = 160;
const byte MinDefaultPosition = 70;
const byte MaxDefaultPosition = 110;

boolean BoardSelectedForConfig;     //Config Mode enabled
boolean ConfigMode;                 //Config Mode enabled

#define NumberOfBlocks 12
#define BlockDebounceThreshold 12
boolean LastBlockState[NumberOfBlocks]= {};
byte BlockDebounce[NumberOfBlocks]= {};  
const byte BlockPins[NumberOfBlocks]= {34,35,32,33,25,26,27,14,17,13,15,4};  

#define NumberOfServos 16
#define EEPROM_SIZE 33              //neds be be min 2 times NumberOfServos + 1
byte eppromBuffer[EEPROM_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   //0=Not Used,1-16=P0 values,17-32=p1 values, 33 bind outputs to turnouts,

boolean TurnoutState[NumberOfServos]= {}; 
boolean LastTurnoutState[NumberOfServos]= {};  

#define NumberOfSignals 42
#define SignalsTestAddress 10
boolean SignalTest = 0;
boolean SignalState[NumberOfSignals]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
const byte Signals[2][NumberOfSignals]= {
  {0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0,0,0, 1, 1, 1, 1,1,1, 1, 1,1,1,1,1,1,1,1,1, 2, 2, 2, 2, 2, 2,2,2,2,2},
  {6,7,2,3,4,5,1,0,12,13,14,15,11,10,9,8,14,15,12,13,8,9,10,11,7,6,5,4,2,3,1,0,12,13,14,15,11,10,9,8,7,6}
};

#define NumberOfAuxOut 14
bool AuxOutState[NumberOfAuxOut]= {}; 
const byte AuxOut[2][NumberOfAuxOut]= {
  {2,2,2,2,2,2, 3, 3, 3, 3, 3, 3,3,3},
  {0,1,2,3,4,5,14,15,13,12,11,10,9,8}
};

const bool AuxIn_Active = 0;
const bool AuxIn_Inactive = 1;
bool AuxInTogglesState;   //True=change in state toggles the last state (ie for momentry buttons), False=direct (ie for latching buttons and switches)
#define NumberOfAuxIn 8
#define AuxInDebounceThreshold 3
bool LastAuxInReadState[NumberOfAuxIn]= {};
byte AuxInDebounce[NumberOfAuxIn]= {};  
const byte AuxIn[2][NumberOfAuxIn]= {
  {3,3,3,3,3,3,3,3},
  {0,1,2,3,4,5,6,7}
};

unsigned long Main_prevmillis;
const int Main_Interval = 500;
unsigned long Secondary_prevmillis;
const int Secondary_Interval = 500;







////////////////////////////////////
//////////////// SETUP /////////////
////////////////////////////////////

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Serial.print("Starting Board No.");
  Serial.print(BaseBoardNo);
  Serial.print(", MQTT Address ");
  Serial.println(BaseAdd);

  ArduinoOTA.setHostname(clientID);
  ArduinoOTA.setPassword((const char *)SECRET_OTAPwd);

  //LEDs
  pinMode(wifiLed, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  pinMode(statusLed, OUTPUT);
  pinMode(configLed, OUTPUT);
  pinMode(actLed, OUTPUT);

  digitalWrite(wifiLed, CAON);
  digitalWrite(mqttLed, CAON);
  digitalWrite(statusLed, CKON);
  digitalWrite(configLed, CAON);
  digitalWrite(actLed, CAON);
  
  delay(1000);
  digitalWrite(wifiLed, CAOFF);
  digitalWrite(mqttLed, CAOFF);
  digitalWrite(statusLed, CKOFF);
  digitalWrite(configLed, CAOFF);
  digitalWrite(actLed, CAOFF);
 
  Wire.begin();
  ioexU0.attach(Wire,32);
  ioexU0.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  ioexU0.direction(PCA95x5::Direction::OUT_ALL);
  ioexU0.write(PCA95x5::Level::L_ALL);
  ioexU1.attach(Wire,33);
  ioexU1.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  ioexU1.direction(PCA95x5::Direction::OUT_ALL);
  ioexU1.write(PCA95x5::Level::L_ALL);
  ioexU2.attach(Wire,34);
  ioexU2.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  ioexU2.direction(PCA95x5::Direction::OUT_ALL);
  ioexU2.write(PCA95x5::Level::L_ALL);
  ioexU3.attach(Wire,35);
  ioexU3.polarity(PCA95x5::Polarity::ORIGINAL_ALL);
  ioexU3.direction(PCA95x5::Direction::OUT_ALL);
  ioexU3.write(PCA95x5::Level::L_ALL);
  
  for (int i=0; i<NumberOfAuxIn; i++)  {
    ioexU3.direction(PORTS[i], PCA95x5::Direction::IN);
  }
  
  EEPROM.begin(EEPROM_SIZE);               //Start Eprom with the number of bytes needed.
  //Load Eprom into buffer
  for (int i=0; i<EEPROM_SIZE; i++){
    EEPROM.get(i, eppromBuffer[i]);
    Serial.print("EPROM Address ");
    Serial.print(i);
    Serial.print(" , Vlaue Read = ");
    Serial.println(eppromBuffer[i]);
  }
  setDefaults();

  //WiFi.disconnect(true); // delete old config
  //delay(1000);
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  //WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  connect_wifi();
  
  //espClient.setInsecure();  //Dont need if using WiFiClient client;
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  
  for (int i=0; i<NumberOfBlocks; i++)  {
    pinMode(BlockPins[i],INPUT);
  }

  Serial.println("Ready");
}














////////////////////////////////////
//////////////// LOOP //////////////
////////////////////////////////////

void loop() {
  unsigned long currentmillis = millis();  
  if (currentmillis - Main_prevmillis > Main_Interval) {
    digitalWrite(statusLed, !digitalRead(statusLed));
    if (ConfigMode && BoardSelectedForConfig){
      digitalWrite(configLed, CAON);
    }
    else{
      digitalWrite(configLed, CAOFF);      
    }  
    for (int i = 0; i < NumberOfAuxOut; ++i) {
      if(AuxOutState[i]){
        writeAuxOutPCAHigh(i);
      }
      else{
        writeAuxOutPCALow(i);
      }
    }
    if(!SignalTest){ 
      for (int i = 0; i < NumberOfSignals; ++i) {
        if(SignalState[i]){                             //add ! to make common cathode instead of common anode
          writeSignalPCALow(i);
        }
        else{
          writeSignalPCAHigh(i);
        }
      }  
    }
    if(SignalTest){
      for (int i = 0; i < NumberOfSignals; ++i) {
        if(digitalRead(statusLed)){
          writeSignalPCALow(i);
        }
        else{
          writeSignalPCAHigh(i);
        }
      }
    }
    CheckAuxIn();
    Main_prevmillis = currentmillis;
  }    
  if (currentmillis - Secondary_prevmillis > Secondary_Interval) {
    CheckBlocks();
    //sendTurnouots();
    Secondary_prevmillis = currentmillis;
  }

  check_connections();
}






////////////////////////////////////
///////////// FUNCTION /////////////
////////////////////////////////////

///////////// CheckAuxIn /////////////
void CheckAuxIn() {
  for (int i = 0; i < NumberOfAuxIn; i++) {
    handleDebounce(i);
    bool currentPortState = ioexU3.read(PORTS[i]);

    if (AuxInTogglesState) {
      handleToggleMode(i, currentPortState);
    } else {
      handleNormalMode(i, currentPortState);
    }
  }
}

void handleDebounce(int index) {
  if (AuxInDebounce[index] > 0) {
    AuxInDebounce[index]--;
  }
}

void handleToggleMode(int index, bool currentPortState) {
  if (currentPortState == AuxIn_Active && AuxInDebounce[index] == 0) {
    AuxInDebounce[index] = AuxInDebounceThreshold;
    
    // Utilisation du logger à la place de Serial.print()
    Log.verbose("Read AUX %d, value = %d, Last value = %d", index, currentPortState, LastAuxInReadState[index]);
    
    LastAuxInReadState[index] = !LastAuxInReadState[index];
    sendAuxIn(index);
  }
}

void handleNormalMode(int index, bool currentPortState) {
  if (currentPortState == AuxIn_Active) {
    if (AuxInDebounce[index] == 0 && LastAuxInReadState[index] != AuxIn_Active) {
      AuxInDebounce[index] = AuxInDebounceThreshold;
      LastAuxInReadState[index] = AuxIn_Active;
      sendAuxIn(index);
      
      // Log au niveau notice pour indiquer une activation
      Log.notice("AUX %d activated", index);
    }
  } else {
    if (AuxInDebounce[index] == 0 && LastAuxInReadState[index] != AuxIn_Inactive) {
      AuxInDebounce[index] = AuxInDebounceThreshold;
      LastAuxInReadState[index] = AuxIn_Inactive;
      sendAuxIn(index);
      
      // Log au niveau notice pour indiquer une désactivation
      Log.notice("AUX %d deactivated", index);
    }
  }
}

///////////// CheckAuxIn /////////////



///////////// sendAux /////////////
void sendAuxIn(int auxInToSend) {
  char sensorTopic[100];
  createSensorTopic(sensorTopic, auxInToSend);
  
  const char* message = (LastAuxInReadState[auxInToSend] == AuxIn_Active) ? "ACTIVE" : "INACTIVE";
  
  // Publie le message au broker MQTT
  if (client.publish(sensorTopic, message, true)) {
    Log.notice("Published to topic %s: %s", sensorTopic, message);
  } else {
    Log.error("Failed to publish to topic %s", sensorTopic);
  }
}

// Fonction qui génère le topic MQTT pour un capteur donné
void createSensorTopic(char* sensorTopic, int auxInToSend) {
  strcpy(sensorTopic, pubbksr);
  
  int sensorAddress = BaseAdd + 21 + auxInToSend;
  
  // Utilise Convert2String pour obtenir une chaîne
  strcat(sensorTopic, Convert2String(sensorAddress));
}

// Fonction pour envoyer l'état de tous les inputs auxiliaires
void sendAllAuxIn() {
  for (int i = 0; i < NumberOfAuxIn; i++) {
    sendAuxIn(i);
  }
}
///////////// sendAux /////////////

// Exemple d'une fonction Convert2String qui retourne une chaîne
const char* Convert2String(int number) {
  static char str[10];  // Un tableau statique pour contenir la chaîne retournée
  itoa(number, str, 10);  // Convertit l'entier en chaîne de caractères
  return str;  // Retourne la chaîne
}





///////////// Blocks /////////////
void CheckBlocks(){
  for (int i=0; i<NumberOfBlocks; i++)  {
    if(!digitalRead(BlockPins[i])){
      BlockDebounce[i] = BlockDebounceThreshold;
      if(LastBlockState[i] != 1){
        LastBlockState[i]  = 1;
        sendBlock(i);
      }
    }
    else {
      if(BlockDebounce[i] > 0){
        BlockDebounce[i] = BlockDebounce[i] - 1;
      }
      else {
        if(LastBlockState[i] != 0){
          LastBlockState[i]  = 0;
          sendBlock(i);
        }
      }    
    }
  }
}

void sendBlock(int blockToSend){
  char SensorTopic[100] = pubbksr;
  int SensorAddress = BaseAdd+1+blockToSend;
  Convert2String(SensorAddress);
  strcat(SensorTopic,Convert2StringReturn);
  if(LastBlockState[blockToSend]){
    client.publish(SensorTopic, "ACTIVE", true);
  }  
  else{
    client.publish(SensorTopic, "INACTIVE", true);      
  }  
}

void sendAllBlocks(){
  for (int i=0; i<NumberOfBlocks; i++)  {
    sendBlock(i);
  }  
}
///////////// Blocks /////////////







void sendTurnouots(int position){
  Wire.beginTransmission(0x08);
  Serial.println("Sending Turnouts:");
  for (int i=0; i<NumberOfServos; i++){ 
    Serial.print(i);
    Serial.print(" value = ");
    if(i+1 == TurnoutToConfigure && BoardSelectedForConfig && ConfigMode){
      Wire.write(position); 
    }
    else if(TurnoutState[i]){
      Wire.write(eppromBuffer[i+1]);  
      Serial.println(eppromBuffer[i+1]);    
    }
    else{
      Wire.write(eppromBuffer[i+17]);
      Serial.println(eppromBuffer[i+17]); 
    }
  }
  Wire.endTransmission(); 
}

void writeSignalPCALow (int signalNo){
  int driverNo = Signals[0][signalNo];
  int pinNo = Signals[1][signalNo];

  switch (driverNo) {
    case 0:
      ioexU0.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 1:
      ioexU1.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 2:
      ioexU2.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 3:
      ioexU3.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
  }
}  
void writeSignalPCAHigh (int signalNo){
  int driverNo = Signals[0][signalNo];
  int pinNo = Signals[1][signalNo];
  switch (driverNo) {
    case 0:
      ioexU0.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 1:
      ioexU1.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 2:
      ioexU2.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 3:
      ioexU3.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
  }
}  


void writeAuxOutPCALow (int AuxOutNo){
  int driverNo = AuxOut[0][AuxOutNo];
  int pinNo = AuxOut[1][AuxOutNo];

  switch (driverNo) {
    case 0:
      ioexU0.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 1:
      ioexU1.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 2:
      ioexU2.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
    case 3:
      ioexU3.write(PORTS[pinNo], PCA95x5::Level::L);
      break;
  }
}  
void writeAuxOutPCAHigh (int AuxOutNo){
  int driverNo = AuxOut[0][AuxOutNo];
  int pinNo = AuxOut[1][AuxOutNo];
  switch (driverNo) {
    case 0:
      ioexU0.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 1:
      ioexU1.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 2:
      ioexU2.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
    case 3:
      ioexU3.write(PORTS[pinNo], PCA95x5::Level::H);
      break;
  }
}  

//void Convert2String(int convertMe){
//  String temp_str; 
//  temp_str = String(convertMe);                                         //convert variable to a string 
//  temp_str.toCharArray(Convert2StringReturn, temp_str.length() + 1);    //packaging up the data to publish to mqtt 
//}


void SendAll(){
  sendAllBlocks();
  sendAllAuxIn();
}
void PublishTurnoutToConfigureP0Status(){
  Convert2String(TurnoutToConfigureP0);
  client.publish(subcfgtp0, Convert2StringReturn, false);
}
void PublishTurnoutToConfigureP1Status(){
  Convert2String(TurnoutToConfigureP1);
  client.publish(subcfgtp1, Convert2StringReturn, false);
}
void PublishConfigModeOffStatus(){
  Convert2String(ConfigMode);
  client.publish(subcfgen, Convert2StringReturn, true);
}

void callback(char* topic, byte* payload, unsigned int length) {
  digitalWrite(actLed, CAON);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //for (int i = 0; i < length; i++) {
  //  Serial.print((char)payload[i]);
  //}

  boolean tempTurnoutState;
  if ((char)payload[0] == 'T'){
    tempTurnoutState = 1;
  }
  else if ((char)payload[0] == 'C'){
    tempTurnoutState = 0;
  }
  Serial.print("Temp Turnout State = ");
  Serial.println(tempTurnoutState? "P1":"P0");
  

  Serial.println("");
  payload[length] = '\0';
  int payloadInt = atoi((char*)payload);
  Serial.print("payloadInt=");
  Serial.println(payloadInt);

  if (payloadInt >= 0 && payloadInt <= 4000){
    Serial.println("payload is valid int");
    //if (strstr(topic, subref)){        //Refresh
    //  Serial.println("Refresh");
    //  SendAll();
    //}
  

    if (!strcmp(topic, subcfgen)){           //Config Enable
      Serial.print("Config Mode=");
      ConfigMode = payloadInt;
      Serial.println(ConfigMode ? "ON":"OFF");
      if (payloadInt == 1 ) {
        client.publish(subcfgtno, "0", false);
        client.publish(subcfgbsf, "0", false);
        client.publish(subcfgbad, "0", false);
        client.subscribe(subcfgrt);
        
        client.publish(subcfginfo, "Config Mode On", false); 
      } 
      else {
        TurnoutToConfigure=0;
        client.unsubscribe(subcfgrt);
        WriteEprom();
        client.publish(subcfginfo, "Config Mode Off", false); 
        client.publish(subcfgbsf, "0", false); 
      }
    }
    else if (ConfigMode && !strcmp(topic, subcfgbad)){              //Config Select Board Address
      if (payloadInt == BaseBoardNo){
        BoardSelectedForConfig=1;
        Convert2String(BaseBoardNo);
        client.publish(subcfgbsf, Convert2StringReturn, false);
      }
      else{
        BoardSelectedForConfig=0;
      }
      
    }
    else if (!strcmp(topic, subcfgtno) && BoardSelectedForConfig && ConfigMode){           //Config Select Turnout
      Serial.print("Configure Turnout=");
      TurnoutToConfigure = payloadInt;

      char infoMessage[50] = "Configure Turnout ";
      Convert2String(TurnoutToConfigure);
      strcat(infoMessage,Convert2StringReturn);
      client.publish(subcfginfo, infoMessage, false);

      Serial.println(TurnoutToConfigure);
      if(TurnoutToConfigure>0 && TurnoutToConfigure <= NumberOfServos){
        TurnoutToConfigureP0 = eppromBuffer[TurnoutToConfigure];
        TurnoutToConfigureP1 = eppromBuffer[TurnoutToConfigure + NumberOfServos];
        PublishTurnoutToConfigureP0Status();
        PublishTurnoutToConfigureP1Status();
      }
      else{
        Serial.println("Turnout address is higher than the number of outputs");
      }
    }
    else if (!strcmp(topic, subcfgtp0) && BoardSelectedForConfig && ConfigMode){           //Config Position 1
      Serial.print("Set Position 0=");
      TurnoutToConfigureP0 = payloadInt;
      Serial.println(TurnoutToConfigureP0);
      if(TurnoutToConfigureP0>=MinPosition && TurnoutToConfigureP0 <=MaxPosition){
        eppromBuffer[TurnoutToConfigure] = TurnoutToConfigureP0;
        Serial.println("Updated Eprom bufferwith new P0 value");
        sendTurnouots(TurnoutToConfigureP0);
      }
      else{
        Serial.print("Error Position 1 is out of bounds");
      }
    }
    else if (!strcmp(topic, subcfgtp1) && BoardSelectedForConfig && ConfigMode){           //Config Position 2
      Serial.print("Set Position 1=");
      TurnoutToConfigureP1 = payloadInt;
      Serial.println(TurnoutToConfigureP1);
      if(TurnoutToConfigureP1>=MinPosition && TurnoutToConfigureP1 <=MaxPosition){
        eppromBuffer[TurnoutToConfigure + NumberOfServos] = TurnoutToConfigureP1;
        Serial.println("Updated Eprom bufferwith new P1 value");
        sendTurnouots(TurnoutToConfigureP1);
      }
      else{
        Serial.print("Error Position 2 is out of bounds");
      }
    }
    else if (strstr(topic, suboual)){ 
      // Check for Signal Test
      char TopicCheck[100] = suboual;
      int CheckAddress = SignalsTestAddress;
      Convert2String(CheckAddress);
      strcat(TopicCheck,Convert2StringReturn);
      Serial.print("Checking for ");
      Serial.println(TopicCheck);
        if (!strcmp(topic, TopicCheck)){
          Serial.print("Received Signal Test ");
          Serial.println(tempTurnoutState? "On":"Off");
          Serial.print("tempTurnoutState=");
          Serial.println(tempTurnoutState);
          SignalTest=tempTurnoutState;
        }

      Serial.println("Checking for turnouts ");
      for (int i=0; i<NumberOfServos; i++){                            //Turnouts
        char TopicCheck[100] = suboual;
        int CheckAddress = BaseAdd+1+i;
        Convert2String(CheckAddress);
        strcat(TopicCheck,Convert2StringReturn);
        Serial.print("Checking for ");
        Serial.println(TopicCheck);
        if (!strcmp(topic, TopicCheck)){
          Serial.print("Turnout ");
          Serial.print(i+1);
          Serial.print(" state = ");
          Serial.println(tempTurnoutState? "P1":"P0");
          Serial.print("tempTurnoutState=");
          Serial.println(tempTurnoutState);
          TurnoutState[i]=tempTurnoutState;
          sendTurnouots(0);
        }
      }
      Serial.println("Checking for signals ");
      for (int i=0; i<NumberOfSignals; i++){                           //Signals
        char TopicCheck[100] = suboual;
        int CheckAddress = BaseAdd+21+i;
        Convert2String(CheckAddress);
        strcat(TopicCheck,Convert2StringReturn);
        Serial.print("Checking for ");
        Serial.println(TopicCheck);
        if (strstr(topic, TopicCheck)){
          Serial.print("Received Signal ");
          Serial.print(i+1);
          Serial.print(" new state = ");
          Serial.println(tempTurnoutState? "On":"Off");
          Serial.print("tempTurnoutState=");
          Serial.println(tempTurnoutState);
          //client.publish(subcfginfo, "Received signal State", false); 
          SignalState[i]=tempTurnoutState;
        }
      }
      for (int i=0; i<NumberOfAuxOut; i++){                           //AuxOut
        char TopicCheck[100] = suboual;
        int CheckAddress = BaseAdd+71+i;
        Convert2String(CheckAddress);
        strcat(TopicCheck,Convert2StringReturn);
        Serial.print("Checking for ");
        Serial.println(TopicCheck);
        if (strstr(topic, TopicCheck)){
          Serial.print("Received Signal ");
          Serial.print(i+1);
          Serial.print(" new state = ");
          Serial.println(tempTurnoutState? "On":"Off");
          Serial.print("tempTurnoutState=");
          Serial.println(tempTurnoutState);
          //client.publish(subcfginfo, "Received signal State", false); 
          AuxOutState[i]=tempTurnoutState;
        }
      }
    } 
    else {  
      Serial.println("Topic not Found");
    }

  }
  else{
    Serial.println("ERROR payload is not a valid int");
    client.publish(subcfginfo, "Invalid Payload received", false); 
  }
  digitalWrite(actLed, CAOFF);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
  Serial.println("WiFi connected");
  Serial.println("Starting OTA");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    delay(500);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    digitalWrite(configLed, !digitalRead(configLed));
    digitalWrite(statusLed, !digitalRead(configLed));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
//  digitalWrite(wifiLed, LOW);
//  Serial.println("Disconnected from WiFi access point");
//  Serial.print("WiFi lost connection. Reason: ");
//  Serial.println(info.wifi_sta_disconnected.reason);
//  connect_wifi();
//}

void check_connections(){
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(wifiLed, CAON);
      ArduinoOTA.handle();
      if (client.connected()) {         //check mqtt connected
        client.loop();
        }
      else{
        reconnectMqtt();
      }
    }
    else{
      WiFi.disconnect(true);
      digitalWrite(wifiLed, CAOFF);
      digitalWrite(mqttLed, CAOFF);
      Serial.println("WiFi disconnected, no services");
      connect_wifi();
    }  
} 

void connect_wifi() {
  Serial.println("WiFi connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLed, !digitalRead(wifiLed));
    delay(250);
  //normalProcess();      
  }
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.print("MQTT ");
  if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.print("connected, code=");
      Serial.println(client.state());
      delay(500);
      client.subscribe(subourt);
      client.subscribe(subcfgen);
      client.subscribe(subcfgbad);
      PublishConfigModeOffStatus();
      //client.publish(subcfginfo, "Connected MQTT", false); 
      SendAll();
      Serial.println("Finished Connecting MQTT");
    } 
    else {
      digitalWrite(mqttLed, CAON);
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println(" try again in a few seconds");
      delay(500);
      digitalWrite(mqttLed, CAOFF);
      delay(500);
    }
    ArduinoOTA.handle();
  }
  digitalWrite(mqttLed, CAON);
}

void WriteEprom(){
  // set the EEPROM data ready for writing
  byte eRead;
  bool commitNeeded = false;
  for (int i=0; i<=EEPROM_SIZE; i++){
    if(EEPROM.get(i, eRead) != eppromBuffer[i]){
      EEPROM.put(i,eppromBuffer[i]);
      commitNeeded = true;
    }  
  }
  if(commitNeeded){
    // write the data to EEPROM
    boolean ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
    //char infoMessage[] = "EPROM Updated Commit";
    client.publish(subcfginfo, "EPROM Updated Commit", false);
  }
  else{
    Serial.println("No new EPROM data to Commit");
  }
}

void setDefaults(){
  for (int i=1; i<=NumberOfServos; i++){
    if(eppromBuffer[i]<MinPosition || eppromBuffer[i]>MaxPosition){
      eppromBuffer[i] = MinDefaultPosition;
    }
    if(eppromBuffer[i+NumberOfServos]<MinPosition || eppromBuffer[i+NumberOfServos]>MaxPosition){
      eppromBuffer[i+NumberOfServos] = MaxDefaultPosition;
    }
  }
  if (eppromBuffer[33] > 1) {
    eppromBuffer[33] = 0;
  }
  WriteEprom();
}