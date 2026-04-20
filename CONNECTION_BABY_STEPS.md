# Water Monitor Connection Guide (Baby Steps)

## 0. Pehle Safety
- Main power OFF rakho jab tak wiring complete na ho.
- Agar motor AC mains par hai, insulated relay module aur proper rated wiring use karo.
- ESP32 ko motor line se direct mat jodo.

## 1. Jo Components Abhi Code Me Hain
- ESP32 board
- 2 flow sensors
- 1 relay module (motor control)
- 1 pump motor
- 1 buzzer
- WiFi + Blynk app

## 2. ESP32 Pin Map (Current Code)
- Flow Sensor 1 signal pin -> GPIO 27
- Flow Sensor 2 signal pin -> GPIO 26
- Relay IN -> GPIO 25
- Buzzer signal -> GPIO 32
- TDS input (reserved) -> GPIO 34
- Water level input (reserved) -> GPIO 33

## 3. Common Ground Rule (Bahut Important)
- ESP32 GND, relay GND, dono flow sensors GND, buzzer GND ko common ground do.
- Agar ground common nahi hoga to readings random aa sakti hain.

## 4. Flow Sensor 1 Wiring (Inlet)
- Flow sensor VCC -> 5V (ya sensor spec ke hisab se)
- Flow sensor GND -> GND
- Flow sensor signal -> ESP32 GPIO 27

## 5. Flow Sensor 2 Wiring (Outlet)
- Flow sensor VCC -> 5V (ya sensor spec ke hisab se)
- Flow sensor GND -> GND
- Flow sensor signal -> ESP32 GPIO 26

## 6. Relay Wiring (Motor Control)
- Relay VCC -> 5V
- Relay GND -> GND
- Relay IN -> ESP32 GPIO 25

## 6A. Agar Motor Direct Socket Se Chalta Hai
- Haan, same motor ko relay se bhi control kiya ja sakta hai.
- Relay ko mains ke sirf live wire ko switch karne do.
- Neutral wire motor tak direct ja sakti hai.
- Earth wire agar available ho to motor body/earth point par do.
- ESP32 side aur mains side ko physically alag rakho.
- Relay contacts ke liye current rating motor ke hisab se hona chahiye.

## 7. Motor Power Line Relay Ke Through
- AC mains demo me:
- Mains live -> Relay COM
- Relay NO -> Motor live input
- Mains neutral -> Motor neutral
- Earth -> Motor body/earth point
- Is tarah relay ON hoga to motor ko mains supply milegi.

## 7A. Baby Step Demo Order
1. Pehle motor ko direct socket par chala kar confirm karo ki motor healthy hai.
2. Phir socket ka live wire relay COM/NO se pass karo.
3. Relay VCC/GND/IN ko ESP32 se jodo.
4. Blynk me V5 ON karke motor start test karo.
5. Leak simulate karke check karo ki motor OFF ho rahi hai ya nahi.

## 8. Buzzer Wiring
- Buzzer + -> ESP32 GPIO 32
- Buzzer - -> GND

## 9. Blynk Datastream Mapping
- V0 -> Flow 1 rate
- V1 -> Flow 2 rate
- V3 -> Leakage status (0/1)
- V4 -> Status message
- V5 -> Motor command (Button/Switch)
- V6 -> Motor state (LED/Display)
- V8 -> Leak reset button

## 10. Blynk Widget Setup (Baby Step)
- Step 1: V5 par Button banao (0/1).
- Step 2: V8 par Button banao (push mode suggested).
- Step 3: V6 par LED ya value display.
- Step 4: V0, V1 par value display widgets.
- Step 5: V3 par indicator/LED.
- Step 6: V4 par labeled value/message widget.

## 11. First Power-On Check
- Serial monitor 115200 par kholo.
- Flow 1 me pani do: V0 value badhni chahiye.
- Flow 2 me pani do: V1 value badhni chahiye.
- V5 ON karo: motor ON honi chahiye.
- Leak condition aane par motor OFF latch honi chahiye.
- V7 press karke leak latch reset check karo.

## 12. Agar Motor ON Nahi Ho Rahi
- Relay polarity check karo (active-low vs active-high).
- Relay click sound aa raha hai ya nahi check karo.
- COM/NO wiring dobara verify karo.
- Motor supply voltage/capacity verify karo.
- Common ground confirm karo.
- AC mains demo me live wire relay se hi pass ho rahi hai ya nahi check karo.
