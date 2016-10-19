// Larson Memorial Scanner
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
// --------------------------------------------------------
//This sketch turns on a sequence of LEDs, one at a time,
//from the least significant bit to the most significant bit,
//then back again using bit shifting.
// ---------------------------------------------------------
//Hardware:
//LEDs wired with their anodes to port C's pins, and dropping
//resistors connecting their cathodes to the ground (-) bus.

//precompiler definitions.
#define DELAYTIME 50 //How many mS (milliseconds) between LEDs?

//declaration of variables.
bool reverse; //Declare our reverse-direction flag

//setup() function - runs only once.
void setup() {
  DDRC = 0b11111111; //Set DDRC to all outputs.
  PORTC = 0b00000001; //Set PORTC to 1. This will turn on pin 22.
  //PORTC can be initalized to any bit you want.
  
  reverse=(PORTC == 1); //Sanity checking for reverse. 
  //If PORTC==1, reverse is set true. Otherwise the
  //boundary logic below barfs.
}

//loop() function - runs forever.
void loop() {
  //Here's that boundary logic. Whatever reverse is, flip it 
  //if PORTC is 128 or 1.
  if  ((PORTC == 128) || (PORTC == 1)) reverse = !reverse;
  
  delay(DELAYTIME); //The Cestino can change the port value
//  thousands of times a second. Our eyes and the LEDs, not so
//  much. 50mS seems to give a nice, quick scan back and forth.

//Here's where we actually set the PORTC values.
  if (reverse) { //shift right
    PORTC = PORTC >> 1;
  } 
  else { //shift left
    PORTC = PORTC << 1;
  }
}
