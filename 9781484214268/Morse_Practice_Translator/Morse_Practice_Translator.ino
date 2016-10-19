// Morse Practice Oscillator
// Copyright 2015,2016 James R. Strickland
//-----------------------------------------------------------
// This program is free software: you can redistribute it
// and/or modify it under the terms of the GNU General 
// Public License as published by the Free Software 
// Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the 
// implied warranty of MERCHANTABILITY or FITNESS FOR A 
// PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program.  If not, see 
// <http://www.gnu.org/licenses/>.
// ---------------------------------------------------
// Morse Code Practice
// This program pulls in morse.h from Mark Fickett's
// "arduinomorse" library and impliments a simple serial
// text to morse code practice sounder.
// ----------------------------------------------------
#include <morse.h> //Pull in the morse.h library. 
#define PIN_SPEAKER 21 //define the correct speaker pin.
#define PIN_LED 1  //Define what pin our LED is on.
/* ----------------------------------------------------
   Create the C++ object "receiver" of the 
   SpeakerMorseSender type.
   Set the output pin to PIN_SPEAKER, set the tone frequency
   to 440hz, -1 carrier frequency (it just adds noise), and
   set the morse code words per minute to 20. If you're
   new to morse code and trying to learn it, 5.0 might
   be a better value.
   If you'd prefer to read the Morse as LED flashes instead,
   Comment out the SpeakerMorseSender instantiation and
   uncomment the LEDMorseSender instantiation instead.
   ----------------------------------------------------*/
SpeakerMorseSender receiver(PIN_SPEAKER, 440, -1, 20.0);
//LEDMorseSender receiver(PIN_LED, 10.0);

/* Create the String object "FromSerial" because traditional
 *  C style string handling is painfully bad. */
String FromSerial = "";

/* The setup() function is one of the standard boilerplate
 * arduino functions. It executes once, so we put all our 
 * configuration code in it. */
void setup() {

  Serial.begin (115200); //Set up the serial console at 115200 Baud.
  receiver.setup(); //tell receiver to set itself up.
  
/* Set the message to be sent to CQ. CQ is traditional 
 * Morse shorthand inviting operators who hear it to respond. */
  receiver.setMessage(String("cq "));

/* Now send the message. Blocking means the sketch can do
 * nothing else while we're sending. */
  receiver.sendBlocking();
}

/* the Loop() function is one of the standard boilerplate
 *  Arduino functions too. It is called over and over again,
 *  forever. */
void loop() {
  
 /* Serial.available() returns the number of bytes available, 
  *  if any, from the serial port. If that number is not zero.
  *  If that number is equal to zero, it is equivilent to
  *  logical false, otherwise is it logical true. We don't 
  *  care how many bytes are available, just whether they 
  *  are or not. If bytes are available, use Serial.read 
  *  to read them into the FromSerial String object until they 
  *  are gone. We assume the user won't send us strings that
  *  are too long. */
  if (Serial.available()) {
    FromSerial += (char)Serial.read();
  } else  
  if (FromSerial.length()) {
    receiver.setMessage(FromSerial);
    receiver.sendBlocking();
    FromSerial = "";
  }
//Remember, in the loop() function, we repeat forever.
}
