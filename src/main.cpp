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

CRGB leds[NUM_LEDS];

String fajr, duhr, asr, maghrib, aisha;  // Store prayer times

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

void updatePrayerTimeLEDs() {
  String currentTime = getCurrentTime();
  Serial.println("Current Time: " + currentTime);

  if (currentTime == fajr) {
    fill_solid(leds, NUM_LEDS, CRGB::Green);  // Green for Fajr
  } else if (currentTime == duhr) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);    // Red for Duhr
  } else if (currentTime == asr) {
    fill_solid(leds, NUM_LEDS, CRGB::Blue);   // Blue for Asr
  } else if (currentTime == maghrib) {
    fill_solid(leds, NUM_LEDS, CRGB::Orange); // Orange for Maghrib
  } else if (currentTime == aisha) {
    fill_solid(leds, NUM_LEDS, CRGB::Purple); // Purple for Isha
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);  // Turn off LEDs if no prayer time
  }
}



void setup() {
  Serial.begin(9600);

  // Connect to Wi-Fi
  connectWiFi();

  // Fetch prayer times
  fetchPrayerTimes();

  // Initialize the FastLED library
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  // Initialize and configure time with NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Clear the LEDs to start
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // Update the LEDs according to prayer times
  updatePrayerTimeLEDs();

  // You can call additional functions to handle other logic, like progression colors
  FastLED.show();
  delay(1000);  // Check every second
}
