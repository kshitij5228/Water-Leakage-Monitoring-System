# Code Working Log

## Current Focus
- Main working file: `water_monitor.ino`
- Pin mapping is now inline inside `water_monitor.ino`.
- Rule: any new code update should be reflected here with a short working note.
- Rule: WiFi and Blynk credentials must not be echoed in responses.

## Current Code Working
### `water_monitor.ino`
- Reads flow pulses from the first sensor as inlet flow and the second sensor as outlet flow.
- Opens the normally closed solenoid/relay when inlet flow is detected.
- Closes the valve when inlet flow stops.
- Prints inlet flow, outlet flow, totals, and valve state to Serial Monitor.
- Sends inlet flow, outlet flow, valve state, and status text to Blynk.
- Uses inline `#define` values in `water_monitor.ino` for hardware and virtual pin definitions.

## Update Rule
- Whenever code changes are made, add a short note here explaining:
  - what changed
  - how it works
  - whether Blynk wiring was updated

## Next Planned Additions
- Manual Blynk valve control
- Leak detection
- TDS reading
- Water level and buzzer logic

## Latest Update
- Removed valve logic and switched relay output to motor control on the same pin (`RELAY_PIN 25`).
- `V5` is now motor command and `V6` is motor state.
- Leakage logic added: if inlet flow is significantly higher than outlet flow for a few seconds, leakage is detected and motor is forced OFF.
- `V3` now reports leakage status (`1` leak, `0` normal), and status message updated accordingly.
- Removed currently unused pins (`TDS`, `WATER_LEVEL`, `BUZZER`) from main code, keeping only active pins: `FLOW_SENSOR_1_PIN`, `FLOW_SENSOR_2_PIN`, `RELAY_PIN`.
- **Cold Boot Crash Fix**: Added 500ms `delay()` + `yield()` before `WiFi.mode(WIFI_STA)` to prevent first-boot SW_RESET caused by ESP32 RF calibration. System now boots cleanly on first attempt without double-reset.
- **Git Safety Update**: Moved WiFi and Blynk credentials to `water_monitor/secrets.h`, added `water_monitor/secrets.example.h` template, and updated `.gitignore` to prevent committing real secrets.
