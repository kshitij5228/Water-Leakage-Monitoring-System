# Water Monitor Connection Guide (Step by Step)

A clean wiring guide for quick setup, safe testing, and smooth demo.

## 1. Before You Start (Safety First)

- Keep main AC power OFF while wiring.
- Do not connect ESP32 pins directly to AC lines.
- Use proper relay module with correct current rating.
- Keep low-voltage wiring (ESP32 side) physically separate from mains wiring.

## 2. Components Required

- ESP32 development board
- 2 x Flow sensors (inlet and outlet)
- 1 x Relay module
- 1 x Water pump / motor
- 1 x TDS sensor module
- 1 x Water level sensor
- 1 x Buzzer
- Jumper wires + stable power supply

## 3. ESP32 Pin Mapping

| Function | ESP32 Pin |
|---|---|
| Flow Sensor 1 (Inlet) | GPIO 27 |
| Flow Sensor 2 (Outlet) | GPIO 26 |
| Relay IN (Motor Control) | GPIO 25 |
| TDS Analog Input | GPIO 34 |
| Water Level Input | GPIO 33 |
| Buzzer Signal | GPIO 32 |

## 4. Common Ground Rule (Very Important)

Connect all low-voltage grounds together:

- ESP32 GND
- Relay module GND
- Flow Sensor 1 GND
- Flow Sensor 2 GND
- TDS module GND
- Water level sensor GND
- Buzzer negative / GND

Without common ground, readings become unstable or random.

## 5. Sensor Wiring

### 5.1 Flow Sensor 1 (Inlet)

- VCC -> 5V (as per sensor spec)
- GND -> GND
- Signal -> GPIO 27

### 5.2 Flow Sensor 2 (Outlet)

- VCC -> 5V (as per sensor spec)
- GND -> GND
- Signal -> GPIO 26

### 5.3 TDS Sensor

- VCC -> 3.3V or module recommended voltage
- GND -> GND
- Analog Output -> GPIO 34

### 5.4 Water Level Sensor

- VCC -> 3.3V / 5V (as per module)
- GND -> GND
- Signal -> GPIO 33

## 6. Relay and Motor Wiring

### Relay Control Side (Low Voltage)

- Relay VCC -> 5V
- Relay GND -> GND
- Relay IN -> GPIO 25

### Motor Power Side (AC Mains)

- Mains Live -> Relay COM
- Relay NO -> Motor Live
- Mains Neutral -> Motor Neutral
- Earth -> Motor body (if available)

Use only insulated terminals and proper safety practice.

## 7. Buzzer Wiring

- Buzzer + -> GPIO 32
- Buzzer - -> GND

## 8. Blynk Datastream Mapping

| Datastream | Purpose |
|---|---|
| V0 | Flow Rate 1 |
| V1 | Flow Rate 2 |
| V2 | TDS Value |
| V3 | Leakage Status |
| V4 | Status Message |
| V5 | Motor Command |
| V6 | Motor State |
| V7 | Water Level State |
| V8 | Leak Reset |

## 9. First Demo Checklist

1. Power ON ESP32 and open Serial Monitor at 115200.
2. Confirm WiFi and Blynk connection logs appear.
3. Move water through inlet and verify Flow 1 updates.
4. Move water through outlet and verify Flow 2 updates.
5. Toggle V5 in Blynk and confirm motor ON/OFF.
6. Simulate leak condition and check motor auto OFF + latch.
7. Press V8 reset and verify motor can restart.
8. Trigger water-level sensor and verify tank-full motor stop.

## 10. Quick Troubleshooting

- No relay action: check relay VCC/GND/IN and active-low behavior.
- Random flow values: verify common ground and clean sensor wiring.
- Motor not switching: recheck COM/NO mains connection.
- Blynk not updating: verify template ID/name/token and internet access.
- Reboots: use stable power supply; avoid USB brownout.

## 11. Pro Wiring Tips (For Better Look)

- Use color coding:
- Red = VCC
- Black = GND
- Yellow/White = Signal
- Bundle wires with zip ties for cleaner panel look.
- Label each wire near ESP32 header for easy maintenance.
- Keep a printed pin table near hardware setup.
