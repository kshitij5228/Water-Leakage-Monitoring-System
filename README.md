# Smart Water Monitor

An ESP32 + Blynk based smart project for real-time flow monitoring, leak safety, tank level control, and water quality (TDS) alerts.

## Why This Project Looks Strong

- Real-time inlet vs outlet flow comparison
- Leak latch protection with manual reset
- Water-level auto stop for tank safety
- TDS quality alerts with event notifications
- Non-blocking buzzer patterns for alerts
- Mobile control and monitoring from Blynk

## Project Snapshot

| Item | Description |
|---|---|
| Firmware | `water_monitor/water_monitor.ino` |
| Secrets Template | `water_monitor/secrets.example.h` |
| Local Secrets (ignored) | `water_monitor/secrets.h` |
| Wiring Guide | `CONNECTION_BABY_STEPS.md` |
| Pin Reference | `pin_config.h` |

## Core Features

1. Flow analytics from two sensors (inlet/outlet)
2. Leak confirmation logic with multi-sample validation
3. Motor control via relay and Blynk command
4. Tank level reached detection and motor cut-off
5. TDS monitoring with warning/severe levels
6. Alarm patterns for critical events

## Architecture (Simple Flow)

```text
Flow Sensor 1 --->\
                  \--> ESP32 Logic --> Relay --> Motor
Flow Sensor 2 --->/

TDS Sensor -------> ESP32 -----> Blynk Dashboard (V0..V8)
Water Level Sensor->/
Buzzer <----------- Alert Patterns
```

## Quick Start

1. Create local secrets file.

```bash
copy water_monitor\secrets.example.h water_monitor\secrets.h
```

2. Edit `water_monitor/secrets.h` and fill:
- Blynk auth token
- WiFi SSID
- WiFi password

3. Open `water_monitor/water_monitor.ino` in Arduino IDE.
4. Select ESP32 board and correct COM port.
5. Install required libraries.
- Blynk
- ESP32 core WiFi support
6. Upload firmware and open Serial Monitor at `115200` baud.

## Safety Notes

- Keep low-voltage ESP32 wiring isolated from AC mains.
- Use correctly rated relay module for your motor load.
- Ensure all grounds on low-voltage side are common.

## Security Notes

- `water_monitor/secrets.h` is private and ignored by git.
- Never commit real credentials/tokens.

## GitHub Push

```bash
git remote add origin <your-repo-url>
git push -u origin main
```
