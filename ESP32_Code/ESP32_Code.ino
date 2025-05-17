#include <Arduino.h>
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

// --- Wi-Fi and Static IP Configuration ---
String userSSID = "YourSSID";
String userPassword = "YourPassword";
bool useHardcodedIP = false; // Set to true to use static IP below
IPAddress hardcodedIP(192, 168, 1, 181);
IPAddress hardcodedGW(192, 168, 1, 1);
IPAddress hardcodedSN(255, 255, 255, 0);

const char* ap_ssid = "RFID-Door-Sim";
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

#define MODE_CARD_READ   0x1E6DC032
#define MODE_CARD_DOOR   0x1E6D9861
#define MODE_CARD_ADD    0x1E6DFCA0
#define MODE_CARD_REMOVE 0x1E6DE636

const unsigned long configCards[4] = {
  MODE_CARD_READ,
  MODE_CARD_DOOR,
  MODE_CARD_ADD,
  MODE_CARD_REMOVE
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
  } else if (server.hasArg("scan")) {
    unsigned long scannedCard = strtoul(server.arg("scan").c_str(), NULL, 16);
    for (uint8_t i = 0; i < 4; i++) {
      if (scannedCard == configCards[i]) {
        currentMode = i + 1;
        break;
      }
    }
  }
  server.send(200, "text/plain", "Mode Set");
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
  server.on("/wifi", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
      String newSSID = server.arg("ssid");
      String newPASS = server.arg("pass");
      String newIP = server.hasArg("ip") ? server.arg("ip") : "";
      String newGW = server.hasArg("gw") ? server.arg("gw") : "";
      String newSN = server.hasArg("subnet") ? server.arg("subnet") : "";

      File config = LittleFS.open("/wifi.txt", "w");
      config.println(newSSID);
      config.println(newPASS);
      config.println(newIP);
      config.println(newGW);
      config.println(newSN);
      config.close();

      server.send(200, "text/plain", "Wi-Fi settings saved. Please reboot.");
    } else {
      server.send(400, "text/plain", "Missing SSID or Password");
    }
  });
}

void connectWiFi() {
  WiFi.mode(WIFI_AP_STA);
  String ssidVal = userSSID;
  String passVal = userPassword;
  IPAddress local_IP, gateway, subnet;

  if (useHardcodedIP) {
    WiFi.config(hardcodedIP, hardcodedGW, hardcodedSN);
  } else if (LittleFS.exists("/wifi.txt")) {
    File f = LittleFS.open("/wifi.txt", "r");
    ssidVal = f.readStringUntil('\n');
    passVal = f.readStringUntil('\n');
    String ip = f.readStringUntil('\n');
    String gw = f.readStringUntil('\n');
    String sn = f.readStringUntil('\n');
    ssidVal.trim(); passVal.trim();
    ip.trim(); gw.trim(); sn.trim();
    f.close();

    if (ip.length() && gw.length() && sn.length()) {
      if (local_IP.fromString(ip) && gateway.fromString(gw) && subnet.fromString(sn)) {
        WiFi.config(local_IP, gateway, subnet);
      }
    }
  }

  WiFi.begin(ssidVal.c_str(), passVal.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("Started Access Point");
    Serial.println(WiFi.softAPIP());
  }
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

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Mode: ");
    display.println(modeName(currentMode));

    display.setCursor(0, 12);
    display.print("Card: 0x");
    display.println(lastCardData, HEX);

    display.setCursor(0, 24);
    display.print("Facility: ");
    display.println(lastFacilityCode);

    display.setCursor(0, 36);
    display.print("ID: ");
    display.println(lastCardNumber);

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

    display.display();
  }
}
