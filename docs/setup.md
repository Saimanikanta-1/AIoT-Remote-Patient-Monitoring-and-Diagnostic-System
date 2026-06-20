# Setup & Deployment Guide

## AIoT Wearable Patient Remote Monitoring System

---

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| VS Code | Latest | IDE |
| PlatformIO Extension | Latest | ESP8266 firmware build |
| Python | 3.9+ | Flask backend |
| pip | Latest | Python packages |
| Wokwi Account | Free | Hardware simulation |
| MQTT Client (optional) | Any | Monitor MQTT messages |

---

## 1. Clone / Open Project

```bash
cd AIoT-Wearable-Patient-Monitoring-System
code .
```

---

## 2. Firmware Setup (ESP8266)

### Install PlatformIO

1. Open VS Code
2. Install **PlatformIO IDE** extension
3. Open the `firmware/` folder

### Configure Network (Optional)

Edit `firmware/src/main.cpp` or set build flags in `platformio.ini`:

| Define | Default | Description |
|--------|---------|-------------|
| `WIFI_SSID` | `Wokwi-GUEST` | WiFi network name |
| `WIFI_PASSWORD` | `` | WiFi password |
| `MQTT_BROKER` | `broker.hivemq.com` | MQTT broker host |
| `API_ENDPOINT` | `http://localhost:5000/api/patient-data` | Flask API URL |

For **physical hardware**, change WiFi credentials:

```ini
build_flags =
    -D WIFI_SSID=\"YourNetwork\"
    -D WIFI_PASSWORD=\"YourPassword\"
    -D API_ENDPOINT=\"http://192.168.1.100:5000/api/patient-data\"
```

### Build Firmware

```bash
cd firmware
pio run
```

### Upload to Hardware

Connect ESP8266 via USB, then:

```bash
pio run --target upload
pio device monitor
```

---

## 3. Backend Setup (Flask + SQLite)

```bash
cd backend
python3 -m venv venv
source venv/bin/activate        # Windows: venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

The API starts at **http://localhost:5000**

### Verify API

```bash
curl http://localhost:5000/api/health
```

### Test POST endpoint

```bash
curl -X POST http://localhost:5000/api/patient-data \
  -H "Content-Type: application/json" \
  -d '{
    "patient_id": "PAT-001",
    "temperature": 36.8,
    "humidity": 45.0,
    "heart_rate": 72,
    "spo2": 98,
    "risk_level": "Normal",
    "anomaly_status": "OK"
  }'
```

---

## 4. Dashboard Setup

### Option A: Open directly in browser

```bash
cd dashboard
python3 -m http.server 8080
```

Open **http://localhost:8080** in your browser.

> Ensure Flask backend is running on port 5000.

### Option B: VS Code Live Server

1. Install **Live Server** extension
2. Right-click `dashboard/index.html` → **Open with Live Server**

---

## 5. Wokwi Simulation

### Step 1: Build firmware

```bash
cd firmware
pio run
```

### Step 2: Open in Wokwi

1. Go to [https://wokwi.com](https://wokwi.com)
2. Create new project → **Import from diagram.json**
3. Upload `simulation/diagram.json`
4. Copy `firmware/` source or link PlatformIO project
5. Place `wokwi.toml` in project root (or configure paths)

### Step 3: Run simulation

1. Click **Start Simulation** (green play button)
2. Open Serial Monitor (115200 baud)
3. Observe vitals, Z-scores, and risk classification
4. LED blinks on Warning; solid on Critical
5. Buzzer activates on Critical alerts

### Wokwi WiFi

Wokwi provides a simulated `Wokwi-GUEST` network automatically — no password required.

### Wokwi + Backend Integration

To forward HTTP from Wokwi to local Flask:

1. Start Flask: `python backend/app.py`
2. In Wokwi, use gateway IP for API endpoint
3. Or monitor MQTT at `broker.hivemq.com` topic `patient/monitoring/data`

---

## 6. VS Code Execution Steps (Full Stack)

### Terminal 1 — Backend

```bash
cd backend && source venv/bin/activate && python app.py
```

### Terminal 2 — Dashboard

```bash
cd dashboard && python3 -m http.server 8080
```

### Terminal 3 — Firmware (Hardware)

```bash
cd firmware && pio run --target upload && pio device monitor
```

### Terminal 3 — Firmware (Wokwi)

```bash
cd firmware && pio run
# Then start Wokwi simulation
```

---

## 7. MQTT Monitoring (Optional)

Subscribe to live data:

```bash
mosquitto_sub -h broker.hivemq.com -t "patient/monitoring/data" -v
```

Or use [HiveMQ Web Client](http://www.hivemq.com/demos/websocket-client/).

---

## 8. Troubleshooting

| Issue | Solution |
|-------|----------|
| DHT11 reads NaN | Check wiring on D4; add 10kΩ pull-up on DATA |
| MAX30102 not found | Verify I2C on D1/D2; in Wokwi, simulation mode auto-activates |
| WiFi won't connect | Verify SSID/password; use `Wokwi-GUEST` in simulation |
| MQTT publish fails | Check broker address; ensure WiFi connected |
| API POST fails | Ensure Flask is running; check firewall on port 5000 |
| Dashboard shows Offline | Start Flask backend; check CORS and API URL in `script.js` |
| No chart data | Wait for at least 1 POST from device or send test curl |

---

## 9. Project Structure Reference

```
AIoT-Wearable-Patient-Monitoring-System/
├── firmware/
│   ├── platformio.ini
│   └── src/main.cpp
├── backend/
│   ├── app.py
│   ├── database.py
│   └── requirements.txt
├── dashboard/
│   ├── index.html
│   ├── style.css
│   └── script.js
├── docs/
│   ├── architecture.md
│   ├── setup.md
│   └── project-report.md
├── simulation/
│   ├── diagram.json
│   └── wokwi.toml
└── README.md
```

---

## 10. Production Deployment Checklist

- [ ] Change default WiFi/MQTT/API credentials
- [ ] Enable HTTPS and MQTTS
- [ ] Deploy Flask with Gunicorn + Nginx
- [ ] Use PostgreSQL instead of SQLite for scale
- [ ] Add authentication to API endpoints
- [ ] Configure OTA firmware updates
- [ ] Calibrate MAX30102 for target skin tone
- [ ] Validate clinical thresholds with medical professional
