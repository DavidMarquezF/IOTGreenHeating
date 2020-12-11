#include "DHTStub.h"

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(byte, nbit) ((byte) |= (1 << (nbit)))
#define bitClear(byte, nbit) ((byte) &= ~(1 << (nbit)))
#define TIMEOUT UINT16_MAX
#define abs(x) ((x)>0?(x):-(x))

// https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp

// D0, A5 have NO interrupt
// D5, D6, D7, A2, A6, WKP, TX, RX can be used with no limitations
DHTStub::DHTStub(uint16_t pin)
{
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

void DHTStub::updateTemp(float temp){
  _temp = (temp < 0 ? 0x8000 : 0) | (uint16_t)(abs(temp) * 10);  
}

void DHTStub::updateHumid(float humid){
  _humid = (int16_t)(humid * 10);
}


void DHTStub::sendData()
{
  DEBUG_PRINTLN("DHT sending data");
 
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  uint16_t checkSum = ((uint8_t)_temp) + ((uint8_t) (_temp >> 8))+ ((uint8_t)_humid) + ((uint8_t)(_humid >> 8));

  // Since we received the high flank, the reader waits for 55 microseconds to start reading low
//  delayMicroseconds(55); This line works with the Adafruit library but not with the Pierre (Pierre works with interrupts )

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
  // If the temperature is negative, the highest bit will be -1 (it's not the same as C2)

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

  /* This is used to clear the pending interrupt flag generated
   * by the changing of the pin in this interrupt
   * since there is a bug with photon when calling 
   * detachInterrupt and attachInterrupt inside an interrupt callback
   * 
   * Detaching and attaching the interrupt would mean that there wouldn't
   * be any pending interrupts (when an interrupt is raised while another interrupts
   * callback is being executed) but since it doesn't work we clear the pending
   * interrupt by hand.
   *  
   * https://community.particle.io/t/interrupts-priority-architecture/18671/26
   */
  STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

  uint16_t gpio_pin = PIN_MAP[_pin].gpio_pin;

  EXTI_ClearITPendingBit(gpio_pin);

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
  

  // We're using a variable and doing polling because we would need
  // to detatch the pin interrupt before sending data
  // If it's not detached the interrupt is registered and called as soon
  // as the current interrupt finishes. Photon has an error with 
  // detachInterrupt, where calling it from inside an interrupt
  // crashes the whole program
  sendData(); 
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
    delayMicroseconds(val ? 70 : 26);
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