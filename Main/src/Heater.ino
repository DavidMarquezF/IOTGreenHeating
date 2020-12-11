#include "Heater.h"

Heater::Heater(uint16_t pin){
    _pin = pin;
}
void Heater::setup(void){
    pinMode(_pin, OUTPUT);
}
void Heater::turnOn(void){
    digitalWrite(_pin, HIGH);

}
void Heater::turnOff(void){
    digitalWrite(_pin, LOW);
}