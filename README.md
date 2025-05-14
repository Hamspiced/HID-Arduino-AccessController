# HID-Arduino-AccessController
This is a project repository for creating a full fledged door access controller using an HID Reader, Arduino, and i2c screen.  This project will allow you to use the reader as a simple badge scanner, act as a door controller, identify authorized access control cards, add cards, and delete cards.

## Parts
 [Arduino Nano](https://a.co/d/4Q9VMcR) - $16.49 per 3 or $5.49 ea
 [USB-C Pigtail](https://a.co/d/i3n7h48) - $7.99 per 10 or $0.80 ea
 [Arduino Breakout Board](https://a.co/d/hGXZX4y) - $8.79 per 3 or $2.93 ea
 [5v Buck Converter](https://a.co/d/7Qql78y) - $11.59 per 5 or $2.32 ea
 [i2c 2 Color OLED](https://a.co/d/0r9uNRJ) - $13.98 per 5 or $2.80 ea
 HID R10 or R15 Multiclass Reader - Ebay or Marketplace Price Varies between 20-90$ Any reader should work that uses the Wiegand protocol

## 3d Printed Cases

 I have the cases uploaded to Thingiverse but in case it goes down ill attach them to the repository.  Ive made 2 cases for the two different form factors of the Rp15 and 
 R10 Multiclass Se Readers 
 
 RP15 Case STL - Top
 RP15 Case STL - Bottom
 R10 Case STL - Top
 R10 Case STL - Bottom

##Pinout
 ### 🔌 HID Reader to Arduino Uno
 Black   → GND
 Green   → D2
 White   → D3
 Brown   → D5
 Orange  → D6
 Yellow  → A3

 ### 📟 OLED to Arduino
 VCC     → 5V
 GND     → GND
 SCL     → A5
 SDA     → A4
 
 ### ⚡ USB-C Pigtail Power In to Step-Down Converter
 Black   → - IN
 Red     → + IN
 - OUT   → GND
 + OUT   → VIN

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
 
## License

This project is licensed under the [Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)](https://creativecommons.org/licenses/by-nc/4.0/).

You are free to use, modify, and share this code for **non-commercial purposes**, provided proper attribution is given.  
**Commercial use is not permitted** without prior written consent. Please [contact the author](mailto:your.email@example.com) to inquire about commercial licensing.
