/*
 * Project Main
 * Description: Main source code file for IoT Green Heating project
 * Author: Ondrej Viskup
 * Date: 07-12-2020
 */

#include "PietteTech_DHT.h"
#include "Heater.h"
#include "Display.h"
#include "GreenProduction.h"
#include "OutTemp.h"

#define HOOK_REPEAT 3

// DHT22 temperature sensor macros
#define DHTTYPE DHT22 // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN D5     // Digital pin for communications

// Particle variables
static double desiredTemp = 24.0;
static double minTemp = 18.0;
static double minGreen = 20.0;
static int checkPeriod = 2;

// Particles functions (setters for the variables)
int setDesiredTemp(String extra)
{
    desiredTemp = extra.toFloat();
    return 0;
}

int setMinTemp(String extra)
{
    minTemp = extra.toFloat();
    return 0;
}

int setMinGreen(String extra)
{
    minGreen = extra.toFloat();
    return 0;
}

int setCheckPeriod(String extra)
{
    checkPeriod = extra.toInt();
    return 0;
}

void initializeParticleVariablesAndFunctions()
{
    Particle.variable("desiredTemp", &desiredTemp, DOUBLE);
    Particle.variable("minTemp", &minTemp, DOUBLE);
    Particle.variable("minGreen", &minGreen, DOUBLE);
    Particle.variable("checkPeriod", &checkPeriod, INT);

    Particle.function("setDesiredTemp", setDesiredTemp);
    Particle.function("setMinTemp", setMinTemp);
    Particle.function("setMinGreen", setMinGreen);
    Particle.function("setCheckPeriod", setCheckPeriod);
}

static Temperature currentTemperature;
static Heater heater(D7);
static PietteTech_DHT DHT(DHTPIN, DHTTYPE);
static Display display;

int tiSuccessfulUpdates = 0;

static int n = 0;



// Update inside temperature using DHT22 sensor
static void updateInsideTemperature()
{
    int tiInvalidCalls;
    bool tiValidResponse;

    tiValidResponse = false;
    tiInvalidCalls = 0;

    while (!tiValidResponse && tiInvalidCalls < HOOK_REPEAT)
    {
        int measurementResult = DHT.acquireAndWait(1000);

        if (measurementResult != DHTLIB_OK)
        {
            // error
            Serial.printf("Call #%d: temperature inside update failed due to internal error!\n", tiInvalidCalls);
            tiInvalidCalls++;
        }
        else
        {
            float measuredTemp = DHT.getCelsius();
            Serial.printf("We obtained temperature of %f C\n", measuredTemp);
            if (measuredTemp < 0)
            {
                // invalid data
                Serial.printf("Call #%d: temperature inside update failed due to invalid data!\n", tiInvalidCalls);
                tiInvalidCalls++;
            }
            else
            {
                Serial.println("TI: Valid data.");
                currentTemperature.inside = measuredTemp;
                tiInvalidCalls = 0;
                tiValidResponse = true;
            }
        }
    }

    // Even after HOOK_REPEAT tries, API failed, we need to reuse the old data
    // and try again next time device will wake up
    if (!tiValidResponse)
    {
        Serial.println("Temperatue sensor failed!");
        if (tiSuccessfulUpdates > 0)
        {
            Serial.println("Using latest valid data...");
        }
        else
        {
            // We cannot show anything, because there is no valid data
            Serial.println("We cannot show anything, because there is no valid data!");
        }
    }
    else
    {
        tiSuccessfulUpdates++;
    }
}

// Helper functions
void printCurrentStatus()
{
    Serial.printf("#%d: CURRENT DEVICE STATUS\n", n);
    Serial.println(Time.timeStr());
    Serial.println("-----------------------------");

    GreenProduction::displayInfo();
    Serial.println("-----------------------------");
    Serial.println("Temperature information: ");
    Serial.printlnf("Inside: %.2f C", currentTemperature.inside);
    Serial.printlnf("Outside: %.2f C", currentTemperature.outside);

    Serial.println("-----------------------------");
}

void handleHeating()
{
    // If the heating is currently on, check the status of the temperature
    if (heater.isTurnedOn())
    {
        // We know the state of the current room temperature
        if (tiSuccessfulUpdates > 0)
        {
            if (currentTemperature.inside >= desiredTemp)
            {
                // Turn the heater off
                heater.turnOff();
                Serial.println("Heater: I am turning off. We have reached the desired temperature.");
            }
            else
            {
                // Let the heater be
                Serial.println("Heater: I am staying on. We have not reached the desired temperature yet.");
            }
            // We do not know anything about the status inside
        }
        else
        {
            Serial.println("Heater: I am on and I do not have data on current temperature inside!");
        }
        // If the heating is off, check the inside temperature
    }
    else
    {
        if (tiSuccessfulUpdates > 0)
        {
            if (currentTemperature.inside < desiredTemp)
            {
                if (GreenProduction::hasValidData())
                {
                    if (GreenProduction::getPercentage() >= minGreen)
                    {
                        // The green production percentage is high enough to start heating
                        heater.turnOn();
                        Serial.println("Heater: I am turning on the ecologic heating now.");
                    }
                    else if (currentTemperature.inside < minTemp)
                    {
                        // It is cold inside, we should turn on the heater no matter the green production percentage
                        heater.turnOn();
                        Serial.println("Heater: Sorry man, it is too cold inside. I am turning the heater on even though it is not ecologic enough for you.");
                    }
                    else
                    {
                        Serial.println("Heater: It is not that cold here and there is not enough green energy in the system right now to start the heating.");
                    }
                    // We do not know anything about the green production status
                }
                else
                {
                    if (currentTemperature.inside < minTemp)
                    {
                        // It is cold inside, we should turn on the heater no matter the green production percentage
                        heater.turnOn();
                        Serial.println("Heater: Sorry man, it is too cold inside. I am turning the heater on even though I do not know anything about the green energy right now.");
                    }
                    else
                    {
                        Serial.println("Heater: I am off and I do not have data on green production!");
                    }
                }
                // There is no need to start heating now, we are above the desired temperature
            }
            else
            {
                Serial.println("Heater: There is no need to start heating now, we are above the desired temperature.");
            }
            // We do not know anything about the status inside
        }
        else
        {
            Serial.println("Heater: I am off and I do not have data on current temperature inside!");
        }
    }
}

// setup() runs once, when the device is first turned on.
void setup()
{
    // Put initialization like pinMode and begin functions here.
    initializeParticleVariablesAndFunctions();

    // Set timezone to Copenhagen
    Time.zone(1);

    // Subscribe to the hook responses

    GreenProduction::setup();
    OutTemp::setup();
    DHT.begin();
    heater.setup();
    if (!display.begin())
    {
        Log.info("SSD1306 allocation failed");
        //TODO: Stop the execution of the program
    }

    Serial.begin(9600);
    Serial.print("DHT LIB version: ");
    Serial.println(DHTLIB_VERSION);
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
    bool requestFinished = true;
    // The core of your code will likely live here.
    // Update once every hour, if we fail to get the data, we can perform extra update
    // Store latest data into variable and reinitialize current
    requestFinished &= GreenProduction::request(n);

    requestFinished &= OutTemp::request(n);

    if (requestFinished)
    {
        updateInsideTemperature();

        printCurrentStatus();

        handleHeating();

        currentTemperature.outside = OutTemp::getTemp();
        display.displayStatus(GreenProduction::hasValidData(), tiSuccessfulUpdates > 0, OutTemp::hasValidData(), heater.isTurnedOn(), currentTemperature, GreenProduction::getPercentage());

        Particle.publish("iotGreenHeating", "{ \"1\": \"" + String(GreenProduction::getPercentage()) + "\", \"2\":\"" + String(currentTemperature.inside) + "\", \"3\":\"" + String(OutTemp::getTemp()) + "\", \"k\":\"EFCEYN9AO5ATXI65\" }", 60, PRIVATE);

        Serial.printlnf("Sleep cycle #%d: Going to sleep for %d minutes...", n, checkPeriod);
        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::STOP)
            .duration(5s /*checkPeriod * 1min*/)
            .flag(SystemSleepFlag::WAIT_CLOUD);
        SystemSleepResult result = System.sleep(config);

        if (result.error() != 0)
        {
            Serial.printlnf("Something went wrong during #%d sleep cycle.", n);
        }
        else
        {
            Serial.printlnf("Sleep cycle #%d: Device successfully woke up from sleep.", n);
        }
        n++;
    }
}