//TTL_Explorer_74xx00
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
//This sketch counts on PORTA from 0 to 255, sending those
//values to a 74xx00 connected as described below. It then
//reads the 74xx00's output pins with the first four bits
//of PORTC. Both values are displayed.
// ---------------------------------------------------------

//Hardware:
// ---------------------------------------------------------
//74xx00, any flavor (mine's an LS)
//
// Wiring: 
// Outputs are wired to PORTC, in order, lowest to highest.
// Inputs are wired to PORTA, in order, lowest to highest.
//Cestino Pin      74xx00    Cestino Pin
//(Pin 40) A0     1     14   Vcc (+ Bus)
//         A1     2     13    A7
//         C0     3     12    A6
//         A2     4     11    C3
//         A3     5     10    A5
//         C1     6     9     A4
// (- Bus) GND    7     8     C2
// ---------------------------------------------------------

//precompiler definitions.
#define DELAYTIME 10 //How many mS (milliseconds) per update?

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
  DDRA = 0b11111111; //Set DDRA to all outputs.
  DDRC = 0b11110000; //Set DDRC so pins C0-C3 are inputs.
  PORTA = 0;         //Initalize PORTA to 1.
  PORTC = 0b00001111; //Set the pullups on bits C0-C3
  Serial.println("Starting...");
}

//loop() function - runs forever.
void loop() {
  Serial.print("Port A:"+zerobee(PORTA));
  Serial.println("\tPORTC:"+zerobee(PINC));
  PORTA++;
  delay(DELAYTIME);  //Wait DELAYTIME milliseconds.
  
  if (PORTA == 0) { //If PORTA reaches zero, loop forever.
    while (true) {} 
  } //otherwise let loop() execute again. And again. And again.
}
