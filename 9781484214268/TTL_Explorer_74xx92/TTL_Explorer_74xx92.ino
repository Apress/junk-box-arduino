// TTL_Explorder_74xx92
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
//This sketch raises the reset lines of the 74xx92 to reset
//it, then holds them low. It then generates 12 negative
//pulses on CP0 (pin 14 of the xx92), delaying DELAYTIME ms
//in both the on and off states. In off states, it reads
//PINC for the results, formats them with zerobee binary
//pretty printer, and sends them up to the console.
// ---------------------------------------------------------

//Hardware:
//-----------------------------------------------------------
//74xx92, any flavor (mine's an LS)
// PORTA0 (pin 40) to pin14
// PORTA1 to pin6
// PORTA2 to pin7
// PORTC0 to pin12
// PORTC1 to pin 11
// PORTC2 to pin 10
// PORTC3 to pin 9
// Pin12 to Pin1
// Pin 5 to + bus
// Pin 10 to - bus
// Note that pin Q3 is outputting /6/ and not /8/

//precompiler definitions.
#define DELAYTIME 100

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
  DDRC = 0b00000000; //Set DDRC to all inputs.
  PORTC = 0b00001111; //Turn C0 to C4 pullups on
  Serial.println("Starting...");
}

//loop() function - runs forever.
void loop() {
  PORTA = 0b00000110;
  Serial.println("Holding both reset pins high. PORTA:" + zerobee(PORTA));
  Serial.println("PIN C should be all zeros. PINC:" + zerobee(0b1111 & PINC) + "\t");
  PORTA = 0b00000000;
  Serial.println("\nReset lines now low. PORTA:" + zerobee(PORTA));
  Serial.println("PIN C should still be 0. PINC:" + zerobee(0b1111 & PINC) + "\t");
  Serial.println("\nGiving the CP0 pin 12 negative pulses");
  for (int c = 1; c <= 24; c++) {
    PORTA = (PORTA + 1 & 0b00000001);
    if (PORTA == 0) {
      Serial.print("Pulse " + (String) (c / 2) );
      Serial.println("\t PIN C: " + zerobee(0b1111 & PINC));
    }
    delay(DELAYTIME); //Delay DELAYTIME ms for each on or off.
  }

  while (true) {} //Loop forever so we don't run again.
}
