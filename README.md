# ESP Weather Predictor
ESP8266 or ESP32 Weather Predictor using 128 x 64 OLED Display

The Original Author of the Weather Predicion Methods used in this sketch is David Bird
[http://g6ejd.dynu.com](<http://g6ejd.dynu.com/>) or [https://github.com/G6EJD](<http://g6ejd.dynu.com/>)

## This sketch is a re-work of the above and facilitates the following
  1.  NTP time method is in full compliance with NTP ORG "[Terms of Service](https://www.ntppool.org/tos.html)" 
  2.  The WiFi connection is only used to update time. At all other times the WiFi is disconnected and the radio is turned Off. 
  3.  Display Page 6 has a Time stamp for the last NTP sync so it's easy to know when things aren't working
  4.  Sketch will auto determin your ESP board type (ESP32 or ESP8266)
  5.  The "FLASH / BOOT" button (GPIO0) is used for screen "WAKE"
  6.  "Manditory" user settings are maked with " ! " in the "USER SETTINGS" section of the sketch
  7.  This sketch is configured to use a BMP280 for Barometric Pressure and Room Temperature. It's easily adapted to a different sensor.
  8.  Sketch was tested using: 
      i.    Arduino IDE v1.8.19 and v2.1.0              (v2.1.0 shows some incorrect "unused parameter" warnings. You can ignor them)
      ii.   ESP8266 v3.1.2 (Generic) board profile
      iii.  ESP32   v2.0.9 (Generic) board profile
      iv.   Adafruit BMP280 Library v2.6.6              (Chinese Clones may need you to change the default address to 0x76 in Adafruit_BMP280.h on line:34)
      v.    ThingPulse v4.4.0 OLED Driver Library
          NOTE: All libraries are available via the Arduino IDE Library Manager

  9.  For more information on <[sys/time.h]( https://cplusplus.com/reference/ctime/strftime/)> "strftime" function click [here](https://cplusplus.com/reference/ctime/strftime/)
  10. NTP Timezone POSIX string [database](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)


#### ESP8266 and ESP32 OLED Driver from ThingPulse IDE Install Screen
![ThingPulse](https://github.com/macca448/ESP_Weather_Predictor/blob/main/esp_weather_predictor/assets/thingpulse_library.png)

#### This sketch has been tested on ESP8266 with and I2C SSD1306 and ESP32 with SPI SSD1309
  

## HOW THIS SKETCH WORKS:
  1. On boot your controller starts two constant timers "millis()" and "micros()". 
  2. We connect to WiFi and get an NTP "Unix Epoch" time update.
  3. <sys/time.h> then use's your "Posix" string  to convert the GMT/UDP "Epoch" to your local time and sync's the time with your controllers millis() clock.
  4. If your "Posix" string supports it the time will be auto-corrected for "Dailight Savings Time (DST)" start and end.
  5. The ESP's don't need time to be corrected for drift any shorter than one hour and you'd find doing it daily would be more than enough
  6. A Time resync only occurs when the screen is OFF. You'll see the on-board LED blinking indicating time update in progress.
  7. As noted above there is a "Last Sync" time stamp on page 6 under the temperature reading.
  8. To further adhear to NTP ORG's "Terms of Service" a randomness has been used as per their recommendations.
  9. The screen turns off after 3 minutes. You can adjust this in the "USER SETTINGS" section. 
  10. Note that there is a "Duty" period for a shared timer statement. 50mS provides the debounce for the button so to calculate a screen time-out period for the macro it is "duration = count * 50" or  3 minutes = "3600 * 50 or 180,000mS". If you want say a 5 minute screen OFF period it would be (5 * 60 * 60 * 1000 /50) = 6000 (#define SCREEN_SLEEP 6000)
11. To "WAKE" the screen press the "FLASH / BOOT" button on your ESP Dev Board. If the screen is awake and you press this button it resets the timeout count to zero.


### Wiring Guide ESP32 I2C SSD1306
![I2C ESP32 SSD1306](https://github.com/macca448/ESP_Weather_Predictor/blob/main/esp_weather_predictor/assets/ESP32_OLED_I2C.png)


### Wiring Guide ESP32 I2C SSD1306
![SPI ESP32 SSD1309](https://github.com/macca448/ESP_Weather_Predictor/blob/main/esp_weather_predictor/assets/OLED_SPI_BMP_ESP32.png)


### Screens Video
[![Watch the video](https://img.youtube.com/vi/-4ZAevAfWxo/maxresdefault.jpg)](https://youtu.be/-4ZAevAfWxo)

 

 	
