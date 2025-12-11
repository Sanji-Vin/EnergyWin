# EnergyWin
# 📡 ESP32 Bluetooth Power Monitor & Relay Controller

This project is a complete **power monitoring and wireless control system** built using an **ESP32**, an **ACS712 current sensor**, and a **relay module**.  
It continuously measures electrical current and voltage, sends the readings as **JSON data via Bluetooth**, and allows **remote relay control** from a phone or PC.

Ideal for smart home automation, power monitoring, IoT prototypes, and Bluetooth-controlled switching.

---

## 🚀 Features

### 📊 Real-Time Current & Voltage Monitoring
- Uses **ACS712** (5A / 20A / 30A)
- Sensor connected to **GPIO 34**
- Auto offset calibration at startup
- Multi-sampling + filtering for stable readings
- Sends JSON data every 500 ms:
```json
{
  "raw": 1980,
  "voltage": 2.3345,
  "current": 0.742312,
  "relay": 1,
  "time_ms": 123456
}
```

---

### 🔌 Wireless Relay Control (GPIO 2)
Allows switching any AC/DC load using a relay module.

Commands via Bluetooth:
- `"1"` / `"ON"` → Relay ON  
- `"0"` / `"OFF"` → Relay OFF  
- `"TOGGLE"` → Switch state  

ESP32 replies with confirmation in JSON.

---

### 📱 Bluetooth Connectivity
ESP32 works as a **Classic Bluetooth SPP device**.

Compatible with:
- Android apps: Serial Bluetooth Terminal, Bluetooth Remote  
- Windows tools: PuTTY, Tera Term, Bluetooth Serial Terminal  
- Python via `pyserial`

---

### 🖥️ PC Logging Support
A Python script can:
- Connect to the ESP32 Bluetooth COM port  
- Receive JSON data  
- Save logs to `.jsonl`  
- Send relay commands  

Perfect for data analysis & automation.

---

## 🔧 Hardware Setup

### Components:
- ESP32 board  
- ACS712 current sensor  
- Relay module  
- Jump wires  

### Wiring:
| Component | ESP32 Pin |
|----------|-----------|
| ACS712 OUT | GPIO 34 |
| ACS712 VCC | 5V |
| ACS712 GND | GND |
| Relay IN | GPIO 2 |
| Relay VCC | 5V |
| Relay GND | GND |

---

## 📡 How It Works

1. ESP32 calibrates ACS712 offset on startup.  
2. Reads ADC → averages → filters noise.  
3. Computes voltage + current.  
4. Sends JSON via Bluetooth.  
5. Listens for ON/OFF/TOGGLE commands.  
6. Updates relay state and sends acknowledgment.  

---

## 🧩 Included in This Project
- ESP32 Arduino firmware  
- Python logging script  
- Documentation for Android apps  

---

## 🧠 Future Improvements
- MQTT / WiFi dashboard  
- SD card logging  
- OLED display  
- Cloud monitoring  

---

## 📜 License
MIT License.

