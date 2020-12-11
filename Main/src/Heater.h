#ifndef _HEATER_H_
#define _HEATER_H_

class Heater{
    public:
        Heater(uint16_t pin);
        void setup(void);
        void turnOn(void);
        void turnOff(void);
        bool isTurnedOn(void);
    private:
        uint16_t _pin;
        bool _status;
};

#endif