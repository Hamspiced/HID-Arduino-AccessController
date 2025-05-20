# HID-Arduino-AccessController
This is a project repository for creating a full fledged door access controller using an HID Reader, Arduino, and i2c screen.  This project will allow you to use the reader as a simple badge scanner, act as a door controller, identify authorized access control cards, add cards, and delete cards.

![R15 Door Simulator](https://github.com/Hamspiced/HID-Arduino-AccessController/raw/main/Images/R15%20Door%20Simulator2.jpeg)


## Parts
 [Arduino Nano](https://a.co/d/4Q9VMcR) - $16.49 per 3 or $5.49 ea
 
 [USB-C Pigtail](https://a.co/d/i3n7h48) - $7.99 per 10 or $0.80 ea
 
 [Arduino Breakout Board](https://a.co/d/hGXZX4y) - $8.79 per 3 or $2.93 ea
 
 [5v Buck Converter](https://a.co/d/7Qql78y) - $11.59 per 5 or $2.32 ea
 
 [i2c 2 Color OLED](https://a.co/d/0r9uNRJ) - $13.98 per 5 or $2.80 ea
 
 HID R10 or R15 Multiclass Reader - Ebay or Marketplace Price Varies between 20-90$ Any reader should work that uses the Wiegand protocol

Total Cost without Reader: $58.84/$14.34

## 3d Printed Cases

 I have the cases uploaded to Thingiverse but in case it goes down ill attach them to the repository.  Ive made 2 cases for the two different form factors of the Rp15 and 
 R10 Multiclass Se Readers 
 
 [RP15 Case STL - Top](https://github.com/Hamspiced/HID-Arduino-AccessController/blob/main/Cases/R15_Bottom.stl)
 
 [RP15 Case STL - Bottom](https://github.com/Hamspiced/HID-Arduino-AccessController/blob/main/Cases/R15__Top.stl)
 
 [R10 Case STL - Top](https://github.com/Hamspiced/HID-Arduino-AccessController/blob/main/Cases/R10_Case_Top.stl)
 
 [R10 Case STL - Bottom](https://github.com/Hamspiced/HID-Arduino-AccessController/blob/main/Cases/R10_Case_bottom.stl)
 

## 📌 ARDUINO Pinout Connections

### 🔌 HID Reader to Arduino Uno

| HID Wire Color | Arduino Uno Pin |
|----------------|------------------|
| Black          | GND              |
| Green          | D2               |
| White          | D3               |
| Brown          | D5               |
| Orange         | D6               |
| Yellow         | A3               |

### 📟 OLED to Arduino

| OLED Pin | Arduino Uno Pin |
|----------|------------------|
| VCC      | 5V               |
| GND      | GND              |
| SCL      | A5               |
| SDA      | A4               |

### ⚡ Power Input to Step-Down Converter

| Power Input Wire | Step-Down Pin |
|------------------|----------------|
| Black            | - IN           |
| Red              | + IN           |

### 🔋 Step-Down Converter to Arduino

| Step-Down Pin | Arduino Uno Pin |
|---------------|------------------|
| - OUT         | GND              |
| + OUT         | VIN              |

 Note: Match the Pins to the pin on that actual microcontroller and not to the printed labels on the breakout board.

## Assembly
 Next assemble the OLED into the case, you may need a dab of hot glue to secureit. 
 Attach the Readerto the case next its easiest to keep the base plate on and thread the screws down from the reader into the case and use nuts to secure the case to the reader and trim off excess of the screw.
 attach the breakout board to the bottom case
 attach MCU to the breakout board.  
 Stepdown module can go in its slot and it should clip into place.  
 Feed the wires for the pig tail USB C into the case from outside and press fit into place, then solder the connection to the stepdown converter In + and -
 You can use Heatset inserts or just screws to attach to the bottom of the case.

Note: When programming you have to connect USB C directly to the arduino, The USB-C Pigtail is power only it wont pass data through so you cant update your Arduino with this connection.  Likewise you cannot power the reader directly by the Arduino power, it isnt poweful enough for that.

# Flashing Arduino via Arduino IDE

Download and install Arduino IDE.
You can copy the code file itself, or copy and paste the code into IDE.  Some MCU's from china require you to select old bootloader, or power up while pressing the reset button. 

# Use
On Startup the Reader defaults to Read Mode.  To switch modes you will need to pre-program 4 cards or add your card UID to the code  by default it is:


### 🏷️ RFID Mode UIDs and Proxmark3 Commands

| Mode         | UID         | LF Command (T5577)             | HF Command (Magic 1K Card)                           |
|--------------|-------------|--------------------------------|------------------------------------------------------|
| Read Mode    | `0x1E6DC032` | `lf em 410xwrite 1E6DC032`     | `hf mf csetuid -u 1E6DC032`                          |
| Door Mode    | `0x1E6D9861` | `lf em 410xwrite 1E6D9861`     | `hf mf csetuid -u 1E6D9861`                          |
| Add Mode     | `0x1E6DFCA0` | `lf em 410xwrite 1E6DFCA0`     | `hf mf csetuid -u 1E6DFCA0`                          |
| Remove Mode  | `0x1E6DE636` | `lf em 410xwrite 1E6DE636`     | `hf mf csetuid -u 1E6DE636`                          |

Door Mode will send a Signal on Pin6 to activate a Door Relay when authorized cards are scanned.
When Add Mode is selected, when a card is scanned it is added to the controller
when Remove mode is selected if that card is scanned it is removed from the controller

TODO: Scanning the remove mode card 3 times will reset all added cards (not mode cards)



# ESP32 Installation

I use Arduino IDE for the ESP32.  I had to download the ESP32Little Fs plugin for Arduino IDE 2.x.   More information on it can be found from [earlephilhower's Github](https://github.com/earlephilhower/arduino-littlefs-upload) the TL;DR is: 

Copy the VSIX file to ~/.arduinoIDE/plugins/ on Mac and Linux or C:\Users\<username>\.arduinoIDE\plugins\ on Windows (you may need to make this directory yourself beforehand).

Once the plugin is in it's folder.  You can load the INO file into your sketch.  In the sketch folder, in the root create a directory called "data" and place the html file in there.  Then, Upload your code to your ESP32 (selecting the proper board and port).  When upload is complete: 

On Windows: press [Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32".

On macOS: press [⌘] + [Shift] + [P] to open the Command Palette in the Arduino IDE, then "Upload LittleFS to Pico/ESP8266/ESP32".

It will upload the data folder with the web page interface.


  ## ESP32 Usage

The ESP32 adds a Web Interface to the RFID Reader.  It will allow you to access a webpage (192.168.4.1 when in Access Point Mode) that will allow you to add Admin Cards, Trigger the door sensor, and display a card log.  

  ##REST Api

  There are Endpoints for the REST API already added to the code.  The Endpoint webhooks are within the code.  I dont use Home Assistant but if you are savvy you should be able to use this with home assistant.
    



## Pinouts for ESP32 devices
   
   ### 📌 Xiao ESP32C3 Pin Mapping

   | Function      | Physical Pin | Arduino Pin |
   |---------------|--------------|-------------|
   | Wiegand D0    | 2            | D2          |
   | Wiegand D1    | 3            | D3          |
   | Beeper        | 6            | D6          |
   | Door Strike   | 7            | D7          |
   | I2C SDA       | 4            | D4          |
   | I2C SCL       | 5            | D5          |

   ### 📌  ESP32 Pin Mapping

   | Function      | GPIO         | Label          |
   |---------------|--------------|----------------|
   | Wiegand D0    | 16           | Wiegand D0 Pin |
   | Wiegand D1    | 17           | Wiegand D1 Pin |
   | Beeper        | 26           | Beeper Pin     |
   | Door Strike   | 27           | Signal Pin     |
   | I2C SDA       | 21           | SSD1306 OLED   |
   | I2C SCL       | 22           | SSD1306 OLED   |


 # Use

 When booted up the esp32 will broadcast a Wifi Signal named: Door-Simulator.  Encryption Key is "12345678".  In the code you can also specify your own wifi network for it to connect to.  By default the default AP will host the webservice on: 192.168.4.1
   

## 🙏 Acknowledgments

Huge thanks to [Peekaboo](https://github.com/Peekaboo-ICU) for their invaluable support in helping bring this project to life.  
From debugging my bad code to offering key insights and improvements, their contributions were instrumental in getting this over the finish line.  

Explore more of their work at [Peekaboo.icu](https://peekaboo.icu)


 
## License

This project is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)](https://creativecommons.org/licenses/by-nc/4.0/).

You are free to use, modify, and share this code for **non-commercial purposes**, provided proper attribution is given.  
**Commercial use is not permitted** without prior written consent. Please [contact the author](mailto:your.email@example.com) to inquire about commercial licensing.
