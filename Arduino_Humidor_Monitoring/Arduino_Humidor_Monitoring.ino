/*
 * Humidor Überwachung mit einem Arduino Mega 2560 R3 und einem ESP8266 WIFI Modul.
 * 
 * Der Arduino wertet die Sensorendaten von Luftfeuchtigkeit und Temperatur aus, und falls es einen Grenzwert
 * unterschreitet, öffnet es die Befeuchtungskammer und schaltet einen Lüfter ein. 
 *  
 * Somit erzeugt es die richtige Luftfeuchtigkeit im Humidor.
 * 
 * Als zusatz wird noch ein OLED Display am Humidor eingebaut um den derzeitigen Status anzuzeigen (leider durch die Größe
 * der Bibliothek musste ich auf einen Arduino Mega umsteigen).
 * Genauso verwenden wir die Bibliothek ArduinoJson und bauen für die Übertragung ein Json String.
 * 
 * Arduino Mega 2560 R3 Pinbelegung:
 * =================================
 * VCC = 5V
 * GND = GND
 * 18  = TX Pin ESP8266 WIFI
 * 19  = RX Pin ESP8266 WIFI
 * 3.3V= VCC ESP8266 WIFI
 * ---
 * D26 = DHT11 Sensor
 * ---
 * D8  = RST
 * 20  = I2C OLED Display (SDA)
 * 21  = I2C OLED Display (SCL)
 * ---
 * D2  = (PWM) Servo Motor
 * D3  = Luefter (Optional)
 * ---
 * D22 = LED (Power)
 * D24 = LED (Activity)
 * D28 = Power Source Servo Motor (HIGH = OFF)
 * ---
 * 
 * OLED Display SSD1306:
 * =====================
 * VCC = 3.3V oder 5V
 * GND = Ground
 * D0  = SCL (I2C)
 * D1  = SDA (I2C)
 * RES = D8
 * 
 * Taster für die Menü Steuerung
 * Taster = D9
 * 
 * Servo Motor:
 * ============
 * VCC (Red) = 5V
 * GND (Black = Ground
 * Control (Yellow) = D5 (PWM)
 * 
 * 
   MIT License:
   Copyright (c) 2017 Günter Bailey

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
   documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   MIT Lizenz in Deutsch:
   ======================
   MIT Lizenz:
   Copyright (c) 2017 Günter Bailey

   Hiermit wird unentgeltlich jeder Person, die eine Kopie der Software und der zugehörigen Dokumentationen (die "Software") erhält,
   die Erlaubnis erteilt, sie uneingeschränkt zu nutzen, inklusive und ohne Ausnahme mit dem Recht, sie zu verwenden,
   zu kopieren, zu verändern, zusammenzufügen, zu veröffentlichen, zu verbreiten, zu unterlizenzieren und/oder zu verkaufen,
   und Personen, denen diese Software überlassen wird, diese Rechte zu verschaffen, unter den folgenden Bedingungen:

   Der obige Urheberrechtsvermerk und dieser Erlaubnisvermerk sind in allen Kopien oder Teilkopien der Software beizulegen.
   DIE SOFTWARE WIRD OHNE JEDE AUSDRÜCKLICHE ODER IMPLIZIERTE GARANTIE BEREITGESTELLT,
   EINSCHLIESSLICH DER GARANTIE ZUR BENUTZUNG FÜR DEN VORGESEHENEN ODER EINEM BESTIMMTEN ZWECK SOWIE JEGLICHER RECHTSVERLETZUNG,
   JEDOCH NICHT DARAUF BESCHRÄNKT. IN KEINEM FALL SIND DIE AUTOREN ODER COPYRIGHTINHABER FÜR JEGLICHEN SCHADEN ODER
   SONSTIGE ANSPRÜCHE HAFTBAR ZU MACHEN, OB INFOLGE DER ERFÜLLUNG EINES VERTRAGES,
   EINES DELIKTES ODER ANDERS IM ZUSAMMENHANG MIT DER SOFTWARE ODER SONSTIGER VERWENDUNG DER SOFTWARE ENTSTANDEN.

   Im Bereich von 65-75% relativer Luftfeuchtigkeit können Zigarren bedenkenlos langfristig gelagert werden.
   Vorsicht ist allerdings geboten, wenn die relative Luftfeuchtigkeit 80% übersteigt.
   In diesen Fällen kann die Zigarre anfangen zu faulen, es können sich Schimmelpilze und andere Pilzarten bilden.

*/
#include <ArduinoJson.h>
// Sender ID for Database Restful
unsigned long int ID = 12345678;
unsigned long int PRJID = 123456;
String SENSID = "asdf123456";

int led_pwr=22;
int led_act=24;

// ESP8266 WIFI
boolean NO_IP=false;
boolean WIFI_CONN=false;
String IP="";
int i,k,x=0;

String SERVER="88.99.175.233"; // DNS or IP to send measurement data to Server
String TOKEN="071f0a755fd206ef71928ef242c936d7f0182e59"; // AUTH Token
//unsigned int PORT=9600; // Port for Server
String URI="/v1/addlog"; // URL Path after DNS or IP
String CURRIP;

//Servo Lib
#include <Servo.h> 
int servo_pin = 2; // Declare the Servo pin
int servo_power = 28; // servo power
Servo servo1; // Create a servo object
int pos_open = 138; // open position in degrees
int pos_close = 85; // close position in degrees

//DHT11 Lib
int dhtPin = 26;
#include "DHT.h"
DHT dht;

// delay between the measurements in Milliseconds
unsigned long stime = 1800000; // delay time between the transmitting (45 min)
unsigned long mtime = 1800000; // delay time between the measurements (15 min)

boolean isopen; // Field to check if open or closed

// Memory pool for JSON object tree.
StaticJsonBuffer<110> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

/*
// time lib
#include <Time.h>
float timestart = 1496771717.958437;
time_t t;
*/
// OLED Display
int RSTpin = 8;
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET RSTpin
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

unsigned int X,Y,textsize;
String content, contbuild;
boolean displayON = true;
// button for menu selection
int inPin = 9;
int val = 0; //value 0 - 255 read from pushbutton
int switchstate = 0;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// my company Sign
//created with LCDAssistant http://en.radzio.dxp.pl/bitmap_converter/
const unsigned char logo [] = {
0x80, 0x00, 0x40, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC,
0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8,
0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
0xC0, 0xE0, 0xF0, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF8,
0xF8, 0xF0, 0xE0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x1F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0x0F, 0x0F, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x3F,
0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xFC, 0xF9, 0xF1, 0xE1, 0xC1,
0xC1, 0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xCF, 0x87, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x07, 0x07, 0x07, 0x07, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3F,
0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xFC, 0xFC, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03,
0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x1F,
0x1F, 0x1F, 0x0F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void act(int state){
  if(state==1){
    digitalWrite(led_act, LOW);
    digitalWrite(led_act, HIGH);
  } else {
    digitalWrite(led_act, LOW);
  }
}

void servo_ctrl(char cmd[5]){
  if(cmd=="open"){
    if(!isopen){
      digitalWrite(servo_power, LOW);
      // servo go to 90 degrees (open)
      for (int i=pos_close; i <= pos_open; i++){
        servo1.write(i);
        delay(25);
      }
      isopen=true;
      Serial.println(F("Servo Control write 130"));
    } else {
      // do nothing
    }
  } else {
    if(isopen){
      digitalWrite(servo_power, LOW);
      // servo go to 0 degrees (close)
      servo1.write(pos_close);
      for (int i=pos_open; i >= pos_close; i--){
        servo1.write(i);
        delay(10);
      }
      isopen=false;
      Serial.println(F("Servo Control write 15"));
    } else {
      // do nothing
    }
  }
  // close by setup
  if(cmd=="start"){
    digitalWrite(servo_power, LOW);
    servo1.write(pos_close);    
    isopen=false;
    Serial.println(F("Servo Control write 15"));
    delay(500);
  }
  digitalWrite(servo_power, HIGH);
}

void connect_wifi(String cmd, int t){
  int temp, i = 0;
  while(1)
  {
    Serial.println(cmd);
    Serial1.println(cmd);
    while(Serial1.available()) {
      if(Serial1.find("OK"))
      i=8;
    }
    delay(t);
    if(i>5)
    break;
    i++;
  }
  if(i==8)
  Serial.println(F("OK"));
  else
  Serial.println(F("Error"));
}

void check_ip(int t1){
  int t2=millis();
  while(t2+t1>millis()){
      while(Serial1.available()>0){
        if(Serial1.find("WIFI GOT IP")){
          NO_IP=true;
          WIFI_CONN=true;
        }
      }
  }
}

void get_ip(){
  IP="";
  char ch=0;
  while(1){
    Serial1.println("AT+CIFSR");
    while(Serial1.available()>0){
      if(Serial1.find("STAIP,")){
        delay(1000);
        Serial.println(F("IP Address;"));
        while(Serial1.available()>0){
          ch=Serial1.read();
          if(ch=='+')
          break;
          IP+=ch;
        }
      }
      if(ch=='+')
      break;
    }
    if(ch=='+')
    break;
    delay(1000);
  }
  if(IP==""){
    // test 
  } else {
    WIFI_CONN=true;
    NO_IP=true;
    Serial.print(IP);
    Serial.print(F("Port:"));
    Serial.println(80);
    CURRIP = IP;
  }
}

void wifi_init(){
  connect_wifi(F("AT"),100);
  connect_wifi(F("AT+CWMODE=1"),100);
  connect_wifi(F("AT+CWQAP"),100);
  connect_wifi(F("AT+RST"),10000);
  check_ip(5000);
  if(!NO_IP){
    Serial.println(F("Connecting Wifi..."));
    connect_wifi("AT+CWJAP=\"WTWLAN01\",\"60wtwlan0120\"",7000); //username,password,waittime
  } else {
    // do nothing
  }
  Serial.println(F("Wifi Connect"));
  //get_ip();
  connect_wifi(F("AT+CIPMUX=0"),100);
  check_ip(5000);
  //connect_wifi(F("AT+CIPSERVER=1,80"),100);
}


void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(led_pwr, OUTPUT);
  pinMode(led_act, OUTPUT);
  pinMode(servo_power, OUTPUT);

  digitalWrite(led_pwr, LOW);
  digitalWrite(led_act, LOW);
  digitalWrite(servo_power, HIGH);

  // build Json string
  root["device"] = ID;
  root["project"] = PRJID;
  root["sensor"] = SENSID;

  // We need to attach the servo to the used pin number 
  servo1.attach(servo_pin);
  // send to close
  servo_ctrl("start");
  
  dht.setup(dhtPin);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  blankOLED();
  printOLED(content="Humidor Monitor v0.3", X=0, Y=0, textsize=1);
  clearOLED();
  
  pinMode(inPin, INPUT); // declare pushbutton

  //only 1 time activate for new wifi
  //wifi_init();

  //time_t t=timestart;
  Serial.println(F("Setup End"));
}

unsigned long task1, task2, task3, task4, task5, task6 = 0;
void loop() {
  unsigned long currmillis = millis();

  //falls die Befeuchtung aktiv ist pruefe alle 30 Sekunden die Werte
  if (isopen) {
    if ((unsigned long)(currmillis - task2) >= 900000) {
      act(1);
      //Serial.println(F("check if hum"));
      if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
        if (!isopen) {
          //oeffne die Belueftung und starte den Luefter
          Serial.println(F("Servo Open"));
          servo_ctrl("open");
        }
        act(0);
      } else {
        if (isopen) {
          // switch fan1 off und schliesse den Schlitz
          Serial.println(F("Servo Close"));
          servo_ctrl("close");
          act(0);
        }
      }
      task2 = millis();
    }
  }


  // Wenn die Zeit (worktime) kleiner als die Vergangene Zeit ist, Sende die Messdaten.
  if ((unsigned long)(currmillis - task1) >= stime || (currmillis == 10000)) {
    act(1);
    root["temp"] = get_temp();
    root["hum"] = get_hum();
    TransmitData();
    task1 = millis();
    act(0);
    Serial.println("send data");
  }

  // Sende Messdaten wenn die Befeuchtung aktiv ist alle 60 Sekunden
  if(isopen){
    if((unsigned long)(currmillis - task3) >= 900000){
      //TransmitData(get_temp(), get_hum());
      act(1);
      root["temp"]=get_temp();
      root["hum"]=get_hum();
      TransmitData();
      task3 = millis();
      act(0);
    }
  } 

  // checke die Luftfeuchtigkeit und wenn zu niedrig schalte die Befeuchtung ein.
  if ((unsigned long)(currmillis - task2) >= mtime) {
    //Serial.println(F("check if hum"));
    if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
      act(1);
      if (!isopen) {
        //oeffne die Belueftung und starte den Luefter
        Serial.println(F("Servo Open"));
        servo_ctrl("open");
      }
      act(0);
    } else {
      if (isopen) {
        // switch fan1 off und schliesse den Schlitz
        Serial.println(F("Servo Close"));
        servo_ctrl("close");
        act(0);
      }
    }
    task2 = millis();
  }

  // show menu
  if ((unsigned long)(currmillis - task4) >= 500){
    val = digitalRead(inPin);
    //Serial.print(F("value = "));
    //Serial.println(val);
    task4 = millis();
    if(val >= 1){  
      dspmenu(switchstate);
      task5 = millis();
    }
    val = 0;
  }

  // switch automatic menu
  if ((unsigned long)(currmillis - task6) >= 5000){
    if(displayON == true){
      dspmenu(switchstate);
    }
    task6 = millis();
  }

  // switch after 10 Seconds display off
  if ((unsigned long)(currmillis - task5) == 120000){
    Serial.println(F("switch off after time"));
    if(displayON == true){
      displaypower(false);
    }
  }
}

/*
// convert integer to 2 digits
void print2digits(int number) {
  if (number >= 0 && number < 10) {
    tpf.write('0');
  }
  tpf.print(number);
}

//get date from time
String stringdate(){
  time_t t;
  String d = String(day(t));
  d += ".";
  d += String(month(t));
  d += ".";
  d += String(year(t));
  return d;
}

//get time from time
String stringhour(){
  time_t t;
  String h = String(hour(t));
  h += ":";
  h += String(minute(t));
  h += ":";
  return h;
}
*/
// display menu
void dspmenu(int m){
  switch (m){
    case 0:
      contbuild = "Humidity\n";
      contbuild += float_to_str(float(45.5));
      //contbuild += float_to_str(float(get_hum()));
      contbuild += "%";
      
      printOLED(content=contbuild, X=0, Y=16, textsize=2);
      clearOLED();
      switchstate = 1;
      break;
    case 1:

      contbuild = "Temperatur\n";
      //contbuild += float_to_str(float(get_temp()));
      contbuild += float_to_str(float(24.7));
      contbuild += " ";
      contbuild += (char(247));
      contbuild += "C";
      printOLED(content=contbuild, X=0, Y=16, textsize=2);
      clearOLED();
      switchstate = 2;
      break;
    case 2:

      contbuild = "IP Adresse\n";
      if(CURRIP == ""){
        contbuild += "NO IP";
      } else {
        contbuild += CURRIP;
      }
      
      printOLED(content=contbuild, X=0, Y=16, textsize=1); //print content
      clearOLED();
      switchstate = 3;
      break;
    case 3:
      contbuild = TOKEN;
      printOLED(content="WEB TOKEN", X=0, Y=0, textsize=2); //print header line
      printOLED(content=contbuild, X=0, Y=16, textsize=2); //print content
      clearOLED();
      switchstate = 0;
      break;
  }
}

// Read temperature as Celsius (the default)
float get_temp() {
  return dht.getTemperature();
}

// Read humidity
float get_hum() {
  return dht.getHumidity();
}

// check if wifi is connected and set WIFI_CONN and NO_IP to True
void chk_wifi_conn(){
  if(!WIFI_CONN){
    // get ip address
    Serial.println(F("get ip"));
    get_ip();
    delay(2000);
    // check ip and NO_IP to True
    check_ip(5000);
  }

  if(!NO_IP && !WIFI_CONN){
    wifi_init();
  } else {
    NO_IP=true;
    WIFI_CONN=true;
  }
}

// Transmit Data over Wifi
void TransmitData(){
  /*
  * Send JSON Data
  */
  Serial.println(F("Start transmit data"));
  if(isopen){
    root["isopen"]="True";
  } else {
    root["isopen"]="False";
  }

  root["vent"] = "False";
  
  if(!WIFI_CONN){
    Serial.println(F("check wifi conn"));
    chk_wifi_conn();
  }

  String jsonstring;
  root.printTo(jsonstring);
  Serial.println(F("connect to server"));

  Serial1.print("AT+CIPSTART=\"TCP\",");
  Serial1.print("\"88.99.175.233\"");
  Serial1.print(",");
  Serial1.println(8000);
  if(Serial1.find("OK")){
    Serial.println(F("TCP connection ready"));
  }
  delay(500);

  String postRequest = "POST "+URI+" HTTP/1.0\r\n"+
  "HOST:"+SERVER+"\r\n"+
  "Accept: *"+"/"+"*\r\n"+
  "Content-Length:"+jsonstring.length()+"\r\n"+
  "Content-Type: application/json\r\n"+
  "Authorization: Token "+TOKEN+"\r\n"+
  "\r\n"+jsonstring;

  Serial.print(F("AT+CIPSEND="));
  Serial.println(postRequest.length() );
  Serial1.print(F("AT+CIPSEND="));
  Serial1.println(postRequest.length() );

  delay(1000);

  if(Serial1.find(">")){
    Serial.println(F("Sending..."));
    Serial1.print(postRequest);
    if(Serial1.find("SEND OK")){
      Serial.println(F("Packet sent"));
      while(Serial1.available()){
        String tmpResponse = Serial1.readString();
        Serial.println(tmpResponse);
        //time_t t=tmpResponse.toFloat();
      }
      Serial1.println(F("AT+CIPCLOSE"));
    }
  } else {
    Serial1.println(F("AT+CIPCLOSE"));
  }
}

void displaypower(boolean state){
  // switch OLED Display ON or OFF
  if(state == true && displayON == false){
    display.ssd1306_command(SSD1306_DISPLAYON); // To switch display back on
    displayON=true;
    Serial.println(F("switch OLED ON"));
  } else if(state == false && displayON == true){
    display.ssd1306_command(SSD1306_DISPLAYOFF); // To switch display off
    Serial.println(F("switch OLED OFF"));
    displayON=false;
  }
}

String float_to_str(float num){
  // convert float to string
  char result[6];
  dtostrf(num, 3, 1, result);
  return result;
}

void clearOLED(){
  //clear displaybuffer
  display.clearDisplay();
}

void blankOLED(){
  // switch display blank
  display.clearDisplay();
  display.display();
}

void printOLED(String content, unsigned int X, unsigned int Y, unsigned int textsize){
  // print content to OLED display
  if(displayON == false){
    displaypower(true);
  }
  display.setTextColor(WHITE); // set text color
  display.setTextSize(textsize); // set text size
  display.setCursor(X,Y); // set text cursor position
  display.print(content); //print content
  display.display();
}

