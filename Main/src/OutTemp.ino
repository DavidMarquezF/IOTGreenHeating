#include "HttpDataHook.h"
#include "OutTemp.h"

// Define macros for better readability
#define TA_HOOK_RESP "hook-response/temperatureAarhus"
#define TA_HOOK_PUB "temperatureAarhus"

static HttpDataHook temperatueOutisdeHttp;
static uint16_t currentId;
static bool firstId;
static int taSuccessfulUpdates = 0;

static float lastOutTemp;

static void handleOutsideTempResponse(const char *event, String *data)
{
    lastOutTemp = data->toFloat();
    taSuccessfulUpdates++;
}

static void handleOutsideTempError(error_code_t code)
{
    if (taSuccessfulUpdates > 0)
    {
        Serial.println("Using latest valid data...");
    }
    else
    {
        // We cannot show anything, because there is no valid data
        Serial.println("We cannot show anything, because there is no valid data!");
    }
}



namespace OutTemp
{
    void setup(void)
    {
        taSuccessfulUpdates = 0;
    temperatueOutisdeHttp.begin(TA_HOOK_RESP, TA_HOOK_PUB, handleOutsideTempResponse, handleOutsideTempError);
        firstId = true;
    }

    bool request(uint16_t id)
    {
        if(!firstId && currentId == id){
            return !temperatueOutisdeHttp.isWaiting();
        }
        temperatueOutisdeHttp.request();
        currentId = id;
        firstId = false;
        return !temperatueOutisdeHttp.isWaiting();
    }
    float getTemp(void)
    {
        return taSuccessfulUpdates > 0 ? lastOutTemp : -1;
    }
    bool hasValidData(void)
    {
        return taSuccessfulUpdates > 0;
    }



} // namespace GreenProduction
