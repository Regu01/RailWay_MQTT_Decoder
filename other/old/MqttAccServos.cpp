// //Version = 4;
// #include <avr/pgmspace.h>
// #include <Servo.h>
// #include <Wire.h>
// #define numberOfServos 16
// #define servoMinDegrees 20
// #define servoMaxDegrees 160
// #define servoChannels 4					//How many servos to move at a time
// #define servoWait 13						  //Servo move time in 100's of milliseconds - 100 (6 = 500ms)

// Servo servoChannel1;
// Servo servoChannel2;
// Servo servoChannel3;
// Servo servoChannel4;

// const byte actLed = A3;
// bool actLedState = 0;
// const bool LEDon = LOW;	
// const bool LEDoff = HIGH;
// byte servoOutputPins[numberOfServos] = {4,3,5,6,7,8,9,10,11,12,13,A0,A1,A2,A6,A7};
// byte servoMovers[servoChannels]={99,99,99,99};
// byte servoMoversCounts[servoChannels]={0,0,0,0};

// byte newServoPosition[numberOfServos]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
// byte lastServoPosition[numberOfServos]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 

// unsigned long prevmillis;
// unsigned long currentmillis;
// unsigned long interval = 100; 			// delay between consecutive checks

// void setup() {
//   pinMode(actLed, OUTPUT);
//   digitalWrite(actLed, LEDon);   		// turn the LED on (HIGH is the voltage level)
//   Serial.begin(115200);
//   Serial.println("Started Serial"); 
//   Wire.begin(0x08);                		// join i2c bus with address #8
//   Wire.onReceive(receiveEvent); 		  // register event
//   Serial.println("Started I2C"); 
//   delay(1000);
//   digitalWrite(actLed, LEDoff);  
// }

// void loop() {
// 	digitalWrite(actLed, actLedState);   	// turn the LED on
 
// 	currentmillis = millis();
// 	if (currentmillis - prevmillis > interval){ 
// 		detachServos();
// 		checkServos();
// 		prevmillis = millis();
// 	}
//   int totalSC;
//   for(int i=0; i<servoChannels; i++){
//     totalSC = totalSC + servoMoversCounts[i];
//   }
//   if(totalSC > 0){
//     actLedState = LEDon;
//   }else{
//     actLedState = LEDoff;
//   }
// }

// void checkServos(){
// 	for(int i=0; i<numberOfServos; i++){
// 		if(newServoPosition[i] != lastServoPosition[i]){
// 			if(newServoPosition[i] >= servoMinDegrees && newServoPosition[i] <= servoMaxDegrees ){
// 				//Serial.print("Servo ");
// 				//Serial.print(i);
// 				//Serial.println(" new value OK");
// 				//assign move job to a servo mover 
// 				bool servoNotAssigned = true;
//         for (int k=0; k<servoChannels; k++){
//           if(servoMovers[k] == i){
//             servoNotAssigned = false;
//             //Serial.println("Servo already moving");
//           }          
//         }
// 				for(int j=0; j<servoChannels; j++){
// 					if(servoMoversCounts[j] == 0 && servoNotAssigned){
// 						//freeChannelFound = true;
//             servoMoversCounts[j] = 1;
//             Serial.print("Servo ");
//             Serial.print(i+1);
//             Serial.print(", channel ");
//             Serial.print(j);
//             Serial.print(", position ");
//             Serial.println(newServoPosition[i]);
// 						lastServoPosition[i] = newServoPosition[i];
// 						servoMovers[j] = i;				//Assing the free servo mover channel to a servo 
// 						moveServo(j,i);					  //(Mover Channel, Servo Number)
// 						break;							      //Exit loop because we have found a free channel
// 					}
//           else{
//             //Serial.print("Channel ");
//             //Serial.print(j);
//             //Serial.println(" in Use");
//           }          
// 				}
// 			}
// 		}
// 	}
// }

// void moveServo(byte servoMoverChannel, byte servoNumber){
//   //Serial.print("attached to Channel ");
//   //Serial.println(servoMoverChannel);
// 	switch (servoMoverChannel) {
// 		case 0:
//       servoChannel1.attach(servoOutputPins[servoNumber]);
//       servoChannel1.write(lastServoPosition[servoNumber]);
// 		break;
// 		case 1:
//       servoChannel2.attach(servoOutputPins[servoNumber]);
//       servoChannel2.write(lastServoPosition[servoNumber]);
// 		break;
// 		case 2:
//       servoChannel3.attach(servoOutputPins[servoNumber]);
//       servoChannel3.write(lastServoPosition[servoNumber]);
// 		break;
// 		case 3:
//       servoChannel4.attach(servoOutputPins[servoNumber]);
//       servoChannel4.write(lastServoPosition[servoNumber]);
// 		break;
// 	}
// }


// void detachServos(){
// 	//Serial.print("Channel:");
//   for(int i=0; i<servoChannels; i++){
// 		if (servoMoversCounts[i] > 0 ){
// 			servoMoversCounts[i] = servoMoversCounts[i] + 1;
// 		}
// 		if (servoMoversCounts[i] >= servoWait) {
// 			servoMoversCounts[i] = 0;
//       servoMovers[i] = 99;
//       //Serial.print("detach Channel ");
//       //Serial.println(i);
// 			switch (i) {
// 				case 0:
// 				  servoChannel1.detach();
// 				break;
// 				case 1:
// 				  servoChannel2.detach();
// 				break;
// 				case 2:
// 				  servoChannel3.detach();
// 				break;
// 				case 3:
// 				  servoChannel4.detach();
// 				break;
// 			}
// 		}
//     //Serial.print(servoMoversCounts[i]);
//     //Serial.print(",");    
// 	}
//   //Serial.println("");
// }


// //void receiveEvent(int howmnay){
// void receiveEvent(int howmnay){
// 	digitalWrite(actLed, LEDon);  
// 	for(int i=0; i<numberOfServos; i++){
// 	  newServoPosition[i] = Wire.read();  
// 	  //Serial.print("I2C ");
// 	  //Serial.print(i);
// 	  //Serial.print(" = ");
// 	  //Serial.println(newServoPosition[i]);
// 	}
// 	digitalWrite(actLed, LEDoff);  
// }