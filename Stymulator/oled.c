/*
 * gui.c
 *
 * Created: 23-04-2014 21:05:13
 *  Author: Dobo
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "oled.h"
#include "ssd1306.h"
#include "font.h"


/***************************************
 *
 *Funkcje drukowania
 *
 ***************************************/


//funkcja pomocnicza
inline static void send_data(uint8_t data)
{
	#if (defined(USE_SPI) && !(defined(BITBANG)))
	SPIC.DATA = (data);
	#else
	ssd1306_send_data(data);
	#endif
}



/************************************************************************/
/* Drukowanie ci¹gu znaków                                                                    */
/************************************************************************/

void oled_print_text(char *string)
{
	uint8_t *char_ptr;
	uint8_t i;

	ssd1306_send_data_enable();
	
	while (*string != 0)
	{
		if (*string < 0x7F) //upewnienie siê, ¿e to znak
		{
			char_ptr = font_table[*string - 32];
			for (i = 1; i <= char_ptr[0]; i++)
			{
				ssd1306_send_data(char_ptr[i]);
			}
			ssd1306_send_data(0x00);
		}
		string++;
	}
	ssd1306_send_data_disable();
}

void oled_print_text_P(const char *string)
{
	uint8_t *char_ptr;
	char ch;
	uint8_t i;

	ssd1306_send_data_enable();
	
	while (ch=pgm_read_byte(string))
	{
		if (*string < 0x7F) //upewnienie siê, ¿e to znak
		{
			char_ptr = font_table[ch - 32];
			for (i = 1; i <= char_ptr[0]; i++)
			{
				ssd1306_send_data(char_ptr[i]);
			}
			ssd1306_send_data(0x00); //przerwa 1px
		}
		string++;
	}
	ssd1306_send_data_disable();
}

void oled_hori_mode_goto_xy(uint8_t x, uint8_t y)
{
	ssd1306_hori_mode_enable();
	ssd1306_hori_mode_set_col_range(x, 127);
	ssd1306_hori_mode_set_page_range(y, 7);
}

void oled_hori_mode_set_range(uint8_t x_1st, uint8_t x_last, 
								uint8_t y_1st, uint8_t y_last)
{
	ssd1306_hori_mode_enable();
	ssd1306_hori_mode_set_col_range(x_1st, x_last);
	ssd1306_hori_mode_set_page_range(y_1st, y_last);
}


void oled_print_put_symbol(uint8_t number) 
{
	ssd1306_send_data_enable();
	
	uint8_t *symbol_ptr;
	uint8_t i;
	
	symbol_ptr = special_table[number];
	for (i = 1; i <= symbol_ptr[0]; i++)
	{
		ssd1306_send_data(symbol_ptr[i]);
	}
	ssd1306_send_data(0x00); //przerwa 1px
			
	ssd1306_send_data_disable();
}

void oled_print_put_char(char one_char)
{
	ssd1306_send_data_enable();
	
	uint8_t *char_ptr;
	uint8_t i;
	
	char_ptr = font_table[one_char - 32];
	for (i = 1; i <= char_ptr[0]; i++)
	{
		ssd1306_send_data(char_ptr[i]);
	}
	ssd1306_send_data(0x00);
	
	ssd1306_send_data_disable();
}

void oled_print_put_byte(uint8_t byte, uint8_t repeat)
{
	ssd1306_send_data_enable();
	while (repeat > 0)
	{
		ssd1306_send_data(byte);
		repeat--;
	}
	ssd1306_send_data_disable();
}

//funkcja przyjmuje jako argument wskaŸnik na tablicê zawieraj¹c¹ numery kolejnych symboli specjalnych do wydrukowania
void oled_print_hori_mode_symbols(uint8_t *table, uint8_t page_first, uint8_t page_last,
											uint8_t col_first, uint8_t col_last)
{
	ssd1306_hori_mode_enable();
	ssd1306_hori_mode_set_col_range(col_first, col_last);
	ssd1306_hori_mode_set_page_range(page_first, page_last);	
	
	ssd1306_send_data_enable();
	
	uint8_t *symbol_ptr;
	uint8_t i, counter=0;
	
	while (counter < ARRAY_NO_OF_ELEMS(table))
	{
		if (*table < 0xFF) //upewnienie siê, ¿e to bajt
		{
			symbol_ptr = special_table[*table];
			for (i = 1; i <= symbol_ptr[0]; i++)
			{
				ssd1306_send_data(symbol_ptr[i]);
			}
			ssd1306_send_data(0x00);
		}
		table++;
	}
	ssd1306_send_data_disable();
}

void oled_print_hori_mode_string(char *string, uint8_t page_first, uint8_t page_last,
uint8_t col_first, uint8_t col_last)
{
	ssd1306_hori_mode_enable();
	ssd1306_hori_mode_set_col_range(col_first, col_last);
	ssd1306_hori_mode_set_page_range(page_first, page_last);
	
	
	uint8_t *char_ptr;
	uint8_t i;
	
	ssd1306_send_data_enable();
	
	while (*string != 0)
	{
		if (*string < 0x7F) //upewnienie siê, ¿e to znak
		{
			char_ptr = font_table[*string - 32];
			for (i = 1; i <= char_ptr[0]; i++)
			{
				ssd1306_send_data(char_ptr[i]);
			}
			ssd1306_send_data(0x00);
		}
		string++;
	}
	ssd1306_send_data_disable();
}


// czyszczenie ekranu

void oled_clear_area(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end)
{
	oled_hori_mode_set_range(x_start,x_end,y_start,y_end);
	uint16_t repeat = ((x_end-x_start+1)*(y_end-y_start+1));
	
	ssd1306_send_data_enable();
	while (repeat+1 > 0)
	{
		
		ssd1306_send_data(0x00);
		repeat--;
	}
	ssd1306_send_data_disable();
	
}


void oled_clear_screen(void)
{
	oled_clear_area(0,127,0,7);
}

