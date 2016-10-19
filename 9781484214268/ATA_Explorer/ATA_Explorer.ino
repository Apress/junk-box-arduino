// ATA_Explorer
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
//
//  Status code is based on the Arduino IDEFat Library.
//  Copyright (C) 2009 by William Greiman
//

//Hardware:
//-----------------------------------------------------------
// CONTROL_PORT
// Signal:  /CS3FX  /CS1FX  DA2 DA1 DA0  /DIOW  /DIOR /RESET
// Means:  3F6-3F7 1F0-1F7 (Drive Addr) (Write)(Read) (Reset)
// IDE PIN:  38      37     36  33  35   23     25      1
// Port Bit:  7       6      5   4   3    2      1      0

// LSB_PORT
// Signal:    DD7  DD6   DD5   DD4   DD3   DD2  DD1  DD0
// IDE Pin:     3    5     7     9    11    13   15   17
// Port Bit:    7    6     5     4     3     2    1    0

// MSB_PORT
// Signal:  DD15  DD14  DD13  DD12  DD11  DD10  DD9  DD8
// IDE Pin:   18    16    14    12    10     8    6    4
// Port Bit:   7     6     5     4     3     2    1    0
//
// GROUND:
// IDE pins 2, 19, 22, 24, 26, 30, and 40 should be grounded.
// Yes really, ground them all.
//-----------------------------------------------------------

//Notes
//-----------------------------------------------------------
// Sector and LBA block are used interchangeably in this
// sketch, because when the drive is identifying itself,
// it's still in cylinder/head/sector mode. All other
// reads and writes deal with LBA blocks. Which some
// people also call LBA sectors.

//Preprocessor #defines
//-----------------------------------------------------------
// This sketch has a ton of preprocessor #defines.
// They use no run-time memory and make the code easier to
// follow.
//-----------------------------------------------------------

//Define what ports do what, and how to access their PORT,
//PIN, and DDR registers.
#define CONTROL_PORT PORTB
#define CONTROL_DDR DDRB
#define CONTROL_PINS PINB
#define LSB_DDR DDRA
#define LSB_PORT PORTA
#define LSB_PINS PINA
#define MSB_DDR DDRC
#define MSB_PORT PORTC
#define MSB_PINS PINC

// Define the two DDR modes for any port exc. control
#define DATA_READ 0b00000000
#define DATA_WRITE 0b11111111

// Define various control port register settings.
// Note that this assumes the control port will be wired
// as shown below, in 0bXXXXXXXX notation.
// bit 0 is on the RIGHT.
// /CS3FX,/CS1FX,DA2,DA1,DA0,/DIOW,/DIOR, and /RESET
#define REG_DEFAULT      0b11000111 //bogus register
#define REG_STAT_COM     0b10111111 //Status/Command Register
#define REG_ASTAT_DEVCOM 0b01110111 //Alt Status/Device Command 
#define REG_ERR_FEAT     0b10000111 //Error/Features 
#define REG_LBA0         0b10011111 //Low byte of LBA address
#define REG_LBA1         0b10100111 //Second byte LBA address
#define REG_LBA2         0b10101111 //Third byte LBA address
#define REG_LBA3         0b10110111 //Fourth byte LBA address
#define REG_DATA         0b10000111 //Data register
#define REG_HEAD REG_LBA3 //Same register, different modes.

// Most actions require a command to be sent
// over the LSB_PORT data lines once the
// register is selected.
// Those commands are defined here.
#define CMD_SLEEP 0x99 //Put the drive to sleep/spin down.
#define CMD_INIT 0x91 //Set drive to a known state.
#define CMD_IDENTIFY 0xEC //Get ID info -> drive sector buffer
#define CMD_READ_WITH_RETRY 0x20 //Read block to drive buffer.
#define CMD_WRITE_BUFFER 0xE8 //Write data to drive buffer
#define CMD_WRITE_WITH_RETRY 0x30 //Write drive buff. to disk.
#define CMD_LBA_MODE 0b01000000 //Set the drive to LBA mode.

// Status byte masks (Cribbed from GeneT's IDEFAT library.)
// I've annotated them in binary to make it clearer
// how they work. These are masks used with AND to pull
// one bit from a status register read.
// All status bits are active high. In order, they are:
// BSY DRDY DWF DSC DRQ CORR IDX ERR
#define BSY  0x80 //0b10000000 - Busy
#define RDY  0x40 //0b01000000 - Ready
#define DF   0x20 //0b00100000 - Drive Write Fault
#define DSC  0x10 //0b00010000 - Drive Seek Complete
#define DRQ  0x08 //0b00001000 - Data Request
#define CORR 0x04 //0b00000100 - Corrected Data
#define IDX  0x02 //0b00000010 - Index
#define ERR  0x01 //0b00000001 - Error

//Define modes of set_drive_hw_lines - Signals are
//  /DIOW,/DIOR, and /RESET
#define HW_OFF   0b111 //Set the read-write lines all off.
#define HW_READ  0b101 //Set the read-write lines for reading.
#define HW_WRITE 0b011 //Set the read-write lines for writing.
#define HW_RESET 0b110 //Lower the reset line to reset.

// Define BIG_ENDIAN as true and LITTLE_ENDIAN as false.
// It makes the switch in transfer_sector_buffer clearer.
#define BIG_ENDIAN true
#define LITTLE_ENDIAN false

// DEBUG mode compiles in a lot of Serial.writeln messages.
// for debugging purposes. Can also be #defined for individual
// functions and #undef'd at the end of the function.
//#define DEBUG

//Code
//-----------------------------------------------------------
// Global Variables and functions. The meat of the sketch.
//-----------------------------------------------------------

//-----------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------
byte block_buffer[512]; //One buffer for reading and writing.
int drive_cylinders = 0;
int drive_heads = 0;
int drive_sectors = 0;
uint32_t LBA_sectors = 0;
boolean non_zero_read = false;
//Last block buffer transfer empty?


//-----------------------------------------------------------
// Function get_drive_status_byte()
// - Select the REG_ASTAT_DEVCOM register and read it.
//-----------------------------------------------------------
byte get_drive_status_byte() {

#ifdef DEBUG
  Serial.print("get_drive_status_byte() called.");
#endif

  byte temp_control_port = CONTROL_PORT;
  byte temp_lsb = LSB_PORT;
  byte temp_lsb_ddr = LSB_DDR;
  //save port states

  byte status_byte = 0;

  CONTROL_PORT = REG_ASTAT_DEVCOM;
  //Use the alternate status register.

  LSB_DDR = DATA_READ; //set LSB port to read mode.
  LSB_PORT = 0b11111111; //Turn on pullup resistors - reading.
  set_drive_hw_lines(HW_READ); //Set the read signals
  status_byte = LSB_PINS; //Read LSB_PINS into status_byte.
  set_drive_hw_lines(HW_OFF); //Clear the read signals.

  CONTROL_PORT = temp_control_port;
  LSB_PORT = temp_lsb;
  LSB_DDR = temp_lsb_ddr;
  //restore port states

#ifdef DEBUG
  Serial.print(" Returning:");
  Serial.print(status_byte, BIN);
  Serial.print("\n");
#endif
  return (status_byte);
};

//-----------------------------------------------------------
// Function write_command_to_register(reg,command)
// Set CONTROL_PORT to reg, then write a command byte
// to it on LSB_PORT
//-----------------------------------------------------------
void write_command_to_register(byte reg, byte command) {
  byte temp = LSB_DDR; //Preserve LSB r/w state
  LSB_DDR = DATA_WRITE; //Set LSB's DDR
  CONTROL_PORT = reg; //Set CONTROL_PORT to register address.
  LSB_PORT = command; //Set LSB_PORT to the command byte.
  set_drive_hw_lines(HW_WRITE); //Strobe write/read lines.
  set_drive_hw_lines(HW_OFF);
  LSB_DDR = temp; //Restore the state of LSB's DDR.
}

//----------------------------------------------------------
// Function set_drive_hw_lines(byte status)
// These are the last three lines  of CONTROL_PORT.
// We switch our read, write, and reset signals
// here, and they're all active low. So it's backwards.
//----------------------------------------------------------
void set_drive_hw_lines(byte status) {
#ifdef DEBUG
  Serial.print("set_drive_hw_lines() called with status:");
  Serial.print(status, DEC);
  Serial.print(" CONTROL_PORT was: ");
  Serial.print(CONTROL_PORT, BIN);
#endif

  byte temp = CONTROL_PORT;
  //preserve CONTROL_PORT's existing state.

  temp = temp & 0b11111000;
  //wipe the hw line bits

  temp = temp | status;
  //OR the remaining bits with the new setting (status)

  CONTROL_PORT = temp; //Set CONTROL_PORT to the new value.

#ifdef DEBUG
  Serial.print(" CONTROL_PORT now ");
  Serial.print(CONTROL_PORT, BIN);
  Serial.print("\n");
#endif
}

//-----------------------------------------------------------
// Function wait_for_drive_drq();
// Do nothing while checking to see if the DRQ line is high
// DRQ is data request. The drive signals it's ready to transfer
// a word of data (8 or 16 bits, in or out) with this signal.
//-----------------------------------------------------------
void wait_for_drive_drq() {

#ifdef DEBUG
  Serial.print("wait_for_drive_drq() called: ");
#endif
  byte status_byte = get_drive_status_byte();
  byte mask = DRQ | BSY; //check bits 3 and 7, DRQ and BSY.
  while (!((status_byte & mask) == DRQ)) {
    status_byte = get_drive_status_byte();
  }
  //if we don't get a DRQ and BSY,check again until we do.

#ifdef DEBUG
  Serial.println("DRQ");
#endif
}

//-----------------------------------------------------------
// Function wait_for_drive_not_busy()
// Do nothing while we wait for the drive to go not busy.
//-----------------------------------------------------------
void wait_for_drive_not_busy() {

#ifdef DEBUG
  Serial.print("wait_for_drive_not_busy() called: ");
#endif
  byte status_byte = get_drive_status_byte();
  byte mask = BSY;
  while (!((status_byte & mask) == 0)) {
    status_byte = get_drive_status_byte();
    //while the highest bit of the status byte is not 0,
    //get the status byte. Do it forever if need be.
  }
#ifdef DEBUG
  Serial.println("Busy Clear");
#endif
}

//-----------------------------------------------------------
// Function wait_for_drive_ready(int ms to wait)
// Do nothing while we wait up to ms_to_wait ms for the drive
// to go ready. It's a crude timeout mechanism, but it works.
//-----------------------------------------------------------
void wait_for_drive_ready(int ms_to_wait) {

#ifdef DEBUG
  Serial.print("wait_for_drive_ready() called: ");
#endif
  byte status_byte = get_drive_status_byte();
  byte mask = BSY | RDY;
  //look at the highest two bits of the status byte.

  int c = 0;
  while ((!((status_byte & mask) == RDY)) && (c <= ms_to_wait)) {
    status_byte = get_drive_status_byte();
    delay(1);
    c++;
  }
  //While the highest two bits of the status byte are not
  //equal to 0x40 (RDY), delay 1ms, increment c,
  //and do it again.

#ifdef DEBUG
  Serial.println("DRIVE READY");
#endif
}

//-----------------------------------------------------------
// string2uint32_t()
// - The String class includes no nice way to turn strings of
//   digits into a numeric value.
//   This function goes through the string from left to right.
//   For each position it advances to the right, it multiplies
//   the existing value by 10 and adds the value of the
//   character at that position minus the value of the
//   character '0'. When we reach the end of the string,
//   return temp.
//-----------------------------------------------------------
uint32_t string2uint32_t(String input) {
  uint32_t temp = 0;
  input.trim();
  for (int c = 0; c < input.length(); c++) {
    temp = temp * 10 + input.charAt(c) - '0';
  }

#ifdef DEBUG
  Serial.print("string2uint32_t called with a string of >" +
               input + "< Returned:");
  Serial.println(temp, DEC);
#endif


  return temp;
}

//-----------------------------------------------------------
//  Function transfer_sector_buffer(write,big_endian)
//  - copy block_buffer to/from the drive.
//
//  NOTE BENE: Drive info is stored big_endian
//  but nearly all microcomputers store data
//  in little_endian.
//-----------------------------------------------------------
void transfer_sector_buffer(boolean write, boolean big_endian) {
#ifdef DEBUG
  Serial.println("transfer_sector_buffer called.");
#endif

  wait_for_drive_not_busy();
  non_zero_read = false; //set false before we start.
  CONTROL_PORT = REG_DATA; //select the REG_DATA register.

  if (write) {
    LSB_DDR = DATA_WRITE;
    MSB_DDR = DATA_WRITE;
    //set both LSB and MSB to write mode.
  }
  else {
    LSB_DDR = DATA_READ;
    MSB_DDR = DATA_READ;
    //Set both LSB and MSB to read mode.
  }

  for (int c = 0; c < 512; c += 2) {
    if (write) { //If we're writing...
      if (big_endian) { //and we're big endian
        MSB_PORT = block_buffer[c];
        LSB_PORT = block_buffer[c + 1];
      }
      else { //or we're little endian
        LSB_PORT = block_buffer[c];
        MSB_PORT = block_buffer[c + 1];
      }
      set_drive_hw_lines(HW_WRITE);//set write signals
    }
    //For big endian writing, put the cestino's buffer into the
    //drive's buffer in pairs, second value first, because
    //the Cestino's sector buffer is little endian.
    //Otherwise put the values into LSB and MSB in the order
    //they are in the Cestino's buffer.

    else {//if we're reading
      set_drive_hw_lines(HW_READ); //set read signals
      if (big_endian) { //if we're big endian
        block_buffer[c] = MSB_PINS;
        block_buffer[c + 1] = LSB_PINS;
        non_zero_read = MSB_PINS | LSB_PINS | non_zero_read;
      }
      else {
        block_buffer[c] = LSB_PINS;
        block_buffer[c + 1] = MSB_PINS;
        non_zero_read = MSB_PINS | LSB_PINS | non_zero_read;
      }
    }
    //For big endian reading, store the MSB byte first, then
    //store the LSB byte. Check to see if either MSB or LSB
    //had anything in them, or if non_zero_read was already
    //set true. If any of those are true, non_zero_read is
    //true. Probably this should be in a function return.

    set_drive_hw_lines(HW_OFF);//Turn the rw signals off.
  }

#ifdef DEBUG
  Serial.println("transfer_sector_buffer done.");
#endif
}


//-----------------------------------------------------------
// Function dump_block_buffer()
//  - Pretty-print the block buffer.
//-----------------------------------------------------------
void dump_block_buffer() {
  Serial.println("Dumping Block Buffer");
  int c = 0;
  String hex_data = "";
  String human_readable_data = "";
  //We have some variables. Initialize them.

  for (int row = 0; row < 32; row++) {
    for (int col = 0; col < 16; col++) {
      //We're printing blocks of 512 bytes.
      //each is 2 characters + two spaces
      //in hex, plus 1 character in text.

      if (block_buffer[c] < 0x10) hex_data += "0";
      //Add leading zero for hex values below 0x10.

      hex_data += String(block_buffer[c], HEX);
      hex_data += " ";
      //Add block_buffer[c] as a hex string to hex_data.

      if (isprint(block_buffer[c])) {
        human_readable_data += (char)block_buffer[c];
      } else {
        human_readable_data += ".";
      }
      //If block_buffer[c] is printable, add it to
      //human_readable_data. Otherwise add a . for
      //a placeholder.
      c++;
    }

    Serial.println(hex_data + " | " + human_readable_data);
    hex_data = "";
    human_readable_data = "";
    //Combine the two strings in one Serial.println.
    //Then clear them.

  }
  Serial.print("Bytes dumped:");
  Serial.print(c);
  Serial.print("\n");
}

//-----------------------------------------------------------
// Function read_write_drive_LBA_block(block_num)
//
// -LBA mode only. Set the drive to LBA mode
// and put the contents in the drive's block buffer.
// Then read the drive's block buffer into the
// Cestino's block buffer. Block numbers are 28 bits,
// and little-endian.
//-----------------------------------------------------------
void read_write_drive_LBA_block(uint32_t block_num, bool write_enable) {
  union {
    uint32_t uint;
    byte byte_array[4];
  } block_num_union;
  //This union takes a uint32_t (4 byte unsigned int)
  //and represents it also as an array of 4 bytes. Very handy,
  //but watch for endian problems if you run this on a non
  //ATmega Arduino.

  block_num_union.uint = block_num;

#ifdef DEBUG
  Serial.print("read_drive_LBA_block called on block ");
  Serial.print(block_num);
  Serial.print("\n");
  Serial.println("third byte is:" + String(block_num_union.byte_array[3], BIN));
#endif
   

  wait_for_drive_not_busy(); //wait for the drive to signal it's not busy.
  write_command_to_register(REG_LBA0, block_num_union.byte_array[0]);
  write_command_to_register(REG_LBA1, block_num_union.byte_array[1]);
  write_command_to_register(REG_LBA2, block_num_union.byte_array[2]);
  write_command_to_register(REG_LBA3, (block_num_union.byte_array[3] | CMD_LBA_MODE));
  //Set all three-and-a-half LBA address bytes.
  //LBA3 (aka the head register) gets four bits of address,

  if (write_enable) {
    write_command_to_register(REG_STAT_COM, CMD_WRITE_WITH_RETRY);
  } else {
    write_command_to_register(REG_STAT_COM, CMD_READ_WITH_RETRY);
  }
  wait_for_drive_drq();
  transfer_sector_buffer(write_enable, LITTLE_ENDIAN);
  reset_drive();
  //Kludge to fix write problems when the ATA bus is noisy.

#ifdef DEBUG
  Serial.println("read_write_drive_LBA_block done.");
#endif
}

//----------------------------------------------------------
// Function reset_drive()
// Raises the reset line, then inits the drive to default mode.
//----------------------------------------------------------
void reset_drive() {
#ifdef DEBUG
  Serial.println("reset_drive() called.");
#endif

  set_drive_hw_lines(HW_RESET); //Lower the /Reset line.
  delay(1);
  set_drive_hw_lines(HW_OFF);

  write_command_to_register(REG_HEAD, CMD_INIT);
  //Init the drive

  wait_for_drive_ready(1024);
  //Wait for the drive to show ready again.

#ifdef DEBUG
  Serial.println("reset complete.");
#endif
}

//-----------------------------------------------------------
// Function sleep_drive()
//   - Set the control port to the status/command register.
//   Set it write, and send the sleep code over the data lines.
//-----------------------------------------------------------
void sleep_drive() {
#ifdef DEBUG
  Serial.println("sleep_drive() called.");
#endif

  write_command_to_register(REG_STAT_COM, CMD_SLEEP);
  //Send the sleep command to REG_STAT_COM register.
  //Most other functions except reset will fail to
  //work due to waiting on ready/not busy/drq signals.

#ifdef DEBUG
  Serial.println("ZZZ");
#endif
};

//-----------------------------------------------------------
// Function identify_drive()
//
//  - Read the drive's identity block. The whole thing. Burp.
//  NOTA BENE: drive info is big-endian, so we tell
//  transfer_sector_buffer to read that way. Also, 32 bit
//  values will be transferred as two sixteen bit words
//  with the least significant word first, which is backwards.
//  Fortunately there's only one, so we can fix it manually.
//-----------------------------------------------------------
void identify_drive() {
#ifdef DEBUG
  Serial.println("identify_drive() called");
#endif

  wait_for_drive_not_busy(); //Wait for bsy flag to be clear.
  write_command_to_register(REG_STAT_COM, CMD_IDENTIFY);
  transfer_sector_buffer(false, BIG_ENDIAN);
  //Send the identify command to the REG_STAT_COM register
  //Then transfer the drive's buffer to the Cestino's.

#ifdef DEBUG
  Serial.println("identify_drive() done.");
#endif
}

//-----------------------------------------------------------
// Function setup() - Arduino Boilerplate. Called once.
//-----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }
  CONTROL_DDR = DATA_WRITE;
  //Set up serial and set the control port to write mode.

#ifdef DEBUG
  Serial.println("DEBUG is set.");
#endif
  //If global debug is set, the user is about to
  // get innundated with debug text.  Warn them...

  reset_drive(); //Any drive out there, reset it.
  Serial.print("Checking for a drive...");
  byte status_byte = get_drive_status_byte();
  //Get the status byte from any drive connected.


  if (get_drive_status_byte() == 255) {

    Serial.println("Floating bus - Drive not detected.");
    while (true) {
      //loop forever.
    }
  } //If no drive is detected, the bus floats high. Stop.

  else
  {
    Serial.println("Found a drive."); //Got valid drive
    identify_drive();
    //pull drive identity information into the block buffer.

    Serial.print("Model: ");
    for (int c = 54; c <= 74; c++) {
      Serial.print((char)block_buffer[c]);
    }

    Serial.print("\nSerial Number: ");
    for (int c = 20; c <= 30; c++) {
      Serial.print((char)block_buffer[c]);
    }

    Serial.print("\nFirmware Version: ");
    for (int c = 46; c <= 50; c++) {
      Serial.print((char)block_buffer[c]);
    }

    drive_cylinders = block_buffer[2] << 8;
    drive_cylinders += block_buffer[3];
    Serial.print("\nCylinders: ");
    Serial.print(drive_cylinders);
    drive_heads = block_buffer[6] << 8;
    drive_heads += block_buffer[7];
    Serial.print("\nHeads: ");
    Serial.print(drive_heads);
    drive_sectors = block_buffer[12] << 8;
    drive_sectors += block_buffer[13];
    Serial.print("\nSectors: ");
    Serial.print(drive_sectors);
    Serial.print("\nLBA Sectors: ");
    LBA_sectors = block_buffer[122];
    LBA_sectors = (LBA_sectors << 8) | block_buffer[123];
    LBA_sectors = (LBA_sectors << 8) | block_buffer[120];
    LBA_sectors = (LBA_sectors << 8) | block_buffer[121];
    Serial.print(LBA_sectors);
    //Why aren't we reading these bytes in order? They are
    //a 32 bit number in big-endian. Our endian converter
    //only understands 16 bit values. This is the only
    //32 bit field we have to deal with, so we grind it
    //manually.

    //Scrape the data in the block buffer for Model,
    //serial number, firmware version, etc.
    //Display those values.

  }
  Serial.println();
} //yay. Done with setup.


//-----------------------------------------------------------
// Function loop() - Arduino Boilerplate. Called repeatedly.
// Create the menu text with a big, hideous #define
// Reset the drive and wait until it's ready.
// Print the menu and wait for serial input.
// Process the menu entry and call one of our functions
//    Seek non-empty and write are more involved
// Since we're in loop(), repeat forever.
//-----------------------------------------------------------
void loop() {
#define menu "\n\n**** MENU ****\n\
1\tReset Drive\n\
2\tSleep Drive\n\
3\tIdentify Drive\n\
4\tSeek Non-Empty Block/Sector\n\
5\tWrite to Block/Sector\n\
6\tQuit\n"
  //This is the menu text, in one big, messy define.

  char menu_item = 0x00; //Initialize menu_item to 0.

  Serial.println(menu); //display the menu
  while (!Serial.available()) { //wait for serial input
    //do nothing
  }
  menu_item = Serial.read();
  //if we're here, we got serial input
  //We act on the input in the switch() below.


  switch (menu_item) {
    case 0x31 : {
        //0x31 is "1". Reset the drive, wait until ready.
        Serial.println("Resetting drive.");
        reset_drive();
        wait_for_drive_ready(1024);
        Serial.println("Drive is reset.");
        break; //Don't try the other cases.
      }


    case 0x32 : { //0x32 is "2" Sleep the drive.
        Serial.print("Sleeping Drive. ");
        Serial.println("Reset to wake up.");
        Serial.println("(Other functions will hang.)");
        sleep_drive(); //sleep the drive.
        break;//Don't try the other cases.
      }


    case 0x33 : { //0x33 is "3". Identify Drive
        Serial.println("Identifying Drive");
        identify_drive(); //put ID info in the block buffer
        dump_block_buffer(); //dump the block buffer
        break;//Don't try the other cases.
      }

    case 0x34 : {
        // 0x34 is "4". Iterate through the sectors
        // until we find a non-empty one.
        Serial.println("Seeking Non-empty Block/Sector.");
        uint32_t sector = 0;
        byte keep_going = 0x79; // lower case y

        while (keep_going == 0x79) {
          // Keep going until the user stops typing "y".

          read_write_drive_LBA_block(sector, false);
          //Read the LBA Block into the block buffer.

          if (!(sector % 100)) {
            Serial.println("Checking Block " +
                           (String)sector);
          }
          //Every 100 blocks give some feedback
          //so the user knows the sketch is still running.

          if (non_zero_read) {
            //Is the block empty? No? print a message and
            // dump the block buffer.

            Serial.print("Found something. Dumping sector ");
            Serial.print(sector);
            Serial.print("\n");
            dump_block_buffer();


            Serial.println("Shall I continue? (y/n)");
            //Does the user want to go on?

            while (!Serial.available()) {
              //do nothing until we get serial input.
            }

            keep_going = Serial.read();
            //got serial, store it in continue.
            //The loop will evaluate it.
          }
          sector++;
        }
        break; //Don't try the other cases.
      }


    case 0x35 : {
        //0x35 is "5". Ask for a block number,
        //write to it, and dump the block back.

        String input_string = "";
        //This case asks for more than single chars.
        //Store here.

        uint32_t block = 0;
        // Block is the block number we will write to.

        Serial.println("What LBA block shall I write to? ");
        Serial.print("The drive has ");
        Serial.print (LBA_sectors);
        Serial.print(" LBA blocks, and 28 bit LBA addresses go to");
        Serial.println(" 268435456.");
        //28 bit LBA can reach 128GiB.
        // To go further means not using ATA-1.

        while (!Serial.available()) {
          //do nothing while we wait for input.
        }
        input_string = Serial.readString();
        ////Store the serial input in input_string

        block = string2uint32_t(input_string);
        //turn input string into a uint32_t.

        input_string = "";//clear input string.

        Serial.println("Writing to block " + (String)block +
                       ". Please type your message and" +
                       " press return.");
        while (!Serial.available()) {
          //do nothing
        }
        input_string = Serial.readString();
        //Get another input string, this time the
        //pithy text message to be written to the
        //block the user selected. No post-processing
        //of the string needed this time.

        Serial.println("Writing \"" + input_string +
                       "\" to Block:" + (String)block);
        //Tell the user what we're writing. The \"
        //marks are escaped quotes so we can print them.

        for (int c = 0; c <= 512; c++) {
          if (c <= input_string.length()) {
            block_buffer[c] = input_string.charAt(c);
          } else block_buffer[c] = 0;
        }
        //Copy input_string into the block buffer.
        //Any bytes not used (because the string
        // is shorter) fill with 0s.
        //reset_drive();
        //The drive can get in the wrong mode to write.
        //resetting beforehand makes sure it's ready for
        //writing.
        
        read_write_drive_LBA_block(block, true);
        //write the contents of the block buffer
        //to the drive's data buffer which writes to
        //the media itself.

        wait_for_drive_ready(1024);
        Serial.println("Done writing. Reading block "
                       + (String)block);
        //wait up to 1024ms for the drive to finish writing.
        //We're only writing one block...
        
        read_write_drive_LBA_block(block, false);
        dump_block_buffer();
        //Read the block back into the block buffer and
        // dump the block buffer so the user can see.

        input_string = "";
        //clean up input_string.

        break;//Don't try the other cases.
      }


    case 0x36 : {
        Serial.println("Sleeping Drive and Exiting.");
        sleep_drive(); //put the drive in its sleep state
        Serial.println("Halted. Reset Cestino to Restart.");
        while (true) { //Do nothing forever.
        }
        break;
        //Don't try the other cases. Not that we'll ever
        //reach this code.
      }
  }
  menu_item = 0x00; //clear menu_item for the next go-round.
}
