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

#### OLED UI Library used  https://github.com/helmut64/OLED_SSD1306 NOTE: The UI library has had variable's set that are not in the linked library so I have included the modifyied version that makes my configuration along with a zip of the full set of drivers in the current version directory
#### NTP mod inspiration https://www.bitsnblobs.com/network-time-using-esp8266 

## Note about the BMP280 library
If you use a Chinese clone BMP280 sensor then you will most likely need to make a simple change to the "Adafruit_BMP280.h" file. You'll know if it's needed as the sensor won't initialise.
The error is on line 32. The clone has a 0x76 address as default so just swap default an ALT values.

## Work in progress (November 12 2021) update
After speaking to the author we both argee that the millis() roll bug is likely in the UI library so some variables have been increased in size and we await the next roll of millis() (approx New Years day). I have reinstated the wake witha 3 minute screen timeout but not by a libray as originally don but simple debouce and a counter. As you will see from the new drawing I have also added a second button with it's function tested that if we still crash on millis() roll should tell me if the ketch is still running as it is a display reset. Additionally I have made use of the onboard LED for some visuals as follows: Display OFF = LED OFF, Normal Wake = LED ON Solid, at one second to millis() roll the LED will start to do a slow blink, if I need to use the screen reset and it works the LED will rapid blink till the 3 minute timeout and then screen OFF. 

### Current Wiring
![image](https://user-images.githubusercontent.com/20883620/142696805-5ef8fd41-6dc6-4d99-a9a7-12b45632e6bd.png)

### Screens Video
[![Watch the video](https://img.youtube.com/vi/-4ZAevAfWxo/maxresdefault.jpg)](https://youtu.be/-4ZAevAfWxo)
