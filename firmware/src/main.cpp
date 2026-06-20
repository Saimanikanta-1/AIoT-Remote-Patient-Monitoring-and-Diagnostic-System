/**
 * AIoT Wearable Patient Remote Monitoring and Diagnostic System
 * ESP8266 NodeMCU - Edge AI Firmware
 *
 * Features:
 *  - DHT11: Temperature & Humidity
 *  - MAX30102: Heart Rate & SpO2
 *  - Z-Score Anomaly Detection (rolling window = 20, threshold = 2)
 *  - Rule-based Predictive Health Risk Engine
 *  - MQTT publishing & REST API data upload
 *  - LED + Buzzer alert outputs
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <ArduinoJson.h>

// ─── Pin Definitions ─────────────────────────────────────────────────────────
#define DHT_PIN         2    // D4  - DHT11 data
#define DHT_TYPE        DHT11
#define LED_PIN         14   // D5  - Status / alert LED
#define BUZZER_PIN      12   // D6  - Alert buzzer
#define I2C_SDA         4    // D2
#define I2C_SCL         5    // D1

// ─── Edge AI Parameters ──────────────────────────────────────────────────────
#define WINDOW_SIZE     20
#define Z_THRESHOLD     2.0f

// ─── Network Configuration (Wokwi defaults) ────────────────────────────────
#ifndef WIFI_SSID
#define WIFI_SSID       "Wokwi-GUEST"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD   ""
#endif
#ifndef MQTT_BROKER
#define MQTT_BROKER     "broker.hivemq.com"
#endif
#define MQTT_PORT       1883
#define MQTT_TOPIC      "patient/monitoring/data"
#ifndef API_ENDPOINT
#define API_ENDPOINT    "http://localhost:5000/api/patient-data"
#endif
#define PATIENT_ID      "PAT-001"

// ─── Timing ──────────────────────────────────────────────────────────────────
#define READ_INTERVAL_MS    3000
#define MQTT_RETRY_MS       5000
#define HR_SAMPLE_COUNT     100

// ─── Sensor Objects ─────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
MAX30105 particleSensor;
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ─── Rolling Window Buffers ─────────────────────────────────────────────────
float tempWindow[WINDOW_SIZE];
float humWindow[WINDOW_SIZE];
float hrWindow[WINDOW_SIZE];
float spo2Window[WINDOW_SIZE];
int   windowIndex = 0;
int   windowCount = 0;

// ─── Current Readings ────────────────────────────────────────────────────────
float temperature   = 0.0f;
float humidity      = 0.0f;
int   heartRate     = 0;
int   spo2          = 0;
float zTemp         = 0.0f;
float zHum          = 0.0f;
float zHR           = 0.0f;
float zSpO2         = 0.0f;
bool  anomalyDetected = false;
String riskLevel    = "Normal";
String anomalyStatus = "OK";

// ─── State ───────────────────────────────────────────────────────────────────
bool dhtReady       = false;
bool maxReady       = false;
bool useSimulation  = false;
unsigned long lastRead = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastBeatTime = 0;   // <-- ADD THIS LINE
String mqttClientId;
// ─── Statistics Helpers ──────────────────────────────────────────────────────
float computeMean(const float* data, int count) {
  if (count == 0) return 0.0f;
  float sum = 0.0f;
  for (int i = 0; i < count; i++) sum += data[i];
  return sum / count;
}

float computeStdDev(const float* data, int count, float mean) {
  if (count < 2) return 1.0f;
  float sumSq = 0.0f;
  for (int i = 0; i < count; i++) {
    float diff = data[i] - mean;
    sumSq += diff * diff;
  }
  float variance = sumSq / (count - 1);
  float stddev = sqrtf(variance);
  return (stddev < 0.001f) ? 0.001f : stddev;
}

float computeZScore(float value, const float* window, int count) {
  if (count < 3) return 0.0f;
  float mean = computeMean(window, count);
  float stddev = computeStdDev(window, count, mean);
  return (value - mean) / stddev;
}

void pushToWindow(float* window, float value) {
  window[windowIndex] = value;
}

// ─── Predictive Risk Engine ──────────────────────────────────────────────────
String classifyRisk(float temp, int hr, int oxygen) {
  int score = 0;

  // Temperature scoring
  if (temp < 35.0f || temp > 39.5f)       score += 3;
  else if (temp < 36.0f || temp > 38.0f)  score += 2;
  else if (temp < 36.5f || temp > 37.5f)  score += 1;

  // Heart rate scoring
  if (hr < 50 || hr > 120)                score += 3;
  else if (hr < 55 || hr > 100)           score += 2;
  else if (hr < 60 || hr > 90)            score += 1;

  // SpO2 scoring
  if (oxygen < 90)                        score += 3;
  else if (oxygen < 94)                   score += 2;
  else if (oxygen < 97)                   score += 1;

  if (score >= 5) return "Critical";
  if (score >= 2) return "Warning";
  return "Normal";
}

// ─── Alert Outputs ───────────────────────────────────────────────────────────
void updateAlerts() {
  bool alert = anomalyDetected || riskLevel == "Critical";

  if (riskLevel == "Warning") {
    digitalWrite(LED_PIN, (millis() / 500) % 2);
    digitalWrite(BUZZER_PIN, LOW);
  } else if (alert) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ─── Simulated Vitals (Wokwi / fallback) ─────────────────────────────────────
void readSimulatedVitals() {
  static float simBaseTemp = 36.6f;
  static int   simBaseHR   = 72;
  static int   simBaseSpO2 = 98;

  simBaseTemp += (random(-10, 11) / 100.0f);
  simBaseHR   += random(-3, 4);
  simBaseSpO2 += random(-1, 2);

  simBaseTemp = constrain(simBaseTemp, 35.5f, 38.5f);
  simBaseHR   = constrain(simBaseHR, 58, 95);
  simBaseSpO2 = constrain(simBaseSpO2, 94, 100);

  temperature = simBaseTemp;
  humidity    = 45.0f + random(-5, 6);
  heartRate   = simBaseHR;
  spo2        = simBaseSpO2;
}

// ─── DHT11 Reading ───────────────────────────────────────────────────────────
void readDHT11() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity    = h;
    dhtReady    = true;
  } else if (useSimulation) {
    readSimulatedVitals();
  }
}

// ─── MAX30102 Heart Rate & SpO2 ──────────────────────────────────────────────
int readHeartRate() {
  if (!maxReady) return 0;

  long irValues[HR_SAMPLE_COUNT];
  int validCount = 0;

  for (int i = 0; i < HR_SAMPLE_COUNT; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
      delay(1);
    }
    irValues[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  float bpm = 0.0f;
  for (int i = 0; i < HR_SAMPLE_COUNT; i++) {
    if (checkForBeat(irValues[i])) {
      long delta = millis() - lastBeatTime;
      lastBeatTime = millis();
      if (delta > 300 && delta < 2000) {
        float instantBpm = 60000.0f / delta;
        if (instantBpm > 40 && instantBpm < 200) {
          bpm = instantBpm;
          validCount++;
        }
      }
    }
  }

  return (validCount > 0) ? (int)bpm : 0;
}

int estimateSpO2() {
  if (!maxReady) return 0;

  long redTotal = 0, irTotal = 0;
  int samples = 0;

  for (int i = 0; i < 50; i++) {
    while (!particleSensor.available()) {
      particleSensor.check();
      delay(1);
    }
    redTotal += particleSensor.getRed();
    irTotal  += particleSensor.getIR();
    particleSensor.nextSample();
    samples++;
  }

  if (samples == 0 || irTotal == 0) return 0;

  float ratio = (float)redTotal / (float)irTotal;
  int estimated = (int)(110.0f - 25.0f * ratio);
  return constrain(estimated, 70, 100);
}

void readMAX30102() {
  if (!maxReady) {
    if (useSimulation) readSimulatedVitals();
    return;
  }

  int hr = readHeartRate();
  int ox = estimateSpO2();

  if (hr > 0) heartRate = hr;
  if (ox > 0) spo2 = ox;

  if (heartRate == 0 || spo2 == 0) {
    if (useSimulation) readSimulatedVitals();
  }
}

// ─── Edge AI Processing ──────────────────────────────────────────────────────
void processEdgeAI() {
  pushToWindow(tempWindow, temperature);
  pushToWindow(humWindow,  humidity);
  pushToWindow(hrWindow,   (float)heartRate);
  pushToWindow(spo2Window, (float)spo2);

  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  if (windowCount < WINDOW_SIZE) windowCount++;

  zTemp = computeZScore(temperature, tempWindow, windowCount);
  zHum  = computeZScore(humidity,    humWindow,  windowCount);
  zHR   = computeZScore((float)heartRate, hrWindow,   windowCount);
  zSpO2 = computeZScore((float)spo2,      spo2Window, windowCount);

  anomalyDetected = (windowCount >= WINDOW_SIZE) &&
                    (fabsf(zTemp) > Z_THRESHOLD ||
                     fabsf(zHum)  > Z_THRESHOLD ||
                     fabsf(zHR)   > Z_THRESHOLD ||
                     fabsf(zSpO2) > Z_THRESHOLD);

  riskLevel = classifyRisk(temperature, heartRate, spo2);

  if (anomalyDetected && riskLevel == "Critical") {
    anomalyStatus = "CRITICAL_ANOMALY";
  } else if (anomalyDetected) {
    anomalyStatus = "ANOMALY_DETECTED";
  } else if (riskLevel == "Critical") {
    anomalyStatus = "CRITICAL_RISK";
  } else if (riskLevel == "Warning") {
    anomalyStatus = "WARNING";
  } else {
    anomalyStatus = "OK";
  }
}

// ─── WiFi ────────────────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connection failed - continuing offline");
  }
}

// ─── MQTT ────────────────────────────────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Reserved for future command subscriptions
}

bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  if (mqtt.connect(mqttClientId.c_str())) {
    Serial.println("[MQTT] Connected to broker");
    return true;
  }
  Serial.print("[MQTT] Failed, rc=");
  Serial.println(mqtt.state());
  return false;
}

bool publishMQTT() {
  if (!mqtt.connected()) return false;

  JsonDocument doc;
  doc["patient_id"]     = PATIENT_ID;
  doc["temperature"]    = roundf(temperature * 100.0f) / 100.0f;
  doc["humidity"]       = roundf(humidity * 100.0f) / 100.0f;
  doc["heart_rate"]     = heartRate;
  doc["spo2"]           = spo2;
  doc["z_temp"]         = roundf(zTemp * 100.0f) / 100.0f;
  doc["z_humidity"]     = roundf(zHum * 100.0f) / 100.0f;
  doc["z_heart_rate"]   = roundf(zHR * 100.0f) / 100.0f;
  doc["z_spo2"]         = roundf(zSpO2 * 100.0f) / 100.0f;
  doc["risk_level"]     = riskLevel;
  doc["anomaly_status"] = anomalyStatus;
  doc["anomaly"]        = anomalyDetected;
  doc["timestamp"]      = millis();

  JsonObject zScores = doc["z_scores"].to<JsonObject>();
  zScores["temperature"]  = doc["z_temp"];
  zScores["humidity"]     = doc["z_humidity"];
  zScores["heart_rate"]   = doc["z_heart_rate"];
  zScores["spo2"]         = doc["z_spo2"];

  char buffer[512];
  serializeJson(doc, buffer);

  bool ok = mqtt.publish(MQTT_TOPIC, buffer, true);
  if (ok) Serial.println("[MQTT] Published to patient/monitoring/data");
  else     Serial.println("[MQTT] Publish failed");
  return ok;
}

// ─── REST API ────────────────────────────────────────────────────────────────
bool postToAPI() {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClient client;
  HTTPClient http;

  String url = String(API_ENDPOINT);
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["patient_id"]  = PATIENT_ID;
  doc["temperature"] = roundf(temperature * 100.0f) / 100.0f;
  doc["humidity"]    = roundf(humidity * 100.0f) / 100.0f;
  doc["heart_rate"]  = heartRate;
  doc["spo2"]        = spo2;
  doc["risk_level"]  = riskLevel;
  doc["z_temp"]      = roundf(zTemp * 100.0f) / 100.0f;
  doc["z_humidity"]  = roundf(zHum * 100.0f) / 100.0f;
  doc["z_heart_rate"]= roundf(zHR * 100.0f) / 100.0f;
  doc["z_spo2"]      = roundf(zSpO2 * 100.0f) / 100.0f;
  doc["anomaly_status"] = anomalyStatus;

  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);
  http.end();

  if (code > 0) {
    Serial.printf("[API] POST response: %d\n", code);
    return (code >= 200 && code < 300);
  }
  Serial.printf("[API] POST failed: %s\n", http.errorToString(code).c_str());
  return false;
}

// ─── Serial Debug Output ─────────────────────────────────────────────────────
void printStatus() {
  Serial.println("─────────────────────────────────────────");
  Serial.printf("Temp: %.1f°C  Hum: %.1f%%  HR: %d bpm  SpO2: %d%%\n",
                temperature, humidity, heartRate, spo2);
  Serial.printf("Z-Scores → T:%.2f H:%.2f HR:%.2f SpO2:%.2f\n",
                zTemp, zHum, zHR, zSpO2);
  Serial.printf("Risk: %s  |  Anomaly: %s\n",
                riskLevel.c_str(), anomalyStatus.c_str());
  Serial.println("─────────────────────────────────────────");
}

// ─── Sensor Initialization ───────────────────────────────────────────────────
void initSensors() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  delay(1000);
  float testTemp = dht.readTemperature();
  dhtReady = !isnan(testTemp);

  Wire.begin(I2C_SDA, I2C_SCL);
  maxReady = particleSensor.begin(Wire, I2C_SPEED_FAST);

  if (maxReady) {
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeIR(0x0A);
    Serial.println("[MAX30102] Initialized");
  } else {
    Serial.println("[MAX30102] Not detected");
  }

#if WOKWI_SIM
  useSimulation = true;
  Serial.println("[SIM] Wokwi simulation mode enabled");
#else
  useSimulation = !dhtReady || !maxReady;
#endif

  if (useSimulation) {
    Serial.println("[SIM] Using simulated vitals for missing sensors");
    randomSeed(analogRead(A0));
  }
}

// ─── Setup & Loop ────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║  AIoT Wearable Patient Monitoring System ║");
  Serial.println("║  Edge AI | ESP8266 NodeMCU               ║");
  Serial.println("╚══════════════════════════════════════════╝");

  mqttClientId = "patient-monitor-" + String(ESP.getChipId(), HEX);

  initSensors();
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqtt.connected() && (millis() - lastMqttAttempt > MQTT_RETRY_MS)) {
    lastMqttAttempt = millis();
    connectMQTT();
  }
  mqtt.loop();

  if (millis() - lastRead >= READ_INTERVAL_MS) {
    lastRead = millis();

    readDHT11();
    readMAX30102();
    processEdgeAI();
    updateAlerts();
    printStatus();

    publishMQTT();
    postToAPI();
  }
}
