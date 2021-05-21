/*
  LoRa Simple Gateway/Node Exemple

  This code uses InvertIQ function to create a simple Gateway/Node logic.

  Gateway - Sends messages with enableInvertIQ()
          - Receives messages with disableInvertIQ()

  Node    - Sends messages with disableInvertIQ()
          - Receives messages with enableInvertIQ()

  With this arrangement a Gateway never receive messages from another Gateway
  and a Node never receive message from another Node.
  Only Gateway to Node and vice versa.

  This code receives messages and sends a message every second.

  InvertIQ function basically invert the LoRa I and Q signals.

  See the Semtech datasheet, http://www.semtech.com/images/datasheet/sx1276.pdf
  for more on InvertIQ register 0x33.

  created 05 August 2018
  by Luiz H. Cassettari
*/
#include <Dictionary.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>

#include "StringSplitter.h"

const long frequency = 433E6;  // LoRa Frequency

const int csPin = 5;          // LoRa radio chip select
const int resetPin = 14;        // LoRa radio reset
const int irqPin = 2;          // change for your board; must be a hardware interrupt pin
int sensorData[2]; // sensors data
String value_1="v1",value_2="v2"; // Keys
String device="N1"; // node name
String msg="";
char id[] = {'a','b','c','d','e','f'};
void setup() {
  Serial.begin(115200);                   // initialize serial
  while (!Serial);

  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.println();
  Serial.println("LoRa Simple Node");
  Serial.println("Only receive messages from gateways");
  Serial.println("Tx: invertIQ disable");
  Serial.println("Rx: invertIQ enable");
  Serial.println();

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();
}
int i=0;
void loop() {
//  sensorData[0]=10;// testing sample values (usually come from sensors)
//  sensorData[1]=4000;
  if (runEvery(100)) { // repeat every 1000 millis
    String message = String(id[i]) ;
    //message=device+value_1+"*"+String(sensorData[0])+"*"+device+value_2+"*"+String(sensorData[1]);
    LoRa_sendMessage(message); // send a message
    Serial.println("Message Sent from Gateway");
    i++;
    if(i==sizeof(id))
    {
      i=0;
    }
  }
}
void LoRa_rxMode(){
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}
/*
void LoRa_rxMode(){
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
}
*/
void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

void onReceive(int packetSize) {
  
  Dictionary *dict = new Dictionary(2); // create dict for two sensors
  String message = "";
  int totalsplits=4; // variable for incoming string message having 4 delimiter
  while (LoRa.available()) {
    message += (char)LoRa.read(); // read message if received from lora transceiver
  }
  StringSplitter *splitter = new StringSplitter(message, '*', totalsplits); // 
  int itemCount = splitter->getItemCount();
  //Serial.println(itemCount);
  int i=0;
  while(i<itemCount)
  {
    String keys = splitter->getItemAtIndex(i); 
    i=i+1; 
    String vals = splitter->getItemAtIndex(i);
    dict->insert(keys, vals); 
    i=i+1;  
  }
  for (int i = 0; i < dict->count(); i++)
    {
      Serial.print(dict->key(i)); // print key
      Serial.print(":"); // 
      Serial.println(dict->value(i));// print value
    }
  Serial.println("Message Received at Gateway!");
  /*Serial.print("Gateway Receive: ");
  Serial.println(message);
 */
  
  //Serial.println(LoRa.packetRssi());
}


void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
