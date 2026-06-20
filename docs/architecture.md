# System Architecture

## AIoT Wearable Patient Remote Monitoring and Diagnostic System

---

## 1. Wearable Architecture

The system is designed as a **compact wrist-band health monitoring device** suitable for continuous ambulatory patient monitoring.

```
┌─────────────────────────────────────────────────────────────────────┐
│                    WEARABLE WRIST-BAND LAYOUT                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌───────────────────────────────────────────────────────────┐    │
│   │  [Flexible Silicone Strap - Adjustable]                   │    │
│   │                                                           │    │
│   │   ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │    │
│   │   │  MAX30102   │  │  ESP8266     │  │    DHT11      │  │    │
│   │   │  Pulse Ox   │  │  NodeMCU     │  │  Temp/Humid   │  │    │
│   │   │  (Underside │  │  (Main PCB)  │  │  (Skin-side)  │  │    │
│   │   │   of wrist) │  │              │  │               │  │    │
│   │   └─────────────┘  └──────────────┘  └───────────────┘  │    │
│   │         │                 │                   │          │    │
│   │         └──────── I2C ────┴────── GPIO ───────┘          │    │
│   │                                                           │    │
│   │   ┌──────┐  ┌────────┐   LiPo 3.7V + TP4056 Charger      │    │
│   │   │ LED  │  │ Buzzer │   (500mAh, ~8hr runtime)          │    │
│   │   └──────┘  └────────┘                                    │    │
│   └───────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Component Placement

| Component | Location | Purpose |
|-----------|----------|---------|
| **MAX30102** | Underside of wrist (optical window) | Heart rate & SpO2 via photoplethysmography |
| **DHT11** | Skin-contact side of enclosure | Body-adjacent temperature & ambient humidity |
| **ESP8266 NodeMCU** | Central PCB module | Edge AI processing, WiFi, MQTT, REST |
| **LED** | Top face of band (visible) | Visual status / alert indicator |
| **Buzzer** | Internal, sound port to exterior | Audible alert for critical events |
| **LiPo Battery** | Opposite side of PCB | Power supply with USB charging |

### Wristband Design

- **Form factor:** 45 mm × 35 mm × 12 mm main module on standard 22 mm watch strap
- **Enclosure:** 3D-printed PLA/PETG biocompatible shell with ventilation slots for DHT11
- **Optical sensor window:** Clear epoxy window for MAX30102 LEDs/photodiode
- **Skin contact:** MAX30102 pressed gently against volar wrist for PPG signal
- **Wireless:** WiFi eliminates bulky radio modules; no patient-worn phone required
- **Edge processing:** All anomaly detection runs on-device — privacy-preserving, low latency

---

## 2. System Architecture Diagram (ASCII)

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           WEARABLE DEVICE LAYER                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐ │
│  │  DHT11   │  │ MAX30102 │  │   LED    │  │  Buzzer  │  │ ESP8266 MCU  │ │
│  │ Temp/Hum │  │  HR/SpO2 │  │  Alert   │  │  Alert   │  │  NodeMCU     │ │
│  └────┬─────┘  └────┬─────┘  └────▲─────┘  └────▲─────┘  └──────┬───────┘ │
│       │             │              │             │                │         │
│       └─────────────┴──────────────┴─────────────┴────────────────┘         │
│                                    │                                         │
│                          ┌─────────▼─────────┐                               │
│                          │   EDGE AI ENGINE   │                               │
│                          │  Z-Score Anomaly   │                               │
│                          │  Detection (W=20)  │                               │
│                          │  Risk Classifier   │                               │
│                          └─────────┬─────────┘                               │
└────────────────────────────────────┼───────────────────────────────────────────┘
                                     │
                    ┌────────────────┼────────────────┐
                    │                │                │
              ┌─────▼─────┐   ┌──────▼──────┐  ┌─────▼─────┐
              │   WiFi    │   │    MQTT     │  │  HTTP REST │
              │  Module   │   │  Publisher  │  │   Client   │
              └─────┬─────┘   └──────┬──────┘  └─────┬─────┘
                    │                │                │
┌───────────────────┼────────────────┼────────────────┼───────────────────────┐
│                   │         CLOUD / LOCAL SERVER LAYER                      │
│                   │                │                │                        │
│            ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐                │
│            │ MQTT Broker │  │ Flask REST  │  │  Dashboard  │                │
│            │  (HiveMQ)   │  │    API      │  │  Chart.js   │                │
│            └─────────────┘  └──────┬──────┘  └──────▲──────┘                │
│                                    │                │                        │
│                             ┌──────▼──────┐         │                        │
│                             │   SQLite    │─────────┘                        │
│                             │  Database   │                                  │
│                             └─────────────┘                                  │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Data Flow Diagram

```
  PATIENT WRIST
       │
       ▼
┌──────────────┐     Raw ADC/I2C      ┌──────────────────┐
│   SENSORS    │ ──────────────────►  │  DATA ACQUISITION │
│ DHT11+MAX30102│                      │  (every 3 sec)    │
└──────────────┘                      └─────────┬─────────┘
                                                │
                                                ▼
                                      ┌──────────────────┐
                                      │  ROLLING WINDOW   │
                                      │  Buffer (N=20)    │
                                      └─────────┬─────────┘
                                                │
                          ┌─────────────────────┼─────────────────────┐
                          ▼                     ▼                     ▼
                 ┌────────────────┐   ┌─────────────────┐   ┌─────────────────┐
                 │ Z-Score Calc   │   │ Risk Engine     │   │ Alert Manager   │
                 │ Z=(X-μ)/σ      │   │ Normal/Warning/ │   │ LED + Buzzer    │
                 │ Threshold: 2.0 │   │ Critical        │   │                 │
                 └───────┬────────┘   └────────┬────────┘   └─────────────────┘
                         │                     │
                         └──────────┬──────────┘
                                    ▼
                          ┌──────────────────┐
                          │  JSON Payload     │
                          │  {temp, hr, spo2, │
                          │   z_scores, risk} │
                          └─────────┬─────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
             MQTT Publish     HTTP POST        Serial Log
          patient/monitoring  /api/patient-data   (Debug)
               /data
                    │               │
                    └───────┬───────┘
                            ▼
                   ┌─────────────────┐
                   │  SQLite Store    │
                   └────────┬────────┘
                            ▼
                   ┌─────────────────┐
                   │  Web Dashboard   │
                   │  Live Charts     │
                   └─────────────────┘
```

---

## 4. Edge AI — Z-Score Anomaly Detection

### Algorithm

```
Z = (X - Mean) / StandardDeviation
```

| Parameter | Value |
|-----------|-------|
| Window Size | 20 samples |
| Anomaly Threshold | \|Z\| > 2.0 |
| Parameters Monitored | Temperature, Humidity, Heart Rate, SpO2 |
| Update Rate | Every 3 seconds |

An anomaly is flagged when **any** monitored parameter exceeds the threshold after the rolling window is full (20 samples = ~60 seconds of baseline).

---

## 5. Predictive Analytics — Rule-Based Risk Engine

| Risk Level | Trigger Conditions (score-based) |
|------------|-------------------------------|
| **Normal** | All vitals within clinical normal ranges |
| **Warning** | One or more vitals in borderline range (score ≥ 2) |
| **Critical** | One or more vitals in dangerous range (score ≥ 5) |

### Scoring Rules

| Parameter | Normal | Warning (+1) | Critical (+3) |
|-----------|--------|--------------|---------------|
| Temperature | 36.5–37.5°C | 36.0–36.5 or 37.5–38.0°C | <36.0 or >38.0°C |
| Heart Rate | 60–90 bpm | 55–60 or 90–100 bpm | <55 or >100 bpm |
| SpO2 | ≥97% | 94–97% | <94% |

---

## 6. Circuit Wiring Table

| Component | Component Pin | ESP8266 Pin | GPIO | Wire Color |
|-----------|--------------|-------------|------|------------|
| DHT11 | VCC | 3V3 | — | Red |
| DHT11 | GND | GND | — | Black |
| DHT11 | DATA | D4 | GPIO2 | Green |
| MAX30102 | VIN | 3V3 | — | Red |
| MAX30102 | GND | GND | — | Black |
| MAX30102 | SDA | D2 | GPIO4 | Blue |
| MAX30102 | SCL | D1 | GPIO5 | Purple |
| LED | Anode (+ 220Ω) | D5 | GPIO14 | Orange |
| LED | Cathode | GND | — | Black |
| Buzzer | Positive (+) | D6 | GPIO12 | Yellow |
| Buzzer | Negative (−) | GND | — | Black |

---

## 7. MQTT Message Schema

**Topic:** `patient/monitoring/data`

```json
{
  "patient_id": "PAT-001",
  "temperature": 36.8,
  "humidity": 45.2,
  "heart_rate": 72,
  "spo2": 98,
  "z_scores": {
    "temperature": 0.12,
    "humidity": -0.05,
    "heart_rate": 0.33,
    "spo2": 0.18
  },
  "risk_level": "Normal",
  "anomaly_status": "OK",
  "anomaly": false,
  "timestamp": 123456789
}
```

---

## 8. REST API Schema

**Endpoint:** `POST /api/patient-data`

**Request Body:**
```json
{
  "patient_id": "PAT-001",
  "temperature": 36.8,
  "humidity": 45.2,
  "heart_rate": 72,
  "spo2": 98,
  "risk_level": "Normal",
  "anomaly_status": "OK"
}
```

**Response (201):**
```json
{
  "success": true,
  "message": "Patient data stored successfully",
  "id": 42,
  "timestamp": "2026-06-21T10:30:00Z",
  "risk_level": "Normal"
}
```

---

## 9. Technology Stack

| Layer | Technologies |
|-------|-------------|
| Embedded | ESP8266, C++, PlatformIO, Arduino Framework |
| Sensors | DHT11, MAX30102 |
| Edge AI | Z-Score Anomaly Detection, Rule-Based ML |
| Communication | WiFi, MQTT, HTTP REST |
| Backend | Python, Flask, SQLite |
| Frontend | HTML5, CSS3, JavaScript, Chart.js |
| Simulation | Wokwi |
| IDE | VS Code |

---

## 10. Security Considerations (Production)

- Use TLS for MQTT (port 8883) and HTTPS for REST API
- Implement device authentication (API keys / JWT)
- Encrypt SQLite database at rest
- HIPAA compliance for real patient deployments
- Secure OTA firmware updates
