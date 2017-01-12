#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

// ATMEL ATMEGA1284P
//
//                   +---\/---+
//           (D 1) PB0  1|        |40  PA0 (AI 0 / D40)
//           (D 2) PB1  2|        |39  PA1 (AI 1 / D39)
//      INT2 (D 3) PB2  3|        |38  PA2 (AI 2 / D38)
//       PWM (D 4) PB3  4|        |37  PA3 (AI 3 / D37)
//    PWM/SS (D 5) PB4  5|        |36  PA4 (AI 4 / D36)
//      MOSI (D 6) PB5  6|        |35  PA5 (AI 5 / D35)
//  PWM/MISO (D 7) PB6  7|        |34  PA6 (AI 6 / D34)
//   PWM/SCK (D 8) PB7  8|        |33  PA7 (AI 7 / D33)
//                 RST  9|        |32  AREF
//                 VCC 10|        |31  GND 
//                 GND 11|        |30  AVCC
//               XTAL2 12|        |29  PC7 (D 29)
//               XTAL1 13|        |28  PC6 (D 28)
//     RX0 (D 14)  PD0 14|        |27  PC5 (D 27) TDI
//     TX0 (D 15)  PD1 15|        |26  PC4 (D 26) TDO
// RX1/INT0 (D 16) PD2 16|        |25  PC3 (D 25) TMS
// TX1/INT1 (D 17) PD3 17|        |24  PC2 (D 24) TCK
//      PWM (D 18) PD4 18|        |23  PC1 (D 23) SDA
//      PWM (D 19) PD5 19|        |22  PC0 (D 22) SCL
//      PWM (D 20) PD6 20|        |21  PD7 (D 21) PWM
//                   +--------+
//

/*DPIN  PCInt
    1   8
    2   9
    3   10
    4   11
    5   12
    6   13
    7   14
    8   15

    14  24
    15  25
    16  26
    17  27
    18  28
    19  29
    20  30

    21  31
    22  16
    23  17
    24  18
    25  19
    26  20
    27  21
    28  22
    29  23

    33  7
    34  6
    35  5
    36  4
    37  3
    38  2
    39  1
    40  0
*/         

#define NUM_DIGITAL_PINS            40
#define NUM_ANALOG_INPUTS           8
#define analogInputToDigitalPin(p)  ((p < NUM_ANALOG_INPUTS) ? (40-(p)) : -1)

#define digitalPinHasPWM(p)         ((p) == 4 || (p) == 5 || (p) == 7 || (p) == 8 || (p) == 18 || (p) == 19 || (p) == 20 || (p) == 21)

static const uint8_t SS   = 5;
static const uint8_t MOSI = 6;
static const uint8_t MISO = 7;
static const uint8_t SCK  = 8;

static const uint8_t SDA = 23;
static const uint8_t SCL = 22;
static const uint8_t LED = 1;

//Analog Pin Definitions
//-------------------------------------------------
// Yes, this works. No, it shouldn't. 
// Something dumb is going on,
// and having the correct pinout here from 40-33
// results in pin 40 working right and 33-39 being
// backwards. Despite having nothing whatever to
// do with my digital pins (which work correctly)
// these analog pin assignments produce the desired
// result. 
//------------------------------------------------
static const uint8_t A0 = 24;
static const uint8_t A1 = 25;
static const uint8_t A2 = 26;
static const uint8_t A3 = 27;
static const uint8_t A4 = 28;
static const uint8_t A5 = 29;
static const uint8_t A6 = 30;
static const uint8_t A7 = 31;

//#define digitalPinToPCICR(p)    (((p) >= 0 && (p) < NUM_DIGITAL_PINS) ? (&PCICR) : ((uint8_t *)0))
#define digitalPinToPCICR(p)  (((p)>=1 && (p)<=8) || \
				(((p)>=14 && (p)<=21) || \
				  (((p)>=22 && (p)<=29) || \
				    (((p)>=33 && (p)<=40)?(&PCICR):\ 
				      ((uint8_t *)0)))))
//#define digitalPinToPCICRbit(p) (((p) <= 8) ? 1 : (((p) <= 21) ? 3 : (((p) <= 29) ? 2 : 0)))
#define digitalPinToPCICRbit(p) (((p)>=1 && (p)<=8)?1:\
				(((p)>=14 && (p)<=21)?3:\
				  (((p)>=22 && (p)<=29)?2:0)))

#define digitalPinToPCMSK(p)    (((p)>=1 && (p) <= 8) ? (&PCMSK2) : (((p)>=14 && (p) <= 21) ? (&PCMSK0) : (((p)>=22 &&(p) <= 29) ? (&PCMSK1) : (((p)>=33 && (p)<=40) : ((uint8_t *)0))))
#define digitalPinToPCMSKbit(p) (((p)<=8)?((p)-1):(((p)<=21)?((p)-14):(((p)<=29)?((p)-22):(abs((p)-40)))))
#ifdef ARDUINO_MAIN

#define PA 1
#define PB 2
#define PC 3
#define PD 4

// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint16_t PROGMEM port_to_mode_PGM[] =
{
	NOT_A_PORT,
	(uint16_t) &DDRA,
	(uint16_t) &DDRB,
	(uint16_t) &DDRC,
	(uint16_t) &DDRD,
};

const uint16_t PROGMEM port_to_output_PGM[] =
{
	NOT_A_PORT,
	(uint16_t) &PORTA,
	(uint16_t) &PORTB,
	(uint16_t) &PORTC,
	(uint16_t) &PORTD,
};

const uint16_t PROGMEM port_to_input_PGM[] =
{
	NOT_A_PORT,
	(uint16_t) &PINA,
	(uint16_t) &PINB,
	(uint16_t) &PINC,
	(uint16_t) &PIND,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] =
{ 
	NOT_A_PIN,
	PB, /* 1 */
	PB,
	PB,
	PB,
	PB,
	PB,
	PB,
	PB,
	NOT_A_PIN, //RST 9
	NOT_A_PIN, //VCC 10
	NOT_A_PIN, //GND 11
	NOT_A_PIN, //xtal2 12
	NOT_A_PIN, //xtal1 13
	PD, /* 14 */
	PD,
	PD,
	PD,
	PD,
	PD,
	PD,
	PD,
	PC, /* 22 */
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	PA, /* 33 */
	PA,
	PA,
	PA,
	PA,
	PA,
	PA,
	PA  /* 40 */
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] =
{
	 NOT_A_PIN,
	_BV(0), /* 1, port B */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	 NOT_A_PIN,
	 NOT_A_PIN,
	 NOT_A_PIN,
	 NOT_A_PIN,
	 NOT_A_PIN,
	_BV(0), /* 14, port D */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 22, port C */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	_BV(7), /* 33, port A */
	_BV(6),
	_BV(5),
	_BV(4),
	_BV(3),
	_BV(2),
	_BV(1),
	_BV(0)
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] =
{
	NOT_A_PIN,
	NOT_ON_TIMER, 	/* 1  - PB0 */
	NOT_ON_TIMER, 	/* 2  - PB1 */
	NOT_ON_TIMER, 	/* 3  - PB2 */
	TIMER0A,     	/* 4  - PB3 */
	TIMER0B, 	/* 5  - PB4 */
	NOT_ON_TIMER, 	/* 6  - PB5 */
	TIMER3A, 	/* 7  - PB6 */
	TIMER3B,	/* 8  - PB7 */
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_ON_TIMER, 	/* 14 - PD0 */
	NOT_ON_TIMER, 	/* 15 - PD1 */
	NOT_ON_TIMER, 	/* 16 - PD2 */
	NOT_ON_TIMER, 	/* 17 - PD3 */
	TIMER1B,     	/* 18 - PD4 */
	TIMER1A,     	/* 19 - PD5 */
	TIMER2B,     	/* 20 - PD6 */
	TIMER2A,     	/* 21 - PD7 */
	NOT_ON_TIMER, 	/* 22 - PC0 */
	NOT_ON_TIMER,   /* 23 - PC1 */
	NOT_ON_TIMER,   /* 24 - PC2 */
	NOT_ON_TIMER,   /* 25 - PC3 */
	NOT_ON_TIMER,   /* 26 - PC4 */
	NOT_ON_TIMER,   /* 27 - PC5 */
	NOT_ON_TIMER,   /* 28 - PC6 */
	NOT_ON_TIMER,   /* 29 - PC7 */
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_A_PIN,
	NOT_ON_TIMER,   /* 33 - PA0 */
	NOT_ON_TIMER,   /* 34 - PA1 */
	NOT_ON_TIMER,   /* 35 - PA2 */
	NOT_ON_TIMER,   /* 36 - PA3 */
	NOT_ON_TIMER,   /* 37 - PA4 */
	NOT_ON_TIMER,   /* 38 - PA5 */
	NOT_ON_TIMER,   /* 39 - PA6 */
	NOT_ON_TIMER   /* 40 - PA7 */
};

#endif // ARDUINO_MAIN

#endif // Pins_Arduino_h
// vim:ai:cin:sts=2 sw=2 ft=cpp
