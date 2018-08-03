// Include application, user and local libraries
#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>


const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

//Check https://github.com/jorgegarciadev/TFT_22_ILI9225/wiki for wiring

#ifdef ARDUINO_ARCH_STM32F1
#define TFT_RST PA1
#define TFT_RS  PA2
#define TFT_CS  PA0 // SS
#define TFT_SDI PA7 // MOSI
#define TFT_CLK PA5 // SCK
#define TFT_LED 0 // 0 if wired to +5V directly
#else
#define TFT_RST 2
#define TFT_RS  4
#define TFT_CS  15  // SS
#define TFT_SDI 13  // MOSI
#define TFT_CLK 14  // SCK
#define TFT_LED 5   // 0 if wired to +5V directly
#endif

#define TFT_BRIGHTNESS 400 // Initial brightness of TFT backlight (optional)

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
// Use software SPI (slower)
//TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);

// Variables and constants
uint16_t x, y;
boolean flag = false;

WiFiClientSecure client;

//json bufferSize calculated at https://arduinojson.org/v5/assistant/
//This is different for most Arduino IDE-compatible boards, so check
const size_t bufferSize = JSON_OBJECT_SIZE(40) + 930;
DynamicJsonBuffer jsonBuffer(bufferSize);

//screenchanger is a boolean that I flip in the code. Depending on its value the position of the text
//on the TFT is changed. This just avoids burn-in on the screen
bool screenchanger = 0;

const char* host = "api.iextrading.com";
const char* fingerprint = "‎‎d1 34 42 d6 30 58 2f 09 a0 8c 48 b6 25 b4 6c 05 69 a4 2e 4e";

void setup() {
  tft.begin();
  Serial.begin(9600);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting...");
  }
}

void loop() {
  Serial.println("Looping...");

  if (WiFi.status() == WL_CONNECTED) 
  {
    String url = "/1.0/stock/STOCK_SHORT_CODE_HERE/quote";
    Serial.print("requesting URL: ");
    Serial.println(url);

    if (!client.connect(host, 443)) {
      Serial.println("connection failed");
      return;
    }

    /*if (client.verify(fingerprint, host)) {
      Serial.println("certificate matches");
    } 
    else {
      Serial.println("certificate doesn't match");
    }*/

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: GetStock\r\n" +
               "Accept: application/json\r\n" +
               "Connection: close\r\n\r\n");

    Serial.println("request sent");
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }
    //String line = client.readStringUntil('\n');

    JsonObject& root = jsonBuffer.parseObject(client.readStringUntil('\n'));
    String time_req = root["latestTime"];
    float price = root["latestPrice"];
    float temp_change = root["changePercent"];
    float change = 100 * temp_change;
    String price_string = "$" + String(price);
    String change_string = String(change) + "%";

    if (price == 0.00) {
      ESP.restart();
    }
        /*if (line.startsWith("{\"state\":\"success\"")) {
      Serial.println("esp8266/Arduino CI successfull!");
    } 
    else {
      Serial.println("esp8266/Arduino CI has failed");
    }*/
    Serial.println("reply was:");
    Serial.println("==========");
    Serial.println(price);
    Serial.println(change);
    Serial.println("==========");
    Serial.println("closing connection");
    if (screenchanger) {
      tft.clear();
      tft.setOrientation(1);
      tft.setFont(Terminal12x16);
      tft.drawText(43, 30, "STOCK_SHORT_CODE stock at", COLOR_GREEN);
      tft.drawText(55, 50, time_req, COLOR_GREEN);
      tft.drawText(80, 80, price_string, COLOR_WHITE);
      tft.drawText(80, 100, change_string, COLOR_WHITE);
      screenchanger = !screenchanger;
    }
    else {
      tft.clear();
      tft.setOrientation(1);
      tft.setFont(Terminal12x16);
      tft.drawText(43, 40, "STOCK_SHORT_CODE stock at", COLOR_GREEN);
      tft.drawText(55, 60, time_req, COLOR_GREEN);
      tft.drawText(72, 90, price_string, COLOR_WHITE);
      tft.drawText(80, 110, change_string, COLOR_WHITE);
      screenchanger = !screenchanger;
    }

  }
  delay(900000);
}
