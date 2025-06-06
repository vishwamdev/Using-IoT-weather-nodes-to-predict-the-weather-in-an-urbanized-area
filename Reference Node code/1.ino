#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>  // ✅ Use WiFiClientSecure for HTTPS

#define BME_CS 15    
#define BME_SCK 14   
#define BME_MISO 12  
#define BME_MOSI 13  

Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); 

const char* ssid = "SLT_Library";       
const char* password = "slt12345"; 
const char* scriptURL = "https://script.google.com/macros/s/AKfycbz6ymxMYD-1_prV0qd63c_KBzICkDyywIb1ojwc0CiULNYBDQv027efsOYB9uUCxM7jyg/exec"; 

WiFiClientSecure client;  // ✅ Use Secure Client for HTTPS

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();  // ✅ Disable SSL certificate validation

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float temperature = bme.readTemperature();
    float pressure = bme.readPressure() / 100.0F;
    float humidity = bme.readHumidity();

    String url = scriptURL;
    url += "?temperature=" + String(temperature);
    url += "&pressure=" + String(pressure);
    url += "&humidity=" + String(humidity);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(client, url);  // ✅ Use WiFiClientSecure

    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.print("Data Sent! Response: ");
      Serial.println(http.getString());
    } else {
      Serial.print("Error on sending request: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected!");
  }

  delay(30000);  
}
