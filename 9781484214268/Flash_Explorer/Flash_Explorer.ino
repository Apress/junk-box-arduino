// Flash_Explorer v2.0
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
// This sketch dumps the contents of the flash (an old BIOS
// flash in this case), then erases it and programs a bit of
// Shakespeare at the beginning, then dumps the first 255
// bytes of the flash again to show that it's been erased
// and reprogrammed.
// ---------------------------------------------------------
//Hardware:
//For a 39SF020A or similar flash IC:
//ATmega Pin    Name   Port name Flash Pin  Name   Func
//         1    PB0    DATA_READ        13   D0     DATA
//         2    PB1    DATA_READ        14   D1     DATA
//         3    PB2    DATA_READ        15   D2     DATA
//         4    PB3    DATA_READ        17   D3     DATA
//         5    PB4    DATA_READ        18   D4     DATA
//         6    PB5    DATA_READ        19   D5     DATA
//         7    PB6    DATA_READ        20   D6     DATA
//         8    PB7    DATA_READ        21   D7     DATA
//
//        16    PD2    BANK_ADDR        02   A16    ADDRESS
//        17    PD3    BANK_ADDR        30   A17    ADDRESS
//        18    PD4    CTRL             22   /E     Chip Enable
//        19    PD5    CTRL             24   /G     Gate
//        20    PD6    CTRL             31   /W     Write Enable
//
//        22    PC0    ADDR_LSB         12   A0     ADDRESS
//        23    PC1    ADDR_LSB         11   A1     ADDRESS
//        24    PC2    ADDR_LSB         10   A2     ADDRESS
//        25    PC3    ADDR_LSB         09   A3     ADDRESS
//        26    PC4    ADDR_LSB         08   A4     ADDRESS
//        27    PC5    ADDR_LSB         07   A5     ADDRESS
//        28    PC6    ADDR_LSB         06   A6     ADDRESS
//        29    PC7    ADDR_LSB         05   A7     ADDRESS
//
//        33    PA7    ADDR_MSB         03   A15    ADDRESS
//        34    PA6    ADDR_MSB         29   A14    ADDRESS
//        35    PA5    ADDR_MSB         28   A13    ADDRESS
//        36    PA4    ADDR_MSB         04   A12    ADDRESS
//        37    PA3    ADDR_MSB         25   A11    ADDRESS
//        38    PA2    ADDR_MSB         23   A10    ADDRESS
//        39    PA1    ADDR_MSB         26   A09    ADDRESS
//        40    PA0    ADDR_MSB         27   A08    ADDRESS

//preprocessor definitions.
#define ADDR_LSB PORTC
#define ADDR_MSB PORTA
#define DATA_READ PINB
#define DATA_WRITE PORTB
#define DATA_DDR DDRB
#define ADDR_BANK_CTRL PORTD
#define MAX_ADDRESS 0x03FFFF
#define FLASH_READ  0b0100
#define FLASH_WRITE 0b0010
#define FLASH_WAIT_WRITE 0b0110

// This is the message we'll be programming into the flash.
// You can change it if you want. The \ marks mean "line 
// continues after the line break."
String message = "Over hill, over dale, \
Thorough bush, thorough brier,Over park, over pale, \
Thorough flood, thorough fire, I do wander everywhere.\
- Shakespeare";

// ----------------------------------------------------------
// set_flash_signals(uint8_t bits)
// ----------------------------------------------------------
// This function sets the signal bits of ADDR_BANK_CTRL (Bits 
// 4-6) to the value of the first three bits passed in the
// bits field. ADDR_BANK_CTRL is a port register and the state
// of the signals on it needs to not have unknown transitional
// states, so we compute the new byte with temp and set the 
// final value to ADDR_BANK_CTRL.
 // ----------------------------------------------------------
void set_flash_signals(uint8_t bits) {
  uint8_t temp = ADDR_BANK_CTRL & 0b00001111;
  //mask the control bits off of ADDR_BANK_CTRL
  //and store the result in temp.
  
  temp |= (bits << 4);
  // OR temp with bits, rotated left by 4 places.
  
  ADDR_BANK_CTRL = temp;
  //set ADDR_BANK_CONTROL to the computed value.
}
// ----------------------------------------------------------
// select_flash_address(address) 
// ----------------------------------------------------------
// This function sets ADDR_LSB, ADDR_MSB, and the first two
// bits of ADDR_BANK_CTRL to the uint32_t (four byte unsigned
// int) address passed to it. These registers are wired to the
// flash IC's address pins.
//
// To break the address down into single byte chunks, we use
// a union, which essentially maps two variables (a 4 byte
// unsigned int and an array of 4 bytes, in this case) to
// one area of memory. Then we can peel off the bytes as we
// want them. Note that this code is endian-sensitive. If
// you run it on some future big-endian Arduino, you'll need
// to change the byte order.
//
// We have to extract the lowest two bits of the third byte
// and rotate them two spaces to the left, then AND them in
// to the ADDR_BANK_CTRL register, since we don't want to 
// disturb the signal lines or the RS232 lines.
// ----------------------------------------------------------

void select_flash_address(uint32_t flash_address) {
  uint8_t temp;
  
  ADDR_LSB = 0;
  ADDR_MSB = 0;
  ADDR_BANK_CTRL &= 0b11110011;
  // Initialize address registers for safety.
   
  union {
    uint32_t address_uint;
    byte byte_array[4];
  } address_union;
  //Declare the union we're going to use for address processing.
  
  address_union.address_uint = flash_address;
  //Set address_union.address_uint to the address we were given.

  ADDR_LSB = address_union.byte_array[0];
  //set the ADDR_LSB register to the first byte of the union.
  
  ADDR_MSB = address_union.byte_array[1];
  //set the ADDR_MSB register to the second byte of the union.

  temp = ADDR_BANK_CTRL;
  temp &= 0b11110011;
  temp |= (address_union.byte_array[2] << 2);
  ADDR_BANK_CTRL = temp;
  // set bits 3 and 4 of ADDR_BANK_CTRL to the first two bits
  // of the third byte in the union.
}

// ----------------------------------------------------------
// send_to_flash(address,data)
// ----------------------------------------------------------
// This function sets the ATmega's data port to write mode selects
// the address pin settings for the flash IC, and writes (data) to
// the data port. We use it to pass commands to the flash IC.
//
// Note that this does not directly write to the flash's storage. 
// This function actually talks to the flash's built-in controller, 
// but no data is written (programmed) to the flash without passing 
// the flash a specific sequence of commands.
//
// NOTE BENE: This function assumes that the flash signals will be
// set to FLASH_WAIT_WRITE when it enters. This must be handled by 
// the calling function, since changing /OE (gate) between 
// transmitted bytes aborts command sequences.
//
// First we set the address lines to the address by calling 
// select_flash_address(). 
// Then we set the data lines to the data we're given.
// Then we pulse the /W signal low by calling set_flash_signals with 
// FLASH_WRITE, and then with FLASH_WAIT_WRITE.
// And then we're done.
// ----------------------------------------------------------
void send_to_flash(uint32_t address, byte data) {
  
  select_flash_address(address); //Set the address lines
  DATA_WRITE = data; //Set the data lines

  set_flash_signals(FLASH_WRITE);
  set_flash_signals(FLASH_WAIT_WRITE);
  //pulse /W low. It only has to go low a few nanoseconds.
  // Nothing on the cestino happens in less than 50, so we're
  // not going too fast for the 85ns 39SF020A. If your flash
  // is slower, you may have to add a delay.
}

// ----------------------------------------------------------
// program_flash(address,data)
// ----------------------------------------------------------
// To actually store (program) data onto the flash, we have to
// send it a specific pattern of addresses and data bytes before
// we send it the address we want and the byte of data we want 
// stored there. 
// To that end, we set DATA_DDR to output so we can write data,
// then call set_flash_signals to set it to FLASH_WAIT_WRITE 
// mode. This disables /OE (gate) but does not enable /W (write).
// We call send_to_flash four times. The first three tell the 
// flash controller what we want to do, and the fourth call 
// gives it our address and data.
// Then clear the DATA_WRITE pins and set DATA_DDR back to 
// read mode. Delay 1ms to because flash is slow and we're 
// not polling its status line to tell when it's done. Set the
// flash's control signals to FLASH_READ mode and we're done.
// ----------------------------------------------------------
void program_flash(uint32_t address, byte data) {
  DATA_DDR = 0xFFFF;
  set_flash_signals(FLASH_WAIT_WRITE);

  //for 39F0020. These may be different for other flash ICs.
  send_to_flash(0x5555, 0xAA); //Tell flash to store data
  send_to_flash(0x2AAA, 0x55);
  send_to_flash(0x5555, 0xA0);
  
  send_to_flash(address, data); //Give flash our data and address.

  DATA_WRITE = 0; //clear the DATA_WRITE pins.
  DATA_DDR = 0x0; //set DATA_DDR back to read mode.
  delay(1); //Delay one ms because flash is slow.
  //We could probably save a few microseconds monitoring the 
  //status bit, but that'd be a lot more code.
  
  set_flash_signals(FLASH_READ);
  //Set the flash signals back to FLASH_READ mode.
}

// ----------------------------------------------------------
// erase_flash()
// ----------------------------------------------------------
// If you guessed that erasing the entire flash would be another
// sequence of commands sent to specific addresses, you guessed 
// right. That's what this function does.
//
// Set DATA_DDR to write mode, and set the flash's signals to
// FLASH_WAIT_WRTTE - which is /W disabled, /OE disabled, and
// /CE enabled. Call send_to_flash 6 times to give it the 
// command sequence to erase the entire flash. 
// Then clear DATA_WRITE, set DATA_DDR to read mode, and
// set the flash's signals back to FLASH_READ.
//
// NOTE BENE: Erasing the flash takes a few ms, and this
// function doesn't include the wait, so the calling function
// needs to do it. The flash returns gibberish if you try to
// read it while it's erasing. It goes without saying that 
// writes fail, too.
// ----------------------------------------------------------
void erase_flash() {
  DATA_DDR = 0xFFFF;
  set_flash_signals(FLASH_WAIT_WRITE);

  // for 39SF020A
  send_to_flash(0x5555, 0xAA);
  send_to_flash(0x2AAA, 0x55);
  send_to_flash(0x5555, 0x80);
  send_to_flash(0x5555, 0xAA);
  send_to_flash(0x2AAA, 0x55);
  send_to_flash(0x5555, 0x10);
  select_flash_address(0);
  //These are specific to the 39SF020A. Other flash chips have
  //different registers that are similarly unlikely patterns 
  //to occur by accident.
  
  DATA_WRITE = 0;
  DATA_DDR = 0x0;
  set_flash_signals(FLASH_READ);
  //Clear data_write and set DATA_DDR back to read mode. Then
  //set the flash signals back to FLASH_READ mode.
}

// ----------------------------------------------------------
// dump_flash(start_addr,end_addr)
// ----------------------------------------------------------
// This function dumps the contents of the flash to the console.
// data_line is an Arduino String object. We initialize it empty.
// Iterate through all the addresses, start_addr to end_addr,
// inclusive.
//
// Start by creating a char array for the line's address, a 
// uint32_t called c to iterate through the addresses with,
// then set DATA_DDR to read mode, and set the flash's signals
// to FLASH_READ mode.
//
// We're going to use the sprintf function to properly format our
// addresses, since Arduino's tabs don't work properly. Sprintf
// takes a pattern of text and variable display information, 
// and fills the variables in based on that pattern. In this case,
// the pattern is 0x%06lx. The first two characters, 0x, are 
// literal. You'll see them in the output. %06lx tells sprintf
// "this is a 6 digit number. Fill in preceding zeros if you
//  need to. The variable will be a long integer (32 bits in 
//  the Arduino implimentation we're using), and it should be 
//  displayed in hexidecimal.
//
//  The normal C printf would send that to the console, but we'd
//  like to put it in a String object. To do that, we use sprintf,
//  pass it the char array address[12] Once it's there, we pull
//  it into the String object data_line, where it makes up the
//  beginning of the line.
//
//  After that, we iterate through the address range we're given,
//  and add the character value of DATA_READ to the string if it's
//  printable, and a . if it's not. When data_line is 64 characters
//  long, we serial.println it, clear it, and set its beginning
//  the same way we did before, then get back to work.
//  When we're done, we serial.println data_line once more to
//  get any data that might have been in an incomplete data line.
// ----------------------------------------------------------
void dump_flash(uint32_t start_addr, uint32_t end_addr ) {
  char address[12];
  uint32_t c;
  DATA_DDR = 0x0;
  set_flash_signals(FLASH_READ);
  
  sprintf(address, "0x%06lX :", start_addr);
  String data_line = String(address);

  for (c = start_addr; c <= end_addr; c++) {
    select_flash_address(c);//iterate through all the addresses

    if ((!(c % 0x40)) && c > 0) { //if we're at 64 characters
      Serial.println(data_line);
      sprintf(address, "0x%06lX :", c);
      data_line = String(address);
    }

    if (isprint(DATA_READ)) { //if the data on DATA_READ is printable
      data_line += (char)DATA_READ; //add it.
    } else {
      data_line += "."; //otherwise add a period.
    }
  }

  Serial.println(data_line);

}


// ----------------------------------------------------------
// setup()
// ----------------------------------------------------------
// Set up serial and port data directions. Tell the user we're
// running. Runs once.
// ----------------------------------------------------------

void setup() {
  Serial.begin(115200);

  DDRC = 0b11111111; //ADDR_LSB - Address LSB Port
  DDRA = 0b11111111; //ADDR_MSB - Address MSB Port
  DDRD |= 0b11111100; //ADDR_BANK_CTRL Bank address and control port.
  Serial.println("\r\rRunning");
}

// ----------------------------------------------------------
// loop()
// ----------------------------------------------------------
// Mostly we just call functions here.
// Dump the entire flash
// Erase the flash
// Program the flash.
// Dump the first 64 characters of the flash again so we can
// see what we programmed. Then do nothing forever.
// ----------------------------------------------------------
void loop() {
  Serial.println("Dumping Flash.");
  dump_flash(0x00, MAX_ADDRESS); //Dump the entire flash.
  Serial.println("Erasing Flash.");
  erase_flash(); //wipe the flash.
  delay(100);
  Serial.println("Done Erasing Flash.");

  Serial.println("Programming Flash.");

  for (int c = 0; c <= message.length(); c++) {
    program_flash(c, message.charAt(c));
  }
  Serial.println("Dumping Flash Again.");
  dump_flash(0x00, 0xff);
  Serial.println("\r\rDone.");
  while (0 == 0) {};
}

