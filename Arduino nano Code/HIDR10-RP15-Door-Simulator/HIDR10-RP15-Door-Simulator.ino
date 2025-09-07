#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wiegand.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define WIEGAND_D0_PIN 2 // Green wire (Data 0)
#define WIEGAND_D1_PIN 3 // White wire (Data 1)
#define BEEPER_PIN A3
#define SIGNAL_PIN 7 // Door strike signal pin

#define MAX_CARDS 20
#define EEPROM_CARD_COUNT_ADDR 0
#define EEPROM_CARD_DATA_ADDR 4
#define CARD_SIZE sizeof(unsigned long)

#define ADMIN_UID_DEFAULT 253411519UL
#define EEPROM_ADMIN_UID_ADDR (EEPROM_CARD_DATA_ADDR + (MAX_CARDS * CARD_SIZE))

// Set to 0 to silence all Serial output
#define ENABLE_SERIAL 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WIEGAND wg;

bool cardDisplayed = false;
bool promptDisplayed = false;
unsigned long lastCardData = 0;
uint8_t lastBitLength = 0;
uint8_t lastFacilityCode = 0;
uint16_t lastCardNumber = 0;

#define MODE_READ 1
#define MODE_DOOR 2
#define MODE_ADD 3
#define MODE_REMOVE 4
uint8_t currentMode = MODE_READ;

unsigned long adminUID = ADMIN_UID_DEFAULT;

const unsigned long configCards[4] = {
  0x1E6DC032, // Read mode
  0x1E6D9861, // Door mode
  0x1E6DFCA0, // Add mode
  0x1E6DE636  // Remove mode
};

unsigned long authorizedCards[MAX_CARDS];
uint8_t cardCount = 0;

uint8_t countBits(uint32_t data) {
  uint8_t count = 0;
  while (data) {
    data >>= 1;
    count++;
  }
  return count;
}

void loadAuthorizedCards() {
  cardCount = EEPROM.read(EEPROM_CARD_COUNT_ADDR);
  if (cardCount > MAX_CARDS) {
    cardCount = 0;
    return;
  }
  for (uint8_t i = 0; i < cardCount; i++) {
    unsigned long cardData = 0;
    for (uint8_t j = 0; j < CARD_SIZE; j++) {
      cardData |= (unsigned long)EEPROM.read(EEPROM_CARD_DATA_ADDR + i * CARD_SIZE + j) << (8 * j);
    }
    authorizedCards[i] = cardData;
  }
}

void saveAuthorizedCards() {
  EEPROM.update(EEPROM_CARD_COUNT_ADDR, cardCount);
  for (uint8_t i = 0; i < cardCount; i++) {
    unsigned long cardData = authorizedCards[i];
    for (uint8_t j = 0; j < CARD_SIZE; j++) {
      EEPROM.update(EEPROM_CARD_DATA_ADDR + i * CARD_SIZE + j, (cardData >> (8 * j)) & 0xFF);
    }
  }
}

bool isAuthorized(unsigned long cardData) {
  for (uint8_t i = 0; i < cardCount; i++) {
    if (authorizedCards[i] == cardData) {
      return true;
    }
  }
  return false;
}

void addCard(unsigned long cardData) {
  if (cardCount < MAX_CARDS && !isAuthorized(cardData)) {
    authorizedCards[cardCount] = cardData;
    cardCount++;
    saveAuthorizedCards();
  }
}

void removeCard(unsigned long cardData) {
  for (uint8_t i = 0; i < cardCount; i++) {
    if (authorizedCards[i] == cardData) {
      for (uint8_t j = i; j < cardCount - 1; j++) {
        authorizedCards[j] = authorizedCards[j + 1];
      }
      cardCount--;
      saveAuthorizedCards();
      break;
    }
  }
}

const char* modeName(uint8_t mode) {
  switch (mode) {
    case MODE_READ:   return "READ MODE";
    case MODE_DOOR:   return "DOOR MODE";
    case MODE_ADD:    return "ADD MODE";
    case MODE_REMOVE: return "DEL MODE";
    default:          return "UNKNOWN";
  }
}

unsigned long loadAdminUID() {
  unsigned long uid = 0;
  for (uint8_t i = 0; i < 4; i++) {
    uid |= (unsigned long)EEPROM.read(EEPROM_ADMIN_UID_ADDR + i) << (8 * i);
  }
  if (uid == 0xFFFFFFFFUL || uid == 0UL) {
    return ADMIN_UID_DEFAULT;
  }
  return uid;
}

void saveAdminUID(unsigned long uid) {
  for (uint8_t i = 0; i < 4; i++) {
    EEPROM.update(EEPROM_ADMIN_UID_ADDR + i, (uid >> (8 * i)) & 0xFF);
  }
}

void appendLogEntry(unsigned long raw, uint8_t fc, uint16_t id) {
  #if ENABLE_SERIAL
  Serial.print("RAW: "); Serial.print(raw);
  Serial.print(" | HEX: 0x"); Serial.print(raw, HEX);
  Serial.print(" | FC: "); Serial.print(fc);
  Serial.print(" | ID: "); Serial.print(id);
  Serial.print(" | BITS: "); Serial.print(countBits(raw));
  Serial.print(" | TYPE: "); Serial.print(wg.getWiegandType() == 26 ? "Wiegand26" : "Unknown");
  Serial.print(" | t="); Serial.println(millis());
  #endif
}

void displayScanPrompt() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(12, 0);
  switch (currentMode) {
    case MODE_READ:   display.println("READ MODE"); break;
    case MODE_DOOR:   display.println("DOOR MODE"); break;
    case MODE_ADD:    display.println("ADD MODE"); break;
    case MODE_REMOVE: display.println("DEL MODE"); break;
  }
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Scan Card to Begin...");
  display.display();
  cardDisplayed = false;
  promptDisplayed = true;
}

void displayCardData(unsigned long cardData, uint8_t bitLength, uint8_t facilityCode, uint16_t cardNumber, const char* message = "") {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(12, 0);
  switch (currentMode) {
    case MODE_READ:   display.println("READ MODE"); break;
    case MODE_DOOR:   display.println("DOOR MODE"); break;
    case MODE_ADD:    display.println("ADD MODE"); break;
    case MODE_REMOVE: display.println("DEL MODE"); break;
  }
  display.setTextSize(1);
  if (currentMode == MODE_READ) {
    display.setCursor(0, 20);
    display.print("Card: 0x");
    display.println(cardData, HEX);
    display.setCursor(0, 30);
    display.print("Facility: ");
    display.println(facilityCode);
    display.setCursor(0, 40);
    display.print("ID: ");
    display.println(cardNumber);
  } else {
    display.setCursor(30, 20); // Center message
    display.println(message);
  }
  display.display();
  cardDisplayed = true;
  promptDisplayed = false;
}

void setup() {
  #if ENABLE_SERIAL
  Serial.begin(115200);
  #endif
  Wire.begin();
  delay(50);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(18, 0);
  display.println("READ MODE");
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Scan Card to Begin...");
  display.display();
  delay(1000);

  loadAuthorizedCards();
  adminUID = loadAdminUID();
  #if ENABLE_SERIAL
  Serial.println("Booting HID RFID Door Simulator (Nano)");
  Serial.print("Loaded "); Serial.print(cardCount); Serial.println(" authorized card(s)");
  Serial.print("Admin UID: "); Serial.println(adminUID);
  Serial.print("Pins - D0:"); Serial.print(WIEGAND_D0_PIN);
  Serial.print(" D1:"); Serial.print(WIEGAND_D1_PIN);
  Serial.print(" BEEPER:"); Serial.print(BEEPER_PIN);
  Serial.print(" SIGNAL:"); Serial.println(SIGNAL_PIN);
  #endif
  displayScanPrompt();

  wg.begin(WIEGAND_D0_PIN, WIEGAND_D1_PIN);

  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, HIGH);
  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW);
}

void loop() {
  if (wg.available()) {
    unsigned long cardData = wg.getCode();

    // Admin card cycles mode
    if (cardData == adminUID) {
      currentMode++;
      if (currentMode > MODE_REMOVE) currentMode = MODE_READ;
      displayScanPrompt();
      digitalWrite(BEEPER_PIN, LOW);
      delay(100);
      digitalWrite(BEEPER_PIN, HIGH);
      #if ENABLE_SERIAL
      Serial.print("Admin card scanned. Mode -> "); Serial.println(modeName(currentMode));
      #endif
      return;
    }

    // Check for configuration card
    for (uint8_t i = 0; i < 4; i++) {
      if (cardData == configCards[i]) {
        currentMode = i + 1;
        displayScanPrompt();
        digitalWrite(BEEPER_PIN, LOW);
        delay(100);
        digitalWrite(BEEPER_PIN, HIGH);
        #if ENABLE_SERIAL
        Serial.print("Config card set mode -> "); Serial.println(modeName(currentMode));
        #endif
        return;
      }
    }

    // Process card data
    uint8_t bitLength = countBits(cardData);
    uint8_t facilityCode = (cardData >> 16) & 0xFF;
    uint16_t cardNumber = cardData & 0xFFFF;

    digitalWrite(BEEPER_PIN, LOW);
    delay(50);
    digitalWrite(BEEPER_PIN, HIGH);

    lastCardData = cardData;
    lastBitLength = bitLength;
    lastFacilityCode = facilityCode;
    lastCardNumber = cardNumber;

    // Handle mode-specific actions
    switch (currentMode) {
      case MODE_READ:
        displayCardData(cardData, bitLength, facilityCode, cardNumber);
        break;
      case MODE_DOOR:
        if (isAuthorized(cardData)) {
          digitalWrite(SIGNAL_PIN, HIGH);
          displayCardData(cardData, bitLength, facilityCode, cardNumber, "AUTHORIZED");
          delay(5000); // Show for 5 seconds
          digitalWrite(SIGNAL_PIN, LOW);
          displayScanPrompt();
          #if ENABLE_SERIAL
          Serial.println("Door triggered: AUTHORIZED (5s)");
          #endif
        } else {
          displayCardData(cardData, bitLength, facilityCode, cardNumber, "NO ENTRY");
          delay(5000); // Show for 5 seconds
          displayScanPrompt();
          #if ENABLE_SERIAL
          Serial.println("Access denied: NO ENTRY");
          #endif
        }
        appendLogEntry(cardData, facilityCode, cardNumber);
        break;
      case MODE_ADD:
        addCard(cardData);
        displayCardData(cardData, bitLength, facilityCode, cardNumber, "Card Added");
        #if ENABLE_SERIAL
        Serial.print("Card added: "); Serial.println(cardData);
        #endif
        break;
      case MODE_REMOVE:
        removeCard(cardData);
        displayCardData(cardData, bitLength, facilityCode, cardNumber, "Card Removed");
        #if ENABLE_SERIAL
        Serial.print("Card removed: "); Serial.println(cardData);
        #endif
        break;
    }

    if (currentMode != MODE_DOOR) {
      appendLogEntry(cardData, facilityCode, cardNumber);
    }
  } else if (cardDisplayed) {
    displayCardData(lastCardData, lastBitLength, lastFacilityCode, lastCardNumber);
  } else {
    if (!promptDisplayed) {
      displayScanPrompt();
    }
  }
}
