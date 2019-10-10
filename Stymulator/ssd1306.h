/**
 * \file
 *
 * \brief SSD1306 OLED display controller driver.
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#define USE_8080
//#define USE_SPI
//#define BITBANG		//bitbang w SPI

#if !(defined(USE_SPI) || defined(USE_8080))
# error wybierz protokol komunikacji z ekranem (ssd1306.h)
#elif defined(USE_SPI) && defined(USE_8080)
# error wybierz SPI *albo* 8080 (ssd1306.h)
#endif

#include <avr/io.h>
#include <util/delay.h>

void protocol_setup(void);
void ssd1306_initialize(void);

void ssd1306_send_command (uint8_t u8Data);
void ssd1306_send_data (uint8_t);
uint8_t ssd1306_read_data(void);
uint8_t ssd1306_read_command(void);

void ssd1306_send_data_enable (void);
void ssd1306_send_data_disable (void);
void ssd1306_page_mode_enable(void);
void ssd1306_page_mode_set_page_address(uint8_t address);
void ssd1306_page_mode_set_column_address(uint8_t address);
void ssd1306_hori_mode_enable(void);
void ssd1306_hori_mode_set_col_range(uint8_t first_col, uint8_t last_col);
void ssd1306_hori_mode_set_page_range(uint8_t first_page, uint8_t last_page);
uint8_t ssd1306_set_contrast(uint8_t contrast);
void ssd1306_set_display_zoom(uint8_t on_1_off_0);
/*************Definicje portu i i pinów************/

//wspólne
#define OLED_CS  PIN4_bm		//SPI chip select (SS)
#define OLED_DC  PIN3_bm		//data/command
#define OLED_RES PIN2_bm		//reset - przy stanie niskim nastêpuje inicjalizacja uk³adu SSD1306

// SPI 4-wire //
#define OLED_PORT PORTC			//Komunikacja z wyœwietlaczem na porcie C

#define OLED_SCL PIN5_bm		//SPI SCK - clock
#define OLED_SDA PIN7_bm		//SPI MOSI - data

// 8080 //
#define OLED_DATA PORTD
#define OLED_CONTROL PORTC

#define OLED_WR PIN5_bm
#define OLED_RD PIN6_bm


/**************Definicje komend********************/
#define SSD1306_CMD_LOW_COL(column)             (0x00 | (column))
#define SSD1306_CMD_HIGH_COL(column)            (0x10 | (column))
#define SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE  0x20
#define SSD1306_CMD_PAGE_ADDRESSING_MODE		0x02
#define SSD1306_CMD_HORI_ADDRESSING_MODE		0x00
#define SSD1306_CMD_SET_HORI_COLUMN_ADDRESS		0x21
#define SSD1306_CMD_HORI_MODE_1ST_COL(col)		(col & 0x7F)
#define SSD1306_CMD_HORI_MODE_LAST_COL(col)		(col & 0x7F)
#define SSD1306_CMD_SET_HORI_PAGE_ADDRESS		0x22
#define SSD1306_CMD_HORI_MODE_1ST_PAGE(page)	(page & 0x07)
#define SSD1306_CMD_HORI_MODE_LAST_PAGE(page)	(page & 0x07)
#define SSD1306_CMD_START_LINE(line)            (0x40 | (line))
#define SSD1306_CMD_SET_CONTRAST				0x81
#define SSD1306_CMD_SET_CHARGE_PUMP_SETTING     0x8D
#define SSD1306_CMD_SEGMENT_RE_MAP_COL0_SEG0    0xA0
#define SSD1306_CMD_SEGMENT_RE_MAP_COL127_SEG0  0xA1
#define SSD1306_CMD_DISPLAY_GDDRAM_ON			0xA4
#define SSD1306_CMD_DISPLAY_ENTIRE_ON           0xA5
#define SSD1306_CMD_NORMAL_DISPLAY              0xA6
#define SSD1306_CMD_INVERSE_DISPLAY             0xA7
#define SSD1306_CMD_SET_MULTIPLEX_RATIO         0xA8
#define SSD1306_CMD_DISPLAY_ON                  0xAF
#define SSD1306_CMD_DISPLAY_OFF                 0xAE
#define SSD1306_CMD_SET_FADE_OR_BLINK			0x23
#define SSD1306_CMD_FADE_OUT_ON					0x20
#define SSD1306_CMD_BLINK_ON(interval)			(interval & 0x3F)
#define SSD1306_CMD_FADE_BLINK_OFF				0x00
#define SSD1306_CMD_PAGE_START_ADDRESS(page)    (0xB0 | (page & 0x07))
#define SSD1306_CMD_COM_OUTPUT_SCAN_UP          0xC0
#define SSD1306_CMD_COM_OUTPUT_SCAN_DOWN        0xC8
#define SSD1306_CMD_DISPLAY_OFFSET              0xD3
#define SSD1306_CMD_DISPLAY_CLOCK_DIVIDE_RATIO  0xD5
#define SSD1306_CMD_PRE_CHARGE_PERIOD           0xD9
#define SSD1306_CMD_COM_PINS                    0xDA
#define SSD1306_CMD_VCOMH_DESELECT_LEVEL        0xDB
#define SSD1306_CMD_NOP                         0xE3
#define SSD1306_SET_ZOOM						0xD6
#define SSD1306_CMD_ZOOM_ON						0x01
#define SSD1306_CMD_ZOOM_OFF					0x00



//ssd1306_send_command(0x23);//set fade
//ssd1306_send_command(0x31);//blink, 16frames


/**********Komendy animacji ekranu*************/
#define SSD1306_CMD_SCROLL_H_RIGHT                  0x26
#define SSD1306_CMD_SCROLL_H_LEFT                   0x27
#define SSD1306_CMD_CONTINUOUS_SCROLL_V_AND_H_RIGHT 0x29
#define SSD1306_CMD_CONTINUOUS_SCROLL_V_AND_H_LEFT  0x2A
#define SSD1306_CMD_DEACTIVATE_SCROLL               0x2E
#define SSD1306_CMD_ACTIVATE_SCROLL                 0x2F
#define SSD1306_CMD_SET_VERTICAL_SCROLL_AREA        0xA3


#define SSD1306_LATENCY 10 //us



/************************************************************************/
/*                       adafruit                                               */
/************************************************************************/

/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/


#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

