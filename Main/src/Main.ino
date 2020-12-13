/*
 * Project Main
 * Description: Main source code file for IoT Green Heating project
 * Author: Ondrej Viskup
 * Date: 07-12-2020
 */

#include "PietteTech_DHT.h"
#include "Heater.h"
#include "Display.h"

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
double desiredTemp = 24.0;
double minTemp = 18.0;
double minGreen = 20.0;
int checkPeriod = 2;

Heater heater(D7);

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

int gpUpdateHour;

int n = 0;

// Library instantion
PietteTech_DHT DHT(DHTPIN, DHTTYPE);
Display display;

// Hook response handlers
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

// Update handlers
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
        gpUpdateHour = Time.hour();
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



// Helper functions
void printCurrentStatus() {
    Serial.printf("#%d: CURRENT DEVICE STATUS\n", n);
    Serial.println(Time.timeStr());
    Serial.println("-----------------------------");

    Serial.println("Green production information:");
    Serial.printlnf("Last update hour: %d", gpUpdateHour);
    Serial.printlnf("Percentage: %.2f %%", currentGreenProduction.percentage);
    Serial.printlnf("Total: %.2f", currentGreenProduction.totalLoad);
    Serial.printlnf("Onshore wind: %.2f", currentGreenProduction.onshoreWind);
    Serial.printlnf("Offshore wind: %.2f", currentGreenProduction.offshoreWind);
    Serial.printlnf("Biomass: %.2f", currentGreenProduction.biomass);
    Serial.printlnf("Solar: %.2f", currentGreenProduction.solarPower);
    Serial.printlnf("Hydro: %.2f", currentGreenProduction.hydroPower);
    Serial.printlnf("Other: %.2f", currentGreenProduction.otherRenewable);

    Serial.println("-----------------------------");
    Serial.println("Temperature information: ");
    Serial.printlnf("Inside: %.2f C", currentTemperature.inside);
    Serial.printlnf("Outside: %.2f C", currentTemperature.outside);

    Serial.println("-----------------------------");
}

void handleHeating() {
    // If the heating is currently on, check the status of the temperature
    if (heater.isTurnedOn()) {
        // We know the state of the current room temperature
        if (tiSuccessfulUpdates > 0) {
            if (currentTemperature.inside >= desiredTemp) {
                // Turn the heater off
                heater.turnOff();
                Serial.println("Heater: I am turning off. We have reached the desired temperature.");
            } else {
                // Let the heater be
                Serial.println("Heater: I am staying on. We have not reached the desired temperature yet.");
            }
        // We do not know anything about the status inside
        } else {
            Serial.println("Heater: I am on and I do not have data on current temperature inside!");
        }
    // If the heating is off, check the inside temperature        
    } else {
        if (tiSuccessfulUpdates > 0) {
            if (currentTemperature.inside < desiredTemp) {
                if (gpSuccessfulUpdates > 0) {
                    if (currentGreenProduction.percentage >= minGreen) {
                        // The green production percentage is high enough to start heating
                        heater.turnOn();
                        Serial.println("Heater: I am turning on the ecologic heating now.");
                    } else if (currentTemperature.inside < minTemp) {
                        // It is cold inside, we should turn on the heater no matter the green production percentage
                        heater.turnOn();
                        Serial.println("Heater: Sorry man, it is too cold inside. I am turning the heater on even though it is not ecologic enough for you.");
                    } else {
                        Serial.println("Heater: It is not that cold here and there is not enough green energy in the system right now to start the heating.");
                    }
                // We do not know anything about the green production status
                } else {
                    if (currentTemperature.inside < minTemp) {
                        // It is cold inside, we should turn on the heater no matter the green production percentage
                        heater.turnOn();
                        Serial.println("Heater: Sorry man, it is too cold inside. I am turning the heater on even though I do not know anything about the green energy right now.");
                    } else {
                        Serial.println("Heater: I am off and I do not have data on green production!");
                    }                    
                }
            // There is no need to start heating now, we are above the desired temperature                             
            } else {
                Serial.println("Heater: There is no need to start heating now, we are above the desired temperature.");
            }
        // We do not know anything about the status inside
        } else {
            Serial.println("Heater: I am off and I do not have data on current temperature inside!");
        }
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
    heater.setup();
    // delay(2s);

    if (!display.begin())
    {
        Log.info("SSD1306 allocation failed");
        //TODO: Stop the execution of the program
    }
    
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    // The core of your code will likely live here.  
    // Update once every hour, if we fail to get the data, we can perform extra update 
    if (Time.hour() > gpUpdateHour || (Time.hour() == 0 && gpUpdateHour == 23) || !gpValidResponse) {
        updateGreenProduction();
    }    

    updateOutsideTemperature();

    updateInsideTemperature();

    delay(2s);
    printCurrentStatus();

    handleHeating();

    display.displayStatus(gpSuccessfulUpdates > 0, tiSuccessfulUpdates > 0, taSuccessfulUpdates > 0, heater.isTurnedOn(), currentTemperature, currentGreenProduction.percentage);

    Particle.publish("iotGreenHeating", "{ \"1\": \"" + String(currentGreenProduction.percentage) + "\", \"2\":\"" + String(currentTemperature.inside) + "\", \"3\":\"" + String(currentTemperature.outside) + "\", \"k\":\"EFCEYN9AO5ATXI65\" }", 60, PRIVATE);

    Serial.printlnf("Sleep cycle #%d: Going to sleep for %d minutes...", n, checkPeriod);
    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::STOP)
          .duration(checkPeriod * 1min)
          .flag(SystemSleepFlag::WAIT_CLOUD);
    SystemSleepResult result = System.sleep(config);

    if (result.error() != 0) {
        Serial.printlnf("Something went wrong during #%d sleep cycle.", n);
    } else {
        Serial.printlnf("Sleep cycle #%d: Device successfully woke up from sleep.", n);
    }

    n++;
}