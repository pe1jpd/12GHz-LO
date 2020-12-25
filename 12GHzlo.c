/*
 * lock frequency of crystal chain to 10 MHz reference.
 * (c) Bas de Jong - PE1JPD 2016-2020
 *
 * avrdude -P COM8 -b 19200 -c avrisp -p t13 -u -U flash:w:"10ghzlock.hex":i
 */

#define F_CPU 		1000000UL

#define F_RF        2943000UL       	// kHz
#define F_REF		  40000UL       	// kHz
#define F_PFD		    500UL         	// kHz

#define sbi(x,y) x |= _BV(y) //set bit - using bitwise OR operator 
#define cbi(x,y) x &= ~(_BV(y)) //clear bit - using bitwise AND operator
#define tbi(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/sleep.h>

// ADF4113 - 12GHzLO pinpout
#define LE          PB0
#define DATA        PB1
#define CLK     	PB2
#define LD			PB4

// funtion prototypes
void initPLL(long int f);
void setPLL(long int r);

void initPLL(long int f)
{
    long int N, A, B;
    long int reg, reg2;

    cbi(PORTB, DATA);
    cbi(PORTB, CLK);
    cbi(PORTB, LE);

	// set function latch
    reg = 2L;					// register 2
    reg |= 1UL<<22;             // P = 16/17
    reg |= 7UL<<15;             // cur1 = 111
    reg |= 0UL<<9;              // fastlock disabled
    reg |= 1UL<<7;              // CP positive
//	reg |= 1UL<<4;              // mux digital LD
	reg |= 2UL<<4;              // mux N divider out
//	reg |= 4UL<<4;              // mux R divider out
//	reg |= 5UL<<4;              // mux analog detect out
    reg |= 1UL<<2;              // R, A, B held in reset
    reg2 = reg;                 // save for later
    setPLL(reg);

    // set ref counter latch
    reg = 0UL;                   	// register 0
    reg |= 2UL<<16;               	// antbacklash 6ns
    reg |= (F_REF/F_PFD)<<2;
    setPLL(reg);
    
    N = f/F_PFD;                	// total N
    B = N/16;                    	// prescaler /16
    A = N%16;                    	// Fvco = (PB+A)F_PFD
    
    // set AB counter latch
    reg = 1L;                   	// register 1
    reg |= 0UL<<21;               	// current setting 1
    reg |= ((B&0x1fff)<<8) + ((A&0x3f)<<2);
	setPLL(reg);
    
    reg = reg2 & ~(1<<2);       	// clear reset bit
    setPLL(reg);                	// and run
}

void setPLL(long int r)
{
    int i;
    
    for (i=0; i<24; i++) {
        if (r & 0x800000)
            sbi(PORTB, DATA);
        else
            cbi(PORTB, DATA);
		_delay_us(10);
        sbi(PORTB, CLK);
        _delay_us(10);
        cbi(PORTB, CLK);
        r <<= 1;
    }
	_delay_us(10);
    sbi(PORTB, LE);
    _delay_us(10);
    cbi(PORTB, LE);
}

int main()
{
	DDRB = 0x07;

	for (;;) {
	
		// set frequency to lock
		initPLL(F_RF);

		sleep_enable();
		sleep_cpu();

		for (;;);
			_delay_ms(500);
			initPLL(F_RF);
	}

	return 0;
}
