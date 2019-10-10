/*
 * gui.h
 *
 * Created: 30-04-2014 21:09:23
 *  Author: Dobo
 */ 


#ifndef GUI_H_
#define GUI_H_

#define ARRAY_NO_OF_ELEMS(x) (sizeof(x) / sizeof(x[0]))



void ssd1306_print_hori_mode_symbols(uint8_t *table, uint8_t page_first, uint8_t page_last, uint8_t col_first, uint8_t col_last);
void oled_print_hori_mode_string(char *string, uint8_t page_first, uint8_t page_last,
											uint8_t col_first, uint8_t col_last);
void oled_hori_mode_goto_xy(uint8_t x, uint8_t y);
void oled_print_put_symbol(uint8_t number);
void oled_print_text(char *string);
void oled_print_text_P(const char *string);
void oled_clear_area(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);
void oled_clear_screen(void);
void oled_print_put_byte(uint8_t byte, uint8_t repeat);


#endif /* GUI_H_ */