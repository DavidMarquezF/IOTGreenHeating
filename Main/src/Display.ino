
#include "Display.h"
// Declaration for SSD1306 display connected using software SPI
#define SCREEN_WIDTH (uint8_t)128 // OLED display width, in pixels
#define SCREEN_HEIGHT (uint8_t)64 // OLED display height, in pixels
#define SCREEN_PADDING (uint8_t)4

#define OLED_MOSI (int8_t) D0
#define OLED_CLK (int8_t) D1
#define OLED_DC (int8_t) D2
#define OLED_RESET (int8_t) D3
#define OLED_CS (int8_t) D4

// based on https://www.flaticon.com/free-icon/leaf_497384
const unsigned char leafIcon [] PROGMEM = {
	// 'leaf, 48x48px
	0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x00, 0x01, 0xff, 
	0xf8, 0x0e, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x1c, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x38, 0x00, 0x03, 
	0xf0, 0x00, 0x00, 0x30, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x70, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xe0, 
	0x00, 0x78, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xf0, 0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, 
	0x01, 0x80, 0x03, 0x80, 0x00, 0x0c, 0x03, 0x80, 0x07, 0x00, 0x00, 0x1c, 0x03, 0x00, 0x0e, 0x00, 
	0x00, 0x78, 0x07, 0x00, 0x1c, 0x00, 0x01, 0xe0, 0x06, 0x00, 0x18, 0x00, 0x03, 0xc0, 0x0e, 0x00, 
	0x38, 0x00, 0x0f, 0x00, 0x0c, 0x00, 0x30, 0x00, 0x1e, 0x00, 0x0c, 0x00, 0x70, 0x00, 0x3c, 0x00, 
	0x1c, 0x00, 0x60, 0x00, 0x70, 0x00, 0x1c, 0x00, 0x60, 0x00, 0xe0, 0x00, 0x18, 0x00, 0xe0, 0x01, 
	0xc0, 0x00, 0x18, 0x00, 0xc0, 0x03, 0x80, 0x00, 0x38, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x30, 0x00, 
	0xc0, 0x0e, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x1c, 0x00, 0x00, 0x30, 0x00, 0xc0, 0x38, 0x00, 0x00, 
	0x70, 0x00, 0xc0, 0x30, 0x00, 0x00, 0x60, 0x00, 0xe0, 0x60, 0x00, 0x00, 0x60, 0x00, 0x60, 0xe0, 
	0x00, 0x00, 0xe0, 0x00, 0x61, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x71, 0x80, 0x00, 0x01, 0xc0, 0x00, 
	0x33, 0x80, 0x00, 0x01, 0x80, 0x00, 0x3f, 0x00, 0x00, 0x03, 0x80, 0x00, 0x1e, 0x00, 0x00, 0x07, 
	0x00, 0x00, 0x0e, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x1f, 0xc0, 
	0x00, 0x38, 0x00, 0x00, 0x19, 0xf8, 0x01, 0xf0, 0x00, 0x00, 0x38, 0x7f, 0xff, 0xe0, 0x00, 0x00, 
	0x30, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00
};


Display::Display(void)
:     display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS)
{
}

bool Display::begin(void){
    if(display.begin(SSD1306_SWITCHCAPVCC)){
        // Clear the buffer
        display.clearDisplay();
        display.display();
        return true;
    }
    else{
        return false;
    }
}

void Display::displayStatus(bool displayGp, bool displayTi, bool displayTa, bool heaterOn, Temperature temp, float greenProdPercentage){
    // display for 30 sec and then do to sleep
    // https://diyprojects.io/using-i2c-128x64-0-96-ssd1306-oled-display-arduino/
    
    // Clear the buffer
    display.clearDisplay();
    display.drawBitmap(0, 0, leafIcon, 48, 48, WHITE);
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    // display curent gp %
    displayGp ? display.printlnf(" %3g %% ", round(greenProdPercentage)) : display.println("  n/a %");    
    display.println("in grid");
    // draw delimiters and the grid
    display.drawLine(51, 0, 51, SCREEN_HEIGHT-1, WHITE);
    display.drawLine(51, 20, SCREEN_WIDTH-1, 20, WHITE);
    display.drawLine(51, 44, SCREEN_WIDTH-1, 44, WHITE);
    display.setCursor(54, 0);

    // display date and time
    // https://docs.particle.io/reference/device-os/firmware/photon/#setformat-
    // http://www.cplusplus.com/reference/ctime/strftime/
    time_t current = Time.now();
    display.println(Time.format(current, " %F "));
    display.setCursor(54, 8);
    display.println(Time.format(current, "    %R   "));
    display.setCursor(54, 24);
    display.println(" Heating is ");
    display.setCursor(54, 32);
    display.println(heaterOn ? "     ON" : "    OFF");
    display.setCursor(54, 48);
    displayTi ? display.printlnf(" IN:  %2.1f C", temp.inside) : display.println(" IN:  n/a C");
    display.setCursor(54, 56);
    displayTa ? display.printlnf(" OUT: %2.1f C", temp.outside) : display.println(" OUT: n/a C");
    display.display();
}