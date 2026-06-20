# AIoT-Based Remote Patient Monitoring and Diagnostic System with Edge AI and Predictive Analytics

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP8266-orange)](https://platformio.org/)
[![Flask](https://img.shields.io/badge/Flask-3.0-blue)](https://flask.palletsprojects.com/)
[![Wokwi](https://img.shields.io/badge/Wokwi-Simulation-green)](https://wokwi.com/)
[![Edge AI](https://img.shields.io/badge/Edge-AI-purple)](docs/architecture.md)

A complete **AIoT healthcare monitoring system** featuring an ESP8266 wearable wrist-band device with **Edge AI anomaly detection**, MQTT/REST cloud connectivity, Flask backend, SQLite storage, and a real-time Chart.js dashboard.

---

## Features

- **Vital Sign Monitoring** — Temperature, Humidity, Heart Rate, SpO2
- **Edge AI** — Z-Score anomaly detection (window=20, threshold=2.0)
- **Predictive Analytics** — Rule-based risk classification (Normal / Warning / Critical)
- **Alerts** — LED + Buzzer on device; dashboard alert banner
- **MQTT** — Publishes to `patient/monitoring/data`
- **REST API** — Flask POST `/api/patient-data`
- **Dashboard** — Real-time charts with Chart.js
- **Wokwi Simulation** — Full hardware simulation without physical components

---

## Key Innovations

- Edge AI anomaly detection directly on ESP8266
- Predictive risk assessment using historical patient data
- Real-time remote healthcare monitoring
- MQTT and REST dual communication architecture
- Wearable healthcare device simulation using Wokwi
- Automated patient status classification (Normal, Warning, Critical)

---

## Project Structure

```
AIoT-Wearable-Patient-Monitoring-System/
│
├── firmware/                  # ESP8266 Edge AI firmware
│   ├── platformio.ini
│   └── src/main.cpp
│
├── backend/                     # Flask REST API + SQLite
│   ├── app.py
│   ├── database.py
│   └── requirements.txt
│
├── dashboard/                   # Real-time web dashboard
│   ├── index.html
│   ├── style.css
│   └── script.js
│
├── docs/                        # Documentation
│   ├── architecture.md
│   ├── setup.md
│   └── project-report.md
│
├── simulation/                  # Wokwi simulation files
│   ├── diagram.json
│   └── wokwi.toml
│
└── README.md
```

---

## Quick Start

### 1. Backend

```bash
cd backend
python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt
python app.py
```

### 2. Dashboard

```bash
cd dashboard && python3 -m http.server 8080
```

Open **http://localhost:8080**

### 3. Firmware (Wokwi)

```bash
cd firmware && pio run
```

Import `simulation/diagram.json` into [Wokwi](https://wokwi.com) and start simulation.

### 4. Firmware (Hardware)

```bash
cd firmware
pio run --target upload
pio device monitor
```

---

## Circuit Wiring

| Component | ESP8266 Pin | GPIO |
|-----------|-------------|------|
| DHT11 DATA | D4 | GPIO2 |
| MAX30102 SDA | D2 | GPIO4 |
| MAX30102 SCL | D1 | GPIO5 |
| LED (+ 220Ω) | D5 | GPIO14 |
| Buzzer | D6 | GPIO12 |

See [docs/architecture.md](docs/architecture.md) for complete wiring table and diagrams.

---

## Architecture Overview

```
┌─────────────┐    Edge AI     ┌──────────┐
│   Sensors   │ ──────────────►│ ESP8266  │
│ DHT11+MAX30102│  Z-Score    │ NodeMCU  │
└─────────────┘                └────┬─────┘
                                    │
                         ┌──────────┼──────────┐
                         ▼          ▼          ▼
                      MQTT       REST      LED/Buzzer
                         │          │
                         ▼          ▼
                     HiveMQ     Flask API
                                   │
                                   ▼
                               SQLite DB
                                   │
                                   ▼
                              Dashboard
```

---

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | Health check |
| POST | `/api/patient-data` | Store patient vitals |
| GET | `/api/patient-data/latest` | Latest reading |
| GET | `/api/patient-data/history` | Chart history |
| GET | `/api/patient-data/stats` | Aggregate stats |

---

## MQTT Topic

**Topic:** `patient/monitoring/data`

**Payload fields:** `temperature`, `humidity`, `heart_rate`, `spo2`, `z_scores`, `risk_level`, `anomaly_status`

---

## Edge AI Parameters

| Parameter | Value |
|-----------|-------|
| Algorithm | Z-Score: Z = (X − μ) / σ |
| Window Size | 20 samples |
| Anomaly Threshold | \|Z\| > 2.0 |
| Monitored | Temperature, Humidity, HR, SpO2 |

---

## Screenshots

### Dashboard
![Dashboard](screenshots/dashboard.png)

### Wokwi Simulation
![Wokwi](screenshots/wokwi.png)

### System Architecture
![Architecture](screenshots/architecture.png)

---

## Technologies

IoT · AIoT · Edge Computing · Edge AI · MQTT · REST API · WiFi · Embedded C++ · PlatformIO · VS Code · ESP8266 · DHT11 · MAX30102 · SQLite · Flask · HTML · CSS · JavaScript · Chart.js · Wokwi · Predictive Analytics · Anomaly Detection · Real-Time Monitoring · Remote Healthcare

---

## Future Enhancements

- Integration with cloud platforms (AWS IoT / Azure IoT)
- Machine learning-based disease prediction
- Mobile application for remote patient access
- Doctor notification and emergency alert system
- Wearable battery optimization
- Multi-patient monitoring support

---

## Documentation

- [Architecture & Diagrams](docs/architecture.md)
- [Setup & Deployment Guide](docs/setup.md)
- [Final Project Report](docs/project-report.md)

---

## License

This project is provided for educational and research purposes.

---

## Author. 

Sai Manikanta

B.Tech Data Science | Mohan Babu University

Academic Project: AIoT-Based Remote Patient Monitoring and Diagnostic System with Edge AI and Predictive AnalyticsAIoT Wearable Patient Monitoring System — Senior IoT / Edge AI Engineering Project
