#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Assuming you are using ArduinoJson v6 or v7
#include "env.h" // Include your env.h file for WiFi credentials (ssid, password)


// Define your server name including the endpoint path
// IMPORTANT: Replace with your actual VM IP and port, followed by the endpoint path



#define Peltier_1_CTRL 14 //This pin controls the Peltier Block 1, Turns on Fans 1 and 2 and Pump 1
#define Peltier_2_CTRL 13 //This pin controls the Peltier Block 2, Turns on Fans 3 and 4 and Pump 2


// Assuming these pins control the stated components based on user feedback
#define FAN_1_PIN     27 // Block 1, Fan 1 (Hot Side)
#define FAN_2_PIN     26 // Block 1, Fan 2 (Cold Side)
#define PUMP_1_PIN    25 // Block 1, Pump 1
#define FAN_3_PIN     16 // Block 2, Fan 3 (Hot Side) // Pin 16 was Fan 4? Double check your wiring
#define FAN_4_PIN    3  // Block 2, Fan 4 (Cold Side) // Pin 4 was ONE_WIRE_BUS? Double check your wiring


#define BUZZER_PIN 5   //Buzzer pin to turn off and on buzzer


#define Red_LED 2       //RGB LED Red pin
#define Green_LED 5     //RGB LED Green pin
#define Blue_LED 12     //RGB LED Blue pin


#define BUTTON_increase 36 //Button to increase the timer, tempearutre setpoint and scrolling
#define BUTTON_decrease 39 //Button to decrease the timer, tempearutre setpoint and scrolling
#define BUTTON_select 34   //Button to select the temperature setpoint and scroll through the menu
#define BUTTON_back 35     //Button to go back to the previous menu



// --- LCD + DS18B20 Setup ---
#define ONE_WIRE_BUS 4 // Assuming this pin is used for OneWire, conflicting with FAN_4_PIN definition above. You need to use different pins.
#define TEMPERATURE_PRECISION 12
LiquidCrystal_I2C lcd(0x27,20,4);   // 20x4 LCD display

OneWire oneWire(ONE_WIRE_BUS); // Uses the pin defined by ONE_WIRE_BUS
DallasTemperature sensors(&oneWire);

// DS18B20 sensor addresses - VERIFY these match your physical sensors
DeviceAddress sensor1 = { 0x28, 0x5E, 0x26, 0x97, 0x94, 0x09, 0x03, 0xC6 }; // Example address, update with yours
DeviceAddress sensor2 = { 0x28, 0x78, 0x16, 0x94, 0x97, 0x0E, 0x03, 0x44 }; // Example address
DeviceAddress sensor3 = { 0x28, 0xF5, 0x04, 0x97, 0x94, 0x04, 0x03, 0x90 }; // Example address
DeviceAddress sensor4 = { 0x28, 0x0B, 0x34, 0x97, 0x94, 0x04, 0x03, 0x75 }; // Example address

// Assign names to sensors based on their addresses and intended locations
// These names should ideally match what you want to see in the API/Database
const char* sensor1Name = "Block1_Hot";
const char* sensor2Name = "Block1_Cold";
const char* sensor3Name = "Block2_Hot";
const char* sensor4Name = "Block2_Cold";


// --- WiFi and LCD setup ---
void setup() {
  Serial.begin(9600);

  // Check for pin conflicts based on your definitions above
  #if ONE_WIRE_BUS == FAN_4_PIN
  #warning "ONE_WIRE_BUS and FAN_4_PIN are the same pin! This will cause conflicts."
  #endif
  // Add checks for other potential conflicts if needed
  pinMode(BUZZER_PIN, OUTPUT);
  sensors.begin();
  sensors.setWaitForConversion(true); // Wait for temperature conversion to complete

  lcd.init(); // Initialize LCD
  lcd.backlight(); // Turn on LCD backlight
  lcd.begin(20,4); // Specify LCD dimensions

  // Connect to WiFi
  lcd.setCursor(0,0);
  Serial.print("Connecting to WiFi...");
  lcd.print("Connecting to WiFi...");
  WiFi.begin(ssid, password); // Use credentials from env.h

  unsigned long wifi_connect_start_time = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
     // Optional: Add a timeout for WiFi connection
    if (millis() - wifi_connect_start_time > 30000) { // 30 second timeout
        Serial.println("\nWiFi connection timed out.");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("WiFi Timeout!");
        // You might want to implement a reset or retry logic here
        break; // Exit the loop after timeout
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connected to WiFi");
    Serial.print("IP: ");
    lcd.setCursor(0,1);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
    Serial.println(WiFi.localIP());
    delay(2000); // Show IP for 2 seconds
    lcd.clear();
  } else {
      Serial.println("\nFailed to connect to WiFi");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("WiFi Failed!");
      // Keep showing failure message or implement retry
  }

  // Initialize control pins (if using digitalWrite/analogWrite)
  // pinMode(Peltier_1_CTRL, OUTPUT); // Example
  // pinMode(Peltier_2_CTRL, OUTPUT); // Example
  // pinMode(FAN_1_PIN, OUTPUT); // Example
  // pinMode(FAN_2_PIN, OUTPUT); // Example
  // pinMode(FAN_3_PIN, OUTPUT); // Example
  // pinMode(FAN_4_PIN, OUTPUT); // Example
  // pinMode(PUMP_1_PIN, OUTPUT); // Example
  // pinMode(PUMP_2_PIN, OUTPUT); // Example
  // pinMode(BUZZER_PIN, OUTPUT); // Example
  // pinMode(Red_LED, OUTPUT);    // Example
  // pinMode(Green_LED, OUTPUT);  // Example
  // pinMode(Blue_LED, OUTPUT);   // Example
  // pinMode(BUTTON_increase, INPUT_PULLUP); // Example for buttons
  // pinMode(BUTTON_decrease, INPUT_PULLUP); // Example
  // pinMode(BUTTON_select, INPUT_PULLUP);   // Example
  // pinMode(BUTTON_back, INPUT_PULLUP);     // Example

}

// --- Function to send temperature data to FastAPI ---
void sendTemperature(float t1, float t2, float t3, float t4) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // serverName is already set globally
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Use StaticJsonDocument, choose size based on expected payload size
    // 1024 should be sufficient for 4 sensor readings + battery_level
    StaticJsonDocument<1024> doc; // Specify the size of the JSON document

    // Create the root JSON array directly from the document
    JsonArray data_array = doc.to<JsonArray>();

    // Sensor 1 Object
    JsonObject sensor1Obj = data_array.add<JsonObject>(); // Add object to the array
    sensor1Obj["sensor_id"] = "ds18b20_1"; // Unique hardware ID
    sensor1Obj["sensor_name"] = sensor1Name; // Descriptive name
    sensor1Obj["temperature"] = t1;
    sensor1Obj["sensor_type"] = "DS18B20";
    sensor1Obj["battery_level"] = 1.0; // Add battery_level (placeholder value)

    // Sensor 2 Object
    JsonObject sensor2Obj = data_array.add<JsonObject>(); // Add object to the array
    sensor2Obj["sensor_id"] = "ds18b20_2";
    sensor2Obj["sensor_name"] = sensor2Name;
    sensor2Obj["temperature"] = t2;
    sensor2Obj["sensor_type"] = "DS18B20";
    sensor2Obj["battery_level"] = 1.0; // Add battery_level (placeholder value)
    // Make sure there is no extra "battery_type" line here!

    // Sensor 3 Object
    JsonObject sensor3Obj = data_array.add<JsonObject>(); // Add object to the array
    sensor3Obj["sensor_id"] = "ds18b20_3";
    sensor3Obj["sensor_name"] = sensor3Name;
    sensor3Obj["temperature"] = t3;
    sensor3Obj["sensor_type"] = "DS18B20";
    sensor3Obj["battery_level"] = 1.0; // Add battery_level (placeholder value)

    // Sensor 4 Object
    JsonObject sensor4Obj = data_array.add<JsonObject>(); // Add object to the array
    sensor4Obj["sensor_id"] = "ds18b20_4";
    sensor4Obj["sensor_name"] = sensor4Name;
    sensor4Obj["temperature"] = t4;
    sensor4Obj["sensor_type"] = "DS18B20";
    sensor4Obj["battery_level"] = 1.0; // Add battery_level (placeholder value)


    String requestBody;
    // Serialize the JsonDocument (which is the array) to a string
    serializeJson(doc, requestBody);

    // --- Print the JSON payload to Serial Monitor ---
    Serial.print("Sending JSON payload: ");
    Serial.println(requestBody);
    // -----------------------------------------------

    // Send the HTTP POST request
    int httpCode = http.POST(requestBody);

    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);

    // Optional: Read and print the response body if code is not 201
    if (httpCode != HTTP_CODE_CREATED && httpCode > 0) { // Check if it's not 201 and a valid HTTP code
        String payload = http.getString();
        Serial.print("HTTP Response Payload: ");
        Serial.println(payload);
    } else if (httpCode < 0) { // Handle connection errors
         Serial.print("HTTP request failed, error: ");
         Serial.println(http.errorToString(httpCode));
    }


    http.end(); // Close the connection
  } else {
    Serial.println("WiFi not connected. Cannot send data.");
  }
}


// --- Loop: read temps, display on LCD, and POST ---
void loop() {
  // lcd.clear(); // Clearing the LCD every loop can cause flicker
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer at the start of the loop
  // Request temperature readings from the sensors
  sensors.requestTemperatures();

  // Get temperatures by sensor address
  float t1 = sensors.getTempC(sensor1);
  float t2 = sensors.getTempC(sensor2);
  float t3 = sensors.getTempC(sensor3);
  float t4 = sensors.getTempC(sensor4);

  // Check if readings are valid (-127.0 is the error code)
  if (t1 == -127.0 || t2 == -127.0 || t3 == -127.0 || t4 == -127.0) {
    Serial.println("Error reading temperatures. Skipping POST.");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp Sensor Error!");
     // Delay before retrying
    delay(5000); // Wait 5 seconds on error
    return; // Skip the rest of the loop and POST
  }


  Serial.println("Sending temperatures:");
  Serial.print("T1: "); Serial.println(t1);
  Serial.print("T2: "); Serial.println(t2);
  Serial.print("T3: "); Serial.println(t3);
  Serial.print("T4: "); Serial.println(t4);


  // Display temperatures on LCD
  lcd.setCursor(0,0);
  lcd.print("HOT1 "); lcd.print(": "); lcd.print(t1); lcd.print(" C "); // Display with 1 decimal place and clear rest of line
  lcd.setCursor(0,1);
  lcd.print("COLD1"); lcd.print(": "); lcd.print(t2); lcd.print(" C ");
  lcd.setCursor(0,2);
  lcd.print("HOT2 "); lcd.print(": "); lcd.print(t3); lcd.print(" C ");
  lcd.setCursor(0,3);
  lcd.print("COLD2"); lcd.print(": "); lcd.print(t4); lcd.print(" C ");


  // Send temperature data to the FastAPI application
  sendTemperature(t1, t2, t3, t4);

  // Delay before the next loop iteration
  // This determines how often data is read and sent
  delay(1000); // Send data every 5 seconds
}