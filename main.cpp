/*

csw-nextplex
Quad numerical segment display test with multiplexing
Based off csw-noplex, csw-7seg-take1 and csw-dubseg

IAR EW project for Coda 0.6/0.7 and compatible.

Property of Criterion Signalworks.
Do not distribute or reproduce without permission.
For more information, contact management@csw.cx or tim@csw.cx

TCS 2020 10

*/

#include <intrinsics.h>
#include <iostm8S103f3.h>
#include <math.h>
#include <stdint.h>

unsigned long sy; // System tick 

void initsc() {			// Initialize SysClk
	CLK_ICKR = 0;			//  Reset the Internal Clock Register
	CLK_ICKR_HSIEN = 1;		//  Enable the HSI
	CLK_ECKR = 0;			//  Disable the external clock
	while (CLK_ICKR_HSIRDY == 0);	//  Wait for the HSI to be ready for use
	CLK_CKDIVR = 0;			//  Ensure the clocks are running at full speed
	CLK_PCKENR1 = 0xff;		//  Enable all peripheral clocks
	CLK_PCKENR2 = 0xff;
	CLK_CCOR = 0;			//  Turn off CCO
	CLK_HSITRIMR = 0;		//  Turn off any HSIU trimming
	CLK_SWIMCCR = 0;		//  Set SWIM to run at clock / 2
	CLK_SWR = 0xe1;			//  Use HSI as the clock source
	CLK_SWCR = 0;			//  Reset the clock switch control register
	CLK_SWCR_SWEN = 1;		//  Enable switching
	while (CLK_SWCR_SWBSY != 0);	//  Pause while the clock switch is busy
}

/*
void delay(unsigned long tg){
  tg = tg- 1;
  unsigned long lm = sy + tg;
  while(lm >= sy);
}
*/

//  TIM2 interrupt handler
#pragma vector = TIM2_OVR_UIF_vector
__interrupt void TIM2_UPD_OVF_IRQHandler(void)
{
    sy++;			//  timer increment
    TIM2_SR1_UIF = 0;		//  Reset the interrupt
}

void inittim2()			//  Setup Timer 2 to generate an interrupt every 1mS based on a 16MHz clock.
{
    TIM2_PSCR = 0x04;		//  Prescaler = 32. actually 16
    TIM2_ARRH = 0x03;		//  High byte of 940. 
    TIM2_ARRL = 0xe6;		//  Low byte 
    TIM2_IER_UIE = 1;		//  Enable the update interrupts.
    TIM2_CR1_CEN = 1;		//  Finally enable the timer.
}

/*void inittim2()			//  Setup Timer 2 to generate an interrupt every 1mS based on a 16MHz clock.
{
    TIM2_PSCR = 0x05;		//  Prescaler = 32.
    TIM2_ARRH = 0x01;		//  High byte of 500. 
    TIM2_ARRL = 0xf4;		//  Low byte of 500.
    TIM2_IER_UIE = 1;		//  Enable the update interrupts.
    TIM2_CR1_CEN = 1;		//  Finally enable the timer.
}
*/

void initports() {		// Initialize all GPIO ports 
	PA_ODR = 0x00;		// Port A
	PA_DDR = 0xFF;
	PA_CR1 = 0xFF;
	PA_CR2 = 0xFF;
	PB_ODR = 0x00; 		// Port B
	PB_DDR = 0xFF;
	PB_CR1 = 0xFF;
	PB_CR2 = 0xFF;
	PC_ODR = 0x00; 		// Port C
	PC_DDR = 0xFF;
	PC_CR1 = 0xFF;
	PC_CR2 = 0xFF;
	PD_ODR = 0x00; 		// Port D
	PD_DDR = 0xFF;
	PD_CR1 = 0xFF;
	PD_CR2 = 0xFF;
}

void clearODR() {
	PA_ODR = 0x00;
//	PB_ODR = 0x00;
	PC_ODR = 0x00;
	PD_ODR = 0x00;
}

int seg[] = {
0xEE, // 0 11101110	7 Segment lookup table layout
0x24, // 1 00100100	Manually synthesized
0xBA, // 2 10111010	TCS 2020 10 R2
0xB6, // 3 10110110	
0x74, // 4 01110100	Physical arrangement
0xD6, // 5 11010110	  A  
0xDE, // 6 11011110	B   C
0xA4, // 7 10100100	  D  
0xFE, // 8 11111110	E   F
0xF6, // 9 11110110	  G   H
0xFC, // A 11111100	
0x5E, // b 01011110	value storage order is ABCDEFGH
0xCA, // C 11001010	H = don't care 
0x3E, // d 00111110
0xDA, // E 11011010
0xD8, // F 11011000
};

int ex[] = {0, 0, 0, 0}; // Current output, LSB at index 0

int bound[] = {10, 10, 10, 6}; // Limit before ticking over (implement 60 seconds in a minute)

						// X Segment
						//   XX Physical pin
void writeBankA(unsigned long in) {		//      XXXXXXXX Bitmask
	PD_ODR_ODR1 = (in & 0x80) != 0;		// A D1 10000000
	PD_ODR_ODR2 = (in & 0x40) != 0;		// B D2 01000000
	PD_ODR_ODR3 = (in & 0x20) != 0;		// C D3 00100000
	PD_ODR_ODR4 = (in & 0x10) != 0;		// D D4 00010000
	PD_ODR_ODR5 = (in & 0x08) != 0;		// E D5 00001000
	PD_ODR_ODR6 = (in & 0x04) != 0;		// F D6 00000100
	PA_ODR_ODR1 = (in & 0x02) != 0;		// G A1 00000010
};

void writeBankB(unsigned long in) {
	PC_ODR_ODR3 = (in & 0x80) != 0;		// A C3 10000000
	PC_ODR_ODR4 = (in & 0x40) != 0;		// B C4 01000000
	PC_ODR_ODR5 = (in & 0x20) != 0;		// C C5 00100000
	PC_ODR_ODR6 = (in & 0x10) != 0;		// D C6 00010000
	PC_ODR_ODR7 = (in & 0x08) != 0;		// E C7 00001000
	PA_ODR_ODR2 = (in & 0x04) != 0;		// F A2 00000100
	PA_ODR_ODR3 = (in & 0x02) != 0;		// G A3 00000010
};

void togglesink() { 				// Switches cathode sink for multiplexing
	PB_ODR_ODR4 = !PB_ODR_ODR4;
	PB_ODR_ODR5 = !PB_ODR_ODR5;
}

int main(void)
{
	initsc(); 			// Initialize SysClk
	initports(); 			// Initialize all GPIO ports 
	inittim2(); 			// Initialize timer 2
	__enable_interrupt();
	
	PB_ODR_ODR4 = 0; 		// Initialize multiplex sink 
	PB_ODR_ODR5 = 1;
	
	int cadence = 10; 		// Set cadence
	int pause = 1; 			// Set multiplex rate
	
	unsigned long tg = cadence;		// Duplicate cadence to target 
	tg = tg- 1;				// Prepare target
	unsigned long lm = sy + tg;		// Limit is current system time plus target 
	
	while (1)
	{
		for (ex[3] = 0; ex[3] < bound[3]; ex[3]++) {
			for (ex[2] = 0; ex[2] < bound[2]; ex[2]++) {
				for (ex[1] = 0; ex[1] < bound[1]; ex[1]++) {
					for (ex[0] = 0; ex[0] < bound[0]; ex[0]++) {
					//  	unsigned long tg = cadence;		// Duplicate cadence to target 
					//	tg = tg- 1;				// Prepare target
					//	unsigned long lm = sy + tg;		// Limit is current system time plus target 
					 	while(lm >= sy) {			// While current time is less than limit: 
							writeBankA(seg[ex[0]]);		// Display decimal 4 (LSB)
							writeBankB(seg[ex[1]]);		// Display decimal 3 at the same time 
						//	delay(pause);			// Wait a little
							__wait_for_interrupt();
							clearODR();			// Turn off all outputs before switching 
							togglesink();			// Switch multiplex channel
							writeBankA(seg[ex[2]]);
							writeBankB(seg[ex[3]]);
						//	delay(pause);
							__wait_for_interrupt();
							clearODR();
							togglesink();
						}
						lm = lm + cadence; 				// big innovation? should produce more accurate clock? 
					}
				}
			}
		}
	}
}


