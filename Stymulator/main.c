/*
 * Stymulator.c
 *
 * Created: 05-03-2014 22:38:52
 *  Author: Pawe³ Dobosz
 */ 


#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>	//itoa

//#define SIMULATOR

#include "ssd1306.h"
#include "buttons.h"
#include "gui.h"
#include "oled.h"

void stimulate(void);
void dac_config(void);
void clock_config(void);
void timer_config(void);

/******************** zmienne globalne (przerwanie) ***************************/
volatile static uint8_t stim_flag = 10;
volatile uint16_t dac_data = 0;

const char PROGMEM txtus[]="us";
const char PROGMEM txtms[]="ms";
const char PROGMEM txtuA[]="uA";
const char PROGMEM txtmA[]="mA";

struct _menuitem const PROGMEM menu_current;
struct _menuitem const PROGMEM menu_deltaT;
struct _menuitem const PROGMEM menu_pulseT;		//deklaracje  zmiennych wymaganych poni¿ej

uint16_t pulseT=50, deltaT=500, current=300; //pocz¹tkowe wartoœci parametrów

struct _menuitem const menu_current PROGMEM = {&current, (uint16_t)9, (uint16_t)20, (uint16_t)20000, txtuA, txtmA, 0, &menu_pulseT};
struct _menuitem const menu_pulseT	PROGMEM = {&pulseT, (uint16_t)49, (uint16_t)50, (uint16_t)1000, txtus, txtms, &menu_current, &menu_deltaT};
struct _menuitem const menu_deltaT	PROGMEM = {&deltaT, (uint16_t)89, (uint16_t)400, (uint16_t)15000, txtus, txtms, &menu_pulseT, 0};




int main(void) 
{					 
	dac_config();	
	clock_config();	
	timer_config();

	buttons_setup();
	protocol_setup();
	ssd1306_initialize();	//czyszczenie ekranu z losowych wartoœci po uruchomieniu
	oled_clear_screen();	

  	ssd1306_send_command(0x81);//kontrast
	ssd1306_send_command(30);

	print_menu();

	sei();          // globalne odblokowanie przerwañ


/************************************************************************/
/*                                g³ówna pêtla - odczyt przycisków                           */
/************************************************************************/
uint8_t button_press_val=255, button_prev_val=255;
uint16_t button_repeat=0, 
		 repeat_trigger=0; // u¿ywane przy przytrzymaniu przycisku 

	while(1)
	{	
		button_prev_val = button_press_val;
		button_press_val = button_press();
		
		if (button_prev_val == button_press_val)	// zapobieganie zbyt czêstemu powtarzaniu akcji przy przytrzymaniu przycisku
			button_repeat += 1;
		else {button_repeat = 0; repeat_trigger=65000;}		
		
		if (button_repeat == 4000) repeat_trigger = 1500;	//kroki i szybkoœæ autopowtarzania
		if (button_repeat == 16000) repeat_trigger = 400;
		if (button_repeat == 25000) repeat_trigger = 200;
		
		if (button_repeat == 0 || (button_repeat%repeat_trigger == 0)) //gdy wciœniêty inny przycisk, ni¿ poprzednio LUB jeden przytrzymany przez x powtórzeñ pêtli
		{
			switch (button_press_val)
			{
				case SELECT:	if (button_prev_val != SELECT) stimulate(); // zapobiegniêcie przytrzymaniu przycisku stymulacji
								break;
				case RIGHT:		menu_select_next(); break;
				case LEFT:		menu_select_prev(); break;
				case DOWN:		modify_value(0); break;
				case UP:		modify_value(1); break;
			}
			//button_repeat = 0;
		}
		
	
	}
		
	
	while(1); //zabezpieczenie
	return 0;
}




//----------------------------------------------------------------------------------------------------------------------------------------------------------------//----------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------//----------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------- //----------------------------------------------------------------------------------------------------------------------------------------------------------------

/************************************************************************/
/*			funkcje                                                                     */
/************************************************************************/

void dac_config(void)
{
	/************************************************************************/
	/*				konfiguracja DAC                                                       */
	/************************************************************************/
	PORTA.DIRSET = PIN2_bm;		// pin2 (dac) jest wyjœciem
	PORTA.PIN2CTRL = PORT_OPC_PULLDOWN_gc;

	//wstêpna konfiguracja DAC
	DACA.CTRLB = DAC_CHSEL_SINGLE_gc ;	//w³¹czenie kana³u 0 dac (gc - group configuration)
	//| DAC_CH0TRIG_bm;		//automatyczne uruchomienie konwersji w wyniku zdarzenia na kanale okreœlonym w EVCTRL
	//DACA.EVCTRL = DAC_EVSEL_0_gc;		//wybór kana³u zdarzeñ nr 0 jako wejœcia dla dac
	
	DACA.CTRLC = DAC_REFSEL_INT1V_gc;	//wybór referencji 1.00V jako napiêcia odniesienia dac
	
	DACA.CTRLA =	DAC_CH0EN_bm;// |			//ch. 0 output enable - wyjœcie kan. 0 dac pojawia siê na pinie, chyba, ¿e IDOEN = 1
					//DAC_ENABLE_bm;			//w³¹czenie dac
	//DAC_IDOEN_bm |		//internal output enable - wyjœcie kana³u 0 dac przekierowane na odczyt wewnêtrzny

	//DACA.STATUS -rejestr stanu dac; lsb (ch0dre) = 1 stanowi o pustym rejestrze danych ch0 dac
	//gm- group mask bm-bit mask gc-group conf
	//todo: kalibracja gain i offset
	
	//dane (right-adjust): CH0DATAH 0bxxxx0000 CH0DATAL 0b00000000
}

void clock_config(void)
{
	/************************************************************************/
	/*				konfiguracja zegara                                                */
	/************************************************************************/
		
	CCP = CCP_IOREG_gc;					//odblokowanie zmian w rejestrach chronionych
	//OSC.CTRL |= OSC_RC32MEN_bm;			// uruchomienie wbudowanego oscylatora 32MHz	
	OSC.CTRL|=OSC_RC2MEN_bm;			// Enable the internal 2 MHz RC oscillator
	while ((OSC.STATUS & OSC_RC2MRDY_bm)==0); // Wait for the internal 2 MHz RC oscillator to stabilize

	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC2M_gc;	// Select the system clock source: 2 MHz Internal RC Osc.
	CCP = CCP_IOREG_gc;	
	OSC.CTRL &= ~(OSC_RC32MEN_bm | OSC_XOSCEN_bm | OSC_PLLEN_bm); //wy³¹czenie wewn. oscylatora 32 MHz, zewn. osc., PLL
   
	/*
	OSC.DFLLCTRL |= OSC_RC32MCREF_RC32K_gc;		// kalibracja oscylatora 32MHz RC za pomoc¹ osc. 32.768kHz
	DFLLRC32M.CTRL|=DFLL_ENABLE_bm;  // w³¹czenie autokalibracji
	while ((OSC.STATUS & OSC_RC32M) == 0);	// czekanie na stabilizacjê
	CCP = CCP_IOREG_gc;					//odblokowanie zmian w rejestrach chronionych
	CLK.CTRL = CLK_SCLKSEL_RC32M_gc;	//wybór oscylatora jako Ÿród³o zegara
	CCP = CCP_IOREG_gc;	
	OSC.CTRL &= ~(OSC_RC2MEN_bm | OSC_XOSCEN_bm | OSC_PLLEN_bm); //wy³¹czenie wewn. oscylatora 2 MHz, zewn. osc., PLL
	*/
	
	//wy³¹czenie zewn. wyjœcia zegara
	PORTCFG.CLKOUT &= ~(PORTCFG_CLKOUT_OFF_gc);
}

void timer_config(void)
{
 /************************************************************************/
 /*                           konfiguracja timerów i przerwañ                                */
 /************************************************************************/
 //TCC5 - zbieranie stanu przycisków
	 // konfiguracja timera TCC5 
 	TCC5.CTRLB = TC45_WGMODE_NORMAL_gc;	// normalny tryb pracy
 	//TCC5.CTRLGSET = TC5_DIR_bm;			// liczenie w dó³
 	TCC5.CTRLA  = TC45_CLKSEL_DIV4_gc;	// ustawienie preskalera - nastêpuje uruchomienie timera
 				//uwaga - nie ma mo¿liwoœci zerowania licznika prescalera - po uruchomieniu zwiêksza licznik po 1-DIVx cyklach
 	TCC5.PER = 2500; //(1/2MHz)*4 =2us    2us*2500= 4,8ms
	 
	//konfiguracja przerwania od TCC5
	TCC5.INTCTRLA = TC45_OVFINTLVL_LO_gc;         // przepe³nienie ma generowaæ przerwanie LO
	PMIC.CTRL |= PMIC_LOLVLEN_bm;            // odblokowanie przerwañ o priorytecie LO

//TCC4 - stymulacja
	// konfiguracja timera TCC4 
	TCC4.CTRLB = TC45_WGMODE_NORMAL_gc;	// normalny tryb pracy
	//TCC4.CTRLGSET = TC4_DIR_bm;			// liczenie w dó³
	TCC4.CTRLA  = TC45_CLKSEL_OFF_gc;	// ustawienie preskalera - nastêpuje uruchomienie timera
	TCC4.PER = 65535; //(1/2MHz)*1 =0,5us    0,5us*30000= 15ms  -tylko po to, ¿eby zd¹¿yæ wy³¹czyæ przerwanie przed przepe³nieniem
	TCC4.CTRLGSET = TC4_STOP_bm;			//zatrzymanie timera przy najbli¿szym przepe³nieniu
	
	//konfiguracja przerwania od TCC4
	TCC4.INTCTRLA = TC45_OVFINTLVL_MED_gc;	// przepe³nienie ma generowaæ przerwanie medium
	PMIC.CTRL |= PMIC_MEDLVLEN_bm;			// odblokowanie przerwañ o priorytecie MED
}

void prepulse_config(void) 
{
	/************************************************************************/
	/*		 konfiguracja wyjœæ prepulsu i Enable konwertera dc-dc		   */
	/************************************************************************/
	
	PORTA.DIRSET = PIN6_bm |			//PA6 po³¹czony z pinem 2 ukladu XL6009 - 1to uruchomianie konwertera boost;
					PIN7_bm;			// PA7 to wyjœcie prepulsu
	PORTA.PIN7CTRL = PORT_OPC_WIREDORPULL_gc |		//ustawienie stanu 1-zasilany VCC / 0-pulldown do masy
						PORT_ISC_INPUT_DISABLE_gc;	//uniemo¿liwienie odczytania wartoœci pinu w rejestrze PORTA.IN (zawsze 0) - zapobiegniêcie b³êdnemu odczytowi przycisków
	PORTA.PIN6CTRL = PORT_OPC_WIREDORPULL_gc |
						PORT_ISC_INPUT_DISABLE_gc;
	PORTA.OUTCLR = PIN6_bm | PIN7_bm;	//wy³¹czenie konwertera dc-dc i prepulsu
}

void stimulate(void)
{
	uint32_t dac_temp = (((uint32_t)current)*4095)/20000;//ew. ró¿ne obliczenia dla ró¿nych zakresów - byæ mo¿e szybsze, ale po co
	dac_data = (uint16_t)dac_temp; //wartoœæ ma maks. 12 bitów po powy¿szym mno¿eniu
	//todo: zmiana podejœcia - przycisk zmienia wartoœæ dac, która dopiero potem jest konwertowana na wartoœæ do wyœwietlenia
	
	
	/*****   prepuls   *****/
	PORTA.OUTSET = PIN6_bm | PIN7_bm;
	#ifndef  SIMULATOR
	_delay_ms(5); //prepuls
	#endif
	PORTA.OUTCLR = PIN7_bm; //wy³¹czenie prepulsu
	#ifndef  SIMULATOR
	_delay_ms(5);	//czekanie na stabilizacjê
	#endif
	
	//wyœwietlenie informacji o stymulacji
	oled_clear_screen();
	oled_print_hori_mode_string("-STYMULACJA-",1,1,30,127);
	ssd1306_set_display_zoom(1);
	
	stim_flag = 1;
	DACA.CTRLA |= DAC_ENABLE_bm;	//pierwszy impuls
	DACA.CH0DATA = dac_data;
	
	TCC4.CTRLA  = TC45_CLKSEL_DIV1_gc;
	TCC4.CNT = 0;
	TCC4.PER = (pulseT*2);			//d³ugoœæ impulsu
	TCC4.CTRLGCLR = TC4_STOP_bm; //uruchomienie timera
}

ISR(TCC4_OVF_vect)		
{
	/************************************************************************/
	/*			przerwanie przepe³nienia TCC4						   */
	/************************************************************************/
	switch (stim_flag)
	{
		//przerwa miêdzy impulsami
		case 1: //wartoœæ 1 i pocz¹tkowe przerwanie (1. impuls) ustawiane w funkcji stimulate()
		{
			DACA.CTRLA &= ~DAC_ENABLE_bm; //koniec pierwszego impulsu
			stim_flag = 2;			
			TCC4.PER = (deltaT*2); //przepe³nienie po ustawionym czasie przerwy
			break;
		}
		
		//drugi impuls
		case 2:
		{
			stim_flag = 3;
			DACA.CTRLA |= DAC_ENABLE_bm;
			DACA.CH0DATA = dac_data;	//start drugiego impulsu
			TCC4.PER = (pulseT*2); //przepe³nienie po ustawionym czasie impulsu
			break;
		}
		
		//wy³¹czenie drugiego impulsu (w³¹czany ponownie w funkcji stimulate()
		case 3: 
		{
			stim_flag = 4;
			DACA.CTRLA &= ~DAC_ENABLE_bm; //koniec drugiego impulsu, wy³¹czenie konwertera
			//PORTA.OUTCLR = PIN6_bm;		//wy³¹czenie konwertera dc-dc
			TCC4.CTRLA  = TC45_CLKSEL_DIV256_gc;	// ustawienie preskalera - nastêpuje uruchomienie timera
			TCC4.PER = 8000; //(1/2MHz)*256 =128us    128us*8000= 1024ms  -wyœwietlenie informacji o stymulacji przez sekundê
			break; 
		}
		
		case 4:
		{
			stim_flag = 0; // zabezpieczenie
			//przywracanie normalnego menu
			ssd1306_set_display_zoom(0);
			oled_clear_screen();
			print_menu();
			
			TCC4.CTRLA  = TC45_CLKSEL_DIV1_gc;	// ustawienie preskalera - nastêpuje uruchomienie timera
			TCC4.PER = 200; //(1/2MHz)*1 =0,5us    0,5us*200= 100us  -wy³¹czenie timera
			TCC4.CTRLGSET = TC4_STOP_bm; //wy³. przy najbli¿szym przepe³nieniu
			break;
		}
		
	}
	TCC4.INTFLAGS |= TC4_OVFIF_bm; //usuniêcie flagi przepe³nienia	
}

