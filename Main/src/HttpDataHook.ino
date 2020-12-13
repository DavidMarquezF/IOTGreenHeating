#include "HttpDataHook.h"
#include <string.h>

void HttpDataHook::begin(const char hookSub[], const char hookPub[], response_callback_t response_callback, error_callback_t error_callback, uint8_t retries)
{
    strcpy(_hook, hookPub);
    _response_callback = response_callback;
    _error_callback = error_callback;
    _retries = retries;
    Particle.subscribe(hookSub, &HttpDataHook::handleResponse, this, MY_DEVICES);
}
void HttpDataHook::request()
{
    if (_waiting)
        return;
    _waiting = true;
    _curr_retry = 0;
    sendRequest();
}

bool HttpDataHook::isWaiting(void){
    return _waiting;
}

void HttpDataHook::sendRequest()
{
    Particle.publish(_hook, PRIVATE);
}

void HttpDataHook::handleResponse(const char *event, const char *data)
{
    Serial.println(Time.timeStr());
    Serial.println("Handle " + String(_hook) + " ---> Got response containing: " + String(data));

    // Although it never happen that we had invalid data, we should check the validity of the response
    String str = String(data);
    if (str != NULL)
    {
        // We have valid data
        _waiting = false;
        if(_response_callback)
            _response_callback(event, &str);
    }
    else
    {
        Serial.printf("Call #%d: TA: Invalid data.\n", _retries);
        if (_curr_retry < _retries)
        {
            _curr_retry++;
            sendRequest();
        }
        else{
            _waiting = false;
            if(_error_callback)
                _error_callback(0);
        }
    }
}