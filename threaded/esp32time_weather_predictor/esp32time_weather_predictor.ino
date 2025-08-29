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
  GitHub  : https://github.com/macca448

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////      ABOUT THIS SKETCH      ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  This sketch is a modifyed version of David Bird's  <https://github.com/G6EJD/ESP32-Weather-Forecaster>
  or <http://g6ejd.dynu.com/micro-controllers/esp32/esp32-weather-forecaster/>

  Modifications I have done greatly improve the performance and stability and are as follows:
 
    #1  NTP time method is in full compliance with NTP ORG "Terms of Service" https://www.ntppool.org/tos.html
    #2  A WiFi connection is only used to update and re sync time. WiFi and Radio Off at all other times
    #3  There is "Last Sync" time stamp on screen 6 below the temperature reading
    #4  Sketch ESP32 only
    #5  The "FLASH" button (GPIO0) has been used for a screen "WAKE" function.
    #6  "Manditory" user settings are maked with " ! "
    #7  This sketch is configured to use a BMP280 for Barometric Pressure and Room Temperature
    #8  Sketch has been tested using
          i.    Arduino IDE v1.8.19 and v2.1.0              (v2.1.0 shows some incorrect "unused parameter" warnings. You can ignor them)
          ii.   ESP8266 v3.1.2 (Generic)
          iii.  ESP32   v2.0.9 (Generic)
          iv.   Adafruit BMP280 Library v2.6.6              (Chinese Clones may need you to change the default address to 0x76 in Adafruit_BMP280.h on line:34)
          v.    ThingPulse v4.4.0 OLED
          NOTE: All libraries are available via Arduino IDE Library Manager

    #9  For information on <sys/time.h> "strftime" function https://cplusplus.com/reference/ctime/strftime/
    #10 NTP Timezone POSIX string database https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

    This sketch has been tested on ESP32 with SPI SSD1309

    HOW THIS SKETCH WORKS:
    1. On boot your controller starts two constant clocks "millis()" and "micros()"
    2. We then connect to WiFi and get an NTP "Unix epoch" time update.
    3. <sys/time.h> then use's your "Posix" string  to convert the GMT/UDP "epoch" to your local time in sync with your controllers millis() clock
    4. If your "Posix" string supports it the time will be auto-corrected for "Dailight Savings Time (DST)" start and end.
    5. The ESP's don't need time to be corrected for drift any shorter than one hour and you'd find doing it daily would be more than enough
    6. A Time re sync only occurs when the screen is OFF. You'll see the on-board LED blinking indicating time update in progress.
    7. As noted above there is a "Last Sync" time stamp on page 6 under the temperature display
    8. To further adhere to NTP ORG's "Terms of Service" a randomness has been used as per their recommendations.
    9. The screen turns off after 3 minutes. You can adjust this in the "USER SETTINGS" below.
    Note that there is a "Duty" period for a shared timer statement. 50mS provides the denounce for the button so to calculate
    the screen time-out it is "duration = count * 50" or  3 minutes = "3600" being (3600 * 50 = 180,000mS)
    If you want say a 5 minute screen OFF period it would be (5 * 60 * 60 * 1000 /50) = 6000 (#define SCREEN_SLEEP 6000)
    10. To "WAKE" the screen press the "FLASH / BOOT" button on your ESP Dev Board

*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////   USER SETTINGS   /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MY_SSID       "YOUR_SSID"                         // ! Enter Your WiFi SSID Details
#define MY_PASS       "YOUR_PASSWORD"                     // ! Enter Your WiFi Password

//#define PRINT                                           // Uncomment to use Serial Print for de-bugging or to get a Serial Monitor Clock

#define ALT_OFFSET    3.8                                 // ! Location Sea Level Altitude Barometric Pressure Offset

#define TZ_INFO       "NZST-12NZDT,M9.5.0,M4.1.0/3"       // ! POSIX TimeZone String see item #10 above
#define SCREEN_SLEEP  3600                                // This creates a screen 3 minute timeout (3600 x 50mS = 180,000mS /1000/60 = 3 minutes
#define PAGE_TIME     5000                                // The duration each OLED page is displayed
#define TRANS_TIME    400                                 // The duration to change to the next page
#define FPS           20                                  // OLED Display Frame Rate

#define WAKE_PIN      0                                   // Wake display or zero display off time-out counter
#define RAN_PIN       A0                                  // To generate a random minute to sync NTP
#define RAN_MIN       71
#define RAN_MAX       87

// ! Only For 128 x 64 OLED SSD1306 or SH1106 Displays
// ! Uncomment the correct screen connection type 
//#define USE_SSD1306I2C                                    // ! For SSD1306 I2C OLED
//#define USE_SH1106I2C                                   // ! For SH1106 I2C OLED
//#define I2C_ADDRESS     0x3C                              // ! Use I2C Scanner to get your OLED's I2C address
#define USE_SSD1306SPI                                  // ! For SSD1306 SPI OLED
//#define USE_SH1106SPI                                   // ! For SH1106 SPI OLED
#define OLED_DC       4                                 // ! SPI OLED_DC PIN
#define OLED_CS       5                                 // ! SPI OLED_CS PIN
#define OLED_RES      27                                // ! SPI OLED_RESET PIN

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// END USER SETTINGS /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <ESP32Time.h>                                     // NTP Time magic lives here. This library is part of the ESP board library
#include <Wire.h>                                         // I2C Driver Library             
#include <Adafruit_BME280.h>                              // If using a Chinese enable "#define CLONE_BMP280" above

// Detect ESP Chip type for WiFi library
#include <WiFi.h>   
#define LED     2
#define ON      HIGH
#define OFF     LOW   

#include <OLEDDisplayUi.h>            //https://github.com/ThingPulse/esp8266-oled-ssd1306

ESP32Time rtc;
time_t now;

#ifdef USE_SSD1306I2C     
    #include <SSD1306.h>              //https://github.com/helmut64/OLED_SSD1306
    SSD1306 display(I2C_ADDRESS, SDA, SCL);  // OLED display object definition (address, SDA, SCL)
#endif
#ifdef USE_SH1106I2C     
    #include <SH1106.h>              //https://github.com/helmut64/OLED_SSD1306
    SH1106 display(I2C_ADDRESS, SDA, SCL);  // OLED display object definition (address, SDA, SCL)
#endif
#ifdef USE_SSD1306SPI
    #include <SPI.h>
    #include <SSD1306Spi.h>           //https://github.com/ThingPulse/esp8266-oled-ssd1306 // For SPI Display
    SSD1306Spi display(OLED_RES, OLED_DC, OLED_CS); 
#endif
#ifdef USE_SH1106SPI
    #include <SH1106Spi.h>           //https://github.com/ThingPulse/esp8266-oled-ssd1306 // For SPI Display
    SH1106Spi display(OLED_RES, OLED_DC, OLED_CS); 
#endif

OLEDDisplayUi ui( &display );
Adafruit_BME280 bme;   //BMP Sensor object

#define icon_width  40
#define icon_height 40

#define  DUTY_CYCLE  50     // For de-bouncing the buttons and to create the screen timeout (see line above)
#define LONGPRESS_MS 1000

const bool  look_3hr = true, look_1hr = false;
String      weather_text, weather_extra_text;

struct STRUCT2{
  int32_t   lastEpoch = 1609459200;
  uint32_t  previousTime = 0; 
  uint16_t  timeout = 0;
  uint8_t   lastSecond;
  uint8_t   last_reading_hour;
  uint8_t   minCount;
  uint8_t   lastMinute;
  uint8_t   randomMinute;
  uint16_t  btnCounter = 0;
  bool      btnHandled = false;
  bool      modeState = false;
  bool      lastModeState = false;
  bool      screenON = true;
  String    sync_stamp = "";
}core_2;

struct STRUCT1{
  int16_t wx_average_1hr;
  int16_t wx_average_3hr;
  uint8_t hr_cnt = 0;
}core_1;

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
  display->drawString(0,0, rtc.getTime("%a %d-%m-%y"));   // day dd-mm-yy
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128,0, rtc.getTime("%T"));   // HH:MM:SS
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
  display->drawString(x-30,y+gscale/2-6,String(core_1.hr_cnt%24));
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
  display->drawString(64,y+40,core_2.sync_stamp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

float read_pressure(){
  int reading = (bme.readPressure()/100.0F+ALT_OFFSET)*10; // Rounded result to 1-decimal place
  return (float)reading/10;
}

float read_temperature(){                                       // Added for a Local Temp reading
  int reading = bme.readTemperature()*100;
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
  if      (core_1.wx_average_1hr >  0) weather_extra_text = ", expect sun";
  else if (core_1.wx_average_1hr == 0) weather_extra_text = ", mainly cloudy";
  else if (core_1.wx_average_1hr <  0) weather_extra_text = ", expect rain";
  else weather_extra_text = "";
}

// Convert 3-hr weather history to text
void wx_history_3hr() {
  if      (core_1.wx_average_3hr >  0) weather_extra_text = ", expect sun";
  else if (core_1.wx_average_3hr == 0) weather_extra_text = ", mainly cloudy";
  else if (core_1.wx_average_3hr <  0) weather_extra_text = ", expect rain";
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

bool doNTP(){
  int32_t epoch = 0;
  uint8_t t_out = 0;
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", TZ_INFO, 1); tzset(); 
  while(epoch < core_2.lastEpoch){          //Wait till the NTP server reponds    
    epoch = time(&now);               //Gets the current Unix UTC/GMT time
    t_out++;
    delay(250);
      if(t_out > 120){
        return false;
      }
  }
  rtc.setTime(epoch);
  core_2.lastEpoch = epoch;
  core_2.randomMinute = random(RAN_MIN, RAN_MAX);
  core_2.lastMinute = rtc.getMinute();
  core_2.lastSecond = rtc.getSecond();
  return true;
}

String getSyncStamp(bool success){
  String stamp = "";
  stamp += "Last Sync ";
  if(success){
    stamp += rtc.getTime("%T");
  }else{
    stamp += "Failed";   
  }
  return stamp;
}

void data_update(void){  
   // If the hour has advanced, then shift readings left and record new values at array element [23]    
    for (int i = 0; i < 23;i++){
      reading[i].pressure     = reading[i+1].pressure;
      reading[i].temperature  = reading[i+1].temperature;
      reading[i].wx_state_1hr = reading[i+1].wx_state_1hr;
      reading[i].wx_state_3hr = reading[i+1].wx_state_3hr;
    }
    reading[23].pressure     = read_pressure(); // Update time=now with current value of pressure
    reading[23].wx_state_1hr = current_wx;
    reading[23].wx_state_3hr = current_wx;
    core_1.hr_cnt++;
    core_1.wx_average_1hr = reading[22].wx_state_1hr + current_wx;           // Used to predict 1-hour forecast extra text
    core_1.wx_average_3hr = 0;
    for (int i=23;i >= 21; i--){                                      // Used to predict 3-hour forecast extra text 
      core_1.wx_average_3hr = core_1.wx_average_3hr + (int)reading[i].wx_state_3hr; // On average the last 3-hours of weather is used for the 'no change' forecast - e.g. more of the same?
    }
  return; 
}

void shortPress(void) {
  if(!core_2.screenON){
    core_2.timeout = 0;
    //display.resetDisplay();
    display.displayOn();            
    ui.switchToFrame(0);
    ui.update();
    core_2.screenON = true;
    digitalWrite(LED, ON);
  }
  return;
}

// === Your reset helper ===
void longPress(void){
  display.init();        // hard reset the SSD1309
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  core_2.timeout = 0;
  core_2.screenON = true;
  digitalWrite(LED, ON);
  ui.switchToFrame(0);   // back to a known page (optional)
  ui.update();           // force redraw
  return;
}

// === Button task (called every 50ms) ===
void buttonTask(void) {
  bool raw = !digitalRead(WAKE_PIN); // active LOW

  if (raw == core_2.lastModeState) {
    if (core_2.modeState != raw) {
      core_2.modeState = raw;
      if (core_2.modeState) {
        // just pressed
        core_2.btnCounter = 0;
        core_2.btnHandled = false;
      } else {
        // just released
        if (!core_2.btnHandled) {
          if (core_2.btnCounter * DUTY_CYCLE < LONGPRESS_MS) {
            shortPress();
          } else {
            longPress();
          }
        }
      }
    } else {
      // stable and pressed
      if (core_2.modeState) {
        core_2.btnCounter++;
        if (!core_2.btnHandled && (core_2.btnCounter * DUTY_CYCLE >= LONGPRESS_MS)) {
          longPress();
          core_2.btnHandled = true;
        }
      }
    }
  }
  core_2.lastModeState = raw;
  return;
}

void loop2(void *pvParameters){    // Core 1 loop - User tasks
  while (1){
      //Duty Cycle Events
      if(millis() - core_2.previousTime >= DUTY_CYCLE){          
        core_2.previousTime += DUTY_CYCLE;
        buttonTask();
          if(core_2.screenON){
            core_2.timeout++;
              if(core_2.timeout >= SCREEN_SLEEP){
                //display.clear();
                display.displayOff();
                core_2.screenON = false;
                digitalWrite(LED, OFF);
              }
          }
      }
      if(rtc.getMinute() != core_2.lastMinute){
        core_2.minCount++;
        core_2.lastMinute = rtc.getMinute();
      }
      if(core_2.minCount >= core_2.randomMinute){
        digitalWrite(LED, ON);
        bool success = doNTP();
          if(!success){
            core_2.sync_stamp = getSyncStamp(0);
            core_2.randomMinute = random(RAN_MIN, RAN_MAX);
          }else{
            core_2.sync_stamp = getSyncStamp(1);
          }
          core_2.minCount = 0;
          digitalWrite(LED, OFF);
      }
      if (rtc.getHour(true) != core_2.last_reading_hour) {
        core_2.last_reading_hour = rtc.getHour(true);
        data_update();
      }
      delay(1);
  }
}

void loop1(void *pvParameters){    // Core 0 loop - User tasks
  while (1){
    int remainingTimeBudget = ui.update();  
      if(remainingTimeBudget > 0){
        // Do stuff here
      }
      delay(remainingTimeBudget);
  }
}

void setup() { 
  #ifdef PRINT
    Serial.begin(115200);
  #endif
  pinMode(WAKE_PIN, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, ON);
  randomSeed(analogRead(RAN_PIN));                        //To generate a random minute for NTP resync
  
  delay(500);
  #ifdef PRINT
    Serial.printf("\nConnecting to WiFi %s ", ssid);
  #endif
  uint8_t wifiTimeOut = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
    while(WiFi.status() != WL_CONNECTED){
      wifiTimeOut++;
      #ifdef PRINT
        Serial.print(".");
      #endif
      delay(250);
        if(wifiTimeOut >= 80){            //20 second Wifi non-connection time-out
          #ifdef PRINT
            Serial.printf("\nFailed to connect to %s\n", ssid);
          #endif
          ESP.restart();
        }
    }
  bool success = doNTP();
    if(!success){
      ESP.restart();
    }else{
      core_2.sync_stamp = getSyncStamp(1);
    }

  unsigned status;
    
    // default settings
    status = bme.begin();  
    // You can also pass in a Wire library object like &Wire2
    // status = bme.begin(0x76, &Wire2)
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }else{
      Serial.println("\nBME Sensor OK!");
    }
                  
  for (int i = 0; i <= 23; i++){ // At the start all array values are the same as a baseline 
    reading[i].pressure     = read_pressure();       // A rounded to 1-decimal place version of pressure
    reading[i].temperature  = read_temperature(); // Enabled
 //   reading[i].humidity     = bme.readHumidity();    // Although not used, but avialable
    reading[i].wx_state_1hr = unknown;               // To begin with  
    reading[i].wx_state_3hr = unknown;               // To begin with 
  }                                                  // Note that only 0,5,11,17,20,21,22,23 are used as display positions
  core_1.wx_average_1hr = 0; // Until we get a better idea
  core_1.wx_average_3hr = 0; // Until we get a better idea

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

  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
}

void loop() {}
