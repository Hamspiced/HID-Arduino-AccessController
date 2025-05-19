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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WIEGAND wg;

bool cardDisplayed = false;
unsigned long lastCardData = 0;
uint8_t lastBitLength = 0;
uint8_t lastFacilityCode = 0;
uint16_t lastCardNumber = 0;

#define MODE_READ 1
#define MODE_DOOR 2
#define MODE_ADD 3
#define MODE_REMOVE 4
uint8_t currentMode = MODE_READ;

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
  display.setCursor(25, 25);
  display.println("Scan a card...");
  display.display();
  cardDisplayed = false;
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
    display.print("Hex: 0x");
    display.println(cardData, HEX);
    display.setCursor(0, 30);
    display.print("Raw: ");
    display.println(cardData);
    display.setCursor(0, 40);
    display.print("Bits: ");
    display.print(bitLength);
    display.println(" (approx)");
    display.setCursor(0, 50);
    display.print("FC: ");
    display.print(facilityCode);
    display.print(" ID: ");
    display.println(cardNumber);
  } else {
    display.setCursor(30, 20); // Center message
    display.println(message);
  }
  display.display();
  cardDisplayed = true;
}

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(25, 16);
  display.println("RFIDsim");
  display.setTextSize(1);
  display.setCursor(31, 40);
  display.println("v1.0 By Hamspiced and Peekaboo");
  display.display();
  delay(1000);

  loadAuthorizedCards();
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

    // Check for configuration card
    for (uint8_t i = 0; i < 4; i++) {
      if (cardData == configCards[i]) {
        currentMode = i + 1;
        displayScanPrompt();
        digitalWrite(BEEPER_PIN, LOW);
        delay(100);
        digitalWrite(BEEPER_PIN, HIGH);
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
          displayCardData(cardData, bitLength, facilityCode, cardNumber, "Authorized");
          delay(5000); // Show for 5 seconds
          digitalWrite(SIGNAL_PIN, LOW);
          displayScanPrompt();
        } else {
          displayCardData(cardData, bitLength, facilityCode, cardNumber, "Access Denied");
          delay(5000); // Show for 5 seconds
          displayScanPrompt();
        }
        break;
      case MODE_ADD:
        addCard(cardData);
        displayCardData(cardData, bitLength, facilityCode, cardNumber, "Card Added");
        break;
      case MODE_REMOVE:
        removeCard(cardData);
        displayCardData(cardData, bitLength, facilityCode, cardNumber, "Card Removed");
        break;
    }
  } else if (cardDisplayed) {
    displayCardData(lastCardData, lastBitLength, lastFacilityCode, lastCardNumber);
  } else {
    displayScanPrompt();
  }
}
