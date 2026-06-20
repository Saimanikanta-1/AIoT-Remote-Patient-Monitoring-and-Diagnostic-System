# Final Project Report

## AIoT-Based Wearable Patient Remote Monitoring and Diagnostic System using Edge AI

**Project Title:** AIoT-Based Wearable Patient Remote Monitoring and Diagnostic System using Edge AI  
**Platform:** ESP8266 NodeMCU | Flask | SQLite | Chart.js | Wokwi  
**Date:** June 2026  

---

## 1. Executive Summary

This project implements a complete **AIoT (Artificial Intelligence of Things)** healthcare monitoring solution in the form of a wearable wrist-band device. The ESP8266 microcontroller collects vital signs from DHT11 (temperature/humidity) and MAX30102 (heart rate/SpO2) sensors, performs **Edge AI anomaly detection** locally using Z-Score statistical analysis, classifies patient health risk, and transmits data to the cloud via **MQTT** and **REST API**. A real-time web dashboard visualizes patient vitals using Chart.js.

The entire system runs in **Wokwi simulation** for demonstration and can be deployed on physical hardware with minimal configuration changes.

---

## 2. Problem Statement

Remote patient monitoring is critical for:

- Early detection of health deterioration
- Reducing hospital readmissions
- Enabling telemedicine and home care
- Continuous monitoring without constant clinical supervision

Traditional cloud-only approaches introduce latency and privacy concerns. This project addresses these by processing AI inference **at the edge** on the ESP8266 before data leaves the device.

---

## 3. Objectives Achieved

| Objective | Status | Implementation |
|-----------|--------|----------------|
| Monitor body temperature | ✅ | DHT11 sensor on GPIO2 (D4) |
| Monitor heart rate | ✅ | MAX30102 PPG + beat detection algorithm |
| Monitor SpO2 | ✅ | MAX30102 red/IR ratio estimation |
| Edge AI anomaly detection | ✅ | Z-Score rolling window (N=20, threshold=2) |
| Predictive health risk | ✅ | Rule-based engine: Normal/Warning/Critical |
| MQTT publishing | ✅ | Topic: `patient/monitoring/data` |
| REST API integration | ✅ | Flask POST `/api/patient-data` |
| Alert system | ✅ | LED + Buzzer on anomaly/critical |
| SQLite storage | ✅ | `patient_data` table |
| Real-time dashboard | ✅ | HTML/CSS/JS + Chart.js |
| Wokwi simulation | ✅ | diagram.json + wokwi.toml |

---

## 4. System Design

### 4.1 Wearable Form Factor

The device is designed as a wrist-band with:

- **MAX30102** on the underside for optical contact with skin
- **DHT11** positioned for ambient/skin-adjacent temperature
- **ESP8266** as the central processing unit
- **LED/Buzzer** on the visible top surface for patient alerts
- **LiPo battery** for portable operation (~8 hours)

### 4.2 Edge AI Architecture

**Z-Score Anomaly Detection:**

```
Z = (X - μ) / σ
```

- Rolling buffer of 20 samples per parameter
- Anomaly flagged when |Z| > 2.0
- Applied to: Temperature, Humidity, Heart Rate, SpO2
- Runs entirely on ESP8266 — no cloud ML inference required

**Predictive Risk Engine:**

A lightweight rule-based classifier scores vitals and outputs:
- **Normal** — all parameters healthy
- **Warning** — borderline values detected
- **Critical** — dangerous vital signs requiring immediate attention

### 4.3 Communication Architecture

```
ESP8266 ──MQTT──► broker.hivemq.com (patient/monitoring/data)
        ──HTTP──► Flask API (POST /api/patient-data)
                              │
                              ▼
                         SQLite DB
                              │
                              ▼
                      Chart.js Dashboard
```

---

## 5. Hardware Implementation

### 5.1 Components

| Component | Model | Interface |
|-----------|-------|-----------|
| Microcontroller | ESP8266 NodeMCU | — |
| Temperature/Humidity | DHT11 | Single-wire digital (D4) |
| Pulse Oximeter | MAX30102 | I2C (D1/D2) |
| Status LED | Red LED + 220Ω | Digital (D5) |
| Alert Buzzer | Active buzzer | Digital (D6) |

### 5.2 Circuit Wiring

| Component | ESP8266 Pin | GPIO |
|-----------|-------------|------|
| DHT11 DATA | D4 | GPIO2 |
| MAX30102 SDA | D2 | GPIO4 |
| MAX30102 SCL | D1 | GPIO5 |
| LED | D5 | GPIO14 |
| Buzzer | D6 | GPIO12 |

---

## 6. Software Implementation

### 6.1 Firmware (C++ / PlatformIO)

- **Sensor drivers:** Adafruit DHT library, SparkFun MAX3010x library
- **Edge AI:** Custom Z-Score implementation with rolling window buffers
- **Risk engine:** Score-based classification function
- **Networking:** ESP8266WiFi, PubSubClient (MQTT), HTTPClient (REST)
- **Serialization:** ArduinoJson for MQTT/API payloads
- **Wokwi fallback:** Simulated vitals when sensors unavailable

### 6.2 Backend (Python / Flask)

- REST API with CORS enabled for dashboard
- Endpoints: POST data, GET latest, GET history, GET stats
- SQLite persistence via `database.py`
- Input validation and error handling

### 6.3 Dashboard (HTML/CSS/JavaScript)

- Dark-themed medical monitoring UI
- Live vital sign cards with Z-Score display
- Risk/alert banner with color-coded states
- Real-time Chart.js line charts (3-second polling)
- Recent readings data table
- Aggregate statistics panel

---

## 7. Database Schema

**Table: `patient_data`**

| Column | Type | Description |
|--------|------|-------------|
| id | INTEGER | Primary key |
| timestamp | TEXT | ISO 8601 UTC timestamp |
| patient_id | TEXT | Patient identifier |
| temperature | REAL | Body temperature (°C) |
| humidity | REAL | Relative humidity (%) |
| heart_rate | INTEGER | Beats per minute |
| spo2 | INTEGER | Blood oxygen saturation (%) |
| risk_level | TEXT | Normal / Warning / Critical |
| z_temp | REAL | Temperature Z-Score |
| z_humidity | REAL | Humidity Z-Score |
| z_heart_rate | REAL | Heart rate Z-Score |
| z_spo2 | REAL | SpO2 Z-Score |
| anomaly_status | TEXT | OK / WARNING / ANOMALY / CRITICAL |

---

## 8. Testing & Validation

### 8.1 Wokwi Simulation

- Verified serial output shows vitals every 3 seconds
- Z-Scores computed after 20-sample window fills
- LED and buzzer respond to risk level changes
- MQTT messages published to public broker

### 8.2 API Testing

```bash
curl -X POST http://localhost:5000/api/patient-data \
  -H "Content-Type: application/json" \
  -d '{"temperature":36.8,"humidity":45,"heart_rate":72,"spo2":98,"risk_level":"Normal"}'
```

### 8.3 Dashboard Testing

- Live cards update every 3 seconds when backend receives data
- Charts populate with historical readings
- Alert banner changes color based on risk level

---

## 9. Results

The system successfully demonstrates:

1. **Continuous vital sign monitoring** at 3-second intervals
2. **Local Edge AI processing** without cloud dependency for inference
3. **Dual cloud connectivity** via MQTT and REST simultaneously
4. **Real-time visualization** on web dashboard
5. **Multi-level alerting** through LED, buzzer, and dashboard banners
6. **Full Wokwi simulation** for zero-hardware demonstration

---

## 10. Technologies Used

IoT · AIoT · Edge Computing · Edge AI · MQTT · REST API · WiFi · Embedded C++ · PlatformIO · VS Code · ESP8266 · DHT11 · MAX30102 · SQLite · Flask · HTML · CSS · JavaScript · Chart.js · Wokwi Simulation · Predictive Analytics · Anomaly Detection · Machine Learning · Real-Time Monitoring · Remote Healthcare

---

## 11. Future Enhancements

- TensorFlow Lite Micro for on-device neural network inference
- BLE connectivity for smartphone companion app
- ECG integration via AD8232 sensor
- Firebase/AWS IoT Core cloud integration
- Multi-patient dashboard with role-based access
- OTA firmware updates via HTTP
- HIPAA-compliant encryption and audit logging
- Clinical validation study with IRB approval

---

## 12. Conclusion

This project delivers a production-quality, end-to-end AIoT healthcare monitoring system that combines embedded systems, edge artificial intelligence, cloud connectivity, and real-time visualization. The wearable wrist-band design, Z-Score anomaly detection, and rule-based predictive analytics provide a solid foundation for remote patient monitoring applications in telemedicine, elderly care, and chronic disease management.

The system is fully documented, simulatable in Wokwi, and ready for deployment on physical ESP8266 hardware with minimal configuration changes.

---

## 13. References

1. Espressif ESP8266 Technical Reference Manual
2. Maxim Integrated MAX30102 Datasheet
3. DHT11 Temperature & Humidity Sensor Specifications
4. PubSubClient MQTT Library Documentation
5. Flask Web Framework Documentation
6. Chart.js Documentation
7. Wokwi ESP8266 Simulation Guide
8. Z-Score Statistical Anomaly Detection — NIST Engineering Statistics Handbook

---

*Report generated for: AIoT Wearable Patient Remote Monitoring and Diagnostic System*  
*Author: Senior IoT / Edge AI Engineering Team*  
*Version: 1.0.0*
