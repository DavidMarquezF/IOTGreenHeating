#include "GreenProduction.h"
#include "HttpDataHook.h"

#define GP_HOOK_RESP "hook-response/greenproduction"
#define GP_HOOK_PUB "greenproduction"
#define INVALID_HOUR ((uint8_t)255)
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

static GreenProdData currentGreenProduction;

static HttpDataHook greenProdHttp;
static int gpSuccessfulUpdates;
static uint8_t gpUpdateHour;

static void handleGreenProdResponse(const char *event, String *data)
{
    if (data->indexOf("~~") == -1 && !data->startsWith("~"))
    {
        // We have valid data
        Serial.println("GP: Valid data.");
        char strBuffer[256] = "";
        data->toCharArray(strBuffer, 256);
        int currentId = String(strtok(strBuffer, "~")).toInt();
        // If ids match, we already have the most recent data
        if (currentGreenProduction._id == currentId)
        {
            Serial.printf("The ids %d and %d match, aborting.\n", currentGreenProduction._id, currentId);
            return;
        }
        else
        {
            currentGreenProduction._id = currentId;
            currentGreenProduction.totalLoad = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.onshoreWind = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.offshoreWind = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.biomass = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.solarPower = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.hydroPower = String(strtok(NULL, "~")).toFloat();
            currentGreenProduction.otherRenewable = String(strtok(NULL, "~")).toFloat();
        }
        
        gpSuccessfulUpdates++;
        gpUpdateHour = Time.hour();
        currentGreenProduction.percentage = 100 * (currentGreenProduction.onshoreWind + currentGreenProduction.offshoreWind + currentGreenProduction.biomass + currentGreenProduction.solarPower + currentGreenProduction.hydroPower + currentGreenProduction.otherRenewable) / currentGreenProduction.totalLoad;
        Serial.printf("Current green production percentage is: %.2f %%\n", currentGreenProduction.percentage);
    }
}

static void handleGrenProdError(error_code_t code)
{
    Serial.println("Green production webhook failed!");
    if (gpSuccessfulUpdates > 0)
    {
        Serial.println("Using latest valid data...");
    }
    else
    {
        // We cannot show anything, because there is no valid data
        Serial.println("We cannot show anything, because there is no valid data!");
    }
}

static bool shouldRequest(void)
{
    return gpUpdateHour == INVALID_HOUR || gpSuccessfulUpdates <= 0 || Time.hour() > gpUpdateHour || (Time.hour() == 0 && gpUpdateHour == 23);
}

static uint16_t currentId;
static bool firstId;
namespace GreenProduction
{
    void setup(void)
    {
        gpSuccessfulUpdates = 0;
        gpUpdateHour = INVALID_HOUR;
        greenProdHttp.begin(GP_HOOK_RESP, GP_HOOK_PUB, handleGreenProdResponse, handleGrenProdError);
        firstId = true;
    }

    bool request(uint16_t id)
    {
        if(!firstId && currentId == id){
            return !greenProdHttp.isWaiting();
        }
        if (!shouldRequest())
            return true;
        greenProdHttp.request();
        currentId = id;
        firstId = false;
        return !greenProdHttp.isWaiting();
    }
    float getPercentage(void)
    {
        return gpSuccessfulUpdates > 0 ? currentGreenProduction.percentage : -1;
    }
    bool hasValidData(void)
    {
        return gpSuccessfulUpdates > 0;
    }

    void displayInfo(void)
    {
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
    }

} // namespace GreenProduction
