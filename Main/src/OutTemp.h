#ifndef _OUT_TEMP_H_
#define _OUT_TEMP_H_

namespace OutTemp{
    void setup(void);
    bool request(uint16_t id); // returns true if it has finished the request
    float getTemp(void);
    bool hasValidData(void);
}

#endif