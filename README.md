# Smart EV Charging Station Optimizer

Edge AI based multi-bay EV charging station simulation using ESP32, MQTT, ThingsBoard and Wokwi.

## What this package contains
- `firmware/` - ESP32 Arduino firmware and generated Edge-AI model header
- `ml/` - Python synthetic-data generation and model-training/export scripts
- `wokwi/` - 3-bay Wokwi simulation files
- `thingsboard/` - telemetry keys, dashboard plan, alarms and RPC notes
- `docs/` - project implementation checklist

## Fast start

### 1. Wokwi
Open the files in `wokwi/` in a Wokwi ESP32 project.
Use the Wokwi WiFi network:
- SSID: Wokwi-GUEST
- Password: empty

### 2. ThingsBoard
Create three devices:
- BAY_01
- BAY_02
- BAY_03

Copy each device access token into the corresponding firmware constants.

MQTT:
- Host: demo.thingsboard.io
- Port: 1883
- Telemetry topic: v1/devices/me/telemetry

For a first test, use the same firmware and change BAY_ID + token for each bay.

### 3. Python ML
From `ml/`:
    python generate_data.py
    python train_models.py

The training script creates/updates `firmware/model.h`.

### 4. Firmware
Install:
- PubSubClient
- ArduinoJson
- DHT sensor library
- Adafruit Unified Sensor

Upload/run the sketch. The ESP32 reads voltage/current/temperature, calculates power and energy, predicts arrival probability and duration, applies ALLOW/THROTTLE/DEFER logic, and publishes telemetry.

## Important
This is a simulation. Do NOT connect the relay circuit to real mains/high-voltage EV charging equipment.
