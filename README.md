# Predict Kit Website

A local-network prototype: ESP32 sensors -> Flask API -> browser dashboard.

## Requirements
- Python 3.10+
- ESP32 board package in Arduino IDE
- Arduino libraries: `OneWire`, `DallasTemperature`
- ESP32 and the computer running Flask must be on the same Wi-Fi/LAN

## 1. Run the website/backend
Open a terminal in this project folder:

```bash
cd backend
python -m venv .venv
```

Windows PowerShell:
```powershell
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python app.py
```

Windows Command Prompt:
```bat
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

Open `http://127.0.0.1:5000` on the computer running the backend.

## 2. Find the computer's local IPv4 address
In Windows Command Prompt, run `ipconfig` and look for the IPv4 Address of the active Wi-Fi adapter, e.g. `192.168.1.10`.

In `esp32/predict_kit.ino`, replace:
- `YOUR_WIFI_NAME`
- `YOUR_WIFI_PASSWORD`
- `http://192.168.1.10:5000/api/readings` with your computer's actual IPv4 address.

Allow Python through Windows Firewall on a private network if prompted. Keep the computer and ESP32 on the same network. Some guest Wi-Fi networks isolate devices.

## 3. Upload ESP32 code
In Arduino IDE:
1. Install `OneWire` and `DallasTemperature` from Library Manager.
2. Select your ESP32 board and its COM port.
3. Update Wi-Fi details, backend URL, and GPIO pin constants to match your wiring.
4. Upload `esp32/predict_kit.ino`.
5. Open Serial Monitor at 115200 baud.

Then open `http://127.0.0.1:5000` and confirm the cards update.

## Important sensor notes
- GPIO numbers and sensor assumptions in the sketch are examples. Verify them against your actual wiring.
- DS18B20 normally needs a 4.7 kΩ pull-up between DATA and 3.3 V.
- ACS712 and ZMPT101B readings in the sketch are demo estimates only. Calibrate them using appropriate procedures before relying on readings.
- Mains voltage is dangerous. Never connect mains directly to the ESP32 or a breadboard. Use properly isolated modules and qualified supervision.
- The displayed health score is a simple demonstration rule set, not an AI/ML model or certified safety system.
- This development server is for a trusted local network. Do not expose it directly to the public internet. For remote access, add authentication, HTTPS, and a production deployment.

## Data flow
ESP32 sends JSON to `POST /api/readings`. The browser requests `GET /api/latest` and `GET /api/history`.
