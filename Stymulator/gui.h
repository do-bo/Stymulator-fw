#ifndef _MENU_H
#define _MENU_H

#include <avr\pgmspace.h>

struct PROGMEM _menuitem
{
	uint16_t *value;
	uint8_t menu_pos;
	const uint16_t range_lo;
	const uint16_t range_hi;
	const char *unit_lo;
	const char *unit_hi;
	const struct _menuitem *prev;
	const struct _menuitem *next;
};

extern struct _menuitem const PROGMEM menu_current;

void menu_select_next(void);
void menu_select_prev(void);
void modify_value(uint8_t sign);
void print_menu(void);




#endif
