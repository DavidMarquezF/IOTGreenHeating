#ifndef _HTTP_DATA_H_
#define _HTTP_DATA_H_
typedef uint16_t error_code_t ;
typedef void (*response_callback_t) (const char * event, String*  data);
typedef void (*error_callback_t) (error_code_t errorCode);

class HttpDataHook{
    public:
        void begin(const char hookSub[], const char hookPub[], response_callback_t response_callback, error_callback_t error_callback, uint8_t retries = 5);
        void request(void);
        bool isWaiting(void);
    private:
        char _hook[30];
        bool _waiting;
        uint8_t _retries;
        uint8_t _curr_retry;
        response_callback_t _response_callback;
        error_callback_t _error_callback;
        void handleResponse(const char * event, const char * data);
        void sendRequest();

};

#endif