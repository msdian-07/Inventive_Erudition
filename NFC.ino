#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define RELAY_PIN 23

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
RTC_DS3231 rtc;

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
const String TEACHER_UID = "04A8D27F";
const String SHEET_ID = "YOUR_GOOGLE_SHEET_ID";
const String API_KEY = "YOUR_GOOGLE_API_KEY";

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  nfc.begin();
  rtc.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  rtc.adjust(DateTime(random(2023, 2025), random(1, 13), random(1, 29), random(0, 24), random(0, 60), random(0, 60)));
}

void loop() {
  if (nfc.tagPresent()) {
    String uid = readNFC();
    if (uid == TEACHER_UID) {
      unlockDoor();
      recordAccess();
    }
  }
  delay(1000);
}

String readNFC() {
  String uid = "";
  uint8_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    return uid;
  }

  nfc.SAMConfig();
  NFC_Type uidType = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
  if (uidType == NFC_FOUND) {
    uid = String(nfc.getUidString());
    Serial.println("NFC UID: " + uid);
  }
  return uid;
}

void unlockDoor() {
  digitalWrite(RELAY_PIN, HIGH);
  delay(3000);
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Door unlocked.");
}

void recordAccess() {
  DateTime now = rtc.now();
  String dateTime = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " +
                    String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

  String json = "{\"values\": [[\"" + dateTime + "\", \"teacher\"]]}";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://sheets.googleapis.com/v4/spreadsheets/" + SHEET_ID + "/values/A1:append?valueInputOption=USER_ENTERED&key=" + API_KEY;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(json);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }
    http.end();
  }
}
