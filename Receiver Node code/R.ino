#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <LoRa.h>

#define SS 15    // LoRa module NSS pin
#define RST 16   // LoRa module RESET pin
#define DIO0 2   // LoRa module DIO0 pin

const char* ssid = "TTC Fiber 2";      // Change this
const char* password = "ttcfiber200M";  // Change this

// URLs for different weather stations
const char* googleAppsScriptURL_WS1 = "https://script.google.com/macros/s/AKfycbwPslLx1sVMZy5uZ_KFn3sI4m1rNOxWPTIeR86at0cK50EhDbYyaAh0sszP4Uvwsb3g/exec";  
const char* googleAppsScriptURL_WS2 = "https://script.google.com/macros/s/AKfycbzzKc4EewCwxloGgsE3RdbGwxDrBq7eaDec1uRQLBO8494KRHaZ6h53d39fCNAnwzS7/exec";  
const char* googleAppsScriptURL_WS3 = "https://script.google.com/macros/s/AKfycbxrnid8mlU1xdDwhmSrdU8wRjca0EqBCuqMyHmfb_Ed0NDZyZ-EnynjPt1fiayCn4WE/exec";  
const char* googleAppsScriptURL_WS4 = "https://script.google.com/macros/s/AKfycbx7-7bw99dCMP1D9q2FXIedbKSzpJUCkXmn4ocJzJXLlS08I5h9Jo0hY7V78s2DzYRL/exec";  

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("LoRa Weather Station Receiver Initializing...");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempt++;
        if (attempt > 40) {  // Restart ESP8266 if WiFi doesn't connect
            Serial.println("\n❌ WiFi connection failed! Restarting...");
            ESP.restart();
        }
    }
    Serial.println("\n✅ WiFi Connected!");

    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa Initialization failed!");
        while (1);
    }
    Serial.println("✅ LoRa Initialized.");
}

void loop() {
    receiveAndProcess("WS1", googleAppsScriptURL_WS1);
    receiveAndProcess("WS2", googleAppsScriptURL_WS2);
    receiveAndProcess("WS3", googleAppsScriptURL_WS3);
    receiveAndProcess("WS4", googleAppsScriptURL_WS4);
}

void receiveAndProcess(String station, const char* scriptURL) {
    char receivedData[128];  
    bool dataReceived = false;
    Serial.println("\n📡 Waiting for " + station + " Weather Data...");
    unsigned long startTime = millis();

    while (millis() - startTime < 5000) {  // Wait for 5 seconds
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            int index = 0;
            while (LoRa.available() && index < 127) {
                receivedData[index++] = (char)LoRa.read();
            }
            receivedData[index] = '\0';  

            String receivedStr = String(receivedData);
            if (receivedStr.startsWith(station + ",")) {
                Serial.print("✅ Received from " + station + ": ");
                Serial.println(receivedStr);

                // Remove station ID
                String weatherData = receivedStr.substring(station.length() + 1);
                sendToGoogleSheets(weatherData, scriptURL);
                dataReceived = true;
                break;
            }
        }
        delay(1);
    }

    if (!dataReceived) {
        Serial.println("❌ No data received from " + station + "!");
    }
    delay(5000);
}

void sendToGoogleSheets(String weatherData, const char* scriptURL) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected! Restarting...");
        ESP.restart();
    }

    WiFiClientSecure client;
    client.setInsecure();  // Bypass SSL verification

    HTTPClient http;
    
    // Encode the weather data
    String encodedData = urlEncode(weatherData);
    String url = String(scriptURL) + "?data=" + encodedData;
    
    Serial.println("🌍 Sending data to Google Sheets: " + url);  // Debugging output

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        Serial.println("✅ Data sent successfully: " + weatherData);
        String payload = http.getString();
        Serial.println("📥 Response: " + payload);  // Debugging response from Google Sheets
    } else {
        Serial.println("❌ Failed to send data. HTTP Code: " + String(httpCode));
    }

    http.end();
}

// URL encoding function
String urlEncode(String str) {
    String encodedString = "";
    char c;
    char hex[4];
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c)) {
            encodedString += c;
        } else {
            sprintf(hex, "%%%02X", c);
            encodedString += hex;
        }
    }
    return encodedString;
}
