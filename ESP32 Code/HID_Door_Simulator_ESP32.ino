#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wiegand.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define WIEGAND_D0_PIN 16
#define WIEGAND_D1_PIN 17
#define BEEPER_PIN     26
#define SIGNAL_PIN     27

const char* ssid = "YourSSID";
const char* password = "YourPassword";
const char* ap_ssid = "Door-Simulator";
const char* ap_password = "12345678";

WebServer server(80);
WIEGAND wg;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MAX_CARDS 20
#define CARD_SIZE sizeof(unsigned long)
const char* cardsFile = "/cards.txt";

unsigned long authorizedCards[MAX_CARDS];
uint8_t cardCount = 0;

#define MODE_READ   1
#define MODE_DOOR   2
#define MODE_ADD    3
#define MODE_REMOVE 4

uint8_t currentMode = MODE_READ;
unsigned long lastCardData = 0;
uint8_t lastBitLength = 0;
uint8_t lastFacilityCode = 0;
uint16_t lastCardNumber = 0;

const unsigned long configCards[4] = {
  0x1E6DC032, 0x1E6D9861, 0x1E6DFCA0, 0x1E6DE636
};

unsigned long parseCard(String line) {
  return strtoul(line.c_str(), NULL, 10);
}

void loadAuthorizedCards() {
  cardCount = 0;
  if (!LittleFS.exists(cardsFile)) return;

  File file = LittleFS.open(cardsFile, "r");
  while (file.available() && cardCount < MAX_CARDS) {
    String line = file.readStringUntil('\n');
    authorizedCards[cardCount++] = parseCard(line);
  }
  file.close();
}

void saveAuthorizedCards() {
  File file = LittleFS.open(cardsFile, "w");
  for (uint8_t i = 0; i < cardCount; i++) {
    file.println(String(authorizedCards[i]));
  }
  file.close();
}

bool isAuthorized(unsigned long cardData) {
  for (uint8_t i = 0; i < cardCount; i++) {
    if (authorizedCards[i] == cardData) return true;
  }
  return false;
}

void addCard(unsigned long cardData) {
  if (cardCount < MAX_CARDS && !isAuthorized(cardData)) {
    authorizedCards[cardCount++] = cardData;
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

String modeName(uint8_t mode) {
  switch (mode) {
    case MODE_READ: return "READ MODE";
    case MODE_DOOR: return "DOOR MODE";
    case MODE_ADD: return "ADD MODE";
    case MODE_REMOVE: return "REMOVE MODE";
    default: return "UNKNOWN";
  }
}

String getCardJSON() {
  String json = "{";
  json += "\"mode\":\"" + modeName(currentMode) + "\",";
  json += "\"card\":\"0x" + String(lastCardData, HEX) + "\",";
  json += "\"fc\":\"" + String(lastFacilityCode) + "\",";
  json += "\"id\":\"" + String(lastCardNumber) + "\"";
  json += "}";
  return json;
}

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleStatus() {
  server.send(200, "application/json", getCardJSON());
}

void handleSetMode() {
  if (server.hasArg("m")) {
    uint8_t mode = server.arg("m").toInt();
    if (mode >= MODE_READ && mode <= MODE_REMOVE) {
      currentMode = mode;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleTriggerDoor() {
  digitalWrite(SIGNAL_PIN, HIGH);
  delay(5000);
  digitalWrite(SIGNAL_PIN, LOW);
  server.send(200, "text/plain", "Triggered");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/mode", handleSetMode);
  server.on("/trigger", handleTriggerDoor);
  server.begin();
}

void connectWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  WiFi.softAP(ap_ssid, ap_password);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(20, 20);
  display.println("RFIDsim");
  display.display();
  delay(1000);

  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, HIGH);
  pinMode(SIGNAL_PIN, OUTPUT);
  digitalWrite(SIGNAL_PIN, LOW);

  wg.begin(WIEGAND_D0_PIN, WIEGAND_D1_PIN);
  loadAuthorizedCards();
  connectWiFi();
  setupWebServer();
}

void loop() {
  server.handleClient();

  if (wg.available()) {
    lastCardData = wg.getCode();
    lastBitLength = 0;
    unsigned long data = lastCardData;
    while (data) { lastBitLength++; data >>= 1; }

    lastFacilityCode = (lastCardData >> 16) & 0xFF;
    lastCardNumber = lastCardData & 0xFFFF;

    digitalWrite(BEEPER_PIN, LOW);
    delay(50);
    digitalWrite(BEEPER_PIN, HIGH);

    switch (currentMode) {
      case MODE_READ:
        Serial.printf("Card: 0x%08lX\n", lastCardData);
        break;
      case MODE_DOOR:
        if (isAuthorized(lastCardData)) {
          digitalWrite(SIGNAL_PIN, HIGH);
          delay(5000);
          digitalWrite(SIGNAL_PIN, LOW);
        }
        break;
      case MODE_ADD:
        addCard(lastCardData);
        break;
      case MODE_REMOVE:
        removeCard(lastCardData);
        break;
    }
  }
}
