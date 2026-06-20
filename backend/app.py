"""
Flask REST API for AIoT Wearable Patient Remote Monitoring System.
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from datetime import datetime

from typing import Tuple

import database as db

app = Flask(__name__)
CORS(app)

db.init_db()


def validate_payload(data: dict) -> Tuple[bool, str]:
    """Validate required fields in incoming patient data."""
    required = ["temperature", "humidity", "heart_rate", "spo2"]
    for field in required:
        if field not in data:
            return False, f"Missing required field: {field}"
    return True, ""


@app.route("/api/health", methods=["GET"])
def health_check():
    """Health check endpoint."""
    return jsonify({
        "status": "ok",
        "service": "Patient Remote Monitoring API",
        "timestamp": datetime.utcnow().isoformat() + "Z",
    })


@app.route("/api/patient-data", methods=["POST"])
def receive_patient_data():
    """
    Receive patient vitals from ESP8266 device.

    Expected JSON body:
    {
        "patient_id": "PAT-001",
        "temperature": 36.8,
        "humidity": 45.2,
        "heart_rate": 72,
        "spo2": 98,
        "risk_level": "Normal",
        "z_temp": 0.12,
        "z_humidity": -0.05,
        "z_heart_rate": 0.33,
        "z_spo2": 0.18,
        "anomaly_status": "OK"
    }
    """
    if not request.is_json:
        return jsonify({"success": False, "error": "Content-Type must be application/json"}), 400

    data = request.get_json(silent=True)
    if not data:
        return jsonify({"success": False, "error": "Invalid JSON body"}), 400

    valid, error = validate_payload(data)
    if not valid:
        return jsonify({"success": False, "error": error}), 400

    try:
        row_id = db.insert_patient_data(data)
        return jsonify({
            "success": True,
            "message": "Patient data stored successfully",
            "id": row_id,
            "timestamp": data.get("timestamp") or datetime.utcnow().isoformat() + "Z",
            "risk_level": data.get("risk_level", "Normal"),
            "anomaly_status": data.get("anomaly_status", "OK"),
        }), 201
    except (ValueError, TypeError) as exc:
        return jsonify({"success": False, "error": f"Invalid data types: {exc}"}), 400
    except Exception as exc:
        return jsonify({"success": False, "error": str(exc)}), 500


@app.route("/api/patient-data/latest", methods=["GET"])
def get_latest_data():
    """Return the most recent patient reading for live dashboard."""
    readings = db.get_latest_reading(limit=1)
    if not readings:
        return jsonify({"success": True, "data": None, "message": "No readings yet"}), 200
    return jsonify({"success": True, "data": readings[0]})


@app.route("/api/patient-data/history", methods=["GET"])
def get_history():
    """Return recent readings for Chart.js visualization."""
    limit = request.args.get("limit", 50, type=int)
    limit = min(max(limit, 1), 500)
    readings = db.get_readings_history(limit=limit)
    return jsonify({"success": True, "count": len(readings), "data": readings})


@app.route("/api/patient-data/stats", methods=["GET"])
def get_statistics():
    """Return aggregate statistics."""
    stats = db.get_stats()
    return jsonify({"success": True, "stats": stats})


if __name__ == "__main__":
    print("Starting Patient Remote Monitoring API on http://localhost:5000")
    app.run(host="0.0.0.0", port=5001, debug=True)
