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

String device1="a";
String device2="b";
String device3="c";
String device4="d";
String device5="e";
String device6="f";// node name

String msg="";


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  20         /* Time ESP32 will go to sleep (in seconds)43200 12 hours */

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
void setup() {
  Serial.begin(115200);                   // initialize serial
  while (!Serial);
//**************** Lora Initialization *****************
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
// *************** Deep sleep *******************

//Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");


}
void loop() {

  sensorData[0]=10;// testing sample values (usually come from sensors)
  sensorData[1]=1000;
 while (msg != "a")
 {
    String message = "";
    message=device1+"*"+String(sensorData[0])+"*"+String(sensorData[1]);//a*10*1000 
    LoRa_sendMessage(message); // send a message
    Serial.println("Message Sent from Node "+device1);
   
    delay(200);
 }
 if(msg == device1)
 {
  msg = "";
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
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
  
//  Dictionary *dict = new Dictionary(2); // create dict for two sensors
  String message = "";
  int totalsplits=4; // variable for incoming string message having 4 delimiter
  while (LoRa.available()) {
    message += (char)LoRa.read(); // read message if received from lora transceiver
  }
//  StringSplitter *splitter = new StringSplitter(message, '*', totalsplits); // 
//  int itemCount = splitter->getItemCount();
//  //Serial.println(itemCount);
//  int i=0;
//  while(i<itemCount)
//  {
//    String keys = splitter->getItemAtIndex(i); 
//    i=i+1; 
//    String vals = splitter->getItemAtIndex(i);
//    dict->insert(keys, vals); 
//    i=i+1;  
//  }
//  for (int i = 0; i < dict->count(); i++)
//    {
//      Serial.print(dict->key(i)); // print key
//      Serial.print(":"); // 
//      Serial.println(dict->value(i));// print value
//    }
  Serial.println("Message Received at Node!");
  Serial.println(message);
  msg=message;
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
