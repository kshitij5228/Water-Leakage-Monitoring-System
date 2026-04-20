# Water Monitor (ESP32 + Blynk)

This repository contains an ESP32-based water monitoring and motor control project.

## Project Structure

- `water_monitor/water_monitor.ino` : Main firmware
- `water_monitor/secrets.example.h` : Credentials template
- `pin_config.h` : Pin and virtual pin reference
- `CONNECTION_BABY_STEPS.md` : Wiring guide
- `code_working_log.md` : Change and behavior log

## Features

- Dual flow sensor monitoring (inlet/outlet)
- Leak detection with latch + manual reset
- TDS measurement and threshold alerts
- Water level auto-stop logic
- Blynk dashboard integration
- Buzzer alarms for leak and tank-full events

## Setup

1. Copy `water_monitor/secrets.example.h` to `water_monitor/secrets.h`.
2. Fill your WiFi SSID, WiFi password, and Blynk auth token in `water_monitor/secrets.h`.
3. Open `water_monitor/water_monitor.ino` in Arduino IDE.
4. Select ESP32 board and correct COM port.
5. Install required libraries:
   - Blynk
   - WiFi (ESP32 core)
6. Upload and monitor logs at 115200 baud.

## Security Note

- Keep `water_monitor/secrets.h` private.
- `.gitignore` is configured so this file is never pushed.

## Git Quick Start

```bash
git init
git add .
git commit -m "Initial commit: water monitor project"
git branch -M main
git remote add origin <your-repo-url>
git push -u origin main
```
