#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define SCREEN_WIDTH (uint8_t)128 // OLED display width, in pixels
#define SCREEN_HEIGHT (uint8_t)64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI
#define OLED_MOSI (int8_t) D0
#define OLED_CLK (int8_t) D1
#define OLED_DC (int8_t) D2
#define OLED_RESET (int8_t) D3
#define OLED_CS (int8_t) D4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
SerialLogHandler logHandler;

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
    delay(1000); // Pause for 2 seconds

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
}

void loop()
{
}
