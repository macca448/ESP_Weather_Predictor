# ESP Weather Predictor
ESP8266 or ESP32 Weather Predictor using 128 x 64 OLED Display

The core code for this project is from this Instructable
https://www.instructables.com/ESP32ESP8266-Weather-ForecasterPredictor/ 

or can be found in this repository
https://github.com/G6EJD/ESP32-Weather-Forecaster

## Some changes I have made to this  project are</br>
#### 1: Upgraded to a 2.4" OLED and changed from I2C to SPI interface. You can still use the 0.9" driver
#### 2: Added extra screen for Room Temperature display
#### 3: Modified the NTP config to add TZ_INFO for auto daylight time update
#### 4: Changed the screen scroll delay in the OLEDDisplayUi.cpp library file.

#### OLED UI Library used  https://github.com/helmut64/OLED_SSD1306
#### NTP mod inspiration https://www.bitsnblobs.com/network-time-using-esp8266 

## Note about the BMP280 library
If you use a Chinese clone BMP280 sensor then you will most likely need to make a simple change to the "Adafruit_BMP280.h" file. You'll know if it's needed as the sensor won't initialise.
The error is on line 32. The clone has a 0x76 address as default so just swap default an ALT values.

## Work in progress (September 19 2021)
I want to be able to have a push button for “displayOn()” and then a 5 minute time-out for “displayOff()” functions. Originally had this configured but discovered that after 2 or 3 months the display will no longer turn on. Have not done an accurate duration test but suspect it's being caused by the millis() roll-over (just under 50 days). Due to this unfinished feature (bug) have  removed adruino-timer library that was used and the millis() button de-bounce. The push-button has been swapped to a slide switch with a simple state engine to control the display so I'm not adding any timer functions. My hope is this will tell me if the error was introduced by me or it's inherent in the code or the hardware used. More to come..........

### Current Wiring
![image](https://user-images.githubusercontent.com/20883620/142696805-5ef8fd41-6dc6-4d99-a9a7-12b45632e6bd.png)

### Screens Video
[![Watch the video](https://img.youtube.com/vi/-4ZAevAfWxo/maxresdefault.jpg)](https://youtu.be/-4ZAevAfWxo)
