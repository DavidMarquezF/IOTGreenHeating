#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "PietteTech_DHT.h"

#define SCREEN_WIDTH (uint8_t)128 // OLED display width, in pixels
#define SCREEN_HEIGHT (uint8_t)64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI
#define OLED_MOSI (int8_t) D0
#define OLED_CLK (int8_t) D1
#define OLED_DC (int8_t) D2
#define OLED_RESET (int8_t) D3
#define OLED_CS (int8_t) D4

#define DHTTYPE  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   D5          // Digital pin for communications

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
SerialLogHandler logHandler;

// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE);
int n;      // counter

void setup()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC))
    {
        Log.info("SSD1306 allocation failed");
        //TODO: Stop the execution of the program
    }
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    display.display();
    // text display tests
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.printlnf("Temperature: %d", 23);
    display.display();
    Particle.subscribe("hook-response/greenproduction", handleGreenProduction, MY_DEVICES);

    Serial.begin(9600);
    while (!Serial.available() && millis() < 30000) {
        Serial.println("Press any key to start.");
        Particle.process();
        delay(1000);
    }
    Serial.println("DHT Simple program using DHT.acquireAndWait");
    Serial.print("LIB version: ");
    Serial.println(DHTLIB_VERSION);
    Serial.println("---------------");
    DHT.begin();
}

void loop()
{
    Particle.publish("greenproduction", PRIVATE);
    delay(15000);
}

void showDataFromSensor() {
    Serial.print("\n");
    Serial.print(n);
    Serial.print(": Retrieving information from sensor: ");
    Serial.print("Read sensor: ");

    int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)

    switch (result) {
    case DHTLIB_OK:
        Serial.println("OK");
        break;
    case DHTLIB_ERROR_CHECKSUM:
        Serial.println("Error\n\r\tChecksum error");
        break;
    case DHTLIB_ERROR_ISR_TIMEOUT:
        Serial.println("Error\n\r\tISR time out error");
        break;
    case DHTLIB_ERROR_RESPONSE_TIMEOUT:
        Serial.println("Error\n\r\tResponse time out error");
        break;
    case DHTLIB_ERROR_DATA_TIMEOUT:
        Serial.println("Error\n\r\tData time out error");
        break;
    case DHTLIB_ERROR_ACQUIRING:
        Serial.println("Error\n\r\tAcquiring");
        break;
    case DHTLIB_ERROR_DELTA:
        Serial.println("Error\n\r\tDelta time to small");
        break;
    case DHTLIB_ERROR_NOTSTARTED:
        Serial.println("Error\n\r\tNot started");
        break;
    default:
        Serial.println("Unknown error");
        break;
    }
    Serial.print("Humidity (%): ");
    Serial.println(DHT.getHumidity(), 2);
    
    display.printlnf("Humidity indoor: %.2f", DHT.getHumidity());

    Serial.print("Temperature (oC): ");
    Serial.println(DHT.getCelsius(), 2);

    display.printlnf("Temperature indoor: %.2f C", DHT.getCelsius());

    Serial.print("Temperature (oF): ");
    Serial.println(DHT.getFahrenheit(), 2);

    Serial.print("Temperature (K): ");
    Serial.println(DHT.getKelvin(), 2);

    Serial.print("Dew Point (oC): ");
    Serial.println(DHT.getDewPoint());

    Serial.print("Dew Point Slow (oC): ");
    Serial.println(DHT.getDewPointSlow());

    n++;
}

void handleGreenProduction(const char *event, const char *data)
{
    display.clearDisplay();
    display.setCursor(0, 0);

    float total, biomass, solarpower, offshorewindpower;

    // %f doesn't work in particle: https://github.com/particle-iot/device-os/issues/1124
    // The current workaround is to do it as a string
    //int result = sscanf(data, "%[^~]~%[^~]~%[^~]~%[^~]", &total, &biomass, &solarpower, &offshorewindpower);
    //Log.info("%s", data);
    //Log.info("%d", result);

    //Idea from https://community.particle.io/t/tutorial-webhooks-and-responses-with-parsing-json-mustache-tokens/14612/21
    String str = String(data);
    char strBuffer[500] = "";
    str.toCharArray(strBuffer, 500);
    total = String(strtok(strBuffer, "~")).toFloat();
    biomass = String(strtok(NULL, "~")).toFloat();
    solarpower = String(strtok(NULL, "~")).toFloat();
    offshorewindpower = String(strtok(NULL, "~")).toFloat();
    
    display.printlnf("Total: %.2f", total);
    display.printlnf("Biomass: %.2f", biomass);
    display.printlnf("Solar power: %.2f", solarpower);
    display.printlnf("Off. Wind: %.2f", offshorewindpower);

    showDataFromSensor();

    display.display();
}
