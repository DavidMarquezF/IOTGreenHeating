#include "DHTStub.h"

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(byte, nbit) ((byte) |= (1 << (nbit)))
#define bitClear(byte, nbit) ((byte) &= ~(1 << (nbit)))
#define TIMEOUT UINT16_MAX

// https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp

// D0, A5 have NO interrupt
// D5, D6, D7, A2, A6, WKP, TX, RX can be used with no limitations
DHTStub::DHTStub(uint16_t pin)
{
  _waitingResponse = false;
  _pin = pin;
  _maxCycles = System.ticksPerMicrosecond() * 2000; // 1 ms of timeout for reading
}

//usec is the pullup time to ACK before transmitting
// It should be 80us, but the default is set a bit higher to make sure
void DHTStub::begin(uint8_t usec)
{
  DEBUG_PRINTLN("DHT Stub initialized");
  DEBUG_PRINT("DHT max clock cycles: ");
  DEBUG_PRINTLN(_maxCycles, DEC);
  pinMode(_pin, INPUT);
  _pullTime = usec;
  attachReadInterrupt();
}

void DHTStub::updateData(float temp, float humid)
{
  _temp = (temp < 0 ? 0x8000 : 0) | (uint16_t)(abs(temp) * 10);
  _humid = (int16_t)(humid * 10);
}

void DHTStub::sendIfNeeded()
{
  if (_waitingResponse)
  {
    sendData();
    _waitingResponse = false;
  }
}
void DHTStub::sendData()
{
  DEBUG_PRINTLN("DHT sending data");

  detatchReadInterrupt();
  noInterrupts();

  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  uint16_t checkSum = ((uint8_t)_temp) + ((uint8_t) (_temp >> 8))+ ((uint8_t)_humid) + ((uint8_t)(_humid >> 8));

  // Since we received the high flank, the reader waits for 55 microseconds to start reading low
  delayMicroseconds(55);
  // Datasheet says pull down for 80us
  // https://cdn-shop.adafruit.com/datasheets/Digital+humidity+and+temperature+sensor+AM2302.pdf
  delayMicroseconds(_pullTime);

  digitalWrite(_pin, HIGH);
  delayMicroseconds(_pullTime);

  // Send the 40 bits.Each bit is sent as a 50
  // microsecond low pulse followed by a variable length high pulse.  If the
  // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
  // then it's a 1.
  // 16 bits for RH, 16 bits for temperature and 8 bits for check sum
  //If the temperature is negative, the highest bit will be -1 (it's not the same as C2)

  DEBUG_VERB_PRINT("Humidity: ")
  sendWord(_humid);
  DEBUG_VERB_PRINTLN("")
  DEBUG_VERB_PRINT("Temp: ")
  sendWord(_temp);
  DEBUG_VERB_PRINTLN("")
  DEBUG_VERB_PRINT("Check: ")
  sendByte(checkSum);
  DEBUG_VERB_PRINTLN("")

  // Finish transmission
  digitalWrite(_pin, LOW);
  delayMicroseconds(10);
  // Reenable all interrupts
  pinMode(_pin, INPUT);
  interrupts();
  attachReadInterrupt();
}

void DHTStub::handleFallingInterrupt(void)
{
  DEBUG_PRINTLN("DHT request received");
  // This is handled inside the interrupt so while this is happening eveything else will be locked
  // It's a desired behavior since there is nothing with more priority thant responding on time

  // First expect a low signal for 1ms followed by a high signal
  // for ~20-40 microseconds again.

  if (expectPulse(LOW) == TIMEOUT)
  {
    DEBUG_PRINTLN("Timeout low pulse");
    return;
  }
  //delayMicroseconds(10);
  /*if (expectPulse(HIGH) == TIMEOUT)
  {
    DEBUG_PRINTLN("Timeout high pulse");
            DEBUG_PRINTLN(digitalRead(_pin));

    return;
  }*/

  _waitingResponse = true;
}

void DHTStub::sendWord(uint16_t value)
{
  sendByte(value >> 8);
  sendByte(value);
}
void DHTStub::sendByte(uint8_t value)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(_pin, LOW);
    bool val = bitRead(value, sizeof(value) * 8 - 1);
    value <<= 1;
    DEBUG_VERB_PRINT(val);
    delayMicroseconds(50);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(val ? 80 : 20);
  }
}

void DHTStub::attachReadInterrupt(void)
{
  attachInterrupt(_pin, &DHTStub::handleFallingInterrupt, this, FALLING);
}
void DHTStub::detatchReadInterrupt(void)
{
  detachInterrupt(_pin);
}

uint16_t DHTStub::expectPulse(bool level)
{
  uint32_t count = 0; // To work fast enough on slower AVR boards
  while (digitalRead(_pin) == level)
  {
    if (count++ >= _maxCycles)
    {
      return TIMEOUT; // Exceeded timeout, fail.
    }
  }

  return count;
}