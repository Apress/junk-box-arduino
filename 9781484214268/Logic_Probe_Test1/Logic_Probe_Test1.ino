// Logic Probe Test 1
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
//----------------------------------------------------------
//This sketch checks to see if the value of PB0 changes due
// to the 74xx00's input rising. We mask out all the bits
// except PB0 so we don't pick up noise from uninvolved pins.
// ---------------------------------------------------------
//Hardware:
//74xx00 Logic Probe
//
// Wiring:
// Logic probe input connected to PB0/Digital1/Pin 1 of the
// ATmega 1284P. Optional: a 10k pull-down resistor from PB0 to 
// the - bus. On my board it doesn't make any difference.

//precompiler definitions.
#define DELAYTIME 100 //How many mS (milliseconds) per update?

//global variables
byte counter = 0;

//-----------------------------------------------------------
//Zerobee
//-----------------------------------------------------------
//This function returns a string with the binary equivilent of
//the value passed to it in input expressed in 0bxxxxxxxx
//notation, with 8 bits for each byte regardless of their value.
//IMHO this makes binary values easier to read and compare.
//-----------------------------------------------------------
String zerobee(byte input) {
  String temp = "";
  for (int c = 0; c <= 7; c++) {
    if (input % 2) {
      temp = "1" + temp;
    } else {
      temp = "0" + temp;
    }
    input = input / 2;
  }
  temp = "0b" + temp;
  return (temp);
}

//setup() function - runs only once.
void setup() {
  Serial.begin(115200);
  DDRB = 0; //Set DDRB so all pins are inputs;
  PORTB = 0; //Clear all pull-up resistors on portB
  Serial.println("Starting...");
}

//loop() function - runs forever.
void loop() {
  for (int c = 0; c <= 20; c++) {
    Serial.println("PORTB:" + zerobee(0x0000001 & PINB));
    delay(DELAYTIME);  //Wait DELAYTIME milliseconds.
  }
  Serial.println("Done.");
  while (0 == 0) {}; //Do nothing forever.
}
