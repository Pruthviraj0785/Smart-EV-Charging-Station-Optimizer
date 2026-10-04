#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "model.h"

// ---------------- USER CONFIGURATION ----------------
#define BAY_ID "BAY_01"
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* TB_SERVER = "demo.thingsboard.io";
const int TB_PORT = 1883;

// Put the ThingsBoard DEVICE ACCESS TOKEN here.
const char* TB_TOKEN = "YOUR_BAY_01_ACCESS_TOKEN";

// ---------------- PINS ----------------
#define VOLTAGE_PIN 34
#define CURRENT_PIN 35
#define DHT_PIN 15
#define DHT_TYPE DHT22

#define RELAY_PIN 26
#define BTN_PLUGIN 32
#define BTN_PLUGOUT 33

#define LED_GREEN 18
#define LED_YELLOW 19
#define LED_RED 23

// ---------------- LIMITS ----------------
float maxStationLoadW = 6000.0;
float overloadCurrentA = 16.0;
float predictionThreshold = 0.50;

// Simulation scale: potentiometer -> 0..250 V and 0..32 A.
const float MAX_SIM_VOLTAGE = 250.0;
const float MAX_SIM_CURRENT = 32.0;

// ---------------- TIMING ----------------
const unsigned long SAMPLE_INTERVAL = 5000;
const unsigned long OPTIMIZE_INTERVAL = 5000;
const unsigned long DEBOUNCE_MS = 200;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastSample = 0;
unsigned long lastOptimize = 0;
unsigned long lastEnergyMillis = 0;

float voltage = 0;
float current = 0;
float power = 0;
float energyWh = 0;
float temperature = 0;

bool bayCharging = false;
bool lastPluginState = HIGH;
bool lastPlugoutState = HIGH;

float predictedArrivalProb = 0.30;
int predictedDurationMin = 30;

String loadDecision = "ALLOW";
int throttleLevel = 100;

// Feature state
float recentAvgCurrent = 0;
float sessionElapsedMin = 0;
int dayOfWeek = 1;

// ---------------- CONNECTION ----------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection timeout.");
  }
}

void connectMQTT() {
  if (mqttClient.connected()) return;

  while (!mqttClient.connected()) {
    connectWiFi();
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.print("Connecting to ThingsBoard...");
    if (mqttClient.connect(BAY_ID, TB_TOKEN, NULL)) {
      Serial.println("connected!");
      mqttClient.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

// ---------------- RPC ----------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];

  Serial.print("RPC: ");
  Serial.println(message);

  JsonDocument doc;
  if (deserializeJson(doc, message)) return;

  const char* method = doc["method"];
  JsonVariant params = doc["params"];

  if (!method) return;

  if (String(method) == "setRelayState") {
    bool state = params.as<bool>();
    bayCharging = state;
  }

  if (String(method) == "setThrottle") {
    throttleLevel = constrain(params.as<int>(), 0, 100);
    if (throttleLevel == 0) bayCharging = false;
  }

  if (String(method) == "getStatus") {
    publishTelemetry();
  }
}

// ---------------- SENSOR / POWER ----------------
float readVoltage() {
  int raw = analogRead(VOLTAGE_PIN);
  return (raw / 4095.0) * MAX_SIM_VOLTAGE;
}

float readCurrent() {
  int raw = analogRead(CURRENT_PIN);
  return (raw / 4095.0) * MAX_SIM_CURRENT;
}

void updateEnergy() {
  unsigned long now = millis();
  if (lastEnergyMillis == 0) {
    lastEnergyMillis = now;
    return;
  }

  float hours = (now - lastEnergyMillis) / 3600000.0;
  if (bayCharging) {
    energyWh += power * hours;
  }
  lastEnergyMillis = now;
}

void readSensors() {
  voltage = readVoltage();
  current = readCurrent();

  float t = dht.readTemperature();
  if (!isnan(t)) temperature = t;

  power = voltage * current;

  // Simple exponential moving average for the feature.
  recentAvgCurrent = 0.7 * recentAvgCurrent + 0.3 * current;

  if (bayCharging) {
    sessionElapsedMin += SAMPLE_INTERVAL / 60000.0;
  }

  updateEnergy();
}

// ---------------- BAY STATUS ----------------
void checkButtons() {
  bool plugin = digitalRead(BTN_PLUGIN);
  bool plugout = digitalRead(BTN_PLUGOUT);

  if (lastPluginState == HIGH && plugin == LOW) {
    bayCharging = true;
    Serial.println("EV PLUG-IN -> CHARGING");
    delay(DEBOUNCE_MS);
  }

  if (lastPlugoutState == HIGH && plugout == LOW) {
    bayCharging = false;
    sessionElapsedMin = 0;
    Serial.println("EV PLUG-OUT -> FREE");
    delay(DEBOUNCE_MS);
  }

  lastPluginState = plugin;
  lastPlugoutState = plugout;
}

// ---------------- EDGE AI ----------------
void runEdgeAI() {
  // The generated model.h contains lightweight local inference.
  float features[5] = {
    (float)(millis() / 3600000UL % 24), // simulated hour
    (float)dayOfWeek,
    bayCharging ? 1.0f : 0.0f,
    recentAvgCurrent,
    sessionElapsedMin
  };

  predictedArrivalProb = predictArrival(features);

  float duration = predictDuration(features);
  predictedDurationMin = constrain((int)round(duration), 1, 180);
}

// ---------------- OPTIMIZER ----------------
void optimizeLocalLoad() {
  loadDecision = "ALLOW";
  throttleLevel = 100;

  if (current > overloadCurrentA) {
    loadDecision = "THROTTLE";
    throttleLevel = 50;
  } else if (power > maxStationLoadW) {
    loadDecision = "THROTTLE";
    throttleLevel = 70;
  } else if (!bayCharging && predictedArrivalProb < predictionThreshold) {
    loadDecision = "DEFER";
    throttleLevel = 0;
  } else {
    loadDecision = "ALLOW";
    throttleLevel = 100;
  }

  // Relay represents charger enable/disable in the simulation.
  if (!bayCharging) {
    digitalWrite(RELAY_PIN, LOW);
  } else if (throttleLevel == 0) {
    digitalWrite(RELAY_PIN, LOW);
  } else {
    digitalWrite(RELAY_PIN, HIGH);
  }
}

String getBayStatus() {
  if (current > overloadCurrentA) return "FAULT";
  if (bayCharging) return "CHARGING";
  return "FREE";
}

void updateLEDs() {
  String status = getBayStatus();

  digitalWrite(LED_GREEN, status == "FREE" ? HIGH : LOW);
  digitalWrite(LED_YELLOW, status == "CHARGING" ? HIGH : LOW);
  digitalWrite(LED_RED, status == "FAULT" ? HIGH : LOW);
}

// ---------------- TELEMETRY ----------------
void publishTelemetry() {
  if (!mqttClient.connected()) return;

  JsonDocument doc;
  doc["bayId"] = BAY_ID;
  doc["voltage"] = voltage;
  doc["current"] = current;
  doc["power"] = power;
  doc["energyWh"] = energyWh;
  doc["temperature"] = temperature;
  doc["bayStatus"] = getBayStatus();
  doc["predictedArrivalProb"] = predictedArrivalProb;
  doc["predictedDurationMin"] = predictedDurationMin;
  doc["loadDecision"] = loadDecision;
  doc["throttleLevel"] = throttleLevel;

  char buffer[768];
  serializeJson(doc, buffer);

  Serial.print("Telemetry: ");
  Serial.println(buffer);

  mqttClient.publish("v1/devices/me/telemetry", buffer);
}

// ---------------- SETUP / LOOP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BTN_PLUGIN, INPUT_PULLUP);
  pinMode(BTN_PLUGOUT, INPUT_PULLUP);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

  dht.begin();

  mqttClient.setServer(TB_SERVER, TB_PORT);
  mqttClient.setCallback(mqttCallback);

  connectWiFi();
  connectMQTT();

  lastEnergyMillis = millis();

  Serial.println("Smart EV Charging Bay started.");
}

void loop() {
  connectWiFi();
  connectMQTT();

  mqttClient.loop();

  checkButtons();

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;

    readSensors();
    updateLEDs();

    if (now - lastOptimize >= OPTIMIZE_INTERVAL) {
      lastOptimize = now;
      runEdgeAI();
      optimizeLocalLoad();
    }

    publishTelemetry();
  }
}
