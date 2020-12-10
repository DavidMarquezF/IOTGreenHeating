#ifndef ROOM_TEMP_MODEL_H_
#define ROOM_TEMP_MODEL_H_

class RoomTempModel{
    public:
        RoomTempModel(uint16_t _periodMs = 500,uint16_t roomC=1000, uint16_t roomR=1, uint16_t heatC=5, uint16_t heatR=1, uint16_t maxHeatTemp=30);
        void begin(uint16_t playSpeed = 256,double startTemp = 20, bool heatEnabled = false);
        void run(void);
        double getTemp(void);
        void setExtTemp(double extTemp);
        void setHeaterEnabled(bool heatEnabled);
        uint16_t getPeriodMs();

    private:
        uint16_t _playSpeed;
        uint16_t _periodMs;
        uint16_t _roomR;           // Room resistance
        uint16_t _roomC; // Room capacity
        uint16_t _heaterR;      // Heater resistance
        uint16_t _heaterC;      // Heater capacity
        double _Oe;           // Exterior temperature
        uint16_t _maxHeatTemp;  // Maximum temperature heater
        ulong _n;               // n room
        ulong _nQ;              // n heater
        ulong _lastTime;
        int16_t _temp0;
        int16_t _heat0;
        uint16_t _roomTau;
        uint16_t _heaterTau;
        bool _heatEnabled;
        double calcTempRoom();
        double calcTempHeat();
};
#endif