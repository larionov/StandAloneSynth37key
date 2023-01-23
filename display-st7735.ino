/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/


#ifdef DISPLAY_ST7735
#include <SPI.h>
#include <Adafruit_GFX.h>

#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#define TFT_MISO       -1
#define TFT_MOSI       11
#define TFT_SCLK       10
#define TFT_CS         9
#define TFT_RST        21 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         18

#define SCREEN_WIDTH 160 // OLED display width, in pixels
#define SCREEN_HEIGHT 80 // OLED display height, in pixels


// #define OLED_RESET      -1// Reset pin # (or -1 if sharing Arduino reset pin)
// #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
#define NUMFLAKES     10 // Number of snowflakes in the animation example
//--------_USE this definition in config.h to turn this module on/off
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

bool   wantsDisplayRefresh;  
//these are all arrays of 8 for the 4 rows and 2 columns that fit with text size 1 as set in init
String    zoneStrings[8]; //Room for 8 char (maybe more) per zone (sector)
uint8_t   zoneColor[8]; //monochrome WHITE or BLACK
uint8_t   zoneBarSize[8] = {24,0,0,0,0,0,0,0};  //could be a rectangle of 64 pix width 8 pix height

void setupst7735() {
  pinMode(45, OUTPUT);
  digitalWrite(45, HIGH);
  display.initR(INITR_MINI160x80); 
  display.setRotation(3);
  Serial.printf("init display\n");
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.fillScreen(ST77XX_GREEN);
  
}
void miniScreenString(uint8_t sector, uint8_t c, String s,bool refresh){
  zoneStrings[sector] = s;
  zoneColor[sector] = c;
  wantsDisplayRefresh |= refresh;
}
//prepares message but doesn't trigger display.display() because that messes with processing samples
//for zones on text size one they are 8 high (32 pix screen height) right half starts at 64
void miniScreenRedraw(){

  display.startWrite();
  //display.fillScreen(ST77XX_RED);

  for(int i = 0; i<8; i++){
     if(zoneStrings[i].length() > 0)
     switch(i){
        case 0:
          display.setCursor(0,0);
          //display.println(zoneStrings[0]);
          break;
        case 1:
          display.setCursor(64,0);
          //display.println(zoneStrings[1]);
          break;  
        case 2:
          display.setCursor(0,8);
          //display.println(zoneStrings[2]);
          break;  
        case 3:
          display.setCursor(64,8);
          //display.println(zoneStrings[3]);
          break;  
        case 4:
          display.setCursor(0,16);
          //display.println(zoneStrings[4]);
          break; 
        case 5:
          display.setCursor(64,16);
          //display.println(zoneStrings[5]);
          break; 
        case 6:
          display.setCursor(0,24);
          //display.println(zoneStrings[6]);
          break; 
        case 7:
          display.setCursor(36,24);
          
          break;  
     }
     if(zoneColor[i])
        display.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
     else
        display.setTextColor(ST77XX_BLACK,ST77XX_WHITE);
     
     if(zoneBarSize[i] > 0){
        miniScreenBarDraw(i);
     } else
     display.println(zoneStrings[i]);

  display.endWrite();
  }
}
void miniScreenBarSize(uint8_t sector, float param){
  
  zoneBarSize[sector] = 64.0f * param;
  miniScreenRedraw();
}

void miniScreenBarDraw(uint8_t sector){
   int x,y;
   if(sector>0) y = (sector / 2) * 8; else y = 0;
   x = (sector % 2) *64;
   display.startWrite();
   display.drawRect(x,y, zoneBarSize[sector], 7, ST77XX_WHITE);
   display.setCursor(x,y);
    
   display.setTextColor(ST77XX_BLACK,ST77XX_WHITE);
   uint8_t fitInBar = zoneBarSize[sector]/7; //numb of characters to fit in bar
   display.print(zoneStrings[sector].substring(0,fitInBar)); //reprint the text  myString.substring(from, to)
   display.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
   display.print(zoneStrings[sector].substring(fitInBar)); 
   
   display.endWrite();

    
 
  //  display.display(); is called by displayRefresh on core0task loop but this loading the display memory can be called any time
}



void displayRefresh()
{
  return;
  if(wantsDisplayRefresh){ 
    wantsDisplayRefresh = LOW;
    //display.display();
    miniScreenRedraw();
  
  }
}
#else 
void miniScreenString(uint8_t sector, uint8_t c, String s,bool refresh){}
void miniScreenBarSize(uint8_t sector, float param){};  
#endif
