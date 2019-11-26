#include "TM4C123GH6PM.h"
#include <stdio.h>

void UART0Tx(char c);
void UART0_init(void);
void UART0_puts(char* s);
void delayMs(int n);
short getECGValue(void);
void UART0SendChunk(char *c, int size);

#define samplespersec 5000
#define numsec 3
#define totalsamples numsec*samplespersec

char sendDataBuffer[totalsamples * 3 / 2];

int main(void) {
	UART0_init(); // initialize UART0 for output 
	// enable clocks 
	SYSCTL->RCGCGPIO |= 0x10; // enable clock to GPIOE 
	SYSCTL->RCGCADC |= 1; // enable clock to ADC0 
	SYSCTL->RCGCWTIMER |= 1; /* enable clock to WTimer Block 0 */

	// initialize PE3 for AIN0 input 
	GPIOE->AFSEL |= 8; // enable alternate function 
	GPIOE->DEN &= ~8; // disable digital function 
	GPIOE->AMSEL |= 8; // enable analog function 

	// initialize ADC0 
	ADC0->ACTSS &= ~8; // disable SS3 during configuration 
	// ADC0->EMUX &= ~0xF000; // software trigger conversion 
	ADC0->EMUX |= 0x5000; /* timer trigger conversion seq 0 */
	ADC0->SSMUX3 = 0; // get input from channel 0 
	ADC0->SSCTL3 |= 6; // take one sample at a time, set flag at 1st sample 
	ADC0->ACTSS |= 8; // enable ADC0 sequencer 3 

	/* initialize wtimer 0 to trigger ADC at 1 sample/sec */
	WTIMER0->CTL = 0; /* disable WTimer before initialization */
	WTIMER0->CFG = 0x04; /* 32-bit option */
	WTIMER0->TAMR = 0x02; /* periodic mode and down-counter */
	WTIMER0->TAILR = 16000000/samplespersec; /* WTimer A interval load value reg (1 s) */
	WTIMER0->CTL |= 0x20; /* timer triggers ADC */
	WTIMER0->CTL |= 0x01; /* enable WTimer A after initialization */


	short valA, valB;
	
	int j;
		
	
	// Assemble
	j = 0;
	for (int i = 0; i < totalsamples / 2; i += 1) {
		valA = getECGValue();
		valB = getECGValue();
		sendDataBuffer[j++] = valA & 0xff;
		sendDataBuffer[j++] = valB & 0xff;
		sendDataBuffer[j++] = ((valA & 0xf00) >> 8) | ((valB & 0xf00) >> 4);
	}
	
	// Reassemble and send strings
	j = 0;
	char buffer[10];
	for (int i = 0; i < totalsamples / 2; i += 1) {
		char valALow = sendDataBuffer[j++];
		char valBLow = sendDataBuffer[j++];
		char highs = sendDataBuffer[j++];
		
		int a = valALow | ((highs & 0x0f) << 8);
		int b = valBLow | ((highs & 0xf0) << 4);
		
		sprintf(buffer, "%d\r\n", a);
		UART0_puts(buffer);
		sprintf(buffer, "%d\r\n", b);
		UART0_puts(buffer);
	} 
	
	while(1){}
}

short getECGValue() {
	ADC0->PSSI |= 8; // start a conversion sequence 3
	while((ADC0->RIS & 8) == 0); // wait for conversion complete
	
	int ecg = ADC0->SSFIFO3 * 3300 / 4096;
	ADC0->ISC = 8;
	
	return ecg;
}

void UART0_init(void) {
 SYSCTL->RCGCUART |= 1; // provide clock to UART0 
 SYSCTL->RCGCGPIO |= 0x01; // enable clock to GPIOA 
 // UART0 initialization 
 UART0->CTL = 0; // disable UART0 
 UART0->IBRD = 8; // 16MHz/16=1MHz, 1MHz/104=9600 baud rate 
 UART0->FBRD = 44; // fraction part 
 UART0->CC = 0; // use system clock 
 UART0->LCRH = 0x60; // 8-bit, no parity, 1-stop bit, no FIFO 
 UART0->CTL = 0x301; // enable UART0, TXE, RXE 

 // UART0 TX0 and RX0 use PA0 and PA1. Set them up. 
 GPIOA->DEN = 0x03; // Make PA0 and PA1 as digital 
 GPIOA->AFSEL = 0x03; // Use PA0,PA1 alternate function 
 GPIOA->PCTL = 0x11; // configure PA0 and PA1 for UART 
}

void UART0SendChunk(char *c, int size) {
	for (int i = 0; i < size; ++i) {
		UART0Tx(*(c++));
	}
}

void UART0Tx(char c) {
 while((UART0->FR & 0x20) != 0); // wait until Tx buffer not full 
 UART0->DR = c; // before giving it another byte 
}

void UART0_puts(char* s) {
 while (*s != 0) // if not end of string 
 UART0Tx(*s++); // send the character through UART0 
}

// delay n milliseconds (16 MHz CPU clock) 
void delayMs(int n) {
 int32_t i, j;
 for(i = 0 ; i < n; i++)
 for(j = 0; j < 3180; j++)
 {} // do nothing for 1 ms 
}
