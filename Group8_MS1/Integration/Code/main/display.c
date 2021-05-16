#include "display.h"

void initDisplay()
{
    ssd1306_128x64_i2c_init();
    ssd1306_setFixedFont(ssd1306xled_font6x8);
}

void textDemo()
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0, 0, "Normal text", STYLE_NORMAL, 1);
    delay(3000);
    ssd1306_clearScreen();
}

void displayText(char* text)
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0,0,text, STYLE_NORMAL, 1);
    
}

void displayTextTime(char* text, int time)
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0,0,text, STYLE_NORMAL, 1);
    delay(time*1000);
    ssd1306_clearScreen();
    
}