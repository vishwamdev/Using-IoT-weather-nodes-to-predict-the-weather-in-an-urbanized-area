#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SS 10    
#define RST 9    
#define DIO0 2    

// BME280 Sensor
Adafruit_BME280 bme;

// MQ135 Gas Sensor (Analog Pin)
#define MQ135_PIN A1 

// Rain Sensor (Analog Pin)
#define RAIN_SENSOR_PIN A0

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("WS1");

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa Initialization failed!");
    while (1);
  }
  Serial.println("✅ LoRa Initialized.");

  // Initialize BME280
  if (!bme.begin(0x76)) {
    Serial.println("❌ BME280 not found!");
  } else {
    Serial.println("✅ BME280 Initialized.");
  }
}

void loop() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;  // Convert to hPa

  int airQuality = analogRead(MQ135_PIN);
  int rainValue = analogRead(RAIN_SENSOR_PIN);  // Read rain sensor analog value

  Serial.println("\n🌦 Sending Weather Data...");
  Serial.print("Rain Sensor Value: ");
  Serial.println(rainValue);  // Print rain sensor analog value

  // Print MQ135 sensor analog value
  Serial.print("MQ135 Sensor Value: ");
  Serial.println(airQuality);  // Print MQ135 analog value

  LoRa.beginPacket();
  LoRa.print("WS1, ");
  
  // Check BME280 readings
  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    LoRa.print("Error, Error, Error, ");
  } else {
    LoRa.print(temperature); LoRa.print("C, ");
    LoRa.print(humidity); LoRa.print("%, ");
    LoRa.print(pressure); LoRa.print("hPa, ");
  }

  // Check Air Quality Sensor
  if (airQuality < 0 || airQuality > 1023) {
    LoRa.print("Error, ");
  } else {
    if (airQuality <= 190) {
      LoRa.print("Good");
    } else if (airQuality > 190 && airQuality <= 300) {
      LoRa.print("Moderate");
    } else {
      LoRa.print("Poor");
    }
    LoRa.print(", ");
  }

  // Check Rain Sensor (Using Analog Value)
  if (rainValue > 1023 || rainValue < 0) {  
    LoRa.print("Error");  
  } else if (rainValue >= 900) {
    LoRa.print("Not raining");
  } else if (rainValue >= 300) {
    LoRa.print("Light rain");
  } else {
    LoRa.print("Raining");
  }

  LoRa.endPacket();
  Serial.println("✅ Data Sent.");

  delay(5000);  
}
