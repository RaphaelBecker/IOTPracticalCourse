#include "display.h"

void initDisplay()
{
    ssd1306_128x64_i2c_init();
    ssd1306_setFixedFont(ssd1306xled_font6x8);

    timestampDisplayString = "00:00";
    groupDisplayString = "G08";
    displayCount = "00";
    displayCountPrediction = "00";

}

void textDemo()
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0, 0, "Normal text", STYLE_NORMAL, 1);
    delay(3000);
    ssd1306_clearScreen();
}

void displayText(char *text)
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0, 0, text, STYLE_NORMAL, 1);
}

void displayTextTime(char *text, int time)
{
    ssd1306_clearScreen();
    ssd1306_printFixedN(0, 0, text, STYLE_NORMAL, 1);
    delay(time * 1000);
    ssd1306_clearScreen();
}

void showRoomState()
{
    while (1)
    {
        ssd1306_clearScreen();
        ssd1306_printFixedN(0, 0, groupDisplayString, STYLE_NORMAL, 1);
        ssd1306_printFixedN(60, 0, timestampDisplayString, STYLE_NORMAL, 1);
        ssd1306_printFixedN(5, 30, displayCount, STYLE_NORMAL, 2);
        ssd1306_printFixedN(65, 30, displayCountPrediction, STYLE_NORMAL, 2);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
        ssd1306_clearScreen();
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}