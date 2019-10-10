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
volatile void accumulate_buttons(void); // wywo³ywana w przerwaniu TCC5

static inline uint8_t debounce(void);

void buttons_setup(void)
{
	PORTA.DIRCLR = PIN_LEFT | PIN_RIGHT | PIN_DOWN | PIN_UP | PIN_SEL |PIN_STIM;	//te piny s¹ wejœciami
	
	PORTCFG.MPCMASK = PIN_LEFT | PIN_RIGHT | PIN_DOWN | PIN_UP | PIN_SEL |PIN_STIM; //zapisanie konfiguracyjnej maski pinów
	PORTA.PIN0CTRL = PORT_OPC_PULLDOWN_gc | PORT_ISC_FALLING_gc;					//dziêki zastosowaniu maski, ustawiane s¹ rejestry CTRL wszystkich pinów w niej zapisanych (i tylko te - np. pin0 niekoniecznie)
	PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;		//uniemo¿liwienie odczytania wartoœci pinu DAC w rejestrze PORTA.IN
}


//funkcja wywo³ywana przerwaniem timera, zbiera x stanów wejœæ
volatile void accumulate_buttons(void) 
{
	static uint8_t index;
	
	saved_state[index] = PORTA.IN; //1 oznacza wciœniêty przycisk - u¿yty pulldown
	index++;	
	if(index == READ_ITERATIONS)
		index = 0;
}


static inline uint8_t debounce(void)
{
	uint8_t debounced_state=0xFF;
	for(uint8_t i=0; i<READ_ITERATIONS; i++)
		debounced_state &= (saved_state[i]);  //1 oznacza wciœniêty przycisk
	
	return debounced_state;		//maska wciœniêtych przycisków
}



uint8_t button_press(void)  //zwraca nr przycisku lub 0xFF, gdy nic nie wciœniête
{
	uint8_t one_button=0xFF;
	uint8_t temp=debounce();
	
	for (uint8_t i=0; i<(B_NO_OF_BTNS); i++)
	{		
		if (temp & 0x01)	// sic! - sprawdza dany bit
			one_button = i;		//zwraca najni¿szy z jednoczeœnie wciœniêtych przycisków
		temp = temp >> 1;
	}
	
	return one_button;
}



ISR(TCC5_OVF_vect)		// przerwanie przepe³nienia TCC5
{
	accumulate_buttons();                     // zamiana stanu diody
	TCC5.INTFLAGS |= TC5_OVFIF_bm; //usuniêcie flagi przepe³nienia
}