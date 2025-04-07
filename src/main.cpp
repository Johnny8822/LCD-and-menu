#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "env.h" // Include your env.h file for WiFi credentials


// --- LCD + DS18B20 Setup ---
#define ONE_WIRE_BUS 4 
#define TEMPERATURE_PRECISION 12
LiquidCrystal_I2C lcd(0x27,20,4);  // 20x4 LCD display

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// DS18B20 sensor addresses
DeviceAddress sensor1 = { 0x28, 0x5E, 0x26, 0x97, 0x94, 0x09, 0x03, 0xC6 };
DeviceAddress sensor2 = { 0x28, 0x78, 0x16, 0x94, 0x97, 0x0E, 0x03, 0x44 };
DeviceAddress sensor3 = { 0x28, 0xF5, 0x04, 0x97, 0x94, 0x04, 0x03, 0x90 };
DeviceAddress sensor4 = { 0x28, 0x0B, 0x34, 0x97, 0x94, 0x04, 0x03, 0x75 };

// --- WiFi and LCD setup ---
void setup() {
  Serial.begin(9600);
  sensors.begin();
  sensors.setWaitForConversion(true);
  lcd.init();
  lcd.backlight();
  lcd.begin(20,4);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// --- Function to send temperature data to FastAPI ---
void sendTemperature(float t1, float t2, float t3, float t4) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<1024> doc;
    JsonArray data = doc.to<JsonArray>();

    JsonObject sensor1Obj = data.add<JsonObject>();
    sensor1Obj["sensor_id"] = "ds18b20_1";
    sensor1Obj["sensor_name"] = "HOT_1";
    sensor1Obj["temperature"] = t1;
    sensor1Obj["sensor_type"] = "DS18B20";

    JsonObject sensor2Obj = data.add<JsonObject>();
    sensor2Obj["sensor_id"] = "ds18b20_2";
    sensor2Obj["sensor_name"] = "COLD_1";
    sensor2Obj["temperature"] = t2;
    sensor2Obj["sensor_type"] = "DS18B20";

    JsonObject sensor3Obj = data.add<JsonObject>();
    sensor3Obj["sensor_id"] = "ds18b20_3";
    sensor3Obj["sensor_name"] = "HOT_2";
    sensor3Obj["temperature"] = t3;
    sensor3Obj["sensor_type"] = "DS18B20";

    JsonObject sensor4Obj = data.add<JsonObject>();
    sensor4Obj["sensor_id"] = "ds18b20_4";
    sensor4Obj["sensor_name"] = "COLD_2";
    sensor4Obj["temperature"] = t4;
    sensor4Obj["sensor_type"] = "DS18B20";

    String requestBody;
    serializeJson(doc, requestBody);
    int httpCode = http.POST(requestBody);

    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);
    http.end();
  } else {
    Serial.println("WiFi not connected.");
  }
}


// --- Loop: read temps, display on LCD, and POST ---
void loop() {
  //lcd.clear();
  sensors.requestTemperatures();

  float t1 = sensors.getTempC(sensor1);
  float t2 = sensors.getTempC(sensor2);
  float t3 = sensors.getTempC(sensor3);
  float t4 = sensors.getTempC(sensor4);

  Serial.println("Sending temperatures:");
  Serial.println(t1); Serial.println(t2); Serial.println(t3); Serial.println(t4);

  lcd.setCursor(0,0);
  lcd.print("HOT1:  "); lcd.print(t1); lcd.print(" C");

  lcd.setCursor(0,1);
  lcd.print("COLD1: "); lcd.print(t2); lcd.print(" C");

  lcd.setCursor(0,2);
  lcd.print("HOT2:  "); lcd.print(t3); lcd.print(" C");

  lcd.setCursor(0,3);
  lcd.print("COLD2: "); lcd.print(t4); lcd.print(" C");

  sendTemperature(t1, t2, t3, t4);

  delay(500); // wait 10 seconds before next reading
}
