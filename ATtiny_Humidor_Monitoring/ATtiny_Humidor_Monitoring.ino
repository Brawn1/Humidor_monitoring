/*
 * Humidor Überwachung mit einem ATtiny85 und einem 433MHz Sender.
 * 
 * Der ATtiny85 wird die Sensorendaten von der Luftfeuchte und Temperatur Auswerten, und falls es einen Grenzwert
 * überschreitet einen Lüfter für die Luftfeuchte einschalten.
 * 
 * Somit erzeugt es die richtige Feuchtigkeit im Humidor.
 * 
 * Da der ATtiny85 nur begrenzte Pins hat, wird noch ein Shiftregister und ein Transistor in die Schaltung eingebaut.
 * 
 * 
 * ATtiny85:
 * ---------
 * VCC = 5V
 * GND = GND
 * PB0 = Shift Register Clock
 * PB1 = Shift Register Serial Data
 * PB2 = Shift Register Latchpin
 * PB3 = 433MHz Sender
 * PB4 = DHT11 Sensor
 * 
 * shiftRegister SN74HC165N Belegung:
 * out1 = schieber / oeffner
 * out2 = Luefter
 * in planung:
 * out3 = 2 Wire LCD Pin 1
 * out4 = 2 Wire LCD Pin 2
 * 
 * Info:
 * Falls ein anderer Arduino Controller verwendet wird, muss man die Pins anpassen.
 * 
 * MIT License:
 * Copyright (c) 2017 Günter Bailey
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * MIT Lizenz in Deutsch:
 * Copyright (c) 2017 Günter Bailey
 * 
 * Hiermit wird unentgeltlich jeder Person, die eine Kopie der Software und der zugehörigen Dokumentationen (die "Software") erhält, 
 * die Erlaubnis erteilt, sie uneingeschränkt zu nutzen, inklusive und ohne Ausnahme mit dem Recht, sie zu verwenden, 
 * zu kopieren, zu verändern, zusammenzufügen, zu veröffentlichen, zu verbreiten, zu unterlizenzieren und/oder zu verkaufen, 
 * und Personen, denen diese Software überlassen wird, diese Rechte zu verschaffen, unter den folgenden Bedingungen:
 * 
 * Der obige Urheberrechtsvermerk und dieser Erlaubnisvermerk sind in allen Kopien oder Teilkopien der Software beizulegen.
 * DIE SOFTWARE WIRD OHNE JEDE AUSDRÜCKLICHE ODER IMPLIZIERTE GARANTIE BEREITGESTELLT, 
 * EINSCHLIESSLICH DER GARANTIE ZUR BENUTZUNG FÜR DEN VORGESEHENEN ODER EINEM BESTIMMTEN ZWECK SOWIE JEGLICHER RECHTSVERLETZUNG, 
 * JEDOCH NICHT DARAUF BESCHRÄNKT. IN KEINEM FALL SIND DIE AUTOREN ODER COPYRIGHTINHABER FÜR JEGLICHEN SCHADEN ODER 
 * SONSTIGE ANSPRÜCHE HAFTBAR ZU MACHEN, OB INFOLGE DER ERFÜLLUNG EINES VERTRAGES, 
 * EINES DELIKTES ODER ANDERS IM ZUSAMMENHANG MIT DER SOFTWARE ODER SONSTIGER VERWENDUNG DER SOFTWARE ENTSTANDEN.
 * 
 * 
 * https://www.arduino.cc/en/Tutorial/ShiftOut
 * 
 * Im Bereich von 65-75% relativer Luftfeuchtigkeit können Zigarren bedenkenlos langfristig gelagert werden. 
 * Vorsicht ist allerdings geboten, wenn die relative Luftfeuchtigkeit 80% übersteigt. 
 * In diesen Fällen kann die Zigarre anfangen zu faulen, es können sich Schimmelpilze und andere Pilzarten bilden.
 * 
 */
#include<stdlib.h>
//VirtualWire lib
#include <VirtualWire.h>
#undef int
#undef abs
#undef double
#undef float
#undef round

int txPin = 3; //ueber Pin 3 die Daten senden

//attiny85 shiftregister
/*
#define latchPin PB2
#define clockPin PB0
#define dataPin PB1
int dhtPin = PB4; //DHT11 Pin
*/
//arduino shiftregister
#define latchPin 9 //PL
#define clockPin 10 //
#define dataPin 8
// shiftregister
byte reg = 0;

//DHT11 und 22 lib
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define dhtPin 5 //DHT Pin
// Uncomment the type of sensor in use:
//#define DHTTYPE DHT11 // DHT 11 
#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)
DHT_Unified dht(dhtPin, DHTTYPE);

//Pausen zwischen den Messungen in Millisekunden
unsigned long stime = 10000;
unsigned long mtime = 10000;

// boolean felder
boolean fan1 = false;

void setup() {
  Serial.begin(115200);
  // Initialise the IO and ISR
  vw_set_ptt_inverted(true); // Required for RF Link module
  vw_setup(1200); // Bits per sec
  vw_set_tx_pin(txPin); // pin 3 is used as the transmit data out into the TX Link module, change this as per your needs
  //shiftregister
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  updateShiftRegister(0);
  delay(20);
  //init DHT device
  dht.begin();
  sensor_t sensor;
  Serial.println(F("setup ende"));
}

unsigned long task1, task2 = 0;
void loop() {
  unsigned long currmillis = millis();
  
  // Wenn die Zeit (worktime) kleiner als die Vergangene Zeit ist, Sende eine Nachricht.
  if ((unsigned long)(currmillis - task1) >= stime || (currmillis == 1000)) {
    Serial.println(F("check DHT22"));
    //send_msg("Testnachricht_1");
    compute_msg(get_temp(), get_hum());
    task1 = millis();
  }

  // checke die Luftfeuchtigkeit und wenn zu niedrig schalte Luefter ein.
  if ((unsigned long)(currmillis - task2) >= mtime) {
    Serial.println(F("check if hum"));
    if ((float)(get_hum() <= 65.0 || get_hum() <= 75.0)) {
      // switch ventilator on
      Serial.println(F("switch Ventilator on"));
      if (!fan1) {
        //oeffne Luefter schlitz
        int i = 1;
        //bitSet(reg, i);
        updateShiftRegister(i);
        delay(60);
        //switch fan1 on
        i = 14;
        bitSet(reg, i);
        updateShiftRegister(i);
        delay(60);
        fan1 = true;
      }
    } else {
      if (fan1) {
        Serial.println(F("Switch Ventilator off"));
        // switch fan1 off und schliesse den Schlitz
        reg = 0;
        updateShiftRegister(0);
        delay(60);
        fan1 = false;
      }
    }
    task2 = millis();
  }
  
}

void updateShiftRegister(int i) {
  Serial.print(F("REG = "));
  Serial.println(i);
  Serial.println(F("use shiftregister"));
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, MSBFIRST, i);
   digitalWrite(latchPin, HIGH);
}

float get_temp() {
  // Read temperature as Celsius (the default)
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  Serial.print(F("Temperature in C = "));
  Serial.println(event.temperature);
  return event.temperature;
}

float get_hum() {
  // Read humidity
  sensors_event_t event;  
  dht.humidity().getEvent(&event);
  Serial.print(F("Humidity in % = "));
  Serial.println(event.relative_humidity);
  return event.relative_humidity;
}

void compute_msg(float temp, float hum){
  /*
   * compute message for sending data
   * dtostrf(FLOAT,WIDTH,PRECSISION,BUFFER);
   */
   Serial.println(temp);
   Serial.println(hum);
   char str1, str2;
   str1 = dtostrf(temp,5,2,7);
   str2 = dtostrf(hum,5,2,7);
   Serial.println(str1);
   send_msg(str1);
}

void send_msg(char message) {
  /*
   * Sending data over 433MHz Sender
   */
  Serial.print(F("Sending Message = "));
  Serial.println(message);
  const char *msg = message; // this is your message to send
  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx(); // Wait for message to finish
}

