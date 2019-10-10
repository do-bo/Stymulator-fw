/*
 * buttons.c
 *
 * Created: 24-04-2014 22:01:37
 *  Author: Dobo
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "buttons.h"
#include "oled.h"

static volatile uint8_t saved_state[READ_ITERATIONS];  // wykorzystywane w przerwaniu TCC5
volatile void accumulate_buttons(void); // wywo�ywana w przerwaniu TCC5

static inline uint8_t debounce(void);

void buttons_setup(void)
{
	PORTA.DIRCLR = PIN_LEFT | PIN_RIGHT | PIN_DOWN | PIN_UP | PIN_SEL |PIN_STIM;	//te piny s� wej�ciami
	
	PORTCFG.MPCMASK = PIN_LEFT | PIN_RIGHT | PIN_DOWN | PIN_UP | PIN_SEL |PIN_STIM; //zapisanie konfiguracyjnej maski pin�w
	PORTA.PIN0CTRL = PORT_OPC_PULLDOWN_gc | PORT_ISC_FALLING_gc;					//dzi�ki zastosowaniu maski, ustawiane s� rejestry CTRL wszystkich pin�w w niej zapisanych (i tylko te - np. pin0 niekoniecznie)
	PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;		//uniemo�liwienie odczytania warto�ci pinu DAC w rejestrze PORTA.IN
}


//funkcja wywo�ywana przerwaniem timera, zbiera x stan�w wej��
volatile void accumulate_buttons(void) 
{
	static uint8_t index;
	
	saved_state[index] = PORTA.IN; //1 oznacza wci�ni�ty przycisk - u�yty pulldown
	index++;	
	if(index == READ_ITERATIONS)
		index = 0;
}


static inline uint8_t debounce(void)
{
	uint8_t debounced_state=0xFF;
	for(uint8_t i=0; i<READ_ITERATIONS; i++)
		debounced_state &= (saved_state[i]);  //1 oznacza wci�ni�ty przycisk
	
	return debounced_state;		//maska wci�ni�tych przycisk�w
}



uint8_t button_press(void)  //zwraca nr przycisku lub 0xFF, gdy nic nie wci�ni�te
{
	uint8_t one_button=0xFF;
	uint8_t temp=debounce();
	
	for (uint8_t i=0; i<(B_NO_OF_BTNS); i++)
	{		
		if (temp & 0x01)	// sic! - sprawdza dany bit
			one_button = i;		//zwraca najni�szy z jednocze�nie wci�ni�tych przycisk�w
		temp = temp >> 1;
	}
	
	return one_button;
}



ISR(TCC5_OVF_vect)		// przerwanie przepe�nienia TCC5
{
	accumulate_buttons();                     // zamiana stanu diody
	TCC5.INTFLAGS |= TC5_OVFIF_bm; //usuni�cie flagi przepe�nienia
}