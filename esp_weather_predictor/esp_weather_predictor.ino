/*
  Copyright (C) 2023  R D McCLEERY

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Contacts:
  Email   : macca448@gmail.com
  Facebook: https://www.facebook.com/macca448
  Github  : https://github.com/macca448

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////      ABOUT THIS SKETCH      ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  Original Author of the Weather Predicion Methods used in this sketch David Bird
  <http://g6ejd.dynu.com/> or <https://github.com/G6EJD>

  This sketch is a re-work of the above and facilitates the following
    #1  NTP time method is in full compliance with NTP ORG "Terms of Service" https://www.ntppool.org/tos.html
    #2  A WiFi connection is only used to update and resync time. WiFi and Radio Off at all other times
    #3  There is "(S or F) Last Sync" time stamp on screen 6 (S = Success, F = Fail)
    #4  Sketch will auto determin your ESP board type (ESP32 or ESP8266)
    #5  The "FLASH" button (GPIO0) has been used for screen "WAKE"
    #6  "Manditory" user settings are maked "// ! "
    #7  This sketch is using the BMP280 for Barometric Pressure and Room Temperature
    #8  Sketch has been tested using 
          i.    Arduino IDE v1.8.19 and v2.1.0              (v2.1.0 shows some incorrect "unused parameter" warnings. You can ignor them)
          ii.   ESP8266 v3.1.2 (Generic)
          iii.  ESP32   v2.0.9 (Generic)
          iv.   Adafruit BMP280 Library v2.6.6              (Chinese Clones may need you to change the default address to 0x76 in Adafruit_BMP280.h on line:34)
          v.    ThingPulse v4.4.0 OLED
          NOTE: All libraries are available via Arduino IDE Library Manager

    #9  For information on <sys/time.h> "strftime" function https://cplusplus.com/reference/ctime/strftime/
    #10 NTP Timezone POSIX string database https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv 

    This sketch has been tested on ESP8266 with and I2C SSD1306 and ESP32 with SPI SSD1309

    HOW THIS SKETCH WORKS:
    On boot your controller starts two constant clocks "millis()" and "micros()"
    We then connect to WiFi and get an NTP "Unix epoch" time update.
    <sys/time.h> then use's your "Posix" string  to convert the GMT/UDP "epoch" to your local time in sync with your controllers millis() clock
    If your "Posix" string supports it the time will be auto-corrected for "Dailight Savings Time (DST)" start and end.
    The ESP's don't need time to be corrected for drift any shorter than one hour and you'd find doing it daily would be more than enough
    A Time resync only occurs when the screen is OFF. You'll see the on-board LED blinking indicating time update in progress.
    As noted above there is a "Last Sync" time stamp on page 6 under the temperature display
    To further adhear to NTP ORG's "Terms of Service" a randomness has been used as per their recommendations.
    The screen turns off after 3 minutes. You can adjust this in "USER SETTINGS". 
    Note that there is a "Duty" period for a shared timer statement. 50mS provides the debounce for the button so to calculate
    the screen time-out macro it is "duration = count * 50" or  3 minutes = "3600 * 50 or 180,000mS"
    If you want say a 5 minute screen OFF period it would be (5 * 60 * 60 * 1000 /50) = 6000 (#define SCREEN_SLEEP 6000)
    To "WAKE" the screen press the "FLASH" button on your ESP Dev Board

*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////   USER SETTINGS   /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MY_SSID       "MillFlat_El_Rancho"                // ! Enter Your WiFi SSID Details
#define MY_PASS       "140824500925"                      // ! Enter Your WiFi Password

//#define PRINT                                           // Uncomment to use Serial Print for de-bugging or to get a Serial Monitor Clock

#define ALT_OFFSET    3.8                                 // ! Location Sea Level Altitude Barometric Pressure Offset

#define TZ_INFO       "NZST-12NZDT,M9.5.0,M4.1.0/3"       // ! POSIX TimeZone String see item #10 above
#define SCREEN_SLEEP  3600                                // This creates a screen 3 minute timeout (3600 x 50mS = 180,000mS /1000/60 = 3 minutes
#define PAGE_TIME     7000                                // The duration each OLED page is displayed
#define TRANS_TIME    750                                 // The duration to change to the next page
#define FPS           20                                  // OLED Display Frame Rate
#define BLINK_RATE    5                                   // Blink LED for the NTP Update (250mS) blink rate 

#define WAKE_PIN      0                                   // Wake display or zero display off time-out counter
#define RAN_PIN       A0                                  // To generate a random minute to sync NTP

// ! Only For 128 x 64 OLED SSD1306 or SH1106 Displays
// ! Uncomment the correct screen connection type 
#define USE_SSD1306I2C                                    // ! For SSD1306 I2C OLED
//#define USE_SH1106I2C                                   // ! For SH1106 I2C OLED
#define I2C_ADDRESS     0x3C                              // ! Use I2C Scanner to get your OLED's I2C address
//#define USE_SSD1306SPI                                  // ! For SSD1306 SPI OLED
//#define USE_SH1106SPI                                   // ! For SH1106 SPI OLED
//#define OLED_DC       4                                 // ! SPI OLED_DC PIN
//#define OLED_CS       5                                 // ! SPI OLED_CS PIN
//#define OLED_RES      16                                // ! SPI OLED_RESET PIN

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// END USER SETTINGS /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <sys/time.h>                                     // NTP Time magic lives here. This library is part of the ESP board library
#include <Wire.h>                                         // I2C Driver Library             
#include <Adafruit_BMP280.h>                              // If using a Chinese enable "#define CLONE_BMP280" above

// Detect ESP Chip type for WiFi library
#if defined (ARDUINO_ARCH_ESP32)                          
    #include <WiFi.h>   
    #define LED     2
    #define ON      HIGH
    #define OFF     LOW   
#elif defined (ARDUINO_ARCH_ESP8266)
    #include <ESP8266WiFi.h>
    #define LED LED_BUILTIN
    #define ON  LOW
    #define OFF HIGH      
#endif

#include <OLEDDisplayUi.h>            //https://github.com/ThingPulse/esp8266-oled-ssd1306

#ifdef USE_SSD1306I2C     
    #include <SSD1306.h>              //https://github.com/helmut64/OLED_SSD1306
    SSD1306 display(I2C_ADDRESS, SDA, SCL);  // OLED display object definition (address, SDA, SCL)
#endif
#ifdef USE_SH1106I2C     
    #include <SH1106.h>              //https://github.com/helmut64/OLED_SSD1306
    SH1106 display(I2C_ADDRESS, SDA, SCL);  // OLED display object definition (address, SDA, SCL)
#endif
#ifdef USE_SSD1306SPI
    #include <SSD1306Spi.h>           //https://github.com/ThingPulse/esp8266-oled-ssd1306 // For SPI Display
    SSD1306Spi display(OLED_RES, OLED_DC, OLED_CS); 
#endif
#ifdef USE_SH1106SPI
    #include <SH1106Spi.h>           //https://github.com/ThingPulse/esp8266-oled-ssd1306 // For SPI Display
    SH1106Spi display(OLED_RES, OLED_DC, OLED_CS); 
#endif

OLEDDisplayUi ui( &display );
Adafruit_BMP280 bmp;   //BMP Sensor object

#define icon_width  40
#define icon_height 40

#define     DUTY_CYCLE 50     // For de-bouncing the buttons and to create the screen timeout (see line above)

const bool  look_3hr = true, look_1hr = false;

uint8_t     lastSecond, current_second, last_reading_hour, current_hour, hr_cnt = 0,
            current_minute, blinkCount = 0, ledState = ON, ntp_sync_hour, random_minute;
uint16_t    timeout = 0;
int16_t     wx_average_1hr, wx_average_3hr; // Indicators of average weather
bool        modePress = false, modeState, lastModeState = HIGH,
            screenON = true, blink = false, updateLED = true;
String      weather_text, weather_extra_text, time_str, sync_stamp;
uint32_t    previousTime = 0, remainingTimeBudget;
int32_t     update_epoch, previous_epoch = 1609459200;        //1st Jan 2021 00:00:00 so we know we get a true epoch at update time

char ssid[]     = MY_SSID;
char password[] = MY_PASS;

enum image_names { // enumerated table used to point to images
                  rain_img, sunny_img, mostlysunny_img, cloudy_img, tstorms_img,
                 } image;

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
  //float humidity;            // humidity at the designated hour
  weather_type wx_state_1hr; // weather state at 1-hour
  weather_type wx_state_3hr; // weather state at 3-hour point
} wx_record_type;

wx_record_type reading[24]; // An array covering 24-hours to enable P, T, % and Wx state to be recorded for every hour

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

/////////////////////////////////////////////////////////////////////////
// What's displayed along the top line - NTP Date and Time Overlay
void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0,0, time_str.substring(0,12));   // day dd-mm-yy
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128,0, time_str.substring(13));   // HH:MM:SS
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
  String tmp_str = String(read_temperature(),1) + String("\xC2\xB0") + String("C");
  display->drawStringMaxWidth(x+30,y+22,97,tmp_str);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64,y+40,sync_stamp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

float read_pressure(){
  int reading = (bmp.readPressure()/100.0F+ALT_OFFSET)*10; // Rounded result to 1-decimal place
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
uint8_t frameCount = 6;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
uint8_t overlaysCount = 1;

void updateTime(void){
    time_t now = time(NULL); 
    struct tm *now_tm;
    now_tm = localtime(&now);
    char buffer [60];
    strftime (buffer,60,"%a %d-%m-%y %H:%M:%S", now_tm);         // See item #9 in header notes
    time_str = buffer;
    current_hour   = now_tm->tm_hour;
    current_minute = now_tm->tm_min;
    current_second = now_tm->tm_sec;
    #ifdef PRINT
      if(current_second != lastSecond){
        Serial.println(buffer);
        lastSecond = current_second;
      }
    #endif
    return;
}

void time_data_update(void){
  updateTime();
  if (current_hour != last_reading_hour) { // If the hour has advanced, then shift readings left and record new values at array element [23]    
    for (int i = 0; i < 23;i++){
      reading[i].pressure     = reading[i+1].pressure;
      reading[i].temperature  = reading[i+1].temperature;
      reading[i].wx_state_1hr = reading[i+1].wx_state_1hr;
      reading[i].wx_state_3hr = reading[i+1].wx_state_3hr;
    }
    reading[23].pressure     = read_pressure(); // Update time=now with current value of pressure
    reading[23].wx_state_1hr = current_wx;
    reading[23].wx_state_3hr = current_wx;
    hr_cnt++;
    wx_average_1hr = reading[22].wx_state_1hr + current_wx;           // Used to predict 1-hour forecast extra text
    wx_average_3hr = 0;
    for (int i=23;i >= 21; i--){                                      // Used to predict 3-hour forecast extra text 
      wx_average_3hr = wx_average_3hr + (int)reading[i].wx_state_3hr; // On average the last 3-hours of weather is used for the 'no change' forecast - e.g. more of the same?
    }
    last_reading_hour = current_hour;
  } 
  return; 
}

void doNTP(void){
    delay(500);
    #ifdef PRINT
      Serial.printf("\nConnecting to WiFi %s ", ssid);
    #endif
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(false);
    while(WiFi.status() != WL_CONNECTED){
      #ifdef PRINT
        Serial.print(".");
      #endif
      delay(250);
    }
    #ifdef PRINT
      Serial.print("\nIP address: ");Serial.println(WiFi.localIP());
      Serial.println("\nWiFi Connected.....");
      Serial.println("Getting NTP Time Update");
    #endif
    time_t now = time(nullptr); 
    struct tm *now_tm;
    now = time(NULL);
    update_epoch = 0;
    while(update_epoch < previous_epoch){
      #if defined (ARDUINO_ARCH_ESP8266)  
        now_tm = localtime(&now);
      #endif
        update_epoch = time(&now);
        delay(500);                     //Small delay needed or we miss the packet
    }
    #if defined (ARDUINO_ARCH_ESP32)
        now_tm = localtime(&now);       //Converts UTC "Epoch" to local time based on POSIX string supplied
    #endif                              //Then sync's the controller millis() to the current time
    delay(1000);
    ntp_sync_hour   = now_tm->tm_hour;
    current_hour    = now_tm->tm_hour;
    current_minute  = now_tm->tm_min;
    current_second  = now_tm->tm_sec;
    #ifdef PRINT
      char buffer [80];
      strftime (buffer,80," NTP Sync Stamp %A, %B %d %Y %H:%M:%S [%Z]\n", now_tm);          // See item #9 in header notes
      Serial.println(buffer);
    #endif
    random_minute = random(11, 31);
    if(WiFi.status() == WL_CONNECTED){
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
         #ifdef PRINT
          Serial.println(" [WiFi] Disconnected from WiFi and radio off!!\n");
        #endif
    }
    char ntp_stamp[30];
    sprintf(ntp_stamp,"%02u:%02u:%02u", ntp_sync_hour, current_minute, current_second);
    delay(500);
    if(update_epoch > previous_epoch){
      previous_epoch = update_epoch;
      sync_stamp = "Last Sync: ";
      sync_stamp += ntp_stamp;
      delay(500);
    }else{
      sync_stamp ="Resync Fail!";
    }
    return;
}

void setup() { 
  #ifdef PRINT
    Serial.begin(115200);
  #endif
  pinMode(WAKE_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, ON);
  randomSeed(analogRead(RAN_PIN));                        //To generate a random minute for NTP resync time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");      //Configure one of two NTP server address's
  setenv("TZ", TZ_INFO, 1); tzset();                      //Setup your Timezone for true local time incl. DST 
  doNTP();
  
  if (!bmp.begin()){ 
    #ifdef PRINT
      Serial.println("Could not find a sensor, check wiring!");
    #endif
  }else{
    #ifdef PRINT
      Serial.println("Found a sensor continuing");
    #endif
    while (isnan(bmp.readPressure())) { 
      #ifdef PRINT 
        Serial.println(bmp.readPressure()); 
      #endif
    }
  }
  
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
                  
  for (int i = 0; i <= 23; i++){ // At the start all array values are the same as a baseline 
    reading[i].pressure     = read_pressure();       // A rounded to 1-decimal place version of pressure
    reading[i].temperature  = bmp.readTemperature(); // Enabled
 //   reading[i].humidity     = bme.readHumidity();    // Although not used, but avialable
    reading[i].wx_state_1hr = unknown;               // To begin with  
    reading[i].wx_state_3hr = unknown;               // To begin with 
  }                                                  // Note that only 0,5,11,17,20,21,22,23 are used as display positions
  wx_average_1hr = 0; // Until we get a better idea
  wx_average_3hr = 0; // Until we get a better idea

  // An ESP is capable of rendering 60fps in 80Mhz mode but leaves little time for anything else,
  // run at 160Mhz mode or just set it to about 30 fps
  ui.setTargetFPS(FPS);                    
  ui.setIndicatorPosition(BOTTOM);         // You can change this to TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorDirection(LEFT_RIGHT);    // Defines where the first frame is located in the bar
  ui.setFrameAnimation(SLIDE_LEFT);        // You can change the transition that is used SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrames(frames, frameCount);        // Add frames
  ui.setOverlays(overlays, overlaysCount); // Add overlays
  ui.setTimePerFrame(PAGE_TIME);
  ui.setTimePerTransition(TRANS_TIME);
  ui.init();                               // Initialising the UI will init the display too.
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  digitalWrite(LED, OFF);
}

void loop() {

  time_data_update();                                 //True every hour roll
  
  if(screenON){
    remainingTimeBudget = ui.update();                //Disabled whenh screen is off    
  }else{
    remainingTimeBudget = 1;
  }
  
  if (remainingTimeBudget > 0){
    modeState = digitalRead(WAKE_PIN); 
      if(modeState != lastModeState){
        previousTime = millis();
        modePress = true;
      }
      //Duty Cycle Events
      if(millis() - previousTime >= DUTY_CYCLE){          
        if(!screenON){
          if(modePress){
            if(modeState == LOW){                  
              timeout = 0;
              display.resetDisplay();
              display.displayOn();
              ui.enableAutoTransition();
              ui.setAutoTransitionForwards();
              screenON = true;
              ledState = ON;
              updateLED = true;                      
            }
            modePress = false;
          }  //NTP update only when screen is off and on a random minute chosen on the last update
          if(ntp_sync_hour != current_hour && current_minute >= random_minute){       //Very random indeed to comply with pool.ntp.org
            blink = true;                                                             //Provides a visual that NTP is being Updated
            doNTP();                                                                  //this does the deed!
            ledState = OFF;
            blink = false;
            updateLED = true;
          }
        }else if(screenON){
          timeout++;
            if(timeout >= SCREEN_SLEEP){
              display.displayOff();
              blink = false;
              ledState = OFF;
              updateLED = true;
              screenON = false;
            }
        }
        if(blink){
          blinkCount++;
            if(blinkCount >= BLINK_RATE){
              blinkCount = 0;
              ledState = !ledState;
              updateLED = true;
            }
        }
        if(updateLED){
          digitalWrite(LED, ledState);
          updateLED = false;
        }
        previousTime = millis();
      }
      if(screenON){
        delay(remainingTimeBudget);
      }
  }
  lastModeState = modeState;
}
