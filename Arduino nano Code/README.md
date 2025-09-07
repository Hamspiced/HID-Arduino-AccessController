# Arduino Nano HID Door Simulator

Minimal offline door access controller for HID Wiegand readers using an Arduino Nano and SSD1306 OLED.

- Modes: READ, DOOR (relay output), ADD, REMOVE
- First-boot admin enrollment (prompt to scan an Admin card; stored in EEPROM)
- Authorized card list stored in EEPROM
- Simple OLED UI and beeper feedback; no WiFi/web/HA on Nano

## Hardware

- Reader (Wiegand):
  - D0 → Arduino `D2`
  - D1 → Arduino `D3`
  - Beeper → Arduino `A3`
- Door strike/relay signal: Arduino `D7`
- OLED (SSD1306 I2C):
  - SDA → `A4`
  - SCL → `A5`
- Power: 5V via step-down; the Arduino USB-C pigtail is power-only and does not pass data

## First Boot: Admin Enrollment

On first boot (or after EEPROM reset) the OLED shows:
- "ADMIN ENROLL"
- "Scan Admin Card..."

Scan the badge you want to act as the Admin. It is saved to EEPROM and used to cycle modes when scanned later. This prompt will not reappear unless you clear EEPROM.

## Using the Modes

- READ: shows card info on OLED
- DOOR: checks authorization; if authorized, raises `D7` for ~5 seconds
- ADD: scans are added to the authorized list (EEPROM)
- REMOVE: scans are removed from the authorized list (EEPROM)

You can switch modes in two ways:
1) Scan the Admin card to cycle READ → DOOR → ADD → REMOVE → READ
2) Scan one of these pre-programmed “mode cards” (Wiegand raw):
   - READ: `0x1E6DC032`
   - DOOR: `0x1E6D9861`
   - ADD: `0x1E6DFCA0`
   - REMOVE: `0x1E6DE636`

## Serial Logging (optional)

Serial is disabled by default. To enable:

```cpp
#define ENABLE_SERIAL 1
```

Then open the Serial Monitor at 115200 baud to see card scans and actions.

## EEPROM Layout (summary)

- `[0]` card count (1 byte)
- `[4..]` authorized cards table (`MAX_CARDS * 4` bytes, little-endian)
- `EEPROM_ADMIN_UID_ADDR` (4 bytes): admin UID
- `EEPROM_ADMIN_FLAG_ADDR` (1 byte): `0xA5` when admin enrolled

To reset admin or clear all authorized cards, wipe the EEPROM (e.g., with a small Arduino sketch) and reboot to re-enroll.

## Building/Flashing

- Board: Arduino Nano (select the correct processor/old bootloader if required by your unit)
- Open `HIDR10-RP15-Door-Simulator/HIDR10-RP15-Door-Simulator.ino`
- Install required libraries: `Adafruit_GFX`, `Adafruit_SSD1306`, `Wiegand`, `EEPROM`
  - Wiegand library: `monkeyboard/Wiegand-Protocol-Library-for-Arduino` (`https://github.com/monkeyboard/Wiegand-Protocol-Library-for-Arduino`)
- Compile and upload via USB directly to the Arduino (the in-case USB-C pigtail is power-only)

## Notes

- The Nano build intentionally omits WiFi/web UI/REST/HA integrations. See the repository root README for the ESP32 variant if you need those features.
