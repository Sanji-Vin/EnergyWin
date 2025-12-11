/*
  ACS712 + Relay + Bluetooth JSON
  - ACS712 pin: GPIO34 (SENSOR_PIN)
  - Relay control pin: GPIO2 (RELAY_PIN)
  - Bluetooth name: ACS712_BT_CTRL
  - Sends JSON lines over Bluetooth every SEND_INTERVAL ms
  - Accepts simple commands via BT: "1", "0", "ON", "OFF", "TOGGLE" (case-insensitive)
*/

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
const char* BT_NAME = "ACS712_BT_CTRL";

// Hardware pins
const int SENSOR_PIN = 34;   // ACS712 OUT -> GPIO34 (ADC)
const int RELAY_PIN  = 5;    // Relay IN -> GPIO2

// ADC / ACS712 configuration
const float ADC_REF = 3.3f;
const int ADC_BITS = 12;
const int ADC_MAX = (1 << ADC_BITS) - 1;

// Set the sensitivity for your ACS712 module (choose one):
const float SENSITIVITY_5A  = 0.185f;
const float SENSITIVITY_20A = 0.100f;
const float SENSITIVITY_30A = 0.066f;
// --- Select the model you have here:
const float SENSITIVITY = SENSITIVITY_5A; // change if you have 20A or 30A module

// Sampling & smoothing
const int NUM_SAMPLES = 100;         // samples per reading (averaging)
const float SMOOTHING = 0.2f;       // exponential smoothing factor (0..1)
const int CAL_SAMPLES = 1500;       // samples used for offset calibration (startup)

// Output timing (ms)
const unsigned long SEND_INTERVAL = 500;

float offsetVoltage = 0.0f;
float smoothedCurrent = 0.0f;
unsigned long lastSend = 0;

bool relayState = LOW; // current relay state (LOW/LOW/HIGH depending on wiring)

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN,LOW);

  // Bluetooth
  SerialBT.begin(BT_NAME); // start Classic Bluetooth SPP
  Serial.println();
  Serial.print("Bluetooth SPP started as: ");
  Serial.println(BT_NAME);
  Serial.println("Pair the device and connect with a BT terminal app.");

  // ADC settings
  analogReadResolution(ADC_BITS);
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db); // allow up to ~3.3V

  Serial.println("Calibrating ACS712 offset. Make sure NO CURRENT flows through sensor now.");
  long sum = 0;
  for (int i = 0; i < CAL_SAMPLES; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);
  }
  float avgADC = (float)sum / (float)CAL_SAMPLES;
  offsetVoltage = avgADC * (ADC_REF / ADC_MAX);
  Serial.print("Calibration done. offsetVoltage = ");
  Serial.print(offsetVoltage, 6);
  Serial.println(" V");
  Serial.println();
  lastSend = millis();
}

int readAveragedADC(int pin) {
  long s = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    s += analogRead(pin);
    delayMicroseconds(150);
  }
  return (int)(s / NUM_SAMPLES);
}

void handleBTCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  bool changed = false;
  if (cmd == "1" || cmd == "ON") {
    relayState = HIGH;
    changed = true;
  } else if (cmd == "0" || cmd == "OFF") {
    relayState = LOW;
    changed = true;
  } else if (cmd == "TOGGLE") {
    relayState = !relayState;
    changed = true;
  } else {
    // Could be other commands. Acknowledge unknown.
    String r = "{\"type\":\"ack\",\"result\":\"unknown_command\",\"cmd\":\"" + cmd + "\"}";
    SerialBT.println(r);
    Serial.println("Unknown command received via BT: " + cmd);
    return;
  }

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  Serial.print("Relay ");
  Serial.print(relayState ? "ON" : "OFF");
  Serial.println(" (via BT command)");

  // Send ack JSON
  String ack = "{\"type\":\"ack\",\"result\":\"ok\",\"relay\":";
  ack += relayState ? "1" : "0";
  ack += "}";
  SerialBT.println(ack);
  Serial.println("Sent ack over BT: " + ack);
}

// Helper to build JSON line
String buildJSON(int raw, float vout, float current, bool relay, unsigned long tMs) {
  String s = "{";
  s += "\"raw\":" + String(raw) + ",";
  s += "\"voltage\":" + String(vout, 5) + ",";
  s += "\"current\":" + String(current, 6) + ",";
  s += "\"relay\":" + String(relay ? 1 : 0) + ",";
  s += "\"time_ms\":" + String(tMs);
  s += "}";
  return s;
}

void loop() {
  // 1) Read ACS712 (averaged)
  int raw = readAveragedADC(SENSOR_PIN);
  float voltage = raw * (ADC_REF / ADC_MAX);
  float current = (voltage - offsetVoltage) / SENSITIVITY;

  // Exponential smoothing
  smoothedCurrent = (smoothedCurrent * (1.0f - SMOOTHING)) + (current * SMOOTHING);

  unsigned long now = millis();

  // 2) Send JSON periodically over Bluetooth AND USB-Serial
  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
    String json = buildJSON(raw, voltage, smoothedCurrent, relayState, now);
    Serial.println(json);       // USB debug
    SerialBT.println(json);     // Bluetooth stream
  }

  // 3) Non-blocking check for incoming BT commands
  if (SerialBT.available()) {
    // Read a line if available (or small command)
    String cmd = SerialBT.readStringUntil('\n'); // reads until newline or timeout
    if (cmd.length() == 0) {
      // maybe no newline: read everything
      cmd = "";
      while (SerialBT.available()) {
        char c = (char)SerialBT.read();
        if (c == '\r' || c == '\n') break;
        cmd += c;
      }
    }
    cmd.trim();
    if (cmd.length() > 0) {
      handleBTCommand(cmd);
    }
  }

  // small sleep to avoid busy-loop (keeps reading rate reasonable)
  delay(30);
}
