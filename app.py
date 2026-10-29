from datetime import datetime, timezone
from pathlib import Path
from threading import Lock
import json
import os

from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

BASE_DIR = Path(__file__).resolve().parent.parent
FRONTEND_DIR = BASE_DIR / "frontend"
DATA_DIR = Path(__file__).resolve().parent / "data"
HISTORY_FILE = DATA_DIR / "readings.json"
DATA_DIR.mkdir(parents=True, exist_ok=True)

app = Flask(__name__)
CORS(app)  # For local development. Restrict origins before public deployment.

lock = Lock()
latest = {
    "temperature_c": None,
    "current_a": None,
    "voltage_v": None,
    "vibration": None,
    "door_open": None,
    "device_id": "predict-kit-01",
    "received_at": None,
    "health_score": None,
    "status": "Waiting for ESP32 data",
    "alerts": ["No readings received yet"]
}
history = []

def number_or_none(value):
    try:
        if value is None:
            return None
        result = float(value)
        if result != result or abs(result) == float("inf"):
            return None
        return result
    except (TypeError, ValueError):
        return None

def evaluate(payload):
    """Basic demonstration rules, NOT a trained predictive-maintenance model."""
    temp = number_or_none(payload.get("temperature_c"))
    current = number_or_none(payload.get("current_a"))
    voltage = number_or_none(payload.get("voltage_v"))
    vibration = number_or_none(payload.get("vibration"))
    door_open = payload.get("door_open")

    alerts = []
    score = 100

    if temp is not None:
        if temp >= 70:
            alerts.append("High temperature")
            score -= 35
        elif temp >= 55:
            alerts.append("Temperature elevated")
            score -= 15

    if current is not None and current < 0:
        alerts.append("Current sensor calibration may be incorrect")
        score -= 10

    if voltage is not None:
        if voltage < 180 or voltage > 250:
            alerts.append("Voltage outside demo range (180–250 V)")
            score -= 25

    # Vibration is a raw/demo value unless the sensor is calibrated.
    if vibration is not None and vibration > 1:
        alerts.append("Vibration level elevated (demo threshold)")
        score -= 15

    if door_open is True:
        alerts.append("Door is open")

    score = max(0, min(100, score))
    if score >= 80:
        status = "Healthy"
    elif score >= 50:
        status = "Needs attention"
    else:
        status = "Fault suspected"

    if not alerts:
        alerts = ["No rule-based alerts"]
    return score, status, alerts

@app.get("/api/health")
def api_health():
    return jsonify({"ok": True, "service": "Predict Kit API"})

@app.post("/api/readings")
def receive_readings():
    global latest, history
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        return jsonify({"error": "Send a JSON object"}), 400

    clean = {
        "device_id": str(payload.get("device_id", "predict-kit-01"))[:80],
        "temperature_c": number_or_none(payload.get("temperature_c")),
        "current_a": number_or_none(payload.get("current_a")),
        "voltage_v": number_or_none(payload.get("voltage_v")),
        "vibration": number_or_none(payload.get("vibration")),
        "door_open": payload.get("door_open") if isinstance(payload.get("door_open"), bool) else None,
        "received_at": datetime.now(timezone.utc).isoformat()
    }
    score, status, alerts = evaluate(clean)
    clean.update({"health_score": score, "status": status, "alerts": alerts})

    with lock:
        latest = clean
        history.append(clean)
        history = history[-300:]
        try:
            HISTORY_FILE.write_text(json.dumps(history, indent=2), encoding="utf-8")
        except OSError:
            app.logger.exception("Could not save readings history")

    return jsonify({"ok": True, "reading": clean}), 201

@app.get("/api/latest")
def get_latest():
    with lock:
        return jsonify(latest)

@app.get("/api/history")
def get_history():
    with lock:
        return jsonify(history[-100:])

@app.get("/")
def index():
    return send_from_directory(FRONTEND_DIR, "index.html")

@app.get("/<path:path>")
def frontend_files(path):
    return send_from_directory(FRONTEND_DIR, path)

if __name__ == "__main__":
    # Accessible on your local network so the ESP32 can reach this computer.
    # Do not expose this development server directly to the public internet.
    app.run(host="0.0.0.0", port=5000, debug=False)
