# Lora Gateway and Node Getting Started
For Gateway and Node following libraries will be required

Node Libraries
1. #include <Dictionary.h> download from github from given link
https://github.com/arkhipenko/Dictionary
2. #include <SPI.h> Built in Arduino Library
3. #include <LoRa.h>
from library manager search Lora and install the library given by Sandeep Mistry
4. #include "StringSplitter.h" download from github from given link
https://github.com/aharshac/StringSplitter

Gateway Libraries
1. #include <WiFi.h> built in library
2. #include <AsyncTCP.h> download from github from given link
https://github.com/me-no-dev/AsyncTCP
3. #include <WebServer.h> built in library
4. #include <EEPROM.h> built in library
5. #include <ThingsBoard.h> // ThingsBoard SDK
Search thingsboard in library manager and install it
Remaining are already installed
6. #include <Dictionary.h>
7. #include <SPI.h> // include libraries
8. #include <LoRa.h>
9. #include <StringSplitter.h>

**Note:**
If missing library appears simply copy its name for example if StringSplitter.h is missing appears 
copy this name and google it like this
Google: StringSplitter.h Arduino
First link would be of github or Arduino forum go there and download the library and add it
Hardware Board Installation

To use esp32 microcontroller you will have to install the esp32 board from the board manager 
therefore go to the Tools->Board->Board Manager. Now search for esp32 and install it.
After installing select DOIT ESP32 DEVKIT V1 board
