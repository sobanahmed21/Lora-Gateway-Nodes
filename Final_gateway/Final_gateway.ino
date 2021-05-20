#include <WiFi.h>
#include <AsyncTCP.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ThingsBoard.h>    // ThingsBoard SDK
#include <Dictionary.h>
#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <StringSplitter.h>

const long frequency = 433E6;  // LoRa Frequency

const int csPin = 5;          // LoRa radio chip select
const int resetPin = 14;        // LoRa radio reset
const int irqPin = 2;          // change for your board; must be a hardware interrupt pin
int sensorData[2]; // sensors data
String value_1 = "v1", value_2 = "v2"; // Keys
String device = "N1"; // node name
String msg = "";

//char *nodeId[] = {"av1", "av2"};

Dictionary *nodeId = new Dictionary(2); // create dict for two nodes, you can increase the number to add more nodes


// Main application loop
int ch = 0; String msgG = "";bool recvTrue=false;

// Filtre anti-rebond (debouncer)
#define DEBOUNCE_TIME 250
volatile uint32_t DebounceTimer = 0;
unsigned long previousMillis = 0;
// Pin to which the button, PIR motion detector or radar is connected
#define PIN_BUTTON 25
bool buttonState = false;
int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
const char* tbIP = "text";
const char* tbTOKEN = "text";
String st;
String content, esid, epass, eip, etkn;
#define SERIAL_DEBUG_BAUD    115200

WiFiClient espClient;
WebServer server(80);
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

//Main application loop delay
int quant = 20;
// Period of sending a temperature/humidity data.
int send_delay = 2000;
// Time passed after data was sent, milliseconds.
int send_passed = 0;
// Set to true if application is subscribed for the RPC messages.
bool subscribed = false;

//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
void createWebServer(void);
void checkWifi(void);
void checkInterrupt(void);
void checkTB(void);

void IRAM_ATTR buttonpressed() {
  if ( millis() - DEBOUNCE_TIME  >= DebounceTimer ) {
    DebounceTimer = millis();
    Serial.println("Button has been pressed");
    buttonState = true;
  }
}
// Setup an application
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  while (!Serial);

  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  //----- Read eeprom----------------------------
  Serial.println("Reading EEPROM ssid");
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  Serial.println("Reading EEPROM TB IP");
  for (int i = 96; i < 128; ++i)
  {
    eip += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SerIP: ");
  Serial.println(eip);
  Serial.println("Reading EEPROM Device Token");
  for (int i = 128; i < 160; ++i)
  {
    etkn += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("DevTKN: ");
  Serial.println(etkn);
  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }
  Serial.println();
  Serial.println("Waiting.");
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    server.handleClient();
  }
  pinMode(PIN_BUTTON, INPUT_PULLDOWN);
  attachInterrupt(PIN_BUTTON, buttonpressed, RISING);
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
void loop() {
   
nodeId->insert("a", "a");
nodeId->insert("b", "b");// add nodes by same format
  // -------Reconnect to WiFi, if needed-----
  checkWifi();
  checkInterrupt();
  // Reconnect to ThingsBoard, if needed
  checkTB();
  delay(quant);
  send_passed += quant;

    // recvTrue and msgG are updated when message is received on interrupt from lora
   if(recvTrue)
   {
    String ack = recvSendData(msgG);
    recvTrue=false;
    if(ack != "NULL")
    {
    LoRa_sendMessage(ack);
    }
   }
  // Process messages
  tb.loop();
}


//----FUNCTION----

void LoRa_rxMode() {
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}
/*
  void LoRa_rxMode(){
  LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
  }
*/
void LoRa_txMode() {
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

  // Dictionary *dict = new Dictionary(2); // create dict for two sensors
  String messageI = "";
  //int totalsplits = 4; // variable for incoming string message having 4 delimiter
  while (LoRa.available()) {
    messageI += (char)LoRa.read(); // read message if received from lora transceiver
  }
  msgG = messageI;
  recvTrue=true;
  //Serial.println("Message Received at Gateway!");
  //Serial.println(LoRa.packetRssi());
}
String recvSendData(String message)
{
    //Dictionary *dict = new Dictionary(2); // create dict for two sensors
    //String message = "";
    int totalsplits = 3; // variable for incoming string message having 4 delimiter
    StringSplitter *splitter = new StringSplitter(msgG, '*', totalsplits); //
    int itemCount = splitter->getItemCount();
    int i = 0;
    Serial.println("Message Received at Gateway!");
    String node_id = splitter->getItemAtIndex(0);
    if(nodeId->search(node_id) == node_id)
    {
      for(int i = 1;i<itemCount;i++)
      {
          String nId = node_id + String(i); // key= a+1=a1
          int str_len = nId.length() + 1;
          char char_array[str_len];
          nId.toCharArray(char_array, str_len);
          String value1 = splitter->getItemAtIndex(i);
          tb.sendTelemetryFloat(char_array, int(value1.toInt()));
          Serial.println("Sending Sensor "+String(i)+" value to ThingsBoard");
          Serial.print(char_array);
          Serial.print(":");
          Serial.println(value1);
      }
      return node_id;
    }
    else
    {
    return "NULL";
    }
    
}

void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis1 = 0;
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval)
  {
    previousMillis1 = currentMillis1;
    return true;
  }
  return false;
}
void checkTB(void)
{
  if (!tb.connected()) {
    subscribed = false;
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(eip);
    Serial.print(" with token ");
    Serial.println(etkn);
    if (!tb.connect(eip.c_str(), etkn.c_str())) {
      Serial.println("Failed to connect");
      return;
    }
  }
}
void checkWifi(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi())
    {
      Serial.println("Succesfully Connected!!!");
    }
    else
    {
      //Serial.println("Turning the HotSpot On");
      //launchWeb();
      //setupAP();// Setup HotSpot
      Serial.println(esid.c_str());
      Serial.println(epass.c_str());
      int cc=0;
      WiFi.disconnect();
      Serial.println("Booting Sketch...");
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(esid.c_str(), epass.c_str());
      delay(1000);
       while ((WiFi.status() != WL_CONNECTED))
      {
      Serial.print("-");
      //server.handleClient();
      
      if(WiFi.status() == WL_CONNECTED)
      {
        Serial.println("Connected");
        break;
      }
      unsigned long currentMillis = millis();   
      
      if (currentMillis - previousMillis > 60000) {
        previousMillis = currentMillis;
        Serial.println("HI");
        ESP.restart();
      }
      }
    }
  }
}
void checkInterrupt(void)
{
  if (buttonState)
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
    Serial.println();
    Serial.println("Waiting.");
    while ((WiFi.status() != WL_CONNECTED))
    {
      Serial.print(".");
      server.handleClient();
    }
    buttonState = false;
  }
}
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(esid.c_str(), epass.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

//------------------Change WiFi Credentials--------------------------
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  String mac = String(WiFi.macAddress());
  const char *mac1 = mac.c_str();//(String(WiFi.macAddress())).c_str;
  WiFi.softAP(mac1, "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>SUTD Gateway Credentials";
      content += "<br><br><form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"Available Networks\"></form>";
      //content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID:</label><br><input name='ssid' length=32><br><label>PSWD:</label><br><input name='pass' length=64><br><label>Serv IP:</label><br><input name='tbIP' length=32><br><label>TOKEN:</label><br><input name='tbTOKEN' length=32><br><br><input value='Submit Credentials' type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qip = server.arg("tbIP");
      String qtkn = server.arg("tbTOKEN");
      if (qsid.length() > 0 && qpass.length() > 0 && qip.length() > 0 && qtkn.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 160; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println(qip);
        Serial.println("");
        Serial.println(qtkn);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);

        }
        Serial.print("Wrote: ");
        Serial.println(qsid);
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);

        }
        Serial.print("Wrote: ");
        Serial.println(qpass);
        for (int i = 0; i < qip.length(); ++i)
        {
          EEPROM.write(96 + i, qip[i]);


        }
        Serial.print("Wrote: ");
        Serial.println(qip);
        for (int i = 0; i < qtkn.length(); ++i)
        {
          EEPROM.write(128 + i, qtkn[i]);


        }
        Serial.print("Wrote: ");
        Serial.println(qtkn);
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.restart();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        ESP.restart();
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  }
}
