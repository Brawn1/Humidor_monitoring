/*
 * Humidor Überwachung mit einem Arduino Nano v3 und einem ESP8266 WIFI Modul.
 * 
 * Der Arduino Nano wertet die Sensorendaten von Luftfeuchtigkeit und Temperatur aus, und falls es einen Grenzwert
 * unterschreitet, öffnet es die Befeuchtungskammer und schaltet einen Lüfter ein. 
 *  
 * Somit erzeugt es die richtige Luftfeuchtigkeit im Humidor.
 * 
 * Als zusatz wird noch ein OLED Display am Humidor eingebaut um den derzeitigen Status anzuzeigen.
 * Genauso verwenden wir die Bibliothek ArduinoJson und bauen für die Übertragung ein Json String.
 * 
 * Arduino Nano V3 Pinbelegung:
 * ----------------------------
 * VCC = 5V
 * GND = GND
 * D2  = TX Pin ESP8266 WIFI
 * D3  = RX Pin ESP8266 WIFI
 * 3.3V= VCC ESP8266 WIFI
 * ---
 * D4  = DHT11 Sensor
 * ---
 * A4  = I2C OLED Display (SDA)
 * A5  = I2C OLED Display (SCL)
 * ---
 * D5  = (PWM) Servo Motor
 * D7  = 5V Lüfter (Optional)
 * ---
 * D8  = LED (Activity)
 * D12 = LED (Power)
 * ---
 * 
 * OLED Display SSD1306:
 * ---------------------
 * VCC = 3.3V
 * GND = Ground
 * SCL = SCL (I2C)
 * SDA = SDA (I2C)
 * 
 * Servo Motor:
 * ------------
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
// ID fuer den Sender
unsigned long int ID = 234576987654321;

int fan_pin=7;
//int led_act=8;
//int led_pwr=12;

// ESP8266 WIFI
#include<SoftwareSerial.h>
SoftwareSerial wifi(2,3); //RX, TX
boolean NO_IP=false;
String IP="";
int i,k,x=0;

String SERVER="10.20.50.74";
String TOKEN="5245ADFAFD5784567ADFA324";
//unsigned int PORT=9600;
String URI="/";

//Servo Lib
#include <Servo.h> 
// Declare the Servo pin 
int servo_pin = 5;
// Create a servo object 
Servo servo1;

//DHT lib
int dhtPin = 4;
#include "DHT.h"
DHT dht;

//Pausen zwischen den Messungen in Millisekunden
//unsigned long stime = 1800000; // Zeit zwischen den Sendezeiten
//unsigned long mtime = 900000; // Zeit zwischen den Messungen
unsigned long stime = 20000; // Zeit zwischen den Sendezeiten
unsigned long mtime = 5000; // Zeit zwischen den Messungen

boolean isopen; // Boolean Feld fuer den Status vom Luefter.

// Memory pool for JSON object tree.
StaticJsonBuffer<100> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

/*
// OLED Display
#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
*/

void servo_ctrl(char cmd){
  if(cmd=='open'){
    if(!isopen){
      // servo go to 90 degrees (open)
      servo1.write(90);
      isopen=true;
      delay(1000);
    } else {
      // do nothing
    }
  } else {
    if(isopen){
      // servo go to 0 degrees (close)
      servo1.write(0);
      isopen=false;
      delay(1000);
    } else {
      // do nothing
    }
  }
}

void connect_wifi(String cmd, int t){
  int temp=0,i=0;
  while(1)
  {
    Serial.println(cmd);
    wifi.println(cmd);
    while(wifi.available()) {
      if(wifi.find("OK"))
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
      while(wifi.available()>0){
        if(wifi.find("WIFI GOT IP")){
          NO_IP=true;
        }
      }
  }
}

void get_ip(){
  IP="";
  char ch=0;
  while(1){
    wifi.println("AT+CIFSR");
    while(wifi.available()>0){
      if(wifi.find("STAIP,")){
        delay(1000);
        Serial.println(F("IP Address;"));
        while(wifi.available()>0){
          ch=wifi.read();
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
  Serial.print(IP);
  Serial.print(F("Port:"));
  Serial.println(80);
}

void wifi_init(){
  connect_wifi(F("AT"),100);
  connect_wifi(F("AT+CWMODE=1"),100);
  connect_wifi(F("AT+CWQAP"),100);
  connect_wifi(F("AT+RST"),5000);
  check_ip(5000);
  if(!NO_IP){
    Serial.println(F("Connecting Wifi..."));
    connect_wifi("AT+CWJAP=\"tlapse\",\"Timelapse-2016\"",7000); //username,password,waittime
  } else {
  }
  Serial.println(F("Wifi Connect"));
  //get_ip();
  connect_wifi(F("AT+CIPMUX=0"),100);
  //connect_wifi(F("AT+CIPSERVER=1,80"),100);
}


void setup() {
  Serial.begin(115200);
  wifi.begin(115200);
  wifi_init();

  root["ID"] = ID;
  root["sendtime"] = stime;
  root["measuretime"] = mtime;

  // We need to attach the servo to the used pin number 
  servo1.attach(servo_pin);
  
  pinMode(servo_pin, OUTPUT);
  digitalWrite(servo_pin, LOW);
  
  dht.setup(dhtPin);
}

unsigned long task1, task2, task3 = 0;
void loop() {
  unsigned long currmillis = millis();

  //falls der Luefter laeuft pruefe alle 15 Sekunden die Werte
  if (isopen) {
    if ((unsigned long)(currmillis - task2) >= 15000) {
      byte i;
      //Serial.println(F("check if hum"));
      if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
        if (!isopen) {
          //oeffne die Belueftung und starte den Luefter
          //Serial.println(F("fan ON"));
          servo_ctrl("open");
        }
      } else {
        if (isopen) {
          // switch fan1 off und schliesse den Schlitz
          //Serial.println(F("fan off"));
          servo_ctrl("close");
        }
      }
      task2 = millis();
    }
  }

  // Wenn die Zeit (worktime) kleiner als die Vergangene Zeit ist, Sende die Messdaten.
  if ((unsigned long)(currmillis - task1) >= stime || (currmillis == 1000)) {
    //Serial.println(F("check DHT22"));
    //TransmitData(get_temp(), get_hum());
    root["temperature"] = get_temp();
    root["Humidity"] = get_hum();
    TransmitData();
    task1 = millis();
  }

  // Sende Messdaten wenn die Oeffnungen offen sind alle 30 Sekunden
  if(isopen){
    if((unsigned long)(currmillis - task3) >= 30000){
      //TransmitData(get_temp(), get_hum());
      root["temperature"] = get_temp();
      root["Humidity"] = get_hum();
      TransmitData();
      task3 = millis();
    }
  }

  // checke die Luftfeuchtigkeit und wenn zu niedrig schalte Luefter ein.
  if ((unsigned long)(currmillis - task2) >= mtime) {
    byte i;
    //Serial.println(F("check if hum"));
    if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
      if (!isopen) {
        //oeffne die Belueftung und starte den Luefter
        //Serial.println(F("Switch fan on"));
        servo_ctrl("open");
      }
    } else {
      if (isopen) {
        // switch fan1 off und schliesse den Schlitz
        //Serial.println(F("Switch fan off"));
        servo_ctrl("close");
      }
    }
    task2 = millis();
  }
}

float get_temp() {
  // Read temperature as Celsius (the default)
  return dht.getTemperature();
}

float get_hum() {
  // Read humidity
  return dht.getHumidity();
}

void TransmitData(){
  /*
  * Send JSON Data
  */
  String jsonstring;
  root.printTo(jsonstring);

  wifi.print("AT+CIPSTART=\"TCP\",");
  wifi.print("\"10.20.50.74\"");
  wifi.print(",");
  wifi.println(9600);
  if(wifi.find("OK")){
    Serial.println(F("TCP connection ready"));
  }
  delay(1000);

  String postRequest = "POST "+URI+" HTTP/1.0\r\n"+
  "HOST:"+SERVER+"\r\n"+
  "Accept: *"+"/"+"*\r\n"+
  "Content-Length:"+jsonstring.length()+"\r\n"+
  "Content-Type: application/json\r\n"+
  "Authorization: Token "+TOKEN+"\r\n"+
  "\r\n"+jsonstring;

  Serial.print(F("AT+CIPSEND="));
  Serial.println(postRequest.length() );
  wifi.print(F("AT+CIPSEND="));
  wifi.println(postRequest.length() );

  delay(500);

  if(wifi.find(">")){
    Serial.println(F("Sending..."));
    wifi.print(postRequest);
    if(wifi.find("SEND OK")){
      Serial.println(F("Packet sent"));
      while(wifi.available()){
        String tmpResponse = wifi.readString();
        Serial.println(tmpResponse);
      }
      wifi.println(F("AT+CIPCLOSE"));
        
    }
  }
}

