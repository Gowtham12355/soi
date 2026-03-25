🚗 EV Battery Monitoring System using ESP32

📌 Overview

A real-time EV Battery Monitoring System built using the ESP32 Microcontroller to monitor critical battery and motor parameters.

This project demonstrates how Embedded Systems + IoT can be used to build a basic Battery Management System (BMS).

⚙️ Features
🔋 Battery Voltage Monitoring
⚡ Battery Current using INA219 Current Sensor
🌡 Battery Temperature using DS18B20 Temperature Sensor
⚙ Motor RPM Estimation
🎛 Motor Speed Control via Web Dashboard
🌐 Real-time Web Dashboard (Localhost)
🧰 Hardware Requirements
ESP32 Microcontroller
INA219 Current Sensor
DS18B20 Temperature Sensor
Motor Driver (L298N or similar)
DC Motor
Battery (Li-ion / EV Battery)
Resistors (4.7kΩ, voltage divider)
🔌 Circuit Connections
🔹 INA219 (Current Sensor)
VCC → 3.3V
GND → GND
SDA → GPIO21
SCL → GPIO22
🔹 Battery Path (IMPORTANT)
Battery + → VIN+ (INA219)
VIN- → Motor Driver +
Battery - → GND
🔹 DS18B20 (Temperature Sensor)
VCC → 3.3V
GND → GND
DATA → GPIO2

⚠ Add 4.7kΩ resistor between DATA and VCC

🔹 Motor Driver
ENA → GPIO33
IN1 → GPIO26
IN2 → GPIO27
🧠 Working Principle
INA219 measures battery voltage & current
DS18B20 measures battery temperature
Motor voltage is read via ADC and used to estimate RPM:
RPM = (Motor Voltage / Max Voltage) × Max RPM
ESP32 hosts a web server dashboard
🌐 Web Dashboard

Open in browser:

http://ESP32_IP_ADDRESS

Example:

http://192.168.1.50
Dashboard Shows:
Battery Voltage
Battery Current
Temperature
Motor Voltage
RPM
Motor Speed Control Slider
📦 Libraries Used
Adafruit INA219
DallasTemperature
OneWire
WiFi (ESP32 built-in)
🚀 Getting Started
git clone https://github.com/your-username/ev-battery-monitor.git
Steps:
Open project in Arduino IDE
Install required libraries
Update WiFi credentials:
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
Upload to ESP32
Open Serial Monitor → Get IP
Open dashboard in browser
🧪 Testing
Ensure current flows through INA219
Verify temperature readings
Check Serial Monitor for debug output
Adjust RPM scaling if needed
📸 Project Demo
🔹 Circuit Setup

🔹 Dashboard View

🔮 Future Improvements
📊 Real-time graphs
🔋 Battery percentage calculation
⚠ Over-current & over-temperature protection
☁ IoT Cloud Integration
📱 Mobile App Dashboard
👨‍💻 Author

Gowtham
Electronics & Communication Engineering
