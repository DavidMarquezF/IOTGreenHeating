#include "Temperature.h"

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

class Display{
    public:
        bool begin(void);
        void displayStatus(bool displayGp, bool displayTi, bool displayTa, bool heaterOn,Temperature temp, float greenProdPercentage);
};

#endif