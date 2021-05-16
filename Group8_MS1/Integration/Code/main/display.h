#ifndef DISPLAY_H
#define DISPLAY_H

#include "ssd1306.h"

void initDisplay();
void textDemo();
void displayText(char* text);
void displayTextTime(char* text, int time);
#endif