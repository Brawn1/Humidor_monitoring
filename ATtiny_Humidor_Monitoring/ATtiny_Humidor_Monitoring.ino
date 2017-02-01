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
 */
//attiny85
/*
#define latchPin PB2
#define clockPin PB0
#define dataPin PB1
*/
//arduino
#define latchPin A1
#define clockPin A0
#define dataPin A2
//manchester lib
#include <Manchester.h>
#define TX_PIN  5  //uebertragungs Pin

//Pausen zwischen den Messungen in Millisekunden
unsigned long mtime = 10000;

/*
 * Verschiedene Uebertragungsgeschwindigkeiten sind moeglich.
 * Variiert von der Distanz und Empfangsqualitaet
 * 
 * Bei den Tests hat am besten die MAN_1200 Funktioniert.
 * 
 * MAN_300 0
 * MAN_600 1
 * MAN_1200 2
 * MAN_2400 3
 * MAN_4800 4
 * MAN_9600 5
 * MAN_19200 6
 * MAN_38400 7
 */

void setup() {
  //Serial.begin(115200);

  //manchester lib
  man.workAround1MhzTinyCore(); //patch fuer 1Mhz ATtinyx4/x5
  man.setupTransmit(TX_PIN, MAN_1200);
  
  //shiftregister
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

char msg[] = "adsfED";

byte count = 1;
unsigned long task1 = 0;
void loop() {
  unsigned long currmillis = millis();
  
  // Wenn die Zeit (worktime) kleiner als die Vergangene Zeit ist, schalte ab.
  if ((unsigned long)(currmillis - task1) >= mtime || (currmillis == 1000)) {
    uint16_t msglen = sizeof(msg)-1;
    man.transmitArray(msglen, msg);
    task1 = millis();
  }
  /*
  //if ((unsigned long)(currmillis - task1) == 1000) {
  //rf_receive();
  //}
  */
}

/*
void rf_receive() {
  if (mySwitch.available()) {
    
    int value = mySwitch.getReceivedValue();
    
    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      Serial.print("Received ");
      Serial.print( mySwitch.getReceivedValue() );
      Serial.print(" / ");
      Serial.print( mySwitch.getReceivedBitlength() );
      Serial.print("bit ");
      Serial.print("Protocol: ");
      Serial.println( mySwitch.getReceivedProtocol() );
    }

    mySwitch.resetAvailable();
  }
}
*/
