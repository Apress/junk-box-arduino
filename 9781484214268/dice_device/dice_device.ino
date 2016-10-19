// Dice Device
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
// This sketch generates die rolls based on timing randominity.
// Essentially, this method of random generation assumes that
// human reflexes are not accurate to less than about a
// quarter of a second, and a sufficiently fast counter that
// rolls over will generate good random values when switched
// on and off by a human pushing a button.
//
// This sketch generates the rolls for a full set of
// polyhedral dice used in various role playing games,
// including a properly implimented 3d6 roll, and displays
// them to a multiplexed pair of 7 segment displays.
//-----------------------------------------------------------

// Precompiler #defines
//-----------------------------------------------------------
#define SEG_BUS PORTA
#define MUX_BUS PORTC
// Define the port to talk to the 7 segment display
// and the one that controls multiplexing via a uln2803

#define MSD_COMMON 0x2
#define LSD_COMMON 0x1
// Define the port settings for the multiplexer.

#define DECIMAL 16
#define BAR 17
// Define special characters as integer values.

#define BUTTON_PIN 16
#define BUTTON_PIN_INTERRUPT 0
#define SELECTOR 17
#define SELECTOR_INTERRUPT 1
// Define the pin and interrupt used by the roll button and
// the selector button.

//Global Variables
//-----------------------------------------------------------
const byte seg_decode_array[] = { 
  //7 segment decoding byte array.
  //A B G F D E DP C
  0b00100010, //0
  0b10111110, //1
  0b00010011, //2
  0b00010110, //3
  0b10001110, //4
  0b01000110, //5
  0b01000010, //6
  0b00111110, //7
  0b00000010, //8
  0b00001110, //9
  0b00001010, //A
  0b11000010, //B
  0b01100011, //C
  0b10010010, //D
  0b01000011, //E
  0b01001011, //F
  0b11111101, //Decimal Point
  0b11011111  //Bar
};
volatile unsigned long msecs = 0; //stores time in ISRs for debouncing.
boolean rolled_since_select = false; //Have we rolled?
//If so, this is true. Used in loop()

byte roll = 1; //value of the last roll of the dice.

volatile byte die = 4; //Die, as in d4, d8, d16, etc.
volatile byte d_select = 0; //what die is selected.
byte lsd = BAR; // leftmost digit's integer value.
byte msd = BAR; // rightmost digit's integer value.

volatile boolean lsd_active = true; //Used by timer ISR
// to store whether we're writing to the least significant
// digit or the most significant digit.

//-----------------------------------------------------------
//read selector ISR
//
// This function is an interrupt service routine. It takes no
// parameters and returns no data.
// When the button connected to BUTTON_PIN is pressed, the
// interrupt executes this function. It checks to see if 250ms
// have elapsed since the last press (for debouncing) and if
// so, increments d_select by 1.
// d_select is actually processed in the loop() function.
//-----------------------------------------------------------
void read_selector_isr() {

  if (millis() -msecs > 250) {
    d_select = d_select + 1;
    msecs = millis();
  }
  rolled_since_select = false;
}

//-----------------------------------------------------------
// ISR Timer interrupt service routine)
//
// This function takes no parameters and returns no data.
// it selects which digit is enabled (lsb or msb) by clearing
// MUX_BUS and then setting to LSD_COMMON or MSD_COMMON, then
// sets SEG_BUS to the bit pattern for the value in LSD or
// MSD respectively.
//
// This function also impliments ripple blanking, where
// the MSD is not displayed if it contains zero /unless/
// MSD contains 0xa (10), which we only hit if we're selecting
// or rolling percentile dice.
// 
// We're using timer 3 for the display multiplexer. ATmega328
// based Arduinos will need to use a different timer.
// This ISR is connected to the TIMER3_COMPA interrupt vector.
// This means that when the timer's value matches the contents
// of TIMER_3_COMPA, this ISR will be called.
//-----------------------------------------------------------
ISR(TIMER3_COMPA_vect) {//toggle the LED pin
  SEG_BUS = 0b11111111;
  MUX_BUS = MUX_BUS & 0b11111100;
 
  if (lsd_active) { //write to lsd
    
    MUX_BUS = MUX_BUS | LSD_COMMON;
    SEG_BUS = seg_decode_array[lsd];
  }
  else { //we must be writing to the most significant digit.
    if ((msd != 0)) { //only display msd if it's nonzero.
      if (msd == 0xa) msd = 0;
      // if msd is 0xa set it to zero after the zero test.
      // leading zero will show on d100 select and 100 rolls.

      MUX_BUS = MUX_BUS | MSD_COMMON;
      SEG_BUS = seg_decode_array[msd];
    }
  }
  lsd_active = !lsd_active;
}

//-----------------------------------------------------------
// setup()
//
// This function is called once by the Arduino core. It returns
// no data and takes no parameters.
// It sets up the serial console, sets the ports into the
// necessary states, and sets up the segment decoding array.
// It then turns interrupts off, sets up the timer interrupt
// and attaches the external interrupt to the
// SELECTOR_INTERRUPT interrupt and to the read_selector
// function. After that, it re-enables interrupts.
//-----------------------------------------------------------
void setup() {  
  DDRA = 0b11111111;
  DDRC = 0b00001111;
  PORTC = 0b00001100;
  DDRD = 0b00000000;
  PORTD = PORTD | 0b00001100;
  //Set up the various ports. Note the pullup resistors
  //being set on port D.


  noInterrupts(); //turn interrupts off

  //Set up the timer/counter and timer/counter interrupt.
  //---------------------------------------------------

  //TCCR3A:
  // COM3a1 COM3a0 COM3b1 COM3b0 - - WGM31 WGM30
  TCCR3A=0;
  // All four COM3 bits remain at zero: the port
  // operates normally. Likewise the WGM31 and 30
  // bits remain at zero. We only turn on WGM32,
  // for CTC mode 4, and that's in TCCRB3.
  
  //TCCR3B
  // ICNC3 ICESN0 - WGM33 WGM32 CS32 CS31 CS30
  TCCR3B=0b00001101;
  // turn on WGM32 for CTC mode 4
  // and set the clock select bits to 101 for the 
  // 1024 prescaler. Timer/counter is running as of now.

  //TCCR3C
  // FOC3A, FOC3B, no other pins are used.
  TCCR3C=0;
  // Make sure FOC3A and B are cleared. We don't want to
  // force a compare.

  OCR3A = 0x00A3; //output compare register. 
  // 20mHz/1024 (the prescaler) is about 19.5kHz.
  // Hex 00A3 is 163. 19.5kHz/163 is a bit over 120Hz. 
  // Bearing in mind that each digit is refreshed every other 
  // interrupt, that gives us about 60Hz per digit, 
  // which is enough to avoid flicker. Persistence of vision 
  // FTW.
  // Yes, this could have been done with a timer/counter
  // configuration using the OC3A and OC3B pins and no
  // ISR at all, but then we wouldn't learn how to do 
  // timer ISRs.

  //TMSK3
  // - - ICIE3 - - OCIE3B OCIE3A TOIE3
  TIMSK3 = 0b00000010;
  // Set OCIE3A - enable an interrupt when output compare
  // match A (TCNT3 = OCR3A) occurs.

  //Set up the selector interrupt.
  //---------------------------------------------------

  attachInterrupt(SELECTOR_INTERRUPT, read_selector_isr, FALLING );
  //read_selector interrupt - when the read_selector pin changes
  //from high to low, interrupt.

  interrupts(); //turn interrupts on.  
}

//-----------------------------------------------------------
// loop()
//
// Called over and over forever by the Arduino core.
// Loop sets the values of the globals lsd and msd for the
// display ISR to pick up,
// processes what the read_selector ISR has set in the
// d_select global variable,
//-----------------------------------------------------------
void loop() {
  byte display_value = 0;

  if (rolled_since_select) {
    display_value = roll;
  }
  else {
    display_value = die;
  }

  lsd = display_value % 10;
  msd = display_value / 10;
  //Button is active LOW, so if the roll button is NOT pushed, 
  //display either the selected die or the last roll.

  const int die_value[] = {4, 6, 8, 10, 12, 20, 100, 18};
  die = die_value[d_select];

  if (d_select > 7) d_select = 0;
// set die to whatever value was last selected by the
// select ISR with die_value[].

  if (!digitalRead(BUTTON_PIN)) {
    while (!digitalRead(BUTTON_PIN)) {
      lsd = BAR;
      msd = BAR;
      roll++;
      if (roll > die)roll = 1;
      rolled_since_select = true;
    }
// If the roll button is pressed, display bars in both digits
// and increment roll every time loop is called.Also set 
// rolled_since_select to true.
  
    if (die == 18 && rolled_since_select) { 
      randomSeed(roll);
      roll = (int)random(1, 6) + (int)random(1, 6) + (int)random(1, 6);
      //The D18 mode is really 3d6 for chargen.
      // Seed with roll and call random() 3 times
      //Cast random results as int since they are long
      // otherwise.
    }
  }
}










