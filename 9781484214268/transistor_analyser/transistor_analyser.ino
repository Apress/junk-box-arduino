// Transistor Analyser
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
// --------------------------------------------------------
// This sketch determines the flavor (PNP vs NPN) and the base
// of bipolar and Darlington transistors by measuring the 
// resistance of their leads in relationship to each other. 
// ---------------------------------------------------------
//Hardware:
//PORTA pins 0,1, and 2 are wired to resistors (330 ohms or higher)
//which then connect to pins 3 4 and 5 of PORTA. The transistor is
//connected to all three circuits.
//This circuit is a voltage divider with one known resistor and 
//the transistor on one side of the measuring point and another
//known resistor on the other. PORTA both sources and sinks current
//depending on the bit pattern set on it. 
// ----------------------------------------------------------
// Precompiler #defines
#include <math.h>
#define RESISTOR 330

// ----------------------------------------------------------
// We convert to and from binary pin numbers a lot.
// Going from value to pin number just takes round(pow(2,x))
// but going the other way needs a log2. Since Arduino's math library
// doesn't have a log2, we take the natural log of the value and divide
// it by the natural log of 2 to get a log2.
// ----------------------------------------------------------
byte log2(float val) {
  return ((byte)round(log(val) / log(2)));
}

// ----------------------------------------------------------
// If we know the npn/pnp flavor of the transistor, we can 
// find which pin is the base by determinining which pins
// conduct to/from it.
//
// For NPNs, both other pins will show low resistance from 
// the base to ground.
// For PNPs, both other pins will show low resistance to the 
// base.
// 
// Note Bene: The return value is in the form of binary pin positions
// 1, 2, and 4. To derive pin numbers, take the log2 of these values. 
// ----------------------------------------------------------
byte base_pin(bool npn) {
  bool reject = false;
  float temp = 0.0;
  
// If we're NPN, look for the pin that conducts to both other
// pins.
 
  if (npn) { 
    for (PORTA = 1; PORTA <= 4; PORTA = PORTA << 1) {
      reject = false;
      for (int c = 3; c <= 5; c++) {
        temp = ReadResistance(c);
        //if the value is infinite, this is not the base.
        reject = reject || ((isinf(temp)) || (temp < 0));
      }
      if (!reject) return (PORTA);
    }
  } else {
    
 // Otherwise we're PNP. Look for the pin the others conduct to.
 
    byte pnp_test_patterns[] = {3, 5, 6};
    for (int test_pattern_index = 0; test_pattern_index <= 2; test_pattern_index++) {
      PORTA = (byte)pnp_test_patterns[test_pattern_index];
      reject = false;
      for (int c = 3; c <= 5; c++) {
        temp = ReadResistance(c);
        //we should have negative resistance for any pin
        //conducting to the base. If the value we get isn't
        //this is not the base.
        reject = reject || ((isinf(temp)) || (temp > 0)); 
      }
      if (!reject) return ((~PORTA) & 0b00000111);
    }
  }
}

// ----------------------------------------------------------
//An NPN transistor will have one and only one pin with low resistance
//to the other two pins. Search for that pin. If we find more than one
//or we don't find any, it's a PNP transistor.
// ----------------------------------------------------------
bool is_npn() {
  bool npn = false;
  bool found_one_set = false;
  bool inf_or_negative = false;
  float temp = 0.0;
  
  for (PORTA = 1; PORTA <= 4; PORTA = PORTA << 1) {
    inf_or_negative = false;
    for (int c = 3; c <= 5; c++) {
      temp = ReadResistance(c);
      //Any infinite or negative resistances will disqualify this PORTA value.
      inf_or_negative = inf_or_negative || ((isinf(temp)) || (temp < 0));
    }

    //if we didn't have any infinite or negative resistances, this PORTA value
    //is a good one, but we need to make sure there's only one.
    if (!inf_or_negative) {
      if (found_one_set) { 
        return (false); //If we found more than one valid set, the transistor is not NPN.
      } else {
        found_one_set = true;
      }
    }
  }
  //if we found one and only one valid set (PORTA value) then the transistor is NPN.
  //if we found none, it's not.
  return (found_one_set);
}

// ----------------------------------------------------------
// Read the voltage. Bear in mind that the AtoD converter will
// return a value between 0 and 1024, on a voltage of 0 to 5.
// Multiply by 0.004882813 to get an approximate voltage in volts.
// ----------------------------------------------------------
float ReadVoltage(int pin) {
  return (analogRead(pin) * 0.004882813 );
}

// ----------------------------------------------------------
// Read the voltage in real volts, Using the formula for dividing networks,
// Vout=Vin*(R2/(R1+R2)), transformed algebraically to R1=((R2*Vin)/Vout-R2)
// We can solve for resistance. We know that R1 is R2 + the transistor
// and that our voltage is nominally 5 volts. We can subtract our known
// resistance from R1 in the same equation.
// ----------------------------------------------------------
float ReadResistance(int pin) {
  float temp = ((RESISTOR * 5) / ReadVoltage(pin) - (RESISTOR * 2));
  return (temp);
}

// ----------------------------------------------------------
// Set up the port, set it to all zeros (to clear the pullup
// resistors) and configure serial communication.
// ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  DDRA = 0b00000111;
  PORTA = 0b00000000;
}

// ----------------------------------------------------------
// Print the results to the serial console.
// ----------------------------------------------------------
void loop() {
  bool transistor_is_npn = false;
  Serial.println("Transistor Analyser");
  Serial.println("--------------------------------------------------------");
  Serial.print("Transistor Type:");
  transistor_is_npn = is_npn();
  if (transistor_is_npn) {
    Serial.println("NPN");
  } else {
    Serial.println("PNP");
  }
// Take the log2 of the output of base_pin and add 3 to get the number of the analog pin.
// Remember that they are numbered from left to right, starting with analog 0 pin 40 on the IC.
  Serial.println("Base: A" + (String) (3 + (log2(base_pin(transistor_is_npn)))));
  Serial.println("--------------------------------------------------------");
  
  while (0 == 0) { //As long as we're doing nothing,
    PORTA = 0;     //Turn off all pins so small transistors don't get hot.
  }
}
