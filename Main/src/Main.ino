/*
 * Project Main
 * Description: Main source code file for IoT Green Heating project
 * Author: Ondrej Viskup
 * Date: 07-12-2020
 */

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "PietteTech_DHT.h"

// Define macros for better readability
#define GP_HOOK_RESP "hook-response/greenproduction"
#define GP_HOOK_PUB	"greenproduction"
#define TA_HOOK_RESP "hook-response/temperatureAarhus"
#define TA_HOOK_PUB "temperatureAarhus"

#define HOOK_REPEAT 3

// DHT22 temperature sensor macros
#define DHTTYPE  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D5          // Digital pin for communications

// Particle variables
double desiredTemp = 24.4;
double minTemp = 21.2;
double minGreen = 66.6;
int checkPeriod = 15;

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

// Library instantion
PietteTech_DHT DHT(DHTPIN, DHTTYPE);
int n = 0;

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
        currentGreenProduction.percentage = (currentGreenProduction.onshoreWind + currentGreenProduction.offshoreWind + currentGreenProduction.biomass + currentGreenProduction.solarPower + currentGreenProduction.hydroPower + currentGreenProduction.otherRenewable) / currentGreenProduction.totalLoad;
        Serial.printf("Current green production percentage is: %.2f\n", currentGreenProduction.percentage);
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
    // delay(2s);
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
    n++;
    
    updateGreenProduction();

    updateOutsideTemperature();

    updateInsideTemperature();

    delay(2s);
    printCurrentSituation();

    // sleep
    delay(30s);
}