#ifndef _GREEN_PRODUCTION_H_
#define _GREEN_PRODUCTION_H_

namespace GreenProduction{
    void setup(void);
    bool request(uint16_t id); // returns true if it has finished the request
    float getPercentage(void);
    bool hasValidData(void);
    void displayInfo(void);
}

#endif