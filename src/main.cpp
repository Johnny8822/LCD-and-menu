#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "env.h" // Include your env.h file for WiFi credentials


#define Peltier_1_CTRL 14 //This pin controls the Peltier Block 1, Turns on Fans 1 and 2 and Pump 1
#define Peltier_2_CTRL 13 //This pin controls the Peltier Block 2, Turns on Fans 3 and 4 and Pump 2


#define Peltier_1_FAN_1 27 //This pin controls the Fan 1 of Peltier Block 1
#define Peltier_1_FAN_2 26 //This pin controls the Fan 2 of Peltier Block 1
#define Peltier_2_FAN_3 25 //This pin controls the Pump 1 of Peltier Block 1
#define Peltier_2_FAN_4 16 //This pin controls the Fan 3 of Peltier Block 2


#define BUZZER_PIN 17   //Buzzer pin to turn off and on buzzer


#define Red_LED 2       //RGB LED Red pin
#define Green_LED 5     //RGB LED Green pin
#define Blue_LED 12     //RGB LED Blue pin


#define BUTTON_increase 36 //Button to increase the timer, tempearutre setpoint and scrolling 
#define BUTTON_decrease 39 //Button to decrease the timer, tempearutre setpoint and scrolling
#define BUTTON_select 34   //Button to select the temperature setpoint and scroll through the menu
#define BUTTON_back 35     //Button to go back to the previous menu



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
  lcd.setCursor(0,0);
  Serial.print("Connecting to WiFi...");
  lcd.print("Connecting to WiFi...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi");
  lcd.setCursor(0,1);
  lcd.print("Connected to WiFi"); 
  Serial.print("IP: ");
  lcd.setCursor(0,2); 
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP()); 
  lcd.clear();
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

  delay(100); // wait 500 milliseconds before next reading
}
