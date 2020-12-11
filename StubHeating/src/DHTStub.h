#ifndef DHT_H
#define DHT_H

/* Uncomment to enable printing out nice debug messages. */
//#define DHT_DEBUG
//#define DHT_DEBUG_VERB
#define DEBUG_PRINTER                                                          \
  Serial /**< Define where debug output will be printed.                       \
          */

/* Setup debug printing macros. */
#ifdef DHT_DEBUG
#define DEBUG_PRINT(...)                                                       \
  { DEBUG_PRINTER.print(__VA_ARGS__); }
#define DEBUG_PRINTLN(...)                                                     \
  { DEBUG_PRINTER.println(__VA_ARGS__); }
#   ifdef DHT_DEBUG_VERB
    #define DEBUG_VERB_PRINT(...)                                                       \
  { DEBUG_PRINTER.print(__VA_ARGS__); }
    #define DEBUG_VERB_PRINTLN(...)                                                     \
  { DEBUG_PRINTER.println(__VA_ARGS__); }
#   else
    #define DEBUG_VERB_PRINT(...)                                                       \
    {} /**< Debug Print Placeholder if Debug is disabled */
    #define DEBUG_VERB_PRINTLN(...)                                                     \
    {} /**< Debug Print Line Placeholder if Debug is disabled */

#   endif
#else
#define DEBUG_PRINT(...)                                                       \
  {} /**< Debug Print Placeholder if Debug is disabled */
#define DEBUG_PRINTLN(...)                                                     \
  {} /**< Debug Print Line Placeholder if Debug is disabled */
#define DEBUG_VERB_PRINT(...)                                                       \
    {} /**< Debug Print Placeholder if Debug is disabled */
#define DEBUG_VERB_PRINTLN(...)                                                     \
    {} /**< Debug Print Line Placeholder if Debug is disabled */

#endif


class DHTStub {
    public:
        DHTStub(uint16_t pin);
        void begin(uint8_t usec = 80);
        void updateTemp(float temp);
        void updateHumid(float humid);
    private:
        uint8_t _pullTime;
        uint16_t _pin;
        uint16_t _temp;
        uint16_t _humid;
        uint16_t _maxCycles;
        void sendData();
        void handleFallingInterrupt(void);
        void attachReadInterrupt(void);
        void detatchReadInterrupt(void);
        void sendWord(uint16_t value);
        void sendByte(uint8_t value);
        uint16_t expectPulse(bool level);

};


#endif