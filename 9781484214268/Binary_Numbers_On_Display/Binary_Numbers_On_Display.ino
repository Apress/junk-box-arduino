//Binary Numbers On Display
// Copyright 2015,2016 James R. Strickland
//-----------------------------------------------------------
// This program is free software: you can redistribute it
// and/or modify it under the terms of the GNU General 
// Public License as published by the Free Software 
// Foundation, either version 3 of the License, or
// (at your option) any later version.
//
//  This program is distributed in the hope that it will
//  be useful, but WITHOUT ANY WARRANTY; without even the 
//  implied warranty of MERCHANTABILITY or FITNESS FOR A 
//  PARTICULAR PURPOSE.  See the GNU General Public License
//  for more details.
//
//  You should have received a copy of the GNU General Public
//  License along with this program.  If not, see 
//  <http://www.gnu.org/licenses/>.

//----------------------------------------------------------
//This sketch counts seconds in binary from 0 to 255, then resets.
//It's a really, really short sketch.
// ---------------------------------------------------------
//Hardware:
//LEDs wired with their anodes to port C's pins, and dropping
//resistors connecting their cathodes to the ground (-) bus.
// ----------------------------------------------------------

//precompiler definitions.
#define DELAYTIME 200 //How many mS (milliseconds) per update?


//setup() function - runs only once.
void setup() {
  DDRC = 0b11111111; //Set DDRC to all outputs.
  PORTC=1;
}

//loop() function - runs forever.
void loop() {
  PORTC++; //Take whatever is in PORTC and add 1.
  delay(DELAYTIME);  //Wait DELAYTIME milliseconds.
}
