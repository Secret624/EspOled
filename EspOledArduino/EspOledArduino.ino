#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <NTPClient.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels


#define I2C_SDA 26
#define I2C_SCL 27

const char* ntpServer = "1.fi.pool.ntp.org";
const long timeZoneOffset = 7201;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, timeZoneOffset);
unsigned long lastMillis = 120000;
const unsigned long interval = 60000;

const char* SSID = "";
const char* PASS = "";
const char* FUN = "https://funoulutalvikangas.fi/?controller=ajax&getentriescount=1&locationId=1";
const char* WILLAB = "https://weather.willab.fi/weather.json";


// create an OLED display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

float read(String url, String field){
  
  HTTPClient http;
  String response;

  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return 0;
    }
    return doc[field];

  } else {
    Serial.print("Error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  while (!Serial);

  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }
  delay(2000);         // wait two seconds for initializing
  oled.clearDisplay(); // clear display

  oled.setTextColor(WHITE);

  timeClient.begin();  
}

void loop() {

  float visitors;
  float temperature;

  timeClient.update();

  if (millis() - lastMillis >= interval) {
    lastMillis = millis();
    visitors = read(FUN, "entriesTotal");
    temperature = read(WILLAB, "tempnow");
  }
  
  oled.clearDisplay();
  oled.setCursor(0, 0);
  
  oled.setTextSize(3);
  oled.print("KVJ:");
  oled.println(int(visitors));

  oled.setTextSize(2);
  oled.print(temperature, 1);
  oled.print((char)247);
  oled.println("C");

  oled.setTextSize(2);
  oled.println(timeClient.getFormattedTime());

  oled.display();
  delay(10);
}
