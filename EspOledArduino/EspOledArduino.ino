#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <esp_task_wdt.h> //board manager ESP32 2.0.17
#include <Preferences.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
#define WDT_TIMEOUT 10


#define I2C_SDA 14
#define I2C_SCL 21
#define button1 13
#define encoderCW 16
#define encoderCCW 17
#define encoderSW 18
#define debounce 300
//#define I2C_SDA 27
//#define I2C_SCL 26

const char* ntpServer = "1.fi.pool.ntp.org";
const long timeZoneOffset = 7201;
WiFiUDP ntpUDP;
int daylightSavingTime;
NTPClient timeClient(ntpUDP, ntpServer);
Preferences preferences;

unsigned long lastMillis = 120000;
const unsigned long interval = 65000;
volatile unsigned long last_time;
bool TZFlag = 0;
String SSID;
String PASS;
const char* FUN = "https://funoulutalvikangas.fi/?controller=ajax&getentriescount=1&locationId=1";
const char* WILLAB = "https://weather.willab.fi/weather.json";
bool httpInitialized = false;
HTTPClient http;

bool screenState = 1;

// create an OLED display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

float read(String url, String field){
  
  if (!httpInitialized) {
    http.begin(url); // Initialize the connection only once
    httpInitialized = true;
  }
  
  String response;

  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode == HTTP_CODE_OK) {
    String response = http.getString();

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      //Serial.print(F("deserializeJson() failed: "));
      //Serial.println(error.f_str());
      return 0;
    }
    return doc[field];

  } else {
    //Serial.print("Error: ");
    //Serial.println(httpResponseCode);
  }

  http.end();
}

void IRAM_ATTR ISR() {
  screenState = !screenState;
}

void changeTZ(){
  TZFlag = 0;
  daylightSavingTime = 3600 - daylightSavingTime;
  preferences.putInt("DST", daylightSavingTime);
  timeClient.setTimeOffset(timeZoneOffset + daylightSavingTime);
}

void IRAM_ATTR encSW() {
  if ((millis() - last_time) < debounce)
    return;
  TZFlag = 1;
  last_time = millis();
}

void setup() {
  Serial.begin(115200);
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  Wire.begin(I2C_SDA, I2C_SCL);
  attachInterrupt(button1, ISR, RISING);
  attachInterrupt(encoderSW, encSW, RISING);
  //while (!Serial);
  // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    //Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }
  delay(2000);         // wait two seconds for initializing
  oled.clearDisplay(); // clear display
  esp_task_wdt_reset();
  oled.setTextColor(WHITE);
  oled.dim(1);


  preferences.begin("config", false); // Open the preferences with read/write access

  // Attempt to read the stored DST value
  daylightSavingTime = preferences.getInt("DST", -1); // Default to -1 for debugging
  if (daylightSavingTime == -1) {
    daylightSavingTime = 0;
    preferences.putInt("DST", daylightSavingTime);
  }

  SSID = preferences.getString("SSID", "");
  PASS = preferences.getString("PASS", "");

  if(SSID == ""){
    Serial.println("SSID: ");
    oled.clearDisplay();
    oled.setCursor(4, 4);
    oled.setTextSize(3);
    oled.println("SSID:");
    oled.display();
  }
  while(SSID == ""){
    if(Serial.available()){
      SSID = Serial.readStringUntil('\n');
    }
    esp_task_wdt_reset();
  }
  if(PASS == ""){
    Serial.println("PASS: ");
    oled.clearDisplay();
    oled.setCursor(4, 4);
    oled.setTextSize(3);
    oled.println("PASS:");
    oled.display();
  }
  while(PASS == ""){
    if(Serial.available()){
      PASS = Serial.readStringUntil('\n');
    }
    esp_task_wdt_reset();
  }
  preferences.putString("SSID", SSID);
  preferences.putString("PASS", PASS);

  bool wifiLoop = 0;
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    if(!wifiLoop){
      oled.clearDisplay();
      oled.setCursor(4, 4);
      oled.setTextSize(3);
      oled.println("WAIT");
      oled.display();
    }
    wifiLoop = 1;
    if(TZFlag){
      oled.clearDisplay();
      oled.setCursor(0, 4);
      oled.setTextSize(3);
      oled.println("CLEARED");
      oled.display();
      preferences.clear();
      ESP.restart();
    }
    esp_task_wdt_reset();
  }


  esp_task_wdt_reset();
  timeClient.setTimeOffset(timeZoneOffset + daylightSavingTime);
  timeClient.begin();
}

bool screenColor = 0;

void loop() {

  float visitors;
  float temperature;

  timeClient.update();

  //oled.invertDisplay(timeClient.getMinutes() % 2);

  if (millis() - lastMillis >= interval || !timeClient.getSeconds()) {
    lastMillis = millis();
    visitors = read(FUN, "entriesTotal");
    //temperature = read(WILLAB, "tempnow");
    //while (WiFi.status() != WL_CONNECTED) {}
  }
  
  oled.clearDisplay();
  oled.setCursor(4, 4);
  oled.setTextSize(3);
  oled.print("KVJ:");
  oled.println(int(visitors));
  oled.setCursor(4, 38);
  if(timeClient.getHours() < 10){
    oled.print("0");
  }
  oled.print(timeClient.getHours());
  oled.print(":");
  if(timeClient.getMinutes() < 10){
    oled.print("0");
  }
  oled.print(timeClient.getMinutes());
  oled.setCursor(91, 45);
  oled.setTextSize(2);
  oled.println(":");
  oled.setCursor(100, 45);
  if(timeClient.getSeconds() < 10){
    oled.print("0");
  }
  oled.println(timeClient.getSeconds());
  if(!screenState){
    oled.clearDisplay();
    oled.setRotation(0);
  }
  oled.display();
  esp_task_wdt_reset();
  delay(10);
  if(TZFlag){
    changeTZ();
  }
}
