/* October 16 2020
 * This is not my code but I have modified it from the original as posted in 
 * https://www.instructables.com/ESP32ESP8266-Weather-forecasterPredictor/  
 * You will find my hardware list and pin map on this post as nz-macca
 *
 * My changes in this sketch are:-
 * #1 Upgraded to a 2.4" OLED and changed from I2C to SPI interface. You can still use the 0.9" driver
 * #2 Added extra screen for Room Temperature display
 * #3 Added code and library for timer to create a screen timeout (5min)
 * #4 Modifyied the NTP config so it knows your region and will auto adjust for daylight savings changes
 * #5 I also changed the screen scroll delay in the OLEDDisplayUi.cpp library file.
 * # Timer library  https://github.com/contrem/arduino-timer
 * # OLED UI Library used  https://github.com/helmut64/OLED_SSD1306
 * # NTP MODS from  https://www.bitsnblobs.com/network-time-using-esp8266 
 * #########################################################################
 * 
 * // Original Header
 * Last udpate: Added enumerated weather types, improved efficiency
 * Last update: 07-Aug-17, with improved forecast rules
 * 
 * ESP8266 and BMP180 or BME280 and OLED SH1106 or SSD1306 display Weather Forecaster 
 * Using air pressure changes to predict weather based on an advanced set of forecasting rules. 
 * The 'MIT License (MIT) Copyright (c) 2016 by David Bird'. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish,  
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the 
 * following conditions:    
 * The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the   
 * software use is visible to an end-user.  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER    
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * See more at http://dsbird.org.uk
 * 
 */

#include <SPI.h>                //Needed as I'm using and SPI OLED not I2C OLED
#include <arduino-timer.h>      //https://github.com/contrem/arduino-timer // Library added to create screen sleep
//#include "SSD1306.h"          //https://github.com/helmut64/OLED_SSD1306 // For I2C Display
#include "SSD1306Spi.h"         //https://github.com/helmut64/OLED_SSD1306 // For SPI Display
#include "OLEDDisplayUi.h"      //https://github.com/helmut64/OLED_SSD1306
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <Wire.h>               //Original sketch only needed Wire lib for sensor and OLED
#include <Adafruit_BMP280.h>
//#include <Adafruit_BMP085.h> // If using BMP180 or BMP085 and update line:205 accordingly e.g. BME and BME or BMP and BMP 

#define OLED_DC 4
#define OLED_CS 5
#define OLED_RES 2

#define icon_width  40
#define icon_height 40

// Define each of the *icons for display
const uint8_t rain_icon[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x81, 0xFF, 0xFF, 0xFF, 0x3F, 0x04, 0xFE, 0xFF, 0xFF, 0xDF, 0xF0, 0xFC, 
  0xFF, 0xFF, 0xE7, 0xFF, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xF3, 0xFF, 0xFF, 
  0xFD, 0xFF, 0xE7, 0xFF, 0xFF, 0xFD, 0xFF, 0x0F, 0xFE, 0x0F, 0xF8, 0xFF, 
  0x8F, 0xFC, 0xE7, 0xFB, 0xFF, 0xC7, 0xF9, 0xF7, 0xE3, 0xFF, 0xC3, 0xF3, 
  0xF3, 0xDF, 0xFF, 0xE8, 0xE7, 0xF9, 0xFF, 0xFF, 0xFE, 0xEF, 0xFD, 0xFF, 
  0xFF, 0xFF, 0xE7, 0xF9, 0xFF, 0xFF, 0xFF, 0xF7, 0xFB, 0xFF, 0xFF, 0xFF, 
  0xF3, 0xFB, 0xFF, 0xFF, 0xFF, 0xF9, 0xF7, 0xFF, 0xFF, 0xFF, 0xFC, 0xCF, 
  0xFF, 0xFF, 0x3F, 0xFE, 0x3F, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0x7D, 0xBF, 
  0xEF, 0xFF, 0xFF, 0xBE, 0xDF, 0xF7, 0xFF, 0x7F, 0xDF, 0xEF, 0xFB, 0xFF, 
  0xBF, 0xEF, 0xF7, 0xFD, 0xFF, 0xDF, 0xF7, 0xFB, 0xFE, 0xFF, 0xEF, 0xFB, 
  0x7D, 0xFF, 0xFF, 0xF7, 0xFD, 0xBE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const uint8_t sunny_icon[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xE3, 0xFF, 0xFD, 0xDF, 0xFF, 0xE3, 0xFF, 0xF8, 0x8F, 
  0xFF, 0xE3, 0x7F, 0xF0, 0x07, 0xFF, 0xE3, 0x3F, 0xF8, 0x0F, 0xFE, 0xFF, 
  0x1F, 0xFC, 0x1F, 0x7C, 0x00, 0x0E, 0xFE, 0x3F, 0x18, 0x00, 0x1C, 0xFF, 
  0x7F, 0xCC, 0xFF, 0xB1, 0xFF, 0xFF, 0xE6, 0xFF, 0xE7, 0xFF, 0xFF, 0xF3, 
  0xFF, 0xCF, 0xFF, 0xFF, 0xF1, 0xFF, 0x9F, 0xFF, 0xFF, 0xF9, 0xFF, 0x9F, 
  0xFF, 0xFF, 0xF9, 0xFF, 0x9F, 0xFF, 0xFF, 0xF9, 0xFF, 0x9F, 0xFF, 0x01, 
  0xF9, 0xFF, 0x9F, 0x80, 0x01, 0xF9, 0xFF, 0x9F, 0x80, 0x01, 0xF9, 0xFF, 
  0x9F, 0x80, 0xFF, 0xF9, 0xFF, 0x9F, 0xFF, 0xFF, 0xF1, 0xFF, 0x8F, 0xFF, 
  0xFF, 0xF1, 0xFF, 0x8F, 0xFF, 0xFF, 0xE3, 0xFF, 0xE7, 0xFF, 0xFF, 0xC7, 
  0xFF, 0xF3, 0xFF, 0xFF, 0x8D, 0xFF, 0xD8, 0xFF, 0xFF, 0x38, 0x00, 0x8C, 
  0xFF, 0x7F, 0x70, 0x00, 0x07, 0xFF, 0x3F, 0xF8, 0xFF, 0x0F, 0xFE, 0x1F, 
  0xFC, 0xE3, 0x1F, 0xFC, 0x0F, 0xFE, 0xE3, 0x3F, 0xF8, 0x07, 0xFF, 0xE3, 
  0x7F, 0xF0, 0x8F, 0xFF, 0xE3, 0xFF, 0xF8, 0xDF, 0xFF, 0xE3, 0xFF, 0xFD, 
  0xFF, 0xFF, 0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xE3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const uint8_t mostlysunny_icon[] PROGMEM = {
  0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFF, 0xFF, 0xFD, 0x7E, 
  0xFF, 0xFF, 0xFF, 0xFB, 0xBF, 0xEF, 0xFF, 0xFF, 0x17, 0xE0, 0xF7, 0xFF, 
  0xFF, 0xCF, 0x9F, 0xF9, 0xFF, 0xFF, 0xE6, 0x3F, 0xFD, 0xFF, 0xFF, 0xF5, 
  0x7F, 0xFF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFE, 0xFF, 0xFF, 0xF9, 0xFF, 0x00, 
  0xFF, 0xFF, 0xFD, 0x7F, 0x08, 0xFC, 0xFF, 0xFD, 0xBF, 0xE1, 0xF9, 0xFF, 
  0xFD, 0xCF, 0xFF, 0xF7, 0xFF, 0xFD, 0xF7, 0xFF, 0xE7, 0xFF, 0xF9, 0xFB, 
  0xFF, 0xCF, 0xFF, 0xF3, 0xFB, 0xFF, 0x1F, 0xFC, 0x17, 0xF0, 0xFF, 0x1F, 
  0xF9, 0xC7, 0xF7, 0xFF, 0x8F, 0xF3, 0xEF, 0xC7, 0xFF, 0x87, 0xE7, 0xE7, 
  0xBF, 0xFF, 0xD1, 0xCF, 0xF3, 0xFF, 0xFF, 0xFD, 0xDF, 0xFB, 0xFF, 0xFF, 
  0xFF, 0xCF, 0xF3, 0xFF, 0xFF, 0xFF, 0xEF, 0xF7, 0xFF, 0xFF, 0xFF, 0xE7, 
  0xF7, 0xFF, 0xFF, 0xFF, 0xF3, 0xEF, 0xFF, 0xFF, 0xFF, 0xF9, 0x9F, 0xFF, 
  0xFF, 0x7F, 0xFC, 0x7F, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  
const uint8_t cloudy_icon[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0x7F, 0x78, 0xFC, 0xFF, 
  0xFF, 0xBF, 0xFF, 0xF9, 0xFF, 0xFF, 0xCF, 0xFF, 0xF7, 0xFF, 0xFF, 0xF7, 
  0xFF, 0xE7, 0xFF, 0xFF, 0xFB, 0xFF, 0xCF, 0xFF, 0xFF, 0xFB, 0xFF, 0x1F, 
  0xFC, 0x3F, 0xF0, 0xFF, 0xE7, 0xFB, 0xCF, 0xF7, 0xFF, 0xF3, 0xF7, 0xEF, 
  0xCF, 0xFF, 0xF9, 0xEF, 0xF7, 0xBF, 0xFF, 0xFD, 0xCF, 0xF3, 0xFF, 0xFF, 
  0xFD, 0xDF, 0xFB, 0xFF, 0xFF, 0xFF, 0xDF, 0xFB, 0xFF, 0xFF, 0xFF, 0xEF, 
  0xF7, 0xFF, 0xFF, 0xFF, 0xE7, 0xF7, 0xFF, 0xFF, 0xFF, 0xF3, 0xEF, 0xFF, 
  0xFF, 0xFF, 0xF9, 0x9F, 0xFF, 0xFF, 0x7F, 0xFC, 0x7F, 0x00, 0x00, 0x00, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const uint8_t tstorms_icon[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x81, 0xFF, 0xFF, 0xFF, 0x3F, 0x04, 0xFE, 0xFF, 0xFF, 0xDF, 0xF0, 0xFC, 
  0xFF, 0xFF, 0xE7, 0xFF, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xF3, 0xFF, 0xFF, 
  0xFD, 0xFF, 0xE7, 0xFF, 0xFF, 0xFD, 0xFF, 0x0F, 0xFE, 0x0F, 0xF8, 0xFF, 
  0x8F, 0xFC, 0xE7, 0xFB, 0xFF, 0xC7, 0xF9, 0xF7, 0xE3, 0xFF, 0xC3, 0xF3, 
  0xF3, 0xDF, 0xFF, 0xE8, 0xE7, 0xF9, 0xFF, 0xFF, 0xFE, 0xEF, 0xFD, 0xFF, 
  0xFF, 0xFF, 0xE7, 0xF9, 0xFF, 0xFF, 0xFF, 0xF7, 0xFB, 0xFF, 0xFF, 0xFF, 
  0xF3, 0xFB, 0xFF, 0xFF, 0xFF, 0xF9, 0xF7, 0xFF, 0xFF, 0xFF, 0xFC, 0xCF, 
  0xFF, 0xFF, 0x3F, 0xFE, 0x3F, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0x7F, 0x7F, 
  0xFF, 0xFF, 0xFF, 0xBF, 0xBF, 0xFF, 0xFF, 0xFF, 0xDF, 0x8F, 0xFF, 0xFF, 
  0xFF, 0xEF, 0xF7, 0xFF, 0xFF, 0xFF, 0xF7, 0xFB, 0xFF, 0xFF, 0xFF, 0x4F, 
  0xF7, 0xFF, 0xFF, 0xFF, 0xBF, 0xEF, 0xFF, 0xFF, 0xFF, 0xDF, 0xF1, 0xFF, 
  0xFF, 0xFF, 0xDF, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x8F, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };
  
String      time_str, weather_text, weather_extra_text;
int         last_reading_hour, reading_hour, hr_cnt;
const char* ssid     = "MillFlat_El_Rancho";
const char* password = "140824500925";
enum image_names { // enumerated table used to point to images
                  rain_img, sunny_img, mostlysunny_img, cloudy_img, tstorms_img,
                 } image;

//NTP TZ_INFO for Auto Zone and Daylight savings Offsets
// This from Github    NZST-12NZDT,M9.5.0,M4.1.0/3  -- NZST-12NZDT-13,M10.1.0/02:00:00,M3.3.0/03:00:00
// from <https://remotemonitoringsystems.ca/time-zone-abbreviations.php>
//If the above doesn't work try https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* NTP_SERVER = "pool.ntp.org";  
const char* TZ_INFO    = "NZST-12NZDT,M9.5.0,M4.1.0/3"; 

// Define and enumerated type and assign values to expected weather types.
// These values help to determine the average weather preceeding a 'no-change' forecast e.g. rain, rain then mostlysun = -1 (-1 + -1 + 1) resulting on balance = more rain
enum weather_type {unknown     =  4,
                   sunny       =  2,
                   mostlysunny =  1,
                   cloudy      =  0,
                   rain        = -1,
                   tstorms     = -2
                   };

enum weather_description {GoodClearWeather, BecomingClearer,
                          NoChange, ClearSpells, ClearingWithin12hrs, ClearingAndColder,
                          GettingWarmer, WarmerIn2daysRainLikely,
                          ExpectRain, WarmerRainWithin36hrs, RainIn18hrs, RainHighWindsClearAndCool,
                          GalesHeavyRainSnowInWinter
                          };

weather_type current_wx; // Enable the current wx to be recorded

// An array structure to record pressure, temperaturre, humidity and weather state
typedef struct {
  float pressure;            // air pressure at the designated hour
  float temperature;         // temperature at the designated hour
  float humidity;            // humidity at the designated hour
  weather_type wx_state_1hr; // weather state at 1-hour
  weather_type wx_state_3hr; // weather state at 3-hour point
} wx_record_type;

wx_record_type reading[24]; // An array covering 24-hours to enable P, T, % and Wx state to be recorded for every hour

int wx_average_1hr, wx_average_3hr; // Indicators of average weather

bool look_3hr = true;
bool look_1hr = false;

//SSD1306 display(0x3c); // OLED display object definition (address, SDA, SCL)
SSD1306Spi display(OLED_RES, OLED_DC, OLED_CS);                                 //Because I'm using SPI OLED
OLEDDisplayUi ui     ( &display );

Adafruit_BMP280 bmp;                                                            //BMP Sensor object config

// Timer and Button configuration
Timer<> timer;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long screenTimeOut = 300000; // 5 minute screen timeout
#define buttonPin 12
int buttonReading, buttonState, lastButtonState, screenState; 

WiFiClient client; // wifi client object

// Used to adjust sensor reading to correct pressure for your location
#define pressure_offset 3.8  //5.7 has 1.7 variation to AKL Airport

/////////////////////////////////////////////////////////////////////////
// What's displayed along the top line - NTP Date and Time Overlay
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,0, time_str.substring(0,8));  //HH:MM:SS Sat 05-07-17
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128,0, time_str.substring(9));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// This frame draws a weather icon based on 3-hours of data for the prediction
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  float trend  = reading[23].pressure - reading[20].pressure;                          // Trend over the last 3-hours
  ForecastToImgTxt(get_forecast_text(reading[23].pressure, trend, look_3hr));          // From forecast and trend determine what image to display
  if (image == rain_img) display->drawXbm(x+0,y+15, icon_width, icon_height, rain_icon);               // Display corresponding image
  if (image == sunny_img) display->drawXbm(x+0,y+15, icon_width, icon_height, sunny_icon);             // Display corresponding image
  if (image == mostlysunny_img) display->drawXbm(x+0,y+15, icon_width, icon_height, mostlysunny_icon); // Display corresponding image
  if (image == cloudy_img) display->drawXbm(x+0,y+15, icon_width, icon_height, cloudy_icon);           // Display corresponding image
  if (image == tstorms_img) display->drawXbm(x+0,y+15, icon_width, icon_height, tstorms_icon);         // Display corresponding image
  display->drawStringMaxWidth(x+45,y+12,90,String(reading[23].pressure,1)+" hPA");     // Show current air pressure 
  display->drawStringMaxWidth(x+45,y+25,90,String(trend,1)+" "+get_trend_text(trend)); // and pressure trend
}

// This frame shows a weather description based on 3-hours of data for the prediction
void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  float  trend = reading[23].pressure - reading[20].pressure;                             // Get current trend over last 3-hours
  weather_description wx_text = get_forecast_text(reading[23].pressure, trend, look_3hr); // Convert to forecast text based on 3-hours
  ForecastToImgTxt(wx_text);                                                              // Display corresponding text
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x+0,y+10,127,weather_text);
  display->setFont(ArialMT_Plain_10);
}

// This frame draws a graph of pressure (delta) change for the last 24-hours, see Annex* for more details
void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int gwidth   = 75; // Graph width in pixels
  int gscale   = 30; // Graph height in pixels
  int num_bars = 8;  // Number of bars to display
  #define yscale 8   // Graph +/- y-axis scale  e.g. 8 displays +/-8 and scales data accordingly
  float bar_width = gwidth / (num_bars+1); // Determine bar width based on graph width
  x = 30; // Sets position of graph on screen
  y = 15; // Sets position of graph on screen
  display->drawVerticalLine(x, y, gscale+1);
  display->drawString(x-18,y-6,">+"+String(yscale));
  display->drawString(x-8,y+gscale/2-6,"0");
  display->drawString(x-15,y+gscale-6,"<-"+String(yscale));
  display->drawString(x-30,y+gscale/2-6,String(hr_cnt%24));
  display->drawString(x+2+(bar_width+3)*0, y+gscale,"-24");  // 24hr marker at bar 0
  display->drawString(x+2+(bar_width+3)*2, y+gscale,"-12");  // 12hr marker at bar 2
  display->drawString(x+2+(bar_width+3)*5, y+gscale,"-2");   // 2hr  marker at bar 5
  display->drawString(x+2+(bar_width+3)*7, y+gscale,"0");    // 0hr  marker at bar 7
  int display_points [8] = {0,5,11,17,20,21,22,23}; // Only display time for hours 0,5,11,17,20,21,22,23
  float value;
  for (int bar_num = 0; bar_num < num_bars; bar_num++){      // Now display a bar at each hour position -24, -18, -12, -6, -3, -2, -1 and 0 hour
    value = map(reading[display_points[bar_num]].pressure, reading[23].pressure-yscale, reading[23].pressure+yscale, gscale, 0);
    if (value > gscale) value = gscale;                      // Screen scale is 0 to e.g. 40pixels, this stops drawing beyond graph bounds
    if (value < 0     ) value = 0;                           // 0 is top of graph, this stops drawing beyond graph bounds
    display->drawHorizontalLine(x+bar_num*(bar_width+3)+2, y+value, bar_width);
    for (int yplus=gscale; yplus > value; yplus = yplus - 1) {
      display->drawHorizontalLine(x+bar_num*(bar_width+3)+2, y + yplus, bar_width);
    }
  }
}

// This frame draws a weather icon based on 1-hour of data for the prediction
void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  reading[23].pressure = (reading[23].pressure + read_pressure())/2;                 // Update rolling average, gets reset on the hour transition
  float  trend = reading[23].pressure - reading[22].pressure;                        // Get short-term trend for the last 1-hour
  weather_description wx_text = get_forecast_text(read_pressure(), trend, look_1hr); // Convert to forecast text based on 1-hours
  ForecastToImgTxt(wx_text);
  if (image == rain_img) display->drawXbm(x+0,y+15, icon_width, icon_height, rain_icon);               // Display corresponding image
  if (image == sunny_img) display->drawXbm(x+0,y+15, icon_width, icon_height, sunny_icon);             // Display corresponding image
  if (image == mostlysunny_img) display->drawXbm(x+0,y+15, icon_width, icon_height, mostlysunny_icon); // Display corresponding image
  if (image == cloudy_img) display->drawXbm(x+0,y+15, icon_width, icon_height, cloudy_icon);           // Display corresponding image
  if (image == tstorms_img) display->drawXbm(x+0,y+15, icon_width, icon_height, tstorms_icon);         // Display corresponding image
  display->drawStringMaxWidth(x+45,y+12,90,"1-Hr forecast");
  display->drawStringMaxWidth(x+45,y+22,90,String(read_pressure(),1)+" hPA");
  display->drawStringMaxWidth(x+47,y+32,90,String(trend,1)+" "+get_trend_text(trend));
}

// This frame shows a weather description based on 1-hour of data for the prediction
void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  reading[23].pressure = (reading[23].pressure + read_pressure())/2;                 // Update rolling average
  float  trend = reading[23].pressure - reading[22].pressure;                        // Get short-term trend
  weather_description wx_text = get_forecast_text(read_pressure(), trend, look_1hr); // Convert to forecast text based on 1-hours
  ForecastToImgTxt(wx_text);
  display->drawString(x+0,y+10,"Short-term forecast:");
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x+0,y+18,127,weather_text);
  display->setFont(ArialMT_Plain_10);
}
// Added this fram to display Room Temp
void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawString(x+20,y+10,"Room Temperature:");
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x+30,y+22,97,String(read_temperature())+(" *C"));
  display->setFont(ArialMT_Plain_10);
  //display->drawString(x+20,y+40,("Reset Count: ")+String(countDisplay())); 
}

float read_pressure(){
  int reading = (bmp.readPressure()/100.0F+pressure_offset)*10; // Rounded result to 1-decimal place
  return (float)reading/10;
}

float read_temperature(){                                       // Added for a Local Temp reading
  int reading = bmp.readTemperature()*100;
  return (float)reading /100;
}

// Convert pressure trend to text
String get_trend_text(float trend){
  String trend_str = "Steady"; // Default weather state
  if (trend > 3.5)                          { trend_str = "Rising fast";  }
  else if (trend >   1.5  && trend <= 3.5)  { trend_str = "Rising";       }
  else if (trend >   0.25 && trend <= 1.5)  { trend_str = "Rising slow";  }
  else if (trend >  -0.25 && trend <  0.25) { trend_str = "Steady";       }
  else if (trend >= -1.5  && trend < -0.25) { trend_str = "Falling slow"; }
  else if (trend >= -3.5  && trend < -1.5)  { trend_str = "Falling";      }
  else if (trend <= -3.5)                   { trend_str = "Falling fast"; }
  return trend_str;
}

// Convert forecast text to a corresponding image for display together with a record of the current weather
void ForecastToImgTxt(weather_description wx_text){
  if      (wx_text == GoodClearWeather)           {image = sunny_img;       current_wx = sunny;        weather_text = "Good clear weather";}
  else if (wx_text == BecomingClearer)            {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Becoming clearer";}
  else if (wx_text == NoChange)                   {image = cloudy_img;      current_wx = cloudy;       weather_text = "No change, clearing";}
  else if (wx_text == ClearSpells)                {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clear spells";}
  else if (wx_text == ClearingWithin12hrs)        {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clearing within 12-hrs";}
  else if (wx_text == ClearingAndColder)          {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Clearing and colder";}
  else if (wx_text == GettingWarmer)              {image = mostlysunny_img; current_wx = mostlysunny;  weather_text = "Getting warmer";}
  else if (wx_text == WarmerIn2daysRainLikely)    {image = rain_img;        current_wx = rain;         weather_text = "Warmer in 2-days, rain likely";}
  else if (wx_text == ExpectRain)                 {image = rain_img;        current_wx = rain;         weather_text = "Expect rain";}
  else if (wx_text == WarmerRainWithin36hrs)      {image = rain_img;        current_wx = rain;         weather_text = "Warmer, rain within 36-hrs";}
  else if (wx_text == RainIn18hrs)                {image = rain_img;        current_wx = rain;         weather_text = "Rain in 18-hrs";}
  else if (wx_text == RainHighWindsClearAndCool)  {image = rain_img;        current_wx = rain;         weather_text = "Rain, high winds, clear and cool";}
  else if (wx_text == GalesHeavyRainSnowInWinter) {image = tstorms_img;     current_wx = tstorms;      weather_text = "Gales, heavy rain, in winter snow";}
}

// Convert pressure and trend to a weather description either for 1 or 3 hours with the boolean true/false switch
weather_description get_forecast_text(float pressure_now, float trend, bool range) {
  String trend_str = get_trend_text(trend);
  weather_description wx_text = NoChange; //As a default forecast 
  image = cloudy_img; // Generally when there is 'no change' then cloudy is the conditions
  if (pressure_now >= 1022.68 )                                                          {wx_text = GoodClearWeather;}
  if (pressure_now >= 1022.7  && trend_str  == "Falling fast")                           {wx_text = WarmerRainWithin36hrs;}
  if (pressure_now >= 1013.2  && pressure_now <= 1022.68 && 
     (trend_str == "Steady" || trend_str == "Rising slow"))                              {wx_text = NoChange; (range?wx_history_3hr():wx_history_1hr()); }
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 &&
     (trend_str == "Rising" || trend_str == "Rising fast"))                              {wx_text = GettingWarmer;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 && trend_str == "Rising slow")   {wx_text = BecomingClearer;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 && 
     (trend_str == "Falling fast"))                                                      {wx_text = ExpectRain;}
  if (pressure_now >= 1013.2 && pressure_now <= 1022.68 && trend_str  == "Steady")       {wx_text = ClearSpells; (range?wx_history_3hr():wx_history_1hr());};
  if (pressure_now <= 1013.2 && (trend_str == "Falling slow" || trend_str == "Falling")) {wx_text = RainIn18hrs;}
  if (pressure_now <= 1013.2  &&  trend_str == "Falling fast")                           {wx_text = RainHighWindsClearAndCool;}
  if (pressure_now <= 1013.2  && 
     (trend_str == "Rising" || trend_str=="Rising slow"||trend_str=="Rising fast"))      {wx_text = ClearingWithin12hrs;}
  if (pressure_now <= 1009.14 && trend_str  == "Falling fast")                           {wx_text = GalesHeavyRainSnowInWinter;}
  if (pressure_now <= 1009.14 && trend_str  == "Rising fast")                            {wx_text = ClearingAndColder;}
  return wx_text;
}

// Convert 1-hr weather history to text
void wx_history_1hr() {
  if      (wx_average_1hr >  0) weather_extra_text = ", expect sun";
  else if (wx_average_1hr == 0) weather_extra_text = ", mainly cloudy";
  else if (wx_average_1hr <  0) weather_extra_text = ", expect rain";
  else weather_extra_text = "";
}

// Convert 3-hr weather history to text
void wx_history_3hr() {
  if      (wx_average_3hr >  0) weather_extra_text = ", expect sun";
  else if (wx_average_3hr == 0) weather_extra_text = ", mainly cloudy";
  else if (wx_average_3hr <  0) weather_extra_text = ", expect rain";
  else weather_extra_text = "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// This array keeps function pointers to all frames  // Added a 6th frame for Room Temp
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5, drawFrame6 };

// how many frames are there?
int frameCount = 6;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void setup() { 
  float p,t;
  Serial.begin(115200);
 pinMode(buttonPin, INPUT);

  if (!StartWiFi(ssid,password)) Serial.println("Failed to start WiFi Service after 20 attempts");;

  /* configTime(12*3600, 0, "pool.ntp.org"); // +1hour (1*60*60=3600=+1hour) ahead for DST in the UK */
  configTime(0, 0, NTP_SERVER); // if TZ configured
    setenv("TZ", TZ_INFO, 1);   // for TZ and Daylight Auto change
  time_t now = time(nullptr); 
  delay(2000); // Wait for time to start

  if (!bmp.begin()) { Serial.println("Could not find a sensor, check wiring!");}
    else 
  {
    Serial.println("Found a sensor continuing");
    while (isnan(bmp.readPressure())) { Serial.println(bmp.readPressure()); }
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
                  
                  
  while (!update_time());  //Get the latest time
  for (int i = 0; i <= 23; i++){ // At the start all array values are the same as a baseline 
    reading[i].pressure     = read_pressure();       // A rounded to 1-decimal place version of pressure
    reading[i].temperature  = bmp.readTemperature(); // Enabled
 //   reading[i].humidity     = bme.readHumidity();    // Although not used, but avialable
    reading[i].wx_state_1hr = unknown;               // To begin with  
    reading[i].wx_state_3hr = unknown;               // To begin with 
  }                                                  // Note that only 0,5,11,17,20,21,22,23 are used as display positions
  last_reading_hour = reading_hour;
  wx_average_1hr = 0; // Until we get a better idea
  wx_average_3hr = 0; // Until we get a better idea


  // An ESP is capable of rendering 60fps in 80Mhz mode but leaves little time for anything else, run at 160Mhz mode or just set it to about 30 fps
  ui.setTargetFPS(20);
  ui.setIndicatorPosition(BOTTOM);         // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorDirection(LEFT_RIGHT);    // Defines where the first frame is located in the bar
  ui.setFrameAnimation(SLIDE_LEFT);        // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrames(frames, frameCount);        // Add frames
  ui.setOverlays(overlays, overlaysCount); // Add overlays
  ui.init(); // Initialising the UI will init the display too.
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT); 
  screenState = HIGH;                         //Set state for sleep switching
}
void loop() {
     buttonReading = digitalRead(buttonPin);
     if (screenState == HIGH) {
       timer.at(millis() + screenTimeOut, screenOff);
     } 
     if (screenState == LOW){
        if(buttonReading != lastButtonState){
          lastDebounceTime = millis();
        }
        if ((millis() - lastDebounceTime) > debounceDelay) {
            if(buttonReading != buttonState){
                buttonState = buttonReading;
                    if (buttonState == HIGH){
                       display.displayOn();
                       screenState = HIGH;
                    }
            }
        }

     }
  timer.tick(); // This updates the timer when triggered
  int remainingTimeBudget = ui.update();
  update_time_and_data();
  if (remainingTimeBudget > 0) { // Do some work here if required
   // for (int i = 0; i < 24;i++){
   //   Serial.println(String(i)+" "+String(reading[pressure][i]));
   // }
    Serial.println(time_str);
    Serial.println(read_pressure());
    delay(remainingTimeBudget);
  }
  lastButtonState = buttonReading;
}

// Screen Off Timer Call
bool screenOff (void *) {
    display.displayOff();
    screenState = LOW;
  return true;              // repeat? true
}

void update_time_and_data(){
  while (!update_time());
  if (reading_hour != last_reading_hour) { // If the hour has advanced, then shift readings left and record new values at array element [23]
    for (int i = 0; i < 23;i++){
      reading[i].pressure     = reading[i+1].pressure;
      reading[i].temperature  = reading[i+1].temperature;
      reading[i].wx_state_1hr = reading[i+1].wx_state_1hr;
      reading[i].wx_state_3hr = reading[i+1].wx_state_3hr;
    }
    reading[23].pressure     = read_pressure(); // Update time=now with current value of pressure
    reading[23].wx_state_1hr = current_wx;
    reading[23].wx_state_3hr = current_wx;
    last_reading_hour        = reading_hour;
    hr_cnt++;
    wx_average_1hr = reading[22].wx_state_1hr + current_wx;           // Used to predict 1-hour forecast extra text
    wx_average_3hr = 0;
    for (int i=23;i >= 21; i--){                                      // Used to predict 3-hour forecast extra text 
      wx_average_3hr = wx_average_3hr + (int)reading[i].wx_state_3hr; // On average the last 3-hours of weather is used for the 'no change' forecast - e.g. more of the same?
    }
  }  
}

bool update_time(){
  time_t now = time(nullptr); 
  struct tm *now_tm;
  int hour,min,second,day,month,year;
  now = time(NULL);
  now_tm = localtime(&now);
  hour = now_tm->tm_hour;
  min  = now_tm->tm_min;
  int weekday = now_tm->tm_wday;
  String week_days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  String wday = week_days[weekday];
  second = now_tm->tm_sec; 
  day    = now_tm->tm_mday;
  month  = now_tm->tm_mon + 1;
  year   = now_tm->tm_year - 100;// YY only no CC element add 1900 for CC element too
  // Needs to be in the format HH:MM:SS Sat 05-07-17
  time_str = (hour<10?"0":"")+String(hour)+":"+(min<10?"0":"")+String(min)+":"+(second<10?"0":"")+String(second)+" "+wday+(day<10?" 0":" ")+String(day)+"-"+(month<10?"0":"")+String(month)+"-"+String(year);
 // Serial.println(time_str);
  reading_hour = hour;
  return true;
}

int StartWiFi(const char* ssid, const char* password){
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: "+String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if(connAttempts > 20) return false;
    connAttempts++;
  }
  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

/*void BMP280_Reset(){
  Wire.beginTransmission(BMP280_ADDRESS);
  Wire.write((uint8_t)BMP280_REGISTER_SOFTRESET);
  Wire.write((uint8_t)0xB6);
  Wire.endTransmission();
  delay(5000);
  return;
} */
