#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <time.h>

const char* ssid = "Z6";
const char* password = "Hajj2016";
const char* apiURL = "http://api.aladhan.com/v1/timingsByCity?city=LakeForest&country=USA&method=2"; 
const char* baseURL = "http://api.aladhan.com";  // Base URL for relative redirects

#define LED_PIN     2
#define NUM_LEDS    96
#define BRIGHTNESS  50

CRGB prayer_color = CRGB(0xe62e4d);  // Purple color
CRGB time_color = CRGB(0x1010ff);    // Cyan color


CRGB leds[NUM_LEDS];

String fajr, duhr, asr, maghrib, aisha;  // Store prayer times

// Store the LED positions
int fajrPos, duhrPos, asrPos, maghribPos, aishaPos;

// NTP server and time configuration 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;  // Adjust for your timezone (GMT-8 for PST)
const int   daylightOffset_sec = 3600;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void fetchPrayerTimes() {
  HTTPClient http;
  http.begin(apiURL);
  int httpCode = http.GET();

  // Handle HTTP 302 Redirect
  if (httpCode == 302) {
    String redirectURL = http.getLocation();
    Serial.println("Redirected to: " + redirectURL);
    http.end();  // End the previous connection

    // Check if the redirect URL is relative
    if (!redirectURL.startsWith("http")) {
      redirectURL = String(baseURL) + redirectURL;
      Serial.println("Full Redirect URL: " + redirectURL);
    }

    // Follow the redirection
    http.begin(redirectURL);
    httpCode = http.GET();
  }

  if (httpCode == 200) {  // Check if HTTP response is OK
    String payload = http.getString();
    StaticJsonDocument<2000> doc;
    deserializeJson(doc, payload);

    fajr = doc["data"]["timings"]["Fajr"].as<String>();
    duhr = doc["data"]["timings"]["Dhuhr"].as<String>();
    asr = doc["data"]["timings"]["Asr"].as<String>();
    maghrib = doc["data"]["timings"]["Maghrib"].as<String>();
    aisha = doc["data"]["timings"]["Isha"].as<String>();

    Serial.println("Prayer times fetched:");
    Serial.println("Fajr: " + fajr);
    Serial.println("Duhr: " + duhr);
    Serial.println("Asr: " + asr);
    Serial.println("Maghrib: " + maghrib);
    Serial.println("Isha: " + aisha);
  } else {
    Serial.println("Failed to retrieve prayer times. HTTP error code: " + String(httpCode));
  }
  http.end();
}

String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeStr[6]; // Format time as HH:MM
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
  return String(timeStr);
}

int timeToLedPos(String time) {
  int hours = time.substring(0, 2).toInt();   
  int minutes = time.substring(3, 5).toInt();
  int totalMinutes = (hours * 60) + minutes;

  // Assuming each LED represents 15 minutes (96 LEDs for 24 hours)
  int ledPos = totalMinutes / 15;
  return ledPos;
}



void initNewDay(){

  fetchPrayerTimes();

  // Convert prayer times to LED positions
  fajrPos = timeToLedPos(fajr);
  duhrPos = timeToLedPos(duhr);
  asrPos = timeToLedPos(asr);
  maghribPos = timeToLedPos(maghrib);
  aishaPos = timeToLedPos(aisha);

  Serial.println("Fajr LED Position: " + String(fajrPos));
  Serial.println("Duhr LED Position: " + String(duhrPos));
  Serial.println("Asr LED Position: " + String(asrPos));
  Serial.println("Maghrib LED Position: " + String(maghribPos));
  Serial.println("Isha LED Position: " + String(aishaPos));

    // Turn on the LEDs at the calculated prayer positions
  leds[fajrPos] = prayer_color;
  leds[duhrPos] = prayer_color;
  leds[asrPos] = prayer_color;
  leds[maghribPos] = prayer_color;
  leds[aishaPos] = prayer_color;

  FastLED.show();



}

int curr_led;
void setup() {

  Serial.begin(9600);

  // Connect to Wi-Fi
  connectWiFi();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  FastLED.clear();
  FastLED.show();
  delay(2000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  curr_led = timeToLedPos(getCurrentTime());

  for (int i = 0; i <= curr_led; i++) {
    leds[i] = time_color; // Set to any color you like, e.g., Blue
  }

  initNewDay();
  

  }

void loop() {

  if (timeToLedPos(getCurrentTime()) > curr_led)
  {
      if (curr_led == 96){
        initNewDay();
        curr_led = 0;
        FastLED.clear();
        FastLED.show();
      }
      else
      {

        curr_led++;
        if (curr_led != fajrPos && curr_led != duhrPos && curr_led != asrPos && curr_led != maghribPos && curr_led != aishaPos)
        {
          leds[curr_led] = time_color;
          FastLED.show();
        }

      }

  }
   

  
  delay(10000);  // Check every second
}
