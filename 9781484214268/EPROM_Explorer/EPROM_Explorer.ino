//EPROM_Explorer
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
// Dump the readable characters of a 27128 EPROM to the
// Console.
// ---------------------------------------------------------
//Hardware:
//For a 2764 or similar EPROM:
//ATmega Pin    Pin Name   Port name   2764 Pin   Pin Name   Function      
//         1    PB0        DATA_READ         11   DQ0        DATA          
//         2    PB1        DATA_READ         12   DQ1        DATA          
//         3    PB2        DATA_READ         13   DQ2        DATA          
//         4    PB3        DATA_READ         15   DQ3        DATA          
//         5    PB4        DATA_READ         16   DQ4        DATA          
//         6    PB5        DATA_READ         17   DQ5        DATA          
//         7    PB6        DATA_READ         19   DQ6        DATA          
//         8    PB7        DATA_READ         19   DQ7        DATA          
//                                                               
                                                              
//        22    PC0        ADDR_LSB          10   A0         ADDRESS       
//        23    PC1        ADDR_LSB          09   A1         ADDRESS       
//        24    PC2        ADDR_LSB          08   A2         ADDRESS       
//        25    PC3        ADDR_LSB          07   A3         ADDRESS       
//        26    PC4        ADDR_LSB          06   A4         ADDRESS       
//        27    PC5        ADDR_LSB          05   A5         ADDRESS       
//        28    PC6        ADDR_LSB          04   A6         ADDRESS       
//        29    PC7        ADDR_LSB          03   A7         ADDRESS       
//                                                               
//        35    PA5        ADDR_MSB          26   A13        ADDRESS       
//        36    PA4        ADDR_MSB          02   A12        ADDRESS       
//        37    PA3        ADDR_MSB          23   A11        ADDRESS       
//        38    PA2        ADDR_MSB          21   A10        ADDRESS       
//        39    PA1        ADDR_MSB          24   A09        ADDRESS       
//        40    PA0        ADDR_MSB          25   A08        ADDRESS       

// ----------------------------------------------------------
// James R. Strickland
// ----------------------------------------------------------

//preprocessor definitions.
#define ADDR_LSB PORTC
#define ADDR_MSB PORTA
#define DATA_READ PINB
#define MAX_ADDRESS 0x3FFF

// ----------------------------------------------------------
// select_EPROM_address(address)
// ----------------------------------------------------------
// This function sets ADDR_LSB, ADDR_MSB, and the first two
// bits of ADDR_BANK_CTRL to the uint16_t (four byte unsigned
// int) address passed to it. These ports are wired to the
// EPROM IC's address pins.
//
// To break the address down into single byte chunks, we use
// a union, which essentially maps two variables (a 2 byte
// unsigned int and an array of 2 bytes, in this case) to
// one area of memory. Then we can peel off the bytes as we
// want them. Note that this code is endian-sensitive. If
// you run it on some future big-endian Arduino, you'll need
// to change the byte order.
// ----------------------------------------------------------

void select_EPROM_address(uint16_t EPROM_address) {

  //Declare the union we're going to use for address processing.
  union {
    uint16_t address_uint;
    byte byte_array[2];
  } address_union;

  address_union.address_uint = EPROM_address;

  ADDR_LSB = address_union.byte_array[0];
  ADDR_MSB = address_union.byte_array[1];
}

// ----------------------------------------------------------
//dump_EPROM(start_addr,end_addr)
// ----------------------------------------------------------
// This function dumps the contents of the EPROM to the console.
// data_line is an Arduino String object. We initialize it empty.
// Iterate through all the addresses, start_addr to end_addr,
// inclusive.
// If we've gotten to 64 characters gathered, print the start
// address of the line, then print the data_line and clear it.
// If the data on DATA_READ is printable, add it to the current
// data_line. Otherwise add a period.
// ----------------------------------------------------------
void dump_EPROM(uint16_t start_addr, uint16_t end_addr ) {
  String data_line = "";
  for (uint16_t c = start_addr; c <= end_addr; c++) {
    select_EPROM_address(c);//iterate through all the other addresses

    if ((!(c % 0x40)) && c > 0) { //if we're at 64 characters
      Serial.print("0x");
      Serial.print((c - 0x40), HEX);
      Serial.print("\t ");
      Serial.println(data_line);
      data_line = ""; //clear the data line
    }

    if (isprint(DATA_READ)) { //if the data on DATA_READ is printable
      data_line += (char)DATA_READ; //add it.
    } else {
      data_line += "."; //otherwise add a period.
    }
  }
}

// ----------------------------------------------------------
// setup()
// ----------------------------------------------------------
// Set up serial and port data directions. Initalize ADDR_BANK_CTRL
// so that the EPROM is ready to be read. Tell the user that we're
// running. Runs once.
// ----------------------------------------------------------

void setup() {
  Serial.begin(115200);

  DDRC = 0b11111111; //ADDR_LSB - Address LSB Port
  DDRA = 0b11111111; //ADDR_MSB - Address MSB Port
  DDRB = 0b00000000; //DATA_READ - Data port
  Serial.println("\r\rRunning");
}

// ----------------------------------------------------------
// loop()
// ----------------------------------------------------------
// Call dump_EPROM, then do nothing forever.
// ----------------------------------------------------------

void loop() {
  dump_EPROM(0, MAX_ADDRESS);  //Dump the entire EPROM.

  Serial.println("\r\rDone.");
  while (0 == 0) {}; //do nothing forever
}

