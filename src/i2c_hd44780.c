#include "driver/i2c_master.h"
#include "driver/i2c_hd44780.h"

//Part of this code comes from http://hmario.home.xs4all.nl/arduino/LiquidCrystal_I2C/
//And from https://github.com/vanluynm/LiquidCrystal_I2C/

/*
I2C Expander map
Func   Bit 
RS     0
RW     1
Enable 2
BL     3
D4     4
D5     5
D6     6
*/

uint8 _displayfunction;
uint8 _displaycontrol;
uint8 _displaymode;
uint8 _backlightval;
uint8 _numlines;


/************ low level data pushing commands **********/
void LCD_expanderWrite(uint8 _data){                                        
    i2c_master_start();
    i2c_master_writeByte(LCD_ADDRESS << 1);
    if (!i2c_master_checkAck()) 
    {
        i2c_master_stop();
        return;
    }
    //i2c_master_writeByte((uint8)(_data) | _backlightval);
    i2c_master_writeByte((uint8)(_data) | LCD_BACKLIGHT);
    i2c_master_checkAck();
    i2c_master_stop();
}

void LCD_pulseEnable(uint8 _data){
    LCD_expanderWrite(_data | En);  // En high
    os_delay_us(1);     // enable pulse must be >450ns
    
    LCD_expanderWrite(_data & ~En); // En low
    os_delay_us(50);        // commands need > 37us to settle
}

void LCD_write4bits(uint8 value) {
    LCD_expanderWrite(value);
    LCD_pulseEnable(value);
}

// write either command or data
void LCD_send(uint8 value, uint8 mode) {
    uint8 highnib = value & 0xF0;
    uint8 lownib = value << 4;
    LCD_write4bits((highnib)|mode);
    LCD_write4bits((lownib)|mode);
}

/*********** mid level commands, for sending data/cmds */

void LCD_command(uint8 value) {
    LCD_send(value, 0);
}

uint8 LCD_write(uint8 value) {
    LCD_send(value, Rs);
    return 0;
}

/********** high level commands, for the user! */
void LCD_clear(){
    LCD_command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
    os_delay_us(2000);  // this command takes a long time!
}

void LCD_home(){
    LCD_command(LCD_RETURNHOME);  // set cursor position to zero
    os_delay_us(2000);  // this command takes a long time!
}

void LCD_setCursor(uint8 col, uint8 row){
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    _numlines = LCD_ROW;
    if ( row > _numlines ) {
        row = _numlines-1;    // we count rows starting w/0
    }
    LCD_command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LCD_noDisplay() {
    _displaycontrol &= ~LCD_DISPLAYON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_display() {
    _displaycontrol |= LCD_DISPLAYON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCD_noCursor() {
    _displaycontrol &= ~LCD_CURSORON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_cursor() {
    _displaycontrol |= LCD_CURSORON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LCD_noBlink() {
    _displaycontrol &= ~LCD_BLINKON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_blink() {
    _displaycontrol |= LCD_BLINKON;
    LCD_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LCD_scrollDisplayLeft(void) {
    LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LCD_scrollDisplayRight(void) {
    LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LCD_leftToRight(void) {
    _displaymode |= LCD_ENTRYLEFT;
    LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LCD_rightToLeft(void) {
    _displaymode &= ~LCD_ENTRYLEFT;
    LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LCD_autoscroll(void) {
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
    LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LCD_noAutoscroll(void) {
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
    LCD_command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LCD_createChar(uint8 location, uint8 charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    LCD_command(LCD_SETCGRAMADDR | (location << 3));
    int i;
    for (i=0; i<8; i++) {
        LCD_write(charmap[i]);
    }
}

// Turn the (optional) backlight off/on
void LCD_noBacklight(void) {
    _backlightval=LCD_NOBACKLIGHT;
    LCD_expanderWrite(0);
}

void LCD_backlight(void) {
    _backlightval=LCD_BACKLIGHT;
    LCD_expanderWrite(0);
}

void LCD_print(char data[])
{
    uint8 size;
    size = strlen(data);
    uint8 i;
    for (i = 0; i < size; i++) {
        LCD_write(data[i]);
    }
}

uint8 LCD_init(){

    i2c_master_start();
    i2c_master_writeByte(LCD_ADDRESS << 1);
    if (!i2c_master_checkAck()) {
        i2c_master_stop();
        return 0;
    }
    i2c_master_stop();

    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    if (LCD_ROW > 1) {
        _displayfunction |= LCD_2LINE;
    }
    _numlines = LCD_ROW;

    // for some 1 line displays you can select a 10 pixel high font
    if ((LCD_DOTSIZE != 0) && (LCD_ROW == 1)) {
        _displayfunction |= LCD_5x10DOTS;
    }

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
    os_delay_us(50000); 
  
    // Now we pull both RS and R/W low to begin commands
    LCD_expanderWrite(0);   // reset expanderand turn backlight off
    os_delay_us(1000000);

    //put the LCD into 4 bit mode
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46
    
    // we start in 8bit mode, try to set 4 bit mode
    LCD_write4bits(0x30);
    os_delay_us(4500); // wait min 4.1ms
    
    // second try
    LCD_write4bits(0x30);
    os_delay_us(4500); // wait min 4.1ms
    
    // third go!
    LCD_write4bits(0x30); 
    os_delay_us(150);
    
    // finally, set to 4-bit interface
    LCD_write4bits(0x20); 


    // set # lines, font size, etc.
    LCD_command(LCD_FUNCTIONSET | _displayfunction);  
    
    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    LCD_display();
    
    // clear it off
    LCD_clear();
    
    // Initialize to default text direction (for roman languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    
    // set the entry mode
    LCD_command(LCD_ENTRYMODESET | _displaymode);
    
    LCD_home();

    return 1;
}
