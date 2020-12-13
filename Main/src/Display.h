#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "Temperature.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"


class Display{
    public:
        Display();
        bool begin(void);
        void displayStatus(bool displayGp, bool displayTi, bool displayTa, bool heaterOn,Temperature temp, float greenProdPercentage);
    private:
        Adafruit_SSD1306 display;
};

#endif