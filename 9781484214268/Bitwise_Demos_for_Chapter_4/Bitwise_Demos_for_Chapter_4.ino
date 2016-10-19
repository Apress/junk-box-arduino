//Bitwise Demos for Chapter 4
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
//-----------------------------------------------------------

void setup() {
  //All this code is in setup because we’d like to see it run only once.
  Serial.begin (115200); //Set up the serial console at 115200 Baud.
  byte mybyte; //Declare the variable mybyte of the type byte.

  //AND demo
  mybyte = B00000001 & B00000011; //Assign mybyte to the bitwise and of B00000001 and B00000011, which are 1 and 3, respectively.
  Serial.print(mybyte, BIN); //Serial.print the result, formatted as binary. Note that leading 0s will not be displayed.
  Serial.println(); //Print a linefeed.

  //OR demo
  mybyte = 1; //Set mybyte to 1. Note that 1 is equal to B00000001.
  mybyte = mybyte | B00000011; //Set mybyte to the or of itself and 3.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format.
  Serial.println(); //Print a linefeed.


  //XOR demo
  mybyte = B00000001; //Set mybyte to 1.
  mybyte = mybyte ^ B00000011; //Set mybyte to the xor of itself and 3.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format.
  Serial.println(); //Print a linefeed.

  //NOT demo
  mybyte = B00000001; //Set mybyte to 1.
  mybyte = ~mybyte; //Set mybyte to the negation of itself.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format.
  Serial.println(); //Print a linefeed.

  //Bit Shift Demos
  mybyte = B00000001; //Set mybyte to 1.

  mybyte = mybyte << 1; //Bitshift mybyte one step to the left.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format. We should get 10, equal to decimal 2.
  Serial.println(); //Print a linefeed.

  mybyte = mybyte << 6; //Bitshift mybyte six more steps to the left.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format. We should get 10000000, equal to decimal 128.
  Serial.println(); //Print a linefeed.

  mybyte = mybyte >> 1; //Bitshift mybyte one step to the right.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format. We should get 1000000, equal to decimal 64.
  Serial.println(); //Print a linefeed.

  mybyte = mybyte >> 7; //Bitshift mybyte 7 more steps to the right, off the edge of the byte.
  Serial.print(mybyte, BIN); //Serial print mybyte in binary format. We should get 0. We shifted the set bit all the way out of the byte.
  Serial.println(); //Print a linefeed.

  mybyte = mybyte << 1; //Rotate mbyte to the left once to see if that bit is really gone.
  Serial.print(mybyte, BIN); //Serial print the result. We should still get 0.
  Serial.println(); //Print a linefeed.

  //Gotchas Demos: Multi - Byte Data Types
  unsigned int myint; //Declare an unsigned integer - two bytes.
  myint = 1; //set it to 1.
  Serial.print(myint << 8, DEC); //Print it rotated 8 digits to the left.
  Serial.println(); //Print a linefeed.

  //Signed Data Types
  int mysignedint; //declare a signed integer.
  mysignedint = 32767; //set to max value for a signed int.
  mysignedint = mysignedint + 1; //add one.
  Serial.print(mysignedint, DEC); //It's now negative.
  Serial.println();
  mysignedint = mysignedint + 1; //add one more.
  Serial.print(mysignedint, DEC); //it's now a lower negative.
  Serial.println();
}

void loop() {
  if (1 == 1) {}; //Do nothing, over and over, really fast.
}


