/*
 * buttons.h
 *
 * Created: 26-04-2014 18:12:46
 *  Author: Dobo
 */ 

#include <avr/interrupt.h> 

#ifndef BUTTONS_H_
#define BUTTONS_H_

#define LEFT	1
#define RIGHT	3
#define DOWN	0
#define UP		4
#define SELECT	5
#define STIM	6

#define PIN_LEFT	(1<<LEFT)
#define PIN_RIGHT	(1<<RIGHT)
#define PIN_DOWN	(1<<DOWN)
#define PIN_UP		(1<<UP)
#define PIN_SEL		(1<<SELECT)
#define PIN_STIM	(1<<STIM)
#define PIN_DAC		PIN2_bm

#define READ_ITERATIONS 10		//iloœæ kolejno odczytanych stanów wciœniêcia, potrzebnych do akceptacji naciœniêcia 
#define B_NO_OF_BTNS	6		// sprawdzanie ca³ego bajtu, chocia¿ niektóre bity to wyjœcia i zawsze s¹ równe 0



void buttons_setup(void);
uint8_t button_press (void);

#endif /* BUTTONS_H_ */