#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // install ArduinoJson

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "test@123";

// Telegram Bot credentials
const char* botToken = "8109233936:AAHXHoZ02TDt27fmkng58h3sS477dEI2R_U";  
const char* chatID = "975490911";

// Push button pin
const int buttonPin = D5; // Use D5 for push button
bool isSending = false;   // Lock during sending

// Piezo sensor pin and settings
const int piezoPin = A0;  // Piezo sensor connected to analog pin A0
int threshold = 150;      // Set the vibration threshold (adjust based on testing)
int piezoValue = 0;       // Variable to store the sensor reading

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  pinMode(buttonPin, INPUT_PULLUP); // Button is active LOW
  pinMode(piezoPin, INPUT);         // Set the piezo sensor pin as input

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println("Waiting for signal or button press...");
}

void loop() {
  // Check button press
  if (digitalRead(buttonPin) == LOW && !isSending) { 
    isSending = true; // Lock
    Serial.println("Button pressed! Sending emergency location...");
    sendLocationTimes("Need help");
    isSending = false; // Unlock
    delay(500); // little delay to prevent bounce
  }

  // Check piezo sensor
  piezoValue = analogRead(piezoPin);
  
  // If vibration exceeds threshold and not already sending
  if (piezoValue > threshold && !isSending) {
    isSending = true; // Lock
    Serial.println("Fall Detected! Piezo value: " + String(piezoValue));
    Serial.println("Sending emergency location...");
    sendLocationTimes("Fall detected");
    isSending = false; // Unlock
  }

  delay(200); // Small delay to stabilize readings
}

// Function to send location 10 times
void sendLocationTimes(String alertType) {
  for (int i = 0; i < 10; i++) {
    String locationMessage = getLocationByIP(alertType);
    sendTelegramMessage(locationMessage);
    delay(2000); // 2 seconds delay
  }
  Serial.println("Done sending 10 times.");
}

String getLocationByIP(String alertType) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://ip-api.com/json/");

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("IP Location Info:");
      Serial.println(payload);

      // Parse the JSON payload
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String city = doc["city"].as<String>();
        String zip = doc["zip"].as<String>();
        float lat = doc["lat"];
        float lon = doc["lon"];

        String message = alertType + ":\nCity: " + city + "\nZip: " + zip + "\nLat: " + String(lat, 6) + "\nLon: " + String(lon, 6);
        return message;
      } else {
        Serial.println("Failed to parse JSON");
        return alertType + ": Failed to parse location";
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(http.errorToString(httpCode));
      return alertType + ": Failed to get location";
    }
    http.end();
  }
  return alertType + ": WiFi not connected";
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Skip cert validation

    HTTPClient https;
    String url = "https://api.telegram.org/bot" + String(botToken) + "/sendMessage?chat_id=" + String(chatID) + "&text=" + urlencode(message);

    Serial.println("Sending message to Telegram...");
    Serial.println(url);

    if (https.begin(client, url)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        String response = https.getString();
        Serial.println("Telegram response:");
        Serial.println(response);
      } else {
        Serial.print("Telegram Error: ");
        Serial.println(https.errorToString(httpCode));
      }
      https.end();
    } else {
      Serial.println("Unable to connect to Telegram");
    }
  }
}

// Simple URL Encoder
String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i =0; i < str.length(); i++){
    c=str.charAt(i);
    if (isalnum(c)){
      encodedString += c;
    } else {
      encodedString += '%';
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encodedString += char(code0 < 10 ? code0 + '0' : code0 - 10 + 'A');
      encodedString += char(code1 < 10 ? code1 + '0' : code1 - 10 + 'A');
    }
  }
  return encodedString;
}