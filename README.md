# IOTGreenHeating
IOT system that based on the current green energy production and temperature controls the heating system accordingly.


# Features

For this project it is important to have a set of mandatory goals and some possible extensions so that

## Required

* [ ] Collect data through webhooks from the weather forecast and the green energy production API
* [ ] Display green energy production percentage
* [ ] Display temperature inside and outside
* [ ] Control a heating system (two options):
  * Stub heating system that will be controlled by the IOTGreeHeating (probably just a display/LED or something of the likes)
  * Use small heater to heat a small environment (a box for example) to simulate how it would work in a big room

## Extensions

* [ ] Predict the green energy production so that even if the device is not connected it will be able to operate correctly
* [ ] Notifications
* [ ] Generation of different charts that could be displayed in thingspeak
* [ ] Make the system flexible so that the user can use different API to obtain weather or green energy data
* [ ] Quantify the amount of green energy you have used in comparison to what would have been used if this system wasn't in place
* [ ] Predict thermal inertia based on thermal readings from other days so that the heating system can be controlled more efficiently
* [ ] Good interface with graphics in the display

# Plan
## Sensors that we'll use

* DHT22 breakout board

## How do we plan to test the device

### Sensors
 * DHT22 temperature / humidity
 
### Actuators
 * SSD1306 Display (will display the sensor data)

 * Relay + resistor of 5 watts (heater) or simulated (will output heat in case the heater is turned on)

## Relevant documentation/code examples in GitHub

* Particle Photon - [datasheet](https://docs.particle.io/assets/pdfs/datasheets/photon-datasheet.pdf)
* DHT22 - [datasheet](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf) [code example](https://github.com/piettetech/PietteTech_DHT)
* SSD1306 - [datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) [code example](https://github.com/adafruit/Adafruit_SSD1306)
