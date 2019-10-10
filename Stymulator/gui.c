#include <stddef.h>
#include <stdlib.h>
#include <avr\pgmspace.h>
//#include <math.h>

#include "gui.h"
#include "ssd1306.h"
#include "oled.h"

#define menu_get_element(ptr, field) (void*)pgm_read_word(((uint8_t*)ptr)+offsetof(struct _menuitem, field)) //pobiera element menu

static const struct _menuitem *active_menu_ptr=&menu_current;
uint8_t multiplier_counter=3;


static uint16_t power_10(uint8_t pow)
{
	if (pow == 0)
		return 1;	
	else if(5>pow && pow>0)
	{
		uint16_t output=1;
		while(pow>0)
		{
			output *= 10;
			pow--;
		}
		return output;
	}
	else return 0xEEEE; //przekroczony zakres
}

void print_arrows (uint8_t up_1_dn_2)
{
	for (uint8_t i=4; i>0; i--)
	{
		if (i == (multiplier_counter+1)) oled_print_put_symbol(up_1_dn_2); //strza�ka g�ra/d�
		else oled_print_put_symbol(0); // puste pole
	}
}

void update_arrows(uint8_t position)
{
	oled_hori_mode_goto_xy(position,3);
	print_arrows(1);
	oled_hori_mode_goto_xy(position,5);
	print_arrows(2);
}

void clear_arrows(uint8_t position)
{
	oled_hori_mode_goto_xy(position,3);
	for (uint8_t i=5; i>0; i--) oled_print_put_symbol(0);
	oled_hori_mode_goto_xy(position,5);
	for (uint8_t i=5; i>0; i--) oled_print_put_symbol(0);
}



void print_value(void)
{
	char str[4]="000";
	uint16_t displayed_val;
	char *unit;
	
	uint16_t stored_val = *(uint16_t*)(menu_get_element(active_menu_ptr, value));
	uint8_t position = (uint8_t)(menu_get_element(active_menu_ptr, menu_pos));
	
	if (active_menu_ptr == &menu_current)  //wy�wietlenie prawdziwej warto�ci pr�du
	{
		uint32_t true_val = (uint32_t)stored_val;
		true_val = ((true_val * 4095) /20000);  //konwersja do prawdziwej warto�ci dac (*4096/20000)
		true_val = ((true_val * 20000) /4095);  //prawdziwy pr�d z dac
		stored_val = (uint16_t)true_val;
	}
	
	if(stored_val >999) 
	{
		displayed_val = (stored_val/1000);
		unit = menu_get_element(active_menu_ptr, unit_hi); //unit jest tablic� - do adresu pierwszego elem. przypisany jest adres ze struktury 
		
	}
	else 
	{
		displayed_val = stored_val;
		unit = menu_get_element(active_menu_ptr, unit_lo);
	}
	
	//zapewnienie sta�ego po�o�enia cyfr liczby
	uint8_t digit_offset;
	static uint8_t offset_prev;
	if (displayed_val/100) digit_offset = 0;
	else if(displayed_val/10) digit_offset = 1;
		 else digit_offset = 2;
	if(digit_offset != offset_prev)
	{ 
		for (uint8_t i=digit_offset; i>0; i--)	//wyczyszczenie cyfr wy�szych rz�d�w przy przechodzeniu do ni�szych warto�ci
		{
			oled_hori_mode_goto_xy(position+((i-1)*6), 4); 
			oled_print_put_symbol(4); 
		}
		offset_prev = digit_offset;
	}
	
	//wy�wietlenie warto�ci
	utoa(displayed_val, str, 10);
	oled_hori_mode_goto_xy((position+(digit_offset*6)),4); 
	oled_print_text(str);
	oled_hori_mode_goto_xy((position+19),4); //19=3*6 + 1px
	oled_print_text_P(unit);
}

void modify_value(uint8_t add_1_subtr_0)
{
	uint16_t *modified_val = (uint16_t*)(menu_get_element(active_menu_ptr, value));
	uint16_t mod_val_temp = *modified_val;
	uint16_t multiplier = multiplier_counter;
	uint16_t val_max = (uint16_t)(menu_get_element(active_menu_ptr, range_hi));
	uint16_t val_min = (uint16_t)(menu_get_element(active_menu_ptr, range_lo));
	
	if (multiplier != 0) //je�li nie jednostki
	{
		if (*modified_val > 999) multiplier += 3; 
		
		if (add_1_subtr_0)//dodawanie
		{
			*modified_val += power_10(multiplier-1); // liczba mo�e przekroczy� zakres uint16 przy dodawaniu setek mA (setek tysi�cy uA)
			if ((*modified_val > val_max) || (multiplier == 6 /*3+3*/)) //sprawdzenie - aby liczba nie przekroczy�a warto�ci max i zakresu uint16
				*modified_val = val_max;
			
			if(((mod_val_temp + power_10(multiplier-1))>999) && (mod_val_temp <= 999)) //przej�cie do jedno�ci po zwi�kszeniu zakresu
			{
				multiplier_counter = 1;
				update_arrows((uint8_t)(menu_get_element(active_menu_ptr, menu_pos)));
			}
		}
		else //odejmowanie
		{
			if (*modified_val / power_10(multiplier-1)) //sprawdzenie - aby liczba nie osi�gn�a wart. ujemnej (du�ej dodatniej)
				*modified_val -= power_10(multiplier-1);
			else	*modified_val = val_min;
			
			if (((mod_val_temp - power_10(multiplier-1))<1000) && (mod_val_temp >=1000)) // gdy zmienia si� zakres, wymu� warto�� 999uA
			{
				multiplier_counter = 2; //przej�cie do setek po zmniejszeniu zakresu
				update_arrows((uint8_t)(menu_get_element(active_menu_ptr, menu_pos)));
				*modified_val = 999;
			}
		}
	}
	else // multiplier 0 -  jednostki
	{
		if (add_1_subtr_0) //przej�cie z mikro do mili przy zachowaniu bezmianowej warto�ci parametru
		{
			if (*modified_val > 65) *modified_val = val_max; //65 -> 65000 - zapobiegni�cie przepe�nieniu zmiennnej uint16
			else 
			{
				*modified_val *= 1000;
				if (*modified_val > val_max) *modified_val = val_max;
			}
			
		}
		else	//przej�cie z mili do mikro
		{
			*modified_val /= 1000;
			if (*modified_val < val_min) *modified_val = val_min;
		}
		menu_select_prev(); // przej�cie do jedno�ci
	}

	print_value();		
}


		

void menu_select_next()
{	
	volatile uint8_t position = (uint8_t)(menu_get_element(active_menu_ptr, menu_pos)); //adres jako wska�nik na menu_pos, position wskazuje na t� warto��
	if (multiplier_counter == 0)
	{
		if (menu_get_element(active_menu_ptr, next) != 0)	//przeskok do nast�pnego parametru		
		{
			//czyszczenie strza�ek dotychczasowego elem.			
			clear_arrows(position);
						
			//uaktywnienie kolejnego elem. i narysowanie strza�ek
			active_menu_ptr = menu_get_element(active_menu_ptr, next);
			multiplier_counter = 3;
			uint8_t position = (uint8_t)(menu_get_element(active_menu_ptr, menu_pos));
			update_arrows(position);
		}
	}
	else
	{
		multiplier_counter --;
		update_arrows(position);
	}
}

void menu_select_prev()
{	
	volatile uint8_t position = (uint8_t)(menu_get_element(active_menu_ptr, menu_pos));
	if (multiplier_counter == 3)
	{
		if (menu_get_element(active_menu_ptr, prev) != 0) //przeskok do poprzedniego parametru
		{
			//czyszczenie strza�ek dotychczasowego elem.
			clear_arrows(position);
			
			//uaktywnienie kolejnego elem. i narysowanie strza�ek
			active_menu_ptr = menu_get_element(active_menu_ptr, prev);
			multiplier_counter = 1;
			uint8_t position = (uint8_t)(menu_get_element(active_menu_ptr, menu_pos));
			update_arrows(position);
		}
	}
	else
	{
		multiplier_counter ++;
		update_arrows(position);
	}
	
}



//wst�pne narysowanie menu
void print_menu(void)
{
	multiplier_counter = 0;
	
	
	for (uint8_t i=8; i>0; i--) 
	{
		print_value();
		menu_select_next();
		
	}
	for (uint8_t i=9; i>0; i--)
		menu_select_prev();
}

