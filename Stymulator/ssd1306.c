#include "ssd1306.h"
#include "font.h"
#include <util/delay.h>


static inline void ssd1306_display_off(void);
static inline void ssd1306_display_on(void);
static inline void ssd1306_hard_reset(void);
static inline void ssd1306_invert_disable(void);



/* Inicjalizacja sprzêtowego SPI master na porcie C */
void protocol_setup(void)
{
	#ifdef USE_SPI
	//SPI
	OLED_PORT.DIRSET = OLED_SCL | OLED_CS | OLED_SDA | OLED_RES | OLED_DC;
	PORTCFG.MPCMASK = OLED_SCL | OLED_CS | OLED_SDA | OLED_RES | OLED_DC; //zapisanie konfiguracyjnej maski pinów
	PORTC.PIN2CTRL = PORT_OPC_TOTEM_gc;
	
	#ifndef BITBANG
	SPIC.CTRL =	SPI_ENABLE_bm |			//W³¹czenie interfejsu spi
	SPI_MASTER_bm |			//uC pracuje w trybie master
	//SPI_DORD_bm   |			//data order bit =1 - LSB wychodzi z rejestru pierwszy
	SPI_MODE_3_gc |			//transfer mode 0
	SPI_PRESCALER_DIV16_gc;

	SPIC.INTCTRL = SPI_INTLVL_OFF_gc ;	//wy³¹czenie przerwañ
	/*
	*SPIC.INTCTRL = SPI_TXCIE_bm |		//w³¹czenie przerwania koñca transmisji
	SPI_INTLVL_MED_gc;	//œrednia wa¿noœæ przerwania
	*/
	
	SPIC.CTRLB = SPI_BUFMODE_BUFMODE2_gc |	//drugi tryb pracy bufora - bez zapisu dummy byte po uruchomieniu SPI
				 SPI_SSD_bm;				//wy³¹czenie wejœcia Slave Select - mo¿liwe u¿ycie pinu w innym celu
	#endif // BITBANG

	#else
	// 8080
	OLED_CONTROL.DIRSET = OLED_CS | OLED_RES | OLED_DC | OLED_WR | OLED_RD;
	PORTCFG.MPCMASK = OLED_CS | OLED_RES | OLED_DC | OLED_WR | OLED_RD; //zapisanie konfiguracyjnej maski pinów
	PORTC.PIN2CTRL = PORT_OPC_TOTEM_gc;
	
	OLED_DATA.DIR = 0xFF; // wyjœcie na porcie D
	PORTCFG.MPCMASK = 0xFF; //wybrane wszystkie piny
	PORTD.PIN0CTRL = PORT_OPC_PULLDOWN_gc; //totempole + pulldown na wejœciu
	OLED_CONTROL.OUTSET = OLED_WR |OLED_RD;
	#endif // USE_SPI
	
	//wspólne
	OLED_PORT.OUTSET = //OLED_CS |	// deaktywacja komunikacji
						OLED_RES;	// res = 0 powoduje inicjalizacjê kontrolera
	OLED_PORT.OUTCLR = OLED_CS;
} 



/************************************************************************/
/* Podstawowe funkcje do wysy³ania bajtów danych i komend                      */
/************************************************************************/



/************** SPI 4-wire ***********************/
#ifdef USE_SPI

inline void ssd1306_send_data_enable (void){
	OLED_PORT.OUTCLR = OLED_CS;
	OLED_PORT.OUTSET = OLED_DC;				// kontroler w trybie wpisywania danych
}

inline void ssd1306_send_data_disable (void){
	//OLED_PORT.OUTSET = OLED_CS;
}


static inline void ssd1306_send_command_enable (void){
	OLED_PORT.OUTCLR = OLED_CS | OLED_DC;	// kontroler w trybie wpisywania komendy
}

static inline void ssd1306_send_command_disable (void){
	OLED_PORT.OUTSET = //OLED_CS | 
						OLED_DC;	
}


void ssd1306_send_command (uint8_t u8Data) {
	ssd1306_send_command_enable();
	#ifndef BITBANG //sprzêtowe SPI
	SPIC.DATA = u8Data;
	while (! (SPIC.STATUS & SPI_DREIF_bm)); //zapobiegniêcie nadpisywaniu danych - DREIF = 1, gdy bufor jest pusty
	
	#else // SPI bitbang
	OLED_PORT.OUTCLR = OLED_SCL;
	for (int i=8; i>0; i--)
	{		
		if(u8Data & 0x80) OLED_PORT.OUTSET = OLED_SDA; //MSB wychodzi pierwszy
			else OLED_PORT.OUTCLR = OLED_SDA;
		OLED_PORT.OUTSET = OLED_SCL;	// bit zatrzaskiwany na rosn¹cym zboczu sygna³u zegara
		OLED_PORT.OUTCLR = OLED_SCL;
		u8Data << 1;
	}
	#endif //BITBANG
	ssd1306_send_command_disable();
}

void ssd1306_send_data (uint8_t u8Data) {
	ssd1306_send_data_enable();
	#ifndef BITBANG // sprzêtowe SPI	
	SPIC.DATA = u8Data;
	while (! (SPIC.STATUS & SPI_DREIF_bm)); //zapobiegniêcie nadpisywaniu danych
		
	#else //SPI bitbang
	OLED_PORT.OUTCLR = OLED_SCL;
	for (int i=8; i>0; i--)
	{
		if(u8Data & 0x80) OLED_PORT.OUTSET = OLED_SDA; //MSB wychodzi pierwszy
		else OLED_PORT.OUTCLR = OLED_SDA;
		OLED_PORT.OUTSET = OLED_SCL;	// bit zatrzaskiwany na rosn¹cym zboczu sygna³u zegara
		OLED_PORT.OUTCLR = OLED_SCL;
		u8Data << 1;
	}
	#endif //BITBANG
	ssd1306_send_data_disable();
}
#endif //USE_SPI

/*************** 8080 ************************/
#ifdef USE_8080

inline void ssd1306_send_data_enable(void){
	OLED_CONTROL.OUTCLR = OLED_CS;
	OLED_CONTROL.OUTSET = OLED_DC;				// kontroler w trybie czytania/wpisywania danych
}

inline void ssd1306_send_data_disable(void){
	OLED_PORT.OUTSET = OLED_CS;
}


static inline void ssd1306_send_command_enable(void){
	OLED_PORT.OUTCLR = OLED_CS | OLED_DC;	// kontroler w trybie wpisywania komendy
}


static inline void ssd1306_send_command_disable(void){
	OLED_PORT.OUTSET = OLED_CS |
						OLED_DC;
}


void ssd1306_send_command(uint8_t u8Data) {
	ssd1306_send_command_enable();
	OLED_CONTROL.OUTCLR = OLED_WR;
	//OLED_DATA.DIR = 0xFF; //wyjœcie na porcie
	OLED_DATA.OUT = u8Data;	
	OLED_CONTROL.OUTSET = OLED_WR;
	ssd1306_send_command_disable();
}

/* //nieu¿ywane - za ma³e mo¿liwoœci ekranu
uint8_t ssd1306_read_command(void) {
	ssd1306_send_command_enable();
	OLED_CONTROL.OUTCLR = OLED_RD;
	OLED_DATA.OUT = 0;	
	OLED_DATA.DIR = 0;  //wejœcie na porcie
	OLED_CONTROL.OUTSET = OLED_RD;
	received_data = OLED_DATA.IN;
	ssd1306_send_command_disable();
	return received_data;
}
*/

void ssd1306_send_data(uint8_t u8Data) {
	ssd1306_send_data_enable();
	OLED_CONTROL.OUTCLR = OLED_WR;
	//OLED_DATA.DIR = 0xFF; //wyjœcie na porcie
	OLED_DATA.OUT = u8Data;
	OLED_CONTROL.OUTSET = OLED_WR;
	ssd1306_send_data_disable();
}

/* //nieu¿ywane - za ma³e mo¿liwoœci ekranu
uint8_t ssd1306_read_data(void) {
	ssd1306_send_data_enable();
	OLED_CONTROL.OUTCLR = OLED_RD;
	OLED_DATA.OUT = 0;
	OLED_DATA.DIR = 0;  //wejœcie na porcie
	OLED_CONTROL.OUTSET = OLED_RD;
	received_data = OLED_DATA.IN;
	ssd1306_send_data_disable();
	return received_data;
}
*/
#endif //USE_8080

/************************************************************************/
/* Inicjalizacja kontrolera modu³u OLED                                                    */
/************************************************************************/
void ssd1306_initialize(void)
{
/*
	ssd1306_send_command(0xAE);   //display off
	ssd1306_send_command(0x20);   //Set Memory Addressing Mode<00FF>
	ssd1306_send_command(0x10);   //Addressing Modee
	ssd1306_send_command(0xb0);   //Set Page Start Address for Page Addressing Mode,0-7
	ssd1306_send_command(0xc8);   //Set COM Output Scan Direction
	ssd1306_send_command(0x00);   //---set low column address
	ssd1306_send_command(0x10);   //---set high column address
	ssd1306_send_command(0x40);   //--set start line address
	ssd1306_send_command(0x81);   //--set contrast control register
	ssd1306_send_command(0xaf);
	ssd1306_send_command(0xa1);   //--set segment re-map 0 to 127
	ssd1306_send_command(0xa6);   //--set normal display
	ssd1306_send_command(0xa8);   //--set multiplex ratio(1 to 64)
	ssd1306_send_command(0x3F);
	ssd1306_send_command(0xa4);   //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	ssd1306_send_command(0xd3);   //-set display offset
	ssd1306_send_command(0x00);   //-not offset
	ssd1306_send_command(0xd5);   //--set display clock divide ratio/oscillator frequency
	ssd1306_send_command(0xf0);   //--set divide ratio
	ssd1306_send_command(0xd9);   //--set pre-charge period
	ssd1306_send_command(0x22);
	ssd1306_send_command(0xda);   //--set com pins hardware configuration
	ssd1306_send_command(0x12);
	ssd1306_send_command(0xdb);   //--set vcomh
	ssd1306_send_command(0x20);   //0x20,0.77xVcc
	ssd1306_send_command(0x8d);   //--set DC-DC enable
	ssd1306_send_command(0x14);
	ssd1306_send_command(0xaf);   //--turn on oled panel
	
*/

	// Do a hard reset of the OLED display controller
	ssd1306_hard_reset();
	_delay_us(10);

	ssd1306_send_command(0xAE);   //display off

	ssd1306_send_command(0x81);//--set contrast control register
	ssd1306_send_command(0xaf);
	
/*	ssd1306_send_command(0xa0);//--no segment re-map
	ssd1306_send_command(0xc0);//no com remap
*/	
	ssd1306_send_command(0xa1);//--set segment re-map 0 to 127
	ssd1306_send_command(0xc8);//com remap
	
	ssd1306_send_command(0xa6);//--set normal display
	
	ssd1306_send_command(0xa8);//--set multiplex ratio(15 to 63)
	ssd1306_send_command(0x3F);//63 (mux64)

	ssd1306_send_command(0xd3);//-set display offset
	ssd1306_send_command(0x00);//-no offset
	
	ssd1306_send_command(0xd5);//--set display clock divide ratio/oscillator frequency
	ssd1306_send_command(0xf0);//--set divide ratio
	
	ssd1306_send_command(0xd9);//--set pre-charge period
	ssd1306_send_command(0x22); //
	
	ssd1306_send_command(0xda);//--set com pins hardware configuration
	ssd1306_send_command(0x12);
	
	ssd1306_send_command(0xdb);//--set vcomh
	ssd1306_send_command(0x20);//0x20,0.77xVcc
	
	ssd1306_send_command(0x8d);//--set DC-DC enable
	ssd1306_send_command(0x10);//set Charge Pump disable
	ssd1306_send_command(0xaf);//--turn on oled panel
	
	ssd1306_send_command(SSD1306_CMD_SET_CONTRAST);
	ssd1306_send_command(100);
	
}


//funkcja sprzêtowego resetu
static inline void ssd1306_hard_reset(void)
{
	OLED_PORT.OUTCLR = OLED_RES;
	_delay_us(SSD1306_LATENCY); // przynajmniej 3us
	OLED_PORT.OUTSET = OLED_RES;
	_delay_us(SSD1306_LATENCY);
}



/*********Kontrola stanu uœpienia*******************/

static inline void ssd1306_display_off(void)
{
	ssd1306_send_command(SSD1306_CMD_DISPLAY_OFF);
}


static inline void ssd1306_display_on(void)
{
	ssd1306_send_command(SSD1306_CMD_DISPLAY_ON);
}



//*********kontrola parametrów wyœwietlania******************

static inline void ssd1306_set_display_start_line_address(uint8_t address) //Ustawienie adresu pocz¹tkowej linii wyœwietlacza
{
	address &= 0x3F;			//adres musi byæ 6 - bitowy
	ssd1306_send_command(SSD1306_CMD_START_LINE(address));
}


uint8_t ssd1306_set_contrast(uint8_t contrast)  //Kontrast - wartoœæ z przedzia³u 0 and 0xFF
{
	ssd1306_send_command(SSD1306_CMD_SET_CONTRAST);
	ssd1306_send_command(contrast);
	return contrast;
}


inline void ssd1306_set_display_invert(uint8_t on_1_off_0)
{
	if (on_1_off_0)	ssd1306_send_command(SSD1306_CMD_INVERSE_DISPLAY);
	else		ssd1306_send_command(SSD1306_CMD_NORMAL_DISPLAY);
}


inline void ssd1306_set_display_zoom(uint8_t on_1_off_0)
{
	ssd1306_send_command(SSD1306_SET_ZOOM);
	if (on_1_off_0)	ssd1306_send_command(SSD1306_CMD_ZOOM_ON);
	else		ssd1306_send_command(SSD1306_CMD_ZOOM_OFF);
}






/************************************************************************/
/* Ustawienie po³o¿enia kursora w trybie adresowania stronami                   */
/************************************************************************/

inline void ssd1306_page_mode_enable(void) 
{
	ssd1306_send_command(SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE);
	ssd1306_send_command(SSD1306_CMD_PAGE_ADDRESSING_MODE);
}

/*************Ustawienie adresowanej strony pamiêci RAM kontrolera************/
inline void ssd1306_page_mode_set_page_address(uint8_t address)
{
	address &= 0x0F;		//adres musi byæ 4-bitowy (tylko 8 stron pamiêci)
	ssd1306_send_command(SSD1306_CMD_PAGE_START_ADDRESS(address));
}


/*************Ustawienie adresowanej kolumny pamiêci RAM kontrolera************/
inline void ssd1306_page_mode_set_column_address(uint8_t address)
{
	address &= 0x7F;			// adres musi byæ 7 - bitowy (128 kolumn)
	ssd1306_send_command(SSD1306_CMD_HIGH_COL(address >> 4));	//wy³uskanie i wys³anie starszego pó³bajtu adresu
	ssd1306_send_command(SSD1306_CMD_LOW_COL(address & 0x0F));	//wy³uskanie i wys³anie m³odszego pó³bajtu adresu
}




/************************************************************************/
/* Ustawienie po³o¿enia kursora w trybie adresowania horyzontalnego           */
/************************************************************************/

inline void ssd1306_hori_mode_enable(void) 
{
	ssd1306_send_command(SSD1306_CMD_SET_MEMORY_ADDRESSING_MODE);
	ssd1306_send_command(SSD1306_CMD_HORI_ADDRESSING_MODE);
}

/*************Ustawienie zakresu adresów stron pamiêci RAM kontrolera************/
inline void ssd1306_hori_mode_set_page_range(uint8_t first_page, uint8_t last_page)
{
	ssd1306_send_command(SSD1306_CMD_SET_HORI_PAGE_ADDRESS);
	ssd1306_send_command(SSD1306_CMD_HORI_MODE_1ST_PAGE(first_page));
	ssd1306_send_command(SSD1306_CMD_HORI_MODE_LAST_PAGE(last_page));
}


/*************Ustawienie zakresu adresów kolumn pamiêci RAM kontrolera************/
inline void ssd1306_hori_mode_set_col_range(uint8_t first_col, uint8_t last_col)
{
	ssd1306_send_command(SSD1306_CMD_SET_HORI_COLUMN_ADDRESS);
	ssd1306_send_command(SSD1306_CMD_HORI_MODE_1ST_COL(first_col));
	ssd1306_send_command(SSD1306_CMD_HORI_MODE_LAST_COL(last_col));
}

