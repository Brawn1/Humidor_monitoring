/*
 * Humidor Überwachung mit einem ATtiny85 und einem 433MHz Sender.
 * 
 * Der ATtiny85 wird die Sensorendaten von der Luftfeuchte und Temperatur Auswerten, und falls es einen Grenzwert
 * unterschreitet, einen Lüfter für die Luftfeuchte einschalten.
 * 
 * Somit erzeugt es die richtige Feuchtigkeit im Humidor.
 * 
 * Da der ATtiny85 nur begrenzte Pins hat, wird noch ein Shiftregister und ein Transistor in die Schaltung eingebaut.
 * 
 * ATtiny85:
 * ---------
 * VCC = 5V
 * GND = GND
 * PB0 = Shift Register Serial Data
 * PB1 = Shift Register Clock
 * PB2 = Shift Register Latchpin
 * PB3 = 433MHz Sender
 * PB4 = DHT11 Sensor
 * 
 * shiftRegister SN74HC165N Belegung:
 * ----------------------------------
 * out1 = schieber / oeffner
 * out2 = Luefter
 * out3 = LED (Aktiv)
 * 
 * Info:
 * Falls ein anderer Arduino Controller verwendet wird, muss man die Pins anpassen.
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
// ID fuer den Sender
byte ID = 10;
//VirtualWire lib
#include <VirtualWire.h>
/* //mit dieser Option gibt es einen Fehler beim Kompilieren
#undef int
#undef abs
#undef double
#undef float
#undef round
*/
int txPin = 3; //ueber Pin 3 die Daten senden

int out1 = 1;

/*
// Reservierung fuer Display
int sclPin = 2;
int scaPin = 0;
*/

//DHT11 und 22 lib
#include <DHT.h>
#include <DHT_U.h>
#define dhtPin 4 //DHT Pin
// Uncomment the type of sensor in use:
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)
DHT_Unified dht(dhtPin, DHTTYPE);

//Pausen zwischen den Messungen in Millisekunden
unsigned long stime = 10000;
unsigned long mtime = 10000;

boolean fanOn;

void setup() {
  //Serial.begin(115200);
  // VirtualWire Initialise the IO and ISR
  vw_set_ptt_inverted(false); // Required for RF Link module
  vw_set_tx_pin(txPin);
  vw_setup(1200); //Bits pro Sekunde
  
  pinMode(out1, OUTPUT);
  digitalWrite(out1, LOW);
  fanOn = false;
  
  //init DHT device
  dht.begin();
  sensor_t sensor;
}

unsigned long task, task1, task2 = 0;
void loop() {
  unsigned long currmillis = millis();

  //falls der Luefter laeuft pruefe alle 5 Sekunden die Werte
  if (fanOn) {
    if ((unsigned long)(currmillis - task2) >= 5000) {
      byte i;
      //Serial.println(F("check if hum"));
      if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
        if (!fanOn) {
          //oeffne die Belueftung und starte den Luefter
          digitalWrite(out1, HIGH);
          fanOn = true;
        }
      } else {
        if (fanOn) {
          // switch fan1 off und schliesse den Schlitz
          digitalWrite(out1, LOW);
          fanOn = false;
        }
      }
      task2 = millis();
    }
  }

  // Wenn die Zeit (worktime) kleiner als die Vergangene Zeit ist, Sende die Messdaten.
  if ((unsigned long)(currmillis - task1) >= stime || (currmillis == 1000)) {
    //Serial.println(F("check DHT22"));
    TransmitData(get_temp(), get_hum());
    task1 = millis();
  }

  // checke die Luftfeuchtigkeit und wenn zu niedrig schalte Luefter ein.
  if ((unsigned long)(currmillis - task2) >= mtime) {
    byte i;
    //Serial.println(F("check if hum"));
    if ((float)(get_hum() <= 65.0 || get_hum() <= 70.0)) {
      if (!fanOn) {
        //oeffne die Belueftung und starte den Luefter
        //Serial.println(F("Switch fan on"));
        digitalWrite(out1, HIGH);
        fanOn = true;
      }
    } else {
      if (fanOn) {
        // switch fan1 off und schliesse den Schlitz
        //Serial.println(F("Switch fan off"));
        digitalWrite(out1, LOW);
        fanOn = false;
      }
    }
    task2 = millis();
  }
}

float get_temp() {
  // Read temperature as Celsius (the default)
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  return event.temperature;
}

float get_hum() {
  // Read humidity
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  return event.relative_humidity;
}

void TransmitData(float temp, float hum) {
  /*
   * Erstelle eine Datenstruktur und
   * sende es danach mit einer simplen 
   * Pruefsumme an den Empfaenger
   * 
  */

  struct wData_STRUCT {
    byte identity;
    float s1;
    float s2;
    int checksum;
  };

  //Defines structure wData as a variable (retaining structure);
  wData_STRUCT wData;

  //Fill in the data.
  wData.identity = ID;
  wData.s1 = (temp * 100);
  wData.s2 = (hum * 100);
  wData.checksum = ((temp * 100) + (hum * 100)); //erstelle eine Pruefsumme mit den 2 Werten

  vw_send((uint8_t *)&wData, sizeof(wData));
  vw_wait_tx();
}

