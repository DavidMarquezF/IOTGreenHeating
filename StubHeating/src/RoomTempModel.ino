#include "RoomTempModel.h"
#include <math.h>

RoomTempModel::RoomTempModel(uint16_t periodMs, uint16_t roomC,uint16_t roomR,uint16_t heatC,uint16_t heatR,uint16_t maxHeatTemp){
    _periodMs = periodMs;
    _roomR =roomR;
    _roomC = roomC;
    _heaterC = heatC;
    _heaterR = heatR;
    _maxHeatTemp = maxHeatTemp;
    _roomTau = _roomC * _roomR;
    _heaterTau = _heaterR * _heaterC;

}
void RoomTempModel::begin(uint16_t playSpeed, double startTemp, bool heatEnabled)
{
    _playSpeed = playSpeed;
    _temp0 = startTemp;
    _Oe = startTemp;
    _heatEnabled = heatEnabled;
    _heat0 = 0;
    _n  =0;
    _nQ = 0;
}

void RoomTempModel::run(void)
{
    _n++;
    _nQ++;
}

uint16_t RoomTempModel::getPeriodMs(){
    return _periodMs;
}

double RoomTempModel::getTemp(void)
{
    return calcTempRoom() + calcTempHeat();
}

double RoomTempModel::calcTempRoom()
{
    double e = exp(-(_n * (_periodMs /1000.0) * _playSpeed) / ((double)_roomTau));

    return _Oe * (1 - e) + _temp0 * e;
}
double RoomTempModel::calcTempHeat()
{
    double eHeat = exp(-(_nQ * (_periodMs /1000.0)* _playSpeed)/ ((double)_heaterTau));
    return _maxHeatTemp * _heatEnabled * (1 - eHeat) + _heat0 * eHeat;
}

void RoomTempModel::setExtTemp(double extTemp)
{
    _temp0 =calcTempRoom();
    _Oe = extTemp;
    _n = 0;
}
void RoomTempModel::setHeaterEnabled(bool heatEnabled)
{
    if(_heatEnabled == heatEnabled) return;
    _heat0 = calcTempHeat();
    _heatEnabled = heatEnabled;
    _nQ = 0;
}
