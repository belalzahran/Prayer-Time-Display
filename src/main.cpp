#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <time.h>
#include <set>

const char* ssid = "Z6";
const char* password = "Hajj2016";
const char* apiURL = "http://api.aladhan.com/v1/timingsByCity?city=LakeForest&country=USA&method=2"; 
const char* baseURL = "http://api.aladhan.com";  // Base URL for relative redirects

#define LED_PIN     2
#define NUM_LEDS    96
#define BRIGHTNESS  50

int curr_led_pos;

CRGB prayer_color = CRGB(0xec1386);  // PINK
CRGB time_color = CRGB(0x1010ff);    // BLUE
CRGB past_prayer_color = CRGB(0xFFFFFF);

CRGB leds[NUM_LEDS];

String fajr, duhr, asr, maghrib, aisha;  // Store prayer times
// String testPrayer = "15:32";

// Store the LED positions
int fajrPos,duhrPos, asrPos, maghribPos, aishaPos;
// int testPrayerPos;

// NTP server and time configuration 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;  // Adjust for your timezone (GMT-8 for PST)
const int   daylightOffset_sec = 3600;






CRGB savedLEDState[NUM_LEDS];

void save_LED_state(){
  for (int i = 0; i < NUM_LEDS; i++){
    savedLEDState[i] = leds[i];
  }
}

void load_LED_state(){
  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = savedLEDState[i];
  }
  FastLED.show();
}

void blink_led_strip()
{

  for (int i = 0; i <= 29; i++)
  {
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(1000);
    
    FastLED.clear();
    FastLED.show();
    delay(1000);

  }
  
}

void connect_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void fetch_prayer_times_from_api() {
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

String get_current_time_string() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeStr[6]; // Format time as HH:MM
  strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
  return String(timeStr);
}

int time_to_led_position(String time) {
  int hours = time.substring(0, 2).toInt();   
  int minutes = time.substring(3, 5).toInt();
  int totalMinutes = (hours * 60) + minutes;

  // Assuming each LED represents 15 minutes (96 LEDs for 24 hours)
  int ledPos = (totalMinutes / 15);

  return ledPos;
}



void get_show_daily_prayer_times(){

  fetch_prayer_times_from_api();

  // Convert prayer times to LED positions
  fajrPos = time_to_led_position(fajr);
  duhrPos = time_to_led_position(duhr);
  asrPos = time_to_led_position(asr);
  maghribPos = time_to_led_position(maghrib);
  aishaPos = time_to_led_position(aisha);
  // testPrayerPos = time_to_led_position(testPrayer);

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
  // leds[testPrayerPos] = prayer_color;

  FastLED.show();



}



void setup() {

  Serial.begin(9600);

  // Connect to Wi-Fi
  connect_wifi();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  FastLED.clear();
  FastLED.show();
  delay(2000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // for (int i = 0; i < NUM_LEDS; i++){
  //   leds[i] = past_prayer_color;
  //   FastLED.show();
  //   delay(100);
  //   FastLED.clear();
  // }

  get_show_daily_prayer_times();

  std::set<int> prayerPositions = {fajrPos, duhrPos, asrPos, maghribPos, aishaPos};   
  // std::set<int> prayerPositions = {fajrPos, duhrPos, asrPos, maghribPos, aishaPos, testPrayerPos};   
  curr_led_pos = time_to_led_position(get_current_time_string());

  for (int i = 1; i <= curr_led_pos; i++) {

    if (prayerPositions.find(i) != prayerPositions.end()) // if curr led is equal to any of the prayPos
    {
      leds[i-1] = past_prayer_color;
    }
    else{
      leds[i-1] = time_color; // Set to any color you like, e.g., Blue
    }
    
  }

    FastLED.show();

    Serial.println("The current led position including 0 is now: " + String(curr_led_pos));
}

void loop() {

  // Sets without test
  std::set<int> prayerPositions = {fajrPos, duhrPos, asrPos, maghribPos, aishaPos};
  std::set<String> prayerTimes = {fajr, duhr, asr, maghrib, aisha};

  // sets with test
  // std::set<int> prayerPositions = {fajrPos, duhrPos, asrPos, maghribPos, aishaPos, testPrayerPos};
  // std::set<String> prayerTimes = {fajr, duhr, asr, maghrib, aisha, testPrayer};
  if (time_to_led_position(get_current_time_string()) > curr_led_pos)
  {
      if (curr_led_pos == 96){
        get_show_daily_prayer_times();
        curr_led_pos = 0;
        FastLED.clear();
        FastLED.show();
      }
      else
      {

        curr_led_pos++;

        Serial.println("The current led position including 0 is now: " + String(curr_led_pos));
        Serial.println("Current time is " + String(get_current_time_string()));

        if (prayerPositions.find(curr_led_pos) == prayerPositions.end())
        {
          leds[curr_led_pos] = time_color;
          
        }

        FastLED.show();

      }

  }
  
  String curr_time = get_current_time_string();

  if (prayerTimes.find(curr_time) != prayerTimes.end())
  {

    save_LED_state();

    blink_led_strip();

    load_LED_state();

    Serial.println("AT THE END: Current time is " + String(curr_time));
    Serial.println("led position we are one is : " + String(time_to_led_position(curr_time)));

    leds[time_to_led_position(curr_time)] = past_prayer_color;

    delay(20000);
  }

  delay(1000);  // Check every second









}






