/*
  Predict Kit ESP32 -> Flask website
  IMPORTANT:
  - Update Wi-Fi credentials and BACKEND_URL.
  - BACKEND_URL must use your computer's LAN IPv4 address, e.g. http://192.168.1.10:5000/api/readings
  - ESP32 and computer must be on the same Wi-Fi/LAN.
  - GPIOs below are examples. Match them to your actual wiring.
  - ACS712 and ZMPT101B require safe wiring and calibration. Do not connect mains voltage
    to a breadboard or ESP32. Use properly isolated modules and qualified supervision.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- CHANGE THESE ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BACKEND_URL = "http://192.168.1.10:5000/api/readings";
const char* DEVICE_ID = "predict-kit-01";

// Example pins only: change to match your actual kit.
const int DS18B20_PIN = 4;
const int ACS712_PIN = 34;       // Analog input
const int ZMPT101B_PIN = 35;     // Analog input
const int DOOR_PIN = 27;         // Magnetic switch to GND when closed (INPUT_PULLUP)
const int VIBRATION_PIN = 26;    // Digital vibration module output (optional)

// Calibrate these before using measurements for decisions.
// These are demonstration placeholders, not guaranteed sensor calibration values.
const float ACS_ZERO_V = 1.65;   // Example zero-current output voltage for 3.3V ADC setup
const float ACS_SENSITIVITY_V_PER_A = 0.100; // Example only; depends on ACS712 variant
const float ZMPT_VOLTAGE_SCALE = 100.0;       // Placeholder; calibrate against a trusted meter
const int ADC_SAMPLES = 500;
// ---------------------------------

OneWire oneWire(DS18B20_PIN);
DallasTemperature temperatureSensor(&oneWire);

float readAdcAverageVolts(int pin) {
  uint32_t total = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    total += analogRead(pin);
    delayMicroseconds(200);
  }
  float averageRaw = (float)total / ADC_SAMPLES;
  return averageRaw * (3.3f / 4095.0f);
}

// Simplified demonstration estimate. A proper AC RMS calculation and calibration are needed.
float readCurrentDemo() {
  float volts = readAdcAverageVolts(ACS712_PIN);
  float current = (volts - ACS_ZERO_V) / ACS_SENSITIVITY_V_PER_A;
  if (current < 0) current = -current;
  return current;
}

// Simplified demonstration estimate only. Proper AC RMS sampling/calibration is needed.
float readVoltageDemo() {
  float volts = readAdcAverageVolts(ZMPT101B_PIN);
  return volts * ZMPT_VOLTAGE_SCALE;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not connected. Will retry later.");
  }
}

void sendReading(float tempC, float currentA, float voltageV, int vibrationRaw, bool doorOpen) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(4000);

  String body = "{";
  body += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  body += "\"temperature_c\":" + String(tempC, 2) + ",";
  body += "\"current_a\":" + String(currentA, 3) + ",";
  body += "\"voltage_v\":" + String(voltageV, 2) + ",";
  body += "\"vibration\":" + String(vibrationRaw) + ",";
  body += "\"door_open\":" + String(doorOpen ? "true" : "false");
  body += "}";

  int statusCode = http.POST(body);
  Serial.print("POST status: ");
  Serial.println(statusCode);
  Serial.println(body);
  if (statusCode > 0) {
    Serial.println(http.getString());
  } else {
    Serial.println("HTTP POST failed. Check BACKEND_URL and network.");
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(VIBRATION_PIN, INPUT);
  temperatureSensor.begin();
  connectWiFi();
}

void loop() {
  temperatureSensor.requestTemperatures();
  float tempC = temperatureSensor.getTempCByIndex(0);
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20 not detected; check wiring and 4.7k pull-up resistor.");
    tempC = 0; // Replace with a proper missing-value strategy for production.
  }

  float currentA = readCurrentDemo();
  float voltageV = readVoltageDemo();
  int vibrationRaw = digitalRead(VIBRATION_PIN);
  bool doorOpen = (digitalRead(DOOR_PIN) == HIGH);

  Serial.printf("Temp: %.2f C | Current demo: %.3f A | Voltage demo: %.2f V | Vibration raw: %d | Door open: %s\n",
                tempC, currentA, voltageV, vibrationRaw, doorOpen ? "yes" : "no");

  sendReading(tempC, currentA, voltageV, vibrationRaw, doorOpen);
  delay(3000);
}
