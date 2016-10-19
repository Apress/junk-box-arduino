// Z80 Explorer v2.1
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
//-----------------------------------------------------------
// This sketch connects a Z80 microprocessor to your Cestino 
// and uses the Cestino to simulate RAM and to provide a
// simple monitor for dumping and modifying the simulated
// RAM. It also allows control of the Z80's clock from
// 0 Hz up to MAX_CLOCK, defined below.
//-----------------------------------------------------------
// Hardware
//-----------------------------------------------------------
// Z80 Signals Connected to ATmega1284P Port D:
//--------------------------------------------
// Signal: /WAIT /RESET /CLK       /M1 /RD /WR (-) (-)
// class: cpu    cpu    clock_gen  cpu cpu cpu (serial)
//
// All Other Z80 Signals
//----------------------
// /MREQ /IORQ /RFSH /HALT /INT   /NMI /BUSRQ
// n/c   n/c   n/c   n/c   button n/c  n/c
//-----------------------------------------------------------
// Z80 Address + Busses
// Signal:      A0-A7   A8-A15 D0-D7
// ATmega Port: PORTA   PORTB  PORTC
// Class:       cpu     cpu    cpu
//-----------------------------------------------------------

//-----------------------------------------------------------
// Preprocessor #defines
//-----------------------------------------------------------
#define MAX_CLOCK 20000000 // Maximum clock speed
#define MEM_SIZE 0x2000 // 8192 Bytes
#define RESET_MS 5000 // Z80 reset: hold /RESET low this long.

//-----------------------------------------------------------
// Function Prototypes
// Since some member functions of our classes call utility 
// functions, it behoves us to declare function prototypes.
// Arduino 1.6.5 does this for us, but 1.6.7 seems to break 
// that behavior.
//-----------------------------------------------------------
String repeat(int number, char character);
String get_input_string();
uint32_t string2uint32_t(String input);
uint16_t hex_string2uint16_t(String input);
void menu();

//-----------------------------------------------------------
// Classes
//
// Since we're using software running on the ATmega1284P to
// simulate hardware, it makes sense to break the code up
// into objects along the same lines.
//-----------------------------------------------------------

//-----------------------------------------------------------
// cpu class:
//
// An object of this class reads and writes the various pins
// of PORT D to control the Z80, read its status signals,
// provide access to its address bus, and read from or write
// to its data bus.
//-----------------------------------------------------------
class cpu {
  private:
    // These are private variables, constants, and (potentially)
    // member functions. These can't be touched by code outside
    // this class.

    uint8_t saved_control_port = CTRL_DEFAULT;
    // We often need to preserve the state of the
    // control port before we change it, and we may not
    // know exactly what it is. It's stored in this private
    // variable.

    const uint8_t M1_MASK = 0b00010000; //Read the /M1 signal
    const uint8_t RESET = 0b10111111; // Set the /RESET signal
    const uint8_t WAIT = 0b01111111; // Set the /WAIT signal
    // These consts define various bit values we need to
    // set or read the named status signals of the Z80
    // with PORT D.

    const uint8_t CTRL_DEFAULT = 0b11000000; // default Z80 state
    // This is the default state for PORTD and by extension, the Z80.
    // Hold /WAIT and /RESET high. /CLK is under timer control.
    // /WR, /MREQ and /RD are all inputs from the ATmega's view.

    volatile uint8_t& CONTROL_PORT = PORTD;
    volatile uint8_t& CONTROL_PINS = PIND;
    // These are reference variables. They are run-time
    // aliases for PORTD and PIND. Note the & sign.
    // That denotes them as references. Note that they are private.

    volatile uint8_t& DATA_PORT = PORTC;
    volatile uint8_t& DATA_DDR = DDRC;
    volatile uint8_t& DATA_PINS = PINC;
    // Reference variables for our data port, PORTC.  These
    // are private and strictly for programming convenience
    // within this class.

  public:
    cpu() { // constructor
      Serial.println("CPU: Object created.");
      DDRA = 0b00000000; // Address LSB
      DDRB = 0b00000000; // Address MSB
      DDRC = 0b11111111; // Data Port;
      DDRD = 0b11100000; // control_port
      // Initialize the DDRs for all ports. Since only PORTC
      // has a DDR reference varable, I'm using the register
      // names for everything for consistency.

      DATA_PORT = 0b00000000;
      CONTROL_PORT = CTRL_DEFAULT;
      // Initialize the values of DATA_PORT and CONTROL_PORT.
    }
    // This is the constructor of this class. It is called when
    // an object of this class is instantiated. The constructor
    // sets ports A and B up to read the low and high memory
    // address bytes, and port C up (initially) to write data
    // to the Z80, but this port can also read data sent from
    // the Z80 to the system.

    boolean M1() {

      return (CONTROL_PINS & M1_MASK);
    }
    // This function returns the status of the Z80's /M1 signal.
    // It does a logical AND of CONTROL_PINS and the M1_MASK.
    // If any pins in that AND turn up true, the function will
    // return true. Because all Z80 signals are active low,
    // this will mean /M1 is inactive.

    void mode_default() {
      CONTROL_PORT = CTRL_DEFAULT;
    }
    // Set the CONTROL_PORT to its default. Put the Z80 in
    // non-halted, non-waited mode.  Run mode, really.

    void mode_wait() {
      CONTROL_PORT = CONTROL_PORT & WAIT;
    }
    // Lower the Z80's /WAIT signal so the Z80 will
    // not try and read until our ISRs are ready.
    // Even though the ATmega1284P is a universe
    // faster than we're running the Z80, our
    // memory is simulated in software. It's
    // really, really slow.

    void save_mode() {
      saved_control_port = CONTROL_PORT;
    }
    // Preserve the current value of CONTROL_PORT in the
    // private saved_control_port variable.

    void restore_mode() {
      CONTROL_PORT = saved_control_port;
    }
    // set CONTROL_PORT to the value in saved_control_port.
    // Note that we don't check that a save_mode was done
    // first, so use with caution.

    volatile uint8_t& addr_msb = PINB;
    volatile uint8_t& addr_lsb = PINA;
    // These two references give our sketch
    // a consistent name for the address bytes.
    // That way we can access them without
    // adding any more instructions.

    uint8_t data_out() {
      DATA_DDR = 0b00000000;
      return DATA_PINS;
    }
    // Set the DATA_PORT to read mode, and read the
    // Z80's data port for data coming FROM the Z80.
    // Return that data.
    // ATmega is READING. Z80 is WRITING.

    void data_in(uint8_t data) {
      DATA_DDR = 0b11111111;
      DATA_PORT = data;
    }
    // Make sure the data port is in WRITE mode, and
    // set it to the value of data.
    // ATmega is WRITING, Z80 is READING.

    void reset(void) {
      Serial.println("CPU: Resetting...");
      CONTROL_PORT = CONTROL_PORT & RESET;
      delay(RESET_MS);
      Serial.println("CPU: Done Resetting.");
      CONTROL_PORT = CTRL_DEFAULT;
    }
    // Lower the Z80's /RESET signal for RESET_MS milliseconds.
    // In order to reset properly the Z80 needs its /reset
    // signal held low for multiple clock cycles. Given that
    // our clock's minimum speed is 1Hz, 5 seconds seems like
    // a safe value. This could be dynamic based on the clock
    // speed, but we're not in that much of a hurry.
};

//-----------------------------------------------------------
// clock_gen class
//
// This class configures timer/counter 1 to output a square
// wave signal on OCP1, from 1Hz to MAX_CLOCK MHz, depending 
// on how we configure it. There are only two member functions,
// a constructor and the destructor. When an object in this 
// class is instantiated, it's done with a text string 
// parameter containing the text value the user wants 
// the clock set to.
// The constructor handles the rest: generates the prescaler
// value and the match value, and tells the user what it's
// done. Note that because the Cestino has a 20MHz system
// clock, its maximum clock resolution is 50ns. Some
// speeds will be approximations since they don't divide
// evenly by 50ns.
//-----------------------------------------------------------
class clock_gen {
  private:
    int prescale_values[5] = {1024, 256, 64, 8, 1};
    int prescale_bits[5] = {0b101, 0b100, 0b011, 0b010, 0b001};
    // lookup tables for prescale bit fields and their values.

  public:
    // Destructor
    ~clock_gen() {
      TCCR1A = 0;
      TCCR1B = 0;
      OCR1A = 0;
      TCNT1 = 0;
      Serial.println("Clock Generator: Object Deleted.");
    }
    // The destructor is called when an object is deleted.
    // In this case, it clears all the timer registers, which
    // turns the timer off.

    // Constructor
    clock_gen(uint32_t desired_frequency) {
      Serial.println("Clock: Object Created.");
      float counter_value = MAX_CLOCK / desired_frequency;
      float lowest_inaccuracy = 1.0;
      float current_steps = 0;
      int prescaler = 1;
      byte prescaler_config_bits;
      long int match;

      for (int c = 0; c <= 4; c++) {
        current_steps = counter_value / prescale_values[c];
        if ((current_steps - round(current_steps) < lowest_inaccuracy)\
            && (current_steps <= 65535)) {
          lowest_inaccuracy = current_steps - ((int)current_steps);
          prescaler = prescale_values[c];
          match = round(current_steps);
          prescaler_config_bits = prescale_bits[c];
          if (match == 0) match = round(counter_value);
        }
      }
      // Find the highest prescaler value that will both allow the
      // clock to generate the value the user wanted, with the
      // lowest inaccuracy of the resulting clock speed.
      // We do this by determining the number of steps the
      // maximum clock speed available (20MHz on the Cestino)
      // will occur for each step of our clock's output. Then we
      // iterrate from highest to lowest prescaler values to find the
      // one that divides most evenly into the number of steps.
      // We set our match value to the number of steps we want
      // divided by the prescaler.

      Serial.print("We want to count to ");
      Serial.println(counter_value, 2);
      Serial.print("For a clock speed of ");
      Serial.println(desired_frequency, DEC);
      Serial.print("I chose a prescaler of ");
      Serial.println(prescaler, DEC);
      Serial.print("And a match of ");
      Serial.println(match, DEC);
      Serial.print("We'll count to ");
      Serial.println((long int)prescaler * match, DEC);
      // tell the user what values we generated.

      TCCR1A = 0b01000000;
      TCCR1B = (0b00001000 | prescaler_config_bits);
      TCNT1 = 0;
      OCR1A = match;
      Serial.println("Clock Generator: Running");
      // set the timer/counter registers.
    }
};

//-----------------------------------------------------------
// memory_simulator class
// This class contains the array we're using as simulated ram
// for the Z80, and functions to access it.
//-----------------------------------------------------------
class memory_simulator {
  private:
    volatile uint8_t halt = 0x76;
    volatile uint8_t mem_array[MEM_SIZE];
    // declare a variable to hold the halt instruction
    // for the Z80, and declare the array we're using
    // for simulated memory. These are private variables and
    // cannot be touched directly by outside code.

    void dump_page_header() {
      Serial.println("\nAddress\t 0  1  2  3  4  5  6  7  8  9" +
                     String("  a  b  c  d  e  f   Data (text)"));
      Serial.println(repeat(75, '-'));
    }
    //Print the header for the dump pretty printer.

    void m_edit_instructions() {
      Serial.println("\n\t*** Mem-Sim Line Editor ***");
      Serial.print("Enter an address and data in HEX ");
      Serial.println("eg: 0x0000,0x76");
      Serial.println("\"exit\" to quit\n\"dump\" to view memory\n");
      // Print the handy instructions.
    }

  public:
    volatile boolean m_write_enable = true;
    // If m_write_enable is false, m_seek_write() will say it 
    // wrote, but it won't actually do it. A kludge used to
    // protect simulated memory during Z80 resets.

    memory_simulator() { // constructor
      Serial.println("Memory: Initializing...");
      for (int c = 0; c < MEM_SIZE; c++) {
        mem_array[c] = 0;
      }// Wipe the entire array. There's no telling what was
      // in that memory before we declared the array.

      Serial.println("Memory: " + String(MEM_SIZE, DEC) + \
                     " (0x" + String(MEM_SIZE, HEX) + ") bytes free.");
    }
    // tell the user how much memory we have.

    // The constructor of memory_simulator wipes the memory
    // array, which is declared on instantiation.after that,
    // it reports the available RAM to the user. We use the
    // default destructor, since we don't do anything special
    // there.

    volatile uint8_t m_seek_read(volatile uint8_t address_msb, volatile uint8_t address_lsb) {
      volatile union {
        volatile uint16_t sixteen_bit_address;
        volatile uint8_t byte_array[2];
      } addr_byte_union;
      addr_byte_union.byte_array[0] = address_lsb;
      addr_byte_union.byte_array[1] = address_msb;
      // We're using this union to plug in the address_msb and address_lsb
      // variables and get out a uint16_t.  Make sure to put the bytes in
      // in little endian order (lsb first) or your results will be very
      // odd once you go over address 0x0020.  Been there, did that.

      if (addr_byte_union.sixteen_bit_address >= MEM_SIZE) {
        return halt;

      } else {
        return mem_array[addr_byte_union.sixteen_bit_address];
      }
      // Check to see if we're trying to seek a legit address. If not,
      // return a z80 halt instruction and throw an error message.
    }
    // The m_seek_read member function decodes two separate bytes (address_msb and address_lsb)
    // into a single uint16_t address, then goes to that address and returns that cell of the
    // array. If the decoded address exceeds MEM_SIZE, we return halt, otherwise we return
    // the array at that address.

    void m_seek_write(volatile uint8_t address_msb, volatile uint8_t address_lsb, volatile uint8_t data) {
      if (m_write_enable) {
        // On Z80 resets we can get spurious triggering
        // of the write ISR resulting in random writes 
        // to memory. Reset sets m_write_enable to false, then
        // back to true when the reset is done.
        
        volatile union {
          volatile uint16_t sixteen_bit_address;
          volatile uint8_t byte_array[2];
        } addr_byte_union;
        addr_byte_union.byte_array[0] = address_lsb;
        addr_byte_union.byte_array[1] = address_msb;
        // We're using this union to plug in the address_msb and address_lsb
        // variables and get out a uint16_t.  Make sure to put the bytes in
        // in little endian order (lsb first) or your results will be very
        // odd once you go over address 0x0020.  Been there, did that.

        if (addr_byte_union.sixteen_bit_address >= MEM_SIZE) {

        } else {
          mem_array[addr_byte_union.sixteen_bit_address] = data;
        }
        // Check to see if we're trying to seek a legit address. If not,
        // fail silently. Ugh.
      }
    }
    // The m_seek_write member function decodes the two bite field address
    // the same way m_seek_read does. If the decoded address exceeds MEM_SIZE,
    // we throw an error message, otherwise we set the array[address] to data.

    void m_dump() {
      Serial.println("Dumping Simulated Memory");
      int c = 0;
      String line_start_address = "0x";
      uint16_t row_address = 0;
      String hex_data = "";
      String human_readable_data = "";
      // We have some variables. Initialize them.

      dump_page_header();
      //show the header

      for (int row = 0; row < (MEM_SIZE / 16); row++) {
        row_address = 16 * row;
        if (row_address < 0x1000)line_start_address += "0";
        if (row_address < 0x0100)line_start_address += "0";
        if (row_address < 0x0010)line_start_address += "0";
        line_start_address += String(row_address, HEX) + "|";

        for (int col = 0; col < 16; col++) {
          // We're printing blocks of 256 bytes.
          // each is 2 characters + two spaces
          // in hex, plus 1 character in text.
          if (mem_array[c] < 0x10) hex_data += "0";
          // Add leading zero for hex values below 0x10.

          hex_data += String(mem_array[c], HEX);
          hex_data += " ";
          // Add mem_array[c] as a hex string to hex_data.

          if (isprint(mem_array[c])) {
            human_readable_data += (char)mem_array[c];
          } else {
            human_readable_data += ".";
          }
          // If mem_array[c] is printable, add it to
          // human_readable_data. Otherwise add a . for
          // a placeholder.
          c++;
        }

        Serial.println(line_start_address + " " + hex_data + \
                       " | " + human_readable_data);
        hex_data = "";
        human_readable_data = "";
        line_start_address = "0x";
        // Combine the three strings in one Serial.println.
        // Then clear them.
        if (!(c % 256)) {
          Serial.println("Continue (y/n)");
          if (get_input_string() == "n") {
            row = MEM_SIZE / 16;
          } else {
            dump_page_header();
            //show the header for each page, actually.
          }
        }
      }
    }
    // m_dump produces a human-readable dump of the memory array in 256 byte pages,
    // with each line 16 (0xF) items long.

    void m_edit() {
      String input_string = "y";
      uint16_t address;
      uint8_t  data;
      int comma_index = 0;
      String temp;

      m_edit_instructions();

      do {
        // repeat is loop until the 'while' is satisfied.
        input_string = get_input_string();
        // get the input string.
        if (input_string == "dump") {
          m_dump();
          m_edit_instructions();
          // If the user types "dump," dump the memory to
          // the serial console. Don't try and process it
          // into code. Show the instructions again.
        }
        if (input_string.startsWith("0x", 0)) {
          comma_index = input_string.indexOf(",");
          // find the comma in the input string

          temp = input_string.substring(0, comma_index);
          temp.trim();
          // copy the string from the beginning to the comma into temp.
          // also trim it - get rid of leading and trailing spaces.

          address = hex_string2uint16_t(temp);
          // call hex_string2uint16_t with temp. Set the results into address.

          temp = input_string.substring(comma_index + 1);
          temp.trim();
          data = (uint8_t)hex_string2uint16_t(temp);
          // repeat the process with the back half of the input string.

          Serial.println(">  Addr: 0x" + String(address, HEX) + \
                         " Data: 0x" + String(data, HEX) + " [OK]");
          // Print the line describing what is being entered where.

          mem_array[address] = data;
          // actually enter it.
        }
      } while (input_string != "exit");
      // if the user types "exit" stop repeating the loop.
    }
    //m_edit edits the memory array directly, allowing the user
    // to deposit a hex value in 0x notation at a hex address,
    // also in 0x notation. It allows the user to dump the
    // array to the screen at will, and exits on the exit
    //command.
};


//-----------------------------------------------------------
// Global variables
// The big advantage to using C++ objects in this project
// was the drastic reduction of global variables. There
// are still quite a few variables, but most of them are
// contained in functions or in classes and don't exist
// until an object of that class is instantiated.
//-----------------------------------------------------------
clock_gen* ourclock = NULL; // pointer to a clock_gen object.
memory_simulator* mem_sim = NULL; // pointer to memory object.
cpu Z80; // declare our CPU object. Not a pointer.

//-----------------------------------------------------------
// Functions which are not members of a class.
//-----------------------------------------------------------

//-----------------------------------------------------------
// Repeat(int number char character)
// This function creates a string of character
// exactly /number/ long.
//-----------------------------------------------------------
String repeat(int number, char character) {
  String temp = "";
  for (int c = 0; c < number; c++) {
    temp = temp + String(character);
  }
  return temp;
}
//-----------------------------------------------------------
// get_input_string()
// This function loops forever waiting for an input string
// and returns the string when it gets one.
//-----------------------------------------------------------
String get_input_string() {
  while (!Serial.available()) {
  }
  return (Serial.readString());
}

//-----------------------------------------------------------
// string2uint32_t()
//- The String class includes no nice way to turn strings of
// digits into a numeric value. (Actually, there's an
// undocumented one. Classy.)
//
// This function goes through the string from left to right.
// For each position it advances to the right, it multiplies
// the existing value by 10 and adds the value of the
// character at that position minus the value of the
// character '0'. When we reach the end of the string,
// return temp.
//-----------------------------------------------------------
uint32_t string2uint32_t(String input) {
  uint32_t temp = 0;
  input.trim();
  for (int c = 0; c < input.length(); c++) {
    temp = temp * 10 + input.charAt(c) - '0';
  }
  return temp;
}

//-----------------------------------------------------------
// hex_string2uint16_t()
// This function goes through the string from left to right.
// For each position it advances to the right, it multiplies
// the existing value by 16. if the new digit is 0-9 (less
// than the value of ':') add the character's numeric value
// minus the value of '0',  A-F and a-f are processed the
// same way, except that we check to make sure the new
// digit is actually in the range and subtract a different
// value.  Once we have the correct value for the character,
// we add the value of the character at that position minus
// the value of the character '0'. When we reach the end
// of the string, return temp.
//-----------------------------------------------------------
uint16_t hex_string2uint16_t(String input) {
  uint16_t temp = 0;
  char tempchar;
  input.trim();
  for (int c = 2; c < input.length(); c++) {
    temp = (temp * 0x10);
    tempchar = input.charAt(c);
    if (tempchar < ':') temp += (tempchar - '0');
    if ((tempchar > '@') && (tempchar < 'G')) temp += (tempchar - '7');
    if ((tempchar > '`') && (tempchar < 'g')) temp += (tempchar - 'W');
  }
  return temp;
}

//-----------------------------------------------------------
// menu
// This function generates the menu and selects which function
// to call, whether it's in an object or not.
//-----------------------------------------------------------
void menu() {
  String input_string = "";
  Serial.println("\n" + repeat(15, ' ') + "*** Menu ***");
  Serial.println(repeat(42, '-'));
  Serial.println("Z80 Operations Commands");
  Serial.println(repeat(42, '-'));
  Serial.println("(1) Reset Z80");
  Serial.println("(2) Set Clock Speed");
  Serial.println("(3) Stop Clock");
  Serial.println(repeat(42, '-'));
  Serial.println("Free Run ISR Commands");
  Serial.println(repeat(42, '-'));
  Serial.println("(4) Attach Free Run ISR to INT1");
  Serial.println("(5) Detach Free Run ISR from INT1");
  Serial.println(repeat(42, '-'));
  Serial.println("Memory Simulation and Z80 Programming");
  Serial.println(repeat(42, '-'));
  Serial.println("(6) Initialize Simulated Memory");
  Serial.println("(7) Enter Program Into Simulated Memory");
  Serial.println("(8) Run Program (Attach Mem_Sim ISRs)");
  Serial.println("(9) Dump Simulated Memory");
  Serial.println(repeat(42, '-'));
  // print the text of the menu.

  Serial.print("Clock is ");
  if (ourclock != NULL) {
    Serial.println("Running.");
  } else {
    Serial.println("Stopped.");
  }
  Serial.print("Memory is ");
  if (mem_sim != NULL) {
    Serial.println("Available.");
  } else {
    Serial.println("Not Available.");
  }
  Serial.println(repeat(42, '-'));
  // check to see if there's an object attached to ourclock. If not,
  // then the clock is not enabled. Tell the user one way or the other.
  // Likewise, if there's an object attached to mem_sim, memory is
  // enabled. Otherwise it's not. Tell the user.

  switch ((int)string2uint32_t(get_input_string())) {
    case 1: Z80.reset();
      break;
    // Option 1 is reset. Call the reset member function of Z80.

    case 2:
      Serial.println("Menu: Enter Z80 Clock Speed (1 - 20000000Hz)");
      input_string = get_input_string();
      delete ourclock;
      ourclock = NULL; // clear the pointer. Otherwise things get confused.
      ourclock = new clock_gen(string2uint32_t(input_string));
      mem_sim->m_write_enable=false;
      Z80.reset();
      mem_sim->m_write_enable=true;
      break;
    // Option 2 is start the clock/set the clock. Call input_string()
    // and stash the results in a variable by the same name.
    // Delete anything that's already on the ourclock pointer and set it to null.
    // Do the delete to not waste system resources. Do the set to null so as not
    // to get weird clock behavior.
    // Then instantiate a clock_gen object. Take input_string and feed it to
    // string2uint32_t, and feed the output of /that/ to the clock_gen's
    // constructor, and let it do all the work.

    case 3:
      delete ourclock;
      ourclock = NULL; // Clear the pointer, otherwise things get confused.
      break;
    // Option 3 stops the clock. To do this, delete the object pointed to
    // by ourclock, and set ourclock to NULL so the next clock doesn't have
    // weird behavior.
    case 4:
      Serial.println("Menu: Attaching free_run_ISR to INT1");
      noInterrupts();
      detachInterrupt(1);
      attachInterrupt(1, free_run_ISR, FALLING);
      interrupts();
      Z80.reset();
      break;
    // Option 4 hooks up the free running ISR to interrupt 1, which listens for /mreq
    // events. Every time the Z80 asks for memory and pulls this signal low, our ISR will
    // fire. Free-running means we always give the Z80 NOP instructions (do nothing,
    // go on to the next address), so we can watch and see if the address signals
    // change. Turn interrupts off, attach the ISR to interrupt zero on the falling
    // edge (/MREQ is active low), then turn interrupts back on. Finally, reset the
    // Z80 so it starts from address 0x0000 in the output.

    case 5:
      Serial.println("Menu: Detatching free_run_ISR from INT1");
      noInterrupts();
      detachInterrupt(1);
      interrupts();
      Z80.reset();
      break;
    // You know how the last option attached the free_run_ISR?  Option 5 detatches it.
    // Turn interrupts off, detatch interrupt 1, turn interrupts back on, then reset
    // the Z80 on general principles.

    case 6:
      delete mem_sim;
      mem_sim = NULL;
      mem_sim = new memory_simulator();
      break;
    // Option 6 turns on simulated RAM for the Z80. Delete anything on the mem_sim
    // pointer, and set the pointer to NULL to avoid memory strangeness. Then instantiate
    // a memory_simulator object and attach it to the mem_sim pointer. Memory_simulator
    // objects' constructor takes no parameters.

    case 7:
      mem_sim->m_edit();
      break;
    // Option 7 is enter a program into simulated memory.
    // call the m_edit member function of the memory simulator. This lets the user put
    // simple, hand-assembled programs into the simulated memory for the Z80 to run.
    // It's slightly less tedious than doing it with toggle switches on a front panel.
    // (but only slightly).

    case 8:
      Serial.println("Menu: Attaching mem_read_ISR to INT1.");
      Serial.println("Menu: Attaching mem_write_ISR to INT0.");
      Serial.println("Menu: Any program therein should run.");
      noInterrupts();
      detachInterrupt(1);
      detachInterrupt(0);
      mem_sim->m_write_enable=false;
      attachInterrupt(1, mem_read_ISR, FALLING);
      attachInterrupt(0, mem_write_ISR, FALLING);
      mem_sim->m_write_enable=true;
      interrupts();
      Z80.reset();
      break;
    // Just as option 4 attached the free running ISR to interrupt 1,
    // option 8 connects the memory simulator ISR to interrupt 1.
    // This means that when the Z80 lowers its /MREQ signal and
    // requests memory, our ISR will try to service it with calls
    // to the memory_simulator object connected to mem_sim. Why
    // isn't the ISR in the object?  Because the Arduino core
    // won't let you. Same as with option 4. Stop interrupts, detatch
    // anything already connected to interrupt 1, attach mem_sim_ISR
    // to interrupt 1 on the falling edge, then turn interrupts back on
    // and reset the Z80 so our output starts at 0x0000.
    // NOTE - CHNAGED FROM FALLING TO LOW

    case 9:
      mem_sim->m_dump();
      break;
      // Option 9 dumps the simulated memory array to the serial
      // console, one 256 byte page at a time. Which gets tedious
      // going through 8 kilobytes, but it gets there. We just call
      // the m_dump() function of the memory_simulator object
      // connected to the mem_sim pointer.
      // Such a tangled web we weave.
  } // end of case.
} // end of menu function.

//-----------------------------------------------------------
// free_run_ISR()
// This function is an interrupt service routine for
// INT1. When INT1 fires, we're in a read cycle.
// This ISR prints out the 16 bit address
// requested by the Z80, and returns a 0x00 (NOP) to the
// Z80, telling it to do nothing and go the next address,
// allowing us to observe the address lines (and make sure
// they all work and are connected correctly.
//-----------------------------------------------------------
void free_run_ISR() {
  byte temp;
  String output = "";
  Z80.save_mode();
  // Save the control signals we're sending to the Z80.

  Z80.mode_wait();
  // Set the Z80's mode to wait, so it stops asking for
  // new addresses while the ISR is trying to service
  // this request.

  Z80.data_in(0x0);
  // Always send the Z80 a NOP (0x0).

  output += String("free_run_ISR: Address: 0x");
  if (Z80.addr_msb < 0x10) output += String("0");
  output = output + String(Z80.addr_msb, HEX);
  if (Z80.addr_lsb < 0x10) output += String("0");
  output += String(Z80.addr_lsb, HEX);
  Serial.println(output);
  // Build the output string to show the user
  // what address was requested. This is what
  // free running is for.
  Z80.restore_mode();
  // un-wait the Z80.
}
//-----------------------------------------------------------
// mem_read_ISR()
// This function is an interrupt service routine for
// INT1. Like free_run_ISR, the first thing it does
// is save the Z80 control signal state, then set the Z80
// into wait mode, so we can survice this memory request
// before the Z80 asks for the next one. After that, it sends
// data FROM the memory simulator TO the Z80 (on read)
// After that, it builds up a string to tell the user what
// just happened and prints it.
//-----------------------------------------------------------
void mem_read_ISR() {
  uint8_t tempdata = 0;
  String output = "";

  Z80.save_mode();
  // Save the Z80 control signal state.

  Z80.mode_wait();
  // Put the Z80 into wait mode.

  tempdata = mem_sim->m_seek_read(Z80.addr_msb, Z80.addr_lsb);



  if (!Z80.M1()) {
    output += String("mem_read_ISR: Z80 Fetched Address: 0x");
  } else {
    output += String("mem_read_ISR: Z80 Read Address: 0x");
  }
  Z80.data_in(tempdata);
  // On a read cycle (from the Z80's perspective)
  // tell the object pointed to by mem_sim to seek the address
  // present on the Z80's address bus, and send the data
  // stored there to the Z80's data bus.
  // Tell the user that the Z80 read the address.


  if (Z80.addr_msb < 0x10) output += String("0");
  output += String(Z80.addr_msb, HEX);
  if (Z80.addr_lsb < 0x10) output += String("0");
  output += String(Z80.addr_lsb, HEX);
  output += String("\tData: 0x");
  if (tempdata < 0x10) output += String("0");
  output += String(tempdata, HEX);
  Serial.println(output);
  // Build the rest of the output string and display it. /M1
  // is the Z80's signal to indicate it's doing an instruction
  // fetch. If it is, tell the user so.

  Z80.restore_mode();
  //Restore the Z80 to non-halted mode.
}
//-----------------------------------------------------------
// mem_write_ISR()
// This function is an interrupt service routine for INT0.
// like mem_read_ISR(), the first thing it does is
// is save the Z80 control signal state, then set the Z80
// into wait mode, so we can survice this memory request
// before the Z80 asks for the next one. After that, it sends
// data FROM the Z80 TO the memory simulator.
// After that, it builds up a string to tell the user what
// just happened and prints it.
//-----------------------------------------------------------
void mem_write_ISR() {
  String output = "";
  uint8_t tempdata = 0;

  Z80.save_mode();
  // Save the Z80 control signal state.

  Z80.mode_wait();
  // Put the Z80 into wait mode.
  
  tempdata = Z80.data_out();
  mem_sim->m_seek_write(Z80.addr_msb, Z80.addr_lsb, tempdata);
  output += String("mem_write_ISR: Z80 Wrote Address: 0x");
  // On write cycle (from the Z80's perspective)
  // tell the memory simulator object to seek the address
  // present on the Z80's address bus, and set that address
  // of the memory simulator TO the value on the Z80's
  // data bus.

  if (Z80.addr_msb < 0x10) output += String("0");
  output += String(Z80.addr_msb, HEX);
  if (Z80.addr_lsb < 0x10) output += String("0");
  output += String(Z80.addr_lsb, HEX);
  output += String("\tData: 0x");
  if (tempdata < 0x10) output += String("0");
  output += String(tempdata, HEX);
  Serial.println(output);
  //Build the rest of the output string and display it.

  Z80.restore_mode();
  //Restore the Z80 to non-halted mode.
}
//-----------------------------------------------------------
// Setup
// Sets the serial console speed.
//-----------------------------------------------------------
void setup() {
  Serial.begin(115200);
}

//-----------------------------------------------------------
// Loop
// Calls the menu() function. Over and over again.
//-----------------------------------------------------------
void loop() {
  menu();
}

