#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// LoRa Module Pins
#define SS 10       // LoRa Chip Select
#define RST 9       // LoRa Reset
#define DIO0 2      // LoRa IRQ

// BME280 Sensor (Software SPI Mode)
#define BME_CS 8   // Chip Select (D8)
#define BME_MOSI 7 // MOSI (D7)
#define BME_MISO 6 // MISO (D6)
#define BME_SCK 5  // SCK (D5)

// Initialize BME280 with Software SPI
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

// MQ135 Gas Sensor (Analog Pin)
#define MQ135_PIN A1 

// Rain Sensor (Analog Pin)
#define RAIN_SENSOR_PIN A0

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("WS3");

  // Initialize LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa Initialization failed!");
    while (1);
  }
  Serial.println("✅ LoRa Initialized.");

  // Initialize BME280 with Software SPI
  if (!bme.begin()) {  
    Serial.println("❌ BME280 not found (Software SPI mode)!");
  } else {
    Serial.println("✅ BME280 Initialized (Software SPI mode).");
  }
}

void loop() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;  // Convert to hPa

  int airQuality = analogRead(MQ135_PIN);
  int rainValue = analogRead(RAIN_SENSOR_PIN);  // Read rain sensor analog value

  Serial.println("\n🌦 Sending Weather Data...");

  // Print BME280 Data to Serial Monitor
  if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
    Serial.println("❌ BME280 Sensor Error!");
  } else {
    Serial.print("🌡 Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("💧 Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("🌍 Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
  }

  // Print MQ135 sensor value
  Serial.print("🛢 Air Quality (Analog Value): ");
  Serial.println(airQuality);

  // Print Rain Sensor value
  Serial.print("🌧 Rain Sensor Value: ");
  Serial.println(rainValue);

  // Send data via LoRa
  LoRa.beginPacket();
  LoRa.print("WS3, ");
  
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
