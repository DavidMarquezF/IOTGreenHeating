/*
 * Project Main
 * Description: Main source code file for IoT Green Heating project
 * Author: Ondrej Viskup
 * Date: 07-12-2020
 */

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "PietteTech_DHT.h"
#include "Heater.h"

// Define macros for better readability
#define GP_HOOK_RESP "hook-response/greenproduction"
#define GP_HOOK_PUB	"greenproduction"
#define TA_HOOK_RESP "hook-response/temperatureAarhus"
#define TA_HOOK_PUB "temperatureAarhus"

#define HOOK_REPEAT 3

// DHT22 temperature sensor macros
#define DHTTYPE  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D5          // Digital pin for communications

// Declaration for SSD1306 display connected using software SPI
#define SCREEN_WIDTH (uint8_t)128 // OLED display width, in pixels
#define SCREEN_HEIGHT (uint8_t)64 // OLED display height, in pixels
#define SCREEN_PADDING (uint8_t)4

#define OLED_MOSI (int8_t) D0
#define OLED_CLK (int8_t) D1
#define OLED_DC (int8_t) D2
#define OLED_RESET (int8_t) D3
#define OLED_CS (int8_t) D4

// Particle variables
double desiredTemp = 24.4;
double minTemp = 21.2;
double minGreen = 66.6;
int checkPeriod = 15;

Heater heater(D6);

// Particles functions (setters for the variables)
int setDesiredTemp(String extra) {
    desiredTemp = extra.toFloat();
    return 0;
}

int setMinTemp(String extra) {
    minTemp = extra.toFloat();
    return 0;
}

int setMinGreen(String extra) {
    minGreen = extra.toFloat();
    return 0;
}

int setCheckPeriod(String extra) {
    checkPeriod = extra.toInt();
    return 0;
}

void initializeParticleVariablesAndFunctions() {
    Particle.variable("desiredTemp", &desiredTemp, DOUBLE);
    Particle.variable("minTemp", &minTemp, DOUBLE);
    Particle.variable("minGreen", &minGreen, DOUBLE);
    Particle.variable("checkPeriod", &checkPeriod, INT);  

    Particle.function("setDesiredTemp", setDesiredTemp);
    Particle.function("setMinTemp", setMinTemp);
    Particle.function("setMinGreen", setMinGreen);
    Particle.function("setCheckPeriod", setCheckPeriod);     
}

// Struct for keeping latest green production data
struct GreenProdData
{
    int _id;
    float totalLoad;
    float onshoreWind;
    float offshoreWind;
    float biomass;
    float solarPower;
    float hydroPower;
    float otherRenewable;
    float percentage;
};

// Struct for keeping temperature data
struct Temperature
{
    float inside;
    float outside;
};

// Internal structs and variables
GreenProdData latestGreenProduction;
Temperature latestTemperature;

GreenProdData currentGreenProduction;
Temperature currentTemperature;

boolean gpValidResponse;
boolean taValidResponse;
boolean tiValidResponse;
int gpInvalidCalls = 0;
int taInvalidCalls = 0;
int tiInvalidCalls = 0;
int gpSuccessfulUpdates = 0;
int taSuccessfulUpdates = 0;
int tiSuccessfulUpdates = 0;

int n = 0;

// Library instantion
PietteTech_DHT DHT(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void handleGreenProduction(const char *event, const char *data) {
    Serial.println(Time.timeStr());
    Serial.println("Handle greenproduction ---> Got response containing: " + String(data));
    // When API fails it will return 6 tildes "~~~~~~", we should check if the response contains even 2 tildes next to each other
    // That would mean, that one of the values did not get read correctly and we should repeat the call

    //Idea from https://community.particle.io/t/tutorial-webhooks-and-responses-with-parsing-json-mustache-tokens/14612/21
    String str = String(data);
    // Check for data validity
    if (str.indexOf("~~") == -1 && !str.startsWith("~")) {
        // We have valid data
        gpValidResponse = true;
        Serial.println("GP: Valid data.");
        char strBuffer[256] = "";
        str.toCharArray(strBuffer, 256);
        int currentId = String(strtok(strBuffer, "~")).toInt();
        // If ids match, we already have the most recent data
        if (latestGreenProduction._id == currentId) {
            Serial.printf("The ids %d and %d match, aborting.\n", latestGreenProduction._id, currentId);
            currentGreenProduction = latestGreenProduction;
            return;
        } else {
            currentGreenProduction._id = currentId;
            currentGreenProduction.totalLoad = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.onshoreWind = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.offshoreWind = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.biomass = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.solarPower = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.hydroPower = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.otherRenewable = String(strtok(NULL, "~")).toFloat();
        }        
    } else {
        // We have invalid data
        gpValidResponse = false;
        Serial.printf("Call #%d: GP: Invalid data.\n", gpInvalidCalls);
    }
}

// I was getting multiple responses, the thing was that I had another webhook 
// called temp and it was probably string matching temperatureAarhus webhook
// fix: https://community.particle.io/t/webhook-being-repeated-several-times/26860
void handleTemperatureAarhus(const char* event, const char* data) {
    Serial.println(Time.timeStr());
    Serial.println("Handle temperatureAarhus ---> Got response containing: " + String(data));

    // Although it never happen that we had invalid data, we should check the validity of the response
    String str = String(data);
    if (str != NULL) {
        // We have valid data
        taValidResponse = true;
        Serial.println("TA: Valid data.");
        currentTemperature.outside = String(data).toFloat();
    } else {
        taValidResponse = false;
        Serial.printf("Call #%d: TA: Invalid data.\n", taInvalidCalls);
    } 
}

void updateGreenProduction() {
    // Store latest data into variable and reinitialize current
    latestGreenProduction = currentGreenProduction;
    currentGreenProduction = {};

    gpValidResponse = false;
    gpInvalidCalls = 0;

    while (!gpValidResponse && gpInvalidCalls < HOOK_REPEAT) {
        
        // Get green production and wait for response
        Particle.publish(GP_HOOK_PUB, PRIVATE);
        delay(5s);

        if (!gpValidResponse) {
            Serial.printf("Call #%d: green production update failed\n", gpInvalidCalls);
            gpInvalidCalls++;
        } else {
            gpInvalidCalls = 0;
        }
    }

    // Even after HOOK_REPEAT tries, API failed, we need to reuse the old data 
    // and try again next time device will wake up
    if (!gpValidResponse) {
        Serial.println("Green production webhook failed!");
        if (gpSuccessfulUpdates > 0) {
            Serial.println("Using latest valid data...");
            currentGreenProduction = latestGreenProduction;
        } else {
            // We cannot show anything, because there is no valid data
            Serial.println("We cannot show anything, because there is no valid data!");
        }        
    } else {
        gpSuccessfulUpdates++;
    }

    // Calculate the green energy production percentage
    if (gpSuccessfulUpdates > 0) {
        currentGreenProduction.percentage = 100 * (currentGreenProduction.onshoreWind + currentGreenProduction.offshoreWind + currentGreenProduction.biomass + currentGreenProduction.solarPower + currentGreenProduction.hydroPower + currentGreenProduction.otherRenewable) / currentGreenProduction.totalLoad;
        Serial.printf("Current green production percentage is: %.2f %%\n", currentGreenProduction.percentage);
    }  
}

void updateOutsideTemperature() {
    // Store latest data into variable and reinitialize current
    latestTemperature.outside = currentTemperature.outside;
    currentTemperature.outside = {};

    taValidResponse = false;
    taInvalidCalls = 0;

    while (!taValidResponse && taInvalidCalls < HOOK_REPEAT) {

        // Get temperature outside and wait for response
        Particle.publish(TA_HOOK_PUB, PRIVATE);
        delay(5s);

        if (!taValidResponse) {
            Serial.printf("Call #%d: temperature outside update failed\n", taInvalidCalls);
            taInvalidCalls++;
        } else {
            taInvalidCalls = 0;
        }
    }

    // Even after HOOK_REPEAT tries, API failed, we need to reuse the old data 
    // and try again next time device will wake up
    if (!taValidResponse) {
        Serial.println("Temperatue webhook failed!");
        if (taSuccessfulUpdates > 0) {
            Serial.println("Using latest valid data...");
            currentTemperature.outside = latestTemperature.outside;
        } else {
            // We cannot show anything, because there is no valid data
            Serial.println("We cannot show anything, because there is no valid data!");
        }        
    } else {
        taSuccessfulUpdates++;
    }
}

// Update inside temperature using DHT22 sensor
void updateInsideTemperature() {
    // Store latest data into variable and reinitialize current
    latestTemperature.inside = currentTemperature.inside;
    currentTemperature.inside = {};

    tiValidResponse = false;
    tiInvalidCalls = 0;

    while (!tiValidResponse && tiInvalidCalls < HOOK_REPEAT) {
        int measurementResult = DHT.acquireAndWait(1000);

        if (measurementResult != DHTLIB_OK) {
            // error
            Serial.printf("Call #%d: temperature inside update failed due to internal error!\n", tiInvalidCalls);
            tiInvalidCalls++;
        } else {
            float measuredTemp = DHT.getCelsius();
            Serial.printf("We obtained temperature of %f C\n", measuredTemp);
            if (measuredTemp < 0) {
                // invalid data
                Serial.printf("Call #%d: temperature inside update failed due to invalid data!\n", tiInvalidCalls);
                tiInvalidCalls++;
            } else {
                Serial.println("TI: Valid data.");
                currentTemperature.inside = measuredTemp;
                tiInvalidCalls = 0;
                tiValidResponse = true;
            }
        }
    }

    // Even after HOOK_REPEAT tries, API failed, we need to reuse the old data 
    // and try again next time device will wake up
    if (!tiValidResponse) {
        Serial.println("Temperatue sensor failed!");
        if (tiSuccessfulUpdates > 0) {
            Serial.println("Using latest valid data...");
            currentTemperature.inside = latestTemperature.inside;
        } else {
            // We cannot show anything, because there is no valid data
            Serial.println("We cannot show anything, because there is no valid data!");
        }        
    } else {
        tiSuccessfulUpdates++;
    }
}

// based on https://www.flaticon.com/free-icon/leaf_497384
const unsigned char leafIcon [] PROGMEM = {
	// 'leaf, 48x48px
	0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x00, 0x01, 0xff, 
	0xf8, 0x0e, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x1c, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x38, 0x00, 0x03, 
	0xf0, 0x00, 0x00, 0x30, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x70, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xe0, 
	0x00, 0x78, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xf0, 0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, 
	0x01, 0x80, 0x03, 0x80, 0x00, 0x0c, 0x03, 0x80, 0x07, 0x00, 0x00, 0x1c, 0x03, 0x00, 0x0e, 0x00, 
	0x00, 0x78, 0x07, 0x00, 0x1c, 0x00, 0x01, 0xe0, 0x06, 0x00, 0x18, 0x00, 0x03, 0xc0, 0x0e, 0x00, 
	0x38, 0x00, 0x0f, 0x00, 0x0c, 0x00, 0x30, 0x00, 0x1e, 0x00, 0x0c, 0x00, 0x70, 0x00, 0x3c, 0x00, 
	0x1c, 0x00, 0x60, 0x00, 0x70, 0x00, 0x1c, 0x00, 0x60, 0x00, 0xe0, 0x00, 0x18, 0x00, 0xe0, 0x01, 
	0xc0, 0x00, 0x18, 0x00, 0xc0, 0x03, 0x80, 0x00, 0x38, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x30, 0x00, 
	0xc0, 0x0e, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x1c, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x38, 0x00, 0x00, 
	0x70, 0x00, 0xc0, 0x30, 0x00, 0x00, 0x60, 0x00, 0xe0, 0x60, 0x00, 0x00, 0x60, 0x00, 0x60, 0xe0, 
	0x00, 0x00, 0xe0, 0x00, 0x61, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x71, 0x80, 0x00, 0x01, 0xc0, 0x00, 
	0x33, 0x80, 0x00, 0x01, 0x80, 0x00, 0x3f, 0x00, 0x00, 0x03, 0x80, 0x00, 0x1e, 0x00, 0x00, 0x07, 
	0x00, 0x00, 0x0e, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1f, 0xc0, 
	0x00, 0x38, 0x00, 0x00, 0x19, 0xf8, 0x01, 0xf0, 0x00, 0x00, 0x38, 0x7f, 0xff, 0xe0, 0x00, 0x00, 
	0x30, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00
};

// setup() runs once, when the device is first turned on.
void setup() {
    // Put initialization like pinMode and begin functions here.
    initializeParticleVariablesAndFunctions();

    // Set timezone to Copenhagen
    Time.zone(1);

    // Subscribe to the hook responses
    Particle.subscribe(GP_HOOK_RESP, handleGreenProduction, MY_DEVICES);
    Particle.subscribe(TA_HOOK_RESP, handleTemperatureAarhus, MY_DEVICES);

    Serial.begin(9600);
    Serial.print("DHT LIB version: ");
    Serial.println(DHTLIB_VERSION);
    DHT.begin();
    heater.setup();
    // delay(2s);

    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Log.info("SSD1306 allocation failed");
        //TODO: Stop the execution of the program
    }
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2s); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
    display.display();
}

void printCurrentSituation() {
    Serial.printf("#%d: CURRENT DEVICE STATUS\n", n);
    Serial.println(Time.timeStr());
    Serial.println("-----------------------------");

    Serial.println("Green production information:");
    Serial.print("Percentage: ");
    Serial.println(currentGreenProduction.percentage);
    Serial.print("Total: ");
    Serial.println(currentGreenProduction.totalLoad);
    Serial.print("Onshore wind: ");
    Serial.println(currentGreenProduction.onshoreWind);
    Serial.print("Offshore wind: ");
    Serial.println(currentGreenProduction.offshoreWind);
    Serial.print("Biomass: ");
    Serial.println(currentGreenProduction.biomass);
    Serial.print("Solar: ");
    Serial.println(currentGreenProduction.solarPower);
    Serial.print("Hydro: ");
    Serial.println(currentGreenProduction.hydroPower);
    Serial.print("Other: ");
    Serial.println(currentGreenProduction.otherRenewable);

    Serial.println("-----------------------------");
    Serial.println("Temperature information: ");
    Serial.print("Inside: ");
    Serial.println(currentTemperature.inside);
    Serial.print("Outside: ");
    Serial.println(currentTemperature.outside);

    Serial.println("-----------------------------");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    // The core of your code will likely live here.    
    updateGreenProduction();

    updateOutsideTemperature();

    updateInsideTemperature();

    delay(2s);
    printCurrentSituation();

    // display for 30 sec and then do to sleep
    // https://diyprojects.io/using-i2c-128x64-0-96-ssd1306-oled-display-arduino/
    
    // Clear the buffer
    display.clearDisplay();
    // // top border
    // display.drawLine(0, 0, SCREEN_WIDTH-1, 0, WHITE);
    // // left border
    // display.drawLine(0, 0, 0, SCREEN_HEIGHT-1, WHITE);
    // // right border
    // display.drawLine(SCREEN_WIDTH-1, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, WHITE);
    // // bottom border
    // display.drawLine(0, SCREEN_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, WHITE);
    display.drawBitmap(0, 0, leafIcon, 48, 48, WHITE);
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    // display curent gp %
    display.printlnf(" %3g %% ", round(currentGreenProduction.percentage));
    display.println("in grid");
    // draw delimiter
    display.drawLine(51, 0, 51, SCREEN_HEIGHT-1, WHITE);
    display.drawLine(51, 20, SCREEN_WIDTH-1, 20, WHITE);
    display.drawLine(51, 44, SCREEN_WIDTH-1, 44, WHITE);
    display.setCursor(54, 0);
    // display date and time
    // https://docs.particle.io/reference/device-os/firmware/photon/#setformat-
    // http://www.cplusplus.com/reference/ctime/strftime/
    time_t current = Time.now();
    display.print(Time.format(current, " %F \n"));
    display.setCursor(54, 8);
    display.print(Time.format(current, "   %R   \n\n"));
    display.setCursor(54, 24);
    display.print(" Heating is \n");
    display.setCursor(54, 32);
    display.print(" ON / OFF \n");
    display.setCursor(54, 48);
    display.printf(" IN:  %2.1f C\n", currentTemperature.inside);
    display.setCursor(54, 56);
    display.printlnf(" OUT: %2.1f C\n", currentTemperature.outside);
    display.display();

    Serial.printf("Sleep cycle #%d: Going to sleep...\n", n);
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(1min)
          .flag(SystemSleepFlag::WAIT_CLOUD);
    SystemSleepResult result = System.sleep(config);

    if (result.error() != 0) {
        Serial.printf("Something went wrong during #%d sleep cycle.\n", n);
    } else {
        Serial.printf("Sleep cycle #%d: Device successfully woke up from sleep.\n", n);
    }

    n++;
}