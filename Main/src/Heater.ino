#include "Heater.h"

Heater::Heater(uint16_t pin){
    _pin = pin;
}
void Heater::setup(void){
    pinMode(_pin, OUTPUT);
    turnOff();
}
void Heater::turnOn(void){
    digitalWrite(_pin, HIGH);
    _status = true;
}
void Heater::turnOff(void){
    digitalWrite(_pin, LOW);
    _status = false;
}
bool Heater::isTurnedOn(void){
    return _status;
}