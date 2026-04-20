#define BLYNK_TEMPLATE_ID "TMPL3csfmVrdx"
#define BLYNK_TEMPLATE_NAME "Water Monitor"

#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h"

// ─── Pin Map ───────────────────────────────────────────────
#define FLOW_SENSOR_1_PIN 27
#define FLOW_SENSOR_2_PIN 26
#define RELAY_PIN 25
#define TDS_SENSOR_PIN 34
#define WATER_LEVEL_PIN 33
#define BUZZER_PIN 32

// ─── Blynk Virtual Pins ────────────────────────────────────
#define VPIN_FLOW_RATE_1 V0
#define VPIN_FLOW_RATE_2 V1
#define VPIN_TDS_VALUE V2
#define VPIN_LEAKAGE_STATUS V3
#define VPIN_STATUS_MESSAGE V4
#define VPIN_MOTOR_COMMAND V5
#define VPIN_MOTOR_STATE V6
#define VPIN_WATER_LEVEL V7
#define VPIN_LEAK_RESET V8

BlynkTimer timer;

// ─── ISR Variables ─────────────────────────────────────────
volatile uint32_t flowPulses1 = 0;
volatile uint32_t flowPulses2 = 0;
volatile uint32_t lastPulseUs1 = 0;
volatile uint32_t lastPulseUs2 = 0;

// ─── Runtime State ─────────────────────────────────────────
float flowRate1 = 0.0f;
float flowRate2 = 0.0f;
float totalLiters1 = 0.0f;
float totalLiters2 = 0.0f;

bool motorOn = false;
bool motorCommandOn = true;
bool leakDetected = false;
uint32_t leakStartMs = 0;
uint32_t motorStartMs = 0; // Motor ON hone ka timestamp — grace period ke liye

// Leak latch — ek baar leak confirm ho toh manual reset tak motor ON nahi hoga
bool leakLatch = false;

// ─── TDS State ─────────────────────────────────────────────
float tdsValue = 0.0f;      // PPM mein TDS reading
bool tdsAlertSent = false;  // water_quality_alert event ek baar hi bheje
bool tdsSevereSent = false; // water_quality_severe event ek baar hi bheje

// ─── Water Level State ─────────────────────────────────────
bool waterLevelReached = false;
bool prevWaterLevelState = false;

// ─── Anomaly (Pipe Burst) State ────────────────────────────
bool anomalyDetected = false; // pipe burst flag

// ─── Non-blocking Buzzer (pattern-based) ───────────────────
// Even steps = HIGH (buzzer ON), Odd steps = LOW (buzzer OFF)
bool buzzerActive = false;
uint8_t buzzerStep = 0;
uint32_t buzzerNextMs = 0;
uint8_t buzzerPatternLen = 0;
uint16_t buzzerPatternBuf[10]; // max 10 steps ka pattern

// ─── Constants ─────────────────────────────────────────────
const bool RELAY_ACTIVE_LOW = true;
const float FLOW_LPM_THRESHOLD = 0.05f;
const float LEAK_DIFF_LPM_THRESHOLD = 0.70f;
const float PULSES_PER_LITER = 450.0f;
const uint32_t SAMPLE_INTERVAL_MS = 2000;
const uint32_t MIN_PULSE_GAP_US = 1500;
const uint32_t MIN_VALID_PULSES = 2;

// Motor startup grace period — pipe fill hone mein time lagta hai
const uint32_t MOTOR_STARTUP_GRACE_MS = 10000; // 10 seconds

// Leak confirmation — 3 lagaataar suspicious readings ke baad hi leak confirm
const uint8_t LEAK_CONFIRM_READINGS = 3;
uint8_t leakConsecutiveCount = 0;

// TDS sensor constants
const uint8_t TDS_SAMPLE_COUNT = 20;  // analog reads ka average
const float TDS_VREF = 3.3f;          // ESP32 ADC reference voltage
const float TDS_TEMPERATURE = 25.0f;  // default temperature (°C)
const float TDS_WARN_PPM = 500.0f;    // water_quality_alert threshold
const float TDS_SEVERE_PPM = 1000.0f; // water_quality_severe threshold

// Anomaly (pipe burst) — agar diff itna bada ho toh pipe burst maano
const float ANOMALY_DIFF_LPM = 3.0f;

// Water level — sensor HIGH = water reached (adjust agar sensor inverted hai)
const bool WATER_LEVEL_ACTIVE_HIGH = true;

// ─── ISRs ──────────────────────────────────────────────────
void IRAM_ATTR flowSensor1ISR() {
  uint32_t nowUs = micros();
  if ((nowUs - lastPulseUs1) >= MIN_PULSE_GAP_US) {
    flowPulses1++;
    lastPulseUs1 = nowUs;
  }
}

void IRAM_ATTR flowSensor2ISR() {
  uint32_t nowUs = micros();
  if ((nowUs - lastPulseUs2) >= MIN_PULSE_GAP_US) {
    flowPulses2++;
    lastPulseUs2 = nowUs;
  }
}

// ─── Motor Control ─────────────────────────────────────────
void setMotor(bool turnOn) {
  // Agar motor OFF tha aur ab ON ho raha → grace period start
  if (turnOn && !motorOn) {
    motorStartMs = millis();
    leakConsecutiveCount = 0;
    Serial.println(
        "Motor ON — 10s startup grace period started (leak detection paused).");
  }
  motorOn = turnOn;
  bool relayLevel = RELAY_ACTIVE_LOW ? !turnOn : turnOn;
  digitalWrite(RELAY_PIN, relayLevel ? HIGH : LOW);
}

// ─── Blynk: Motor command from app ─────────────────────────
BLYNK_WRITE(VPIN_MOTOR_COMMAND) { motorCommandOn = param.asInt() == 1; }

// Manual leak reset — app me V8 Button (Push mode) se trigger hoga
BLYNK_WRITE(VPIN_LEAK_RESET) {
  if (param.asInt() == 1 && leakLatch) {
    leakLatch = false;
    leakDetected = false;
    leakConsecutiveCount = 0;
    motorCommandOn = true;
    Serial.println("Leak latch manually reset via app.");

    // Immediately motor ON karo — agar water level allow kare
    if (!waterLevelReached) {
      setMotor(true);
      Serial.println("Motor restarted after leak reset.");
    }

    if (Blynk.connected()) {
      Blynk.virtualWrite(VPIN_LEAKAGE_STATUS, "Normal");
      Blynk.virtualWrite(VPIN_MOTOR_STATE, motorOn ? "ON" : "OFF");
      Blynk.virtualWrite(VPIN_MOTOR_COMMAND, 1);
      Blynk.virtualWrite(VPIN_STATUS_MESSAGE, "Reset! Monitoring...");
    }
  }
}

// Blynk reconnect hone par full state sync karo
BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_MOTOR_COMMAND);
  // Boot/reconnect pe Blynk UI reset karo — stale values hata
  Blynk.virtualWrite(VPIN_LEAKAGE_STATUS, "Normal");
  Blynk.virtualWrite(VPIN_MOTOR_STATE, motorOn ? "ON" : "OFF");
  Blynk.virtualWrite(VPIN_WATER_LEVEL, "Not Reached");
  Blynk.virtualWrite(VPIN_STATUS_MESSAGE, "Connected");
  Serial.println("Blynk connected — UI state synced.");
}

// ─── Non-blocking Buzzer (pattern-based) ───────────────────
// buzzerTick() har loop iteration me call hota hai
void buzzerTick() {
  if (!buzzerActive)
    return;
  if (millis() < buzzerNextMs)
    return;

  if (buzzerStep >= buzzerPatternLen) {
    // Pattern complete
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    buzzerStep = 0;
    Serial.println("Buzzer pattern complete.");
    return;
  }

  bool pinHigh = (buzzerStep % 2 == 0); // even = ON, odd = OFF
  digitalWrite(BUZZER_PIN, pinHigh ? HIGH : LOW);
  buzzerNextMs = millis() + buzzerPatternBuf[buzzerStep];
  buzzerStep++;
}

// ─── Leak Alarm: 5 second continuous beep ──────────────────
void startBuzzerLeak() {
  buzzerActive = true;
  buzzerStep = 0;
  buzzerNextMs = millis();
  buzzerPatternLen = 1;
  buzzerPatternBuf[0] = 5000; // 5 sec ON
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("!!! BUZZER: LEAK ALARM — 5 sec continuous !!!");
}

// ─── Water Level Alarm: short-short-long pattern ───────────
// Pattern: beep 200ms, pause 150ms, beep 200ms, pause 150ms, beep 500ms
void startBuzzerWaterLevel() {
  if (buzzerActive)
    return; // don't interrupt active alarm
  buzzerActive = true;
  buzzerStep = 0;
  buzzerNextMs = millis();
  buzzerPatternLen = 5;
  buzzerPatternBuf[0] = 200; // ON  (short beep)
  buzzerPatternBuf[1] = 150; // OFF (pause)
  buzzerPatternBuf[2] = 200; // ON  (short beep)
  buzzerPatternBuf[3] = 150; // OFF (pause)
  buzzerPatternBuf[4] = 500; // ON  (long beep)
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("BUZZER: Water level reached — short-short-long pattern.");
}

// ─── TDS Reading ───────────────────────────────────────────
// Multiple analog reads ka average → voltage → PPM conversion
float readTDS() {
  long analogSum = 0;
  for (uint8_t i = 0; i < TDS_SAMPLE_COUNT; i++) {
    analogSum += analogRead(TDS_SENSOR_PIN);
  }
  float avgAnalog = (float)analogSum / TDS_SAMPLE_COUNT;

  // ESP32 ADC: 12-bit (0-4095), 0-3.3V
  float voltage = avgAnalog * TDS_VREF / 4095.0f;

  // Temperature compensation (25°C default)
  float compensated = voltage / (1.0f + 0.02f * (TDS_TEMPERATURE - 25.0f));

  // Standard TDS conversion formula
  float ppm = (133.42f * compensated * compensated * compensated -
               255.86f * compensated * compensated + 857.39f * compensated) *
              0.5f;

  if (ppm < 0.0f)
    ppm = 0.0f;
  return ppm;
}

// ─── Water Level Check ─────────────────────────────────────
// Returns true when water has reached the sensor
bool checkWaterLevel() {
  bool sensorState = digitalRead(WATER_LEVEL_PIN);
  // Agar sensor inverted hai toh WATER_LEVEL_ACTIVE_HIGH ko false karo
  return WATER_LEVEL_ACTIVE_HIGH ? (sensorState == HIGH) : (sensorState == LOW);
}

// ─── Main Sensor + Control Loop ────────────────────────────
void publishReadings() {
  // ─── Flow Rate Calculation ─────────────────────────────
  uint32_t pulses1, pulses2;
  noInterrupts();
  pulses1 = flowPulses1;
  pulses2 = flowPulses2;
  flowPulses1 = 0;
  flowPulses2 = 0;
  interrupts();

  float intervalSeconds = SAMPLE_INTERVAL_MS / 1000.0f;
  flowRate1 = (pulses1 / intervalSeconds) / 7.5f;
  flowRate2 = (pulses2 / intervalSeconds) / 7.5f;

  if (pulses1 < MIN_VALID_PULSES)
    flowRate1 = 0.0f;
  if (pulses2 < MIN_VALID_PULSES)
    flowRate2 = 0.0f;

  uint32_t validPulses1 = (pulses1 >= MIN_VALID_PULSES) ? pulses1 : 0;
  uint32_t validPulses2 = (pulses2 >= MIN_VALID_PULSES) ? pulses2 : 0;
  totalLiters1 += validPulses1 / PULSES_PER_LITER;
  totalLiters2 += validPulses2 / PULSES_PER_LITER;

  // ─── TDS Reading ───────────────────────────────────────
  tdsValue = readTDS();

  // ─── TDS Quality Alerts ────────────────────────────────
  if (tdsValue >= TDS_SEVERE_PPM && !tdsSevereSent) {
    tdsSevereSent = true;
    tdsAlertSent = true;
    Serial.println("!!! TDS SEVERE — water_quality_severe event !!!");
    if (Blynk.connected())
      Blynk.logEvent("water_quality_severe");
  } else if (tdsValue >= TDS_WARN_PPM && !tdsAlertSent) {
    tdsAlertSent = true;
    Serial.println("!! TDS WARNING — water_quality_alert event !!");
    if (Blynk.connected())
      Blynk.logEvent("water_quality_alert");
  } else if (tdsValue < TDS_WARN_PPM) {
    // Normal range — flags reset toh next spike pe fir event jaye
    tdsAlertSent = false;
    tdsSevereSent = false;
  }

  // ─── Water Level Check ─────────────────────────────────
  bool currentWaterLevel = checkWaterLevel();

  // Edge detection — sirf pehli baar reach hone pe buzzer + event
  if (currentWaterLevel && !prevWaterLevelState) {
    waterLevelReached = true;
    startBuzzerWaterLevel();
    Serial.println(">>> WATER LEVEL REACHED — Motor OFF <<<");
    if (Blynk.connected())
      Blynk.logEvent("level_reached");
  }
  // Agar paani neeche gaya → motor fir se chal sakta hai
  if (!currentWaterLevel && prevWaterLevelState) {
    waterLevelReached = false;
    Serial.println("Water level dropped below sensor — Motor can resume.");
  }
  prevWaterLevelState = currentWaterLevel;

  // ─── Leak Detection Logic ──────────────────────────────
  bool inStartupGrace =
      motorOn && ((millis() - motorStartMs) < MOTOR_STARTUP_GRACE_MS);

  bool inletFlowDetected = flowRate1 > FLOW_LPM_THRESHOLD;
  float flowDiff = flowRate1 - flowRate2;
  bool leakCondition =
      inletFlowDetected && (flowDiff > LEAK_DIFF_LPM_THRESHOLD);

  if (inStartupGrace) {
    uint32_t remainMs = MOTOR_STARTUP_GRACE_MS - (millis() - motorStartMs);
    Serial.print("Startup grace active — leak detection paused (");
    Serial.print(remainMs / 1000);
    Serial.println("s remaining)");
    leakConsecutiveCount = 0;

  } else if (leakCondition) {
    leakConsecutiveCount++;

    Serial.print("Suspicious reading #");
    Serial.print(leakConsecutiveCount);
    Serial.print(" / ");
    Serial.print(LEAK_CONFIRM_READINGS);
    Serial.print("  |  diff = ");
    Serial.print(flowDiff, 2);
    Serial.println(" L/min");

    if (leakConsecutiveCount >= LEAK_CONFIRM_READINGS && !leakDetected) {
      leakDetected = true;
      leakLatch = true;
      Serial.println(
          "!!! LEAK CONFIRMED — Motor latched OFF. Reset via app. !!!");
      startBuzzerLeak();

      // Anomaly check — agar diff bahut zyada hai toh pipe burst
      if (flowDiff >= ANOMALY_DIFF_LPM) {
        anomalyDetected = true;
        Serial.println("!!! ANOMALY (PIPE BURST) DETECTED !!!");
        if (Blynk.connected())
          Blynk.logEvent("anomaly_detected");
      } else {
        if (Blynk.connected())
          Blynk.logEvent("leak_detected");
      }
    }

  } else {
    if (leakConsecutiveCount > 0) {
      Serial.print("Counter reset (was ");
      Serial.print(leakConsecutiveCount);
      Serial.println(") — transient condition, not a leak.");
    }
    leakConsecutiveCount = 0;
    leakDetected = false;

    if (leakLatch) {
      Serial.println("Leak condition cleared but latch active — motor stays "
                     "OFF until manual reset.");
    }
  }

  // ─── Motor Decision ────────────────────────────────────
  // Motor ON sirf tabhi: command ON, no leak, no latch, water level NOT reached
  bool shouldRunMotor =
      motorCommandOn && !leakDetected && !leakLatch && !waterLevelReached;
  setMotor(shouldRunMotor);

  // ─── Serial Debug ──────────────────────────────────────
  Serial.println("──────────────────────────────");
  Serial.print("Flow 1 Inlet  (L/min): ");
  Serial.println(flowRate1, 2);
  Serial.print("Flow 1 Total  (L):     ");
  Serial.println(totalLiters1, 3);
  Serial.print("Flow 2 Outlet (L/min): ");
  Serial.println(flowRate2, 2);
  Serial.print("Flow 2 Total  (L):     ");
  Serial.println(totalLiters2, 3);
  Serial.print("TDS Value     (PPM):   ");
  Serial.println(tdsValue, 1);
  Serial.print("Water Level:           ");
  Serial.println(waterLevelReached ? "REACHED" : "NOT REACHED");
  Serial.print("Motor State:           ");
  Serial.println(motorOn ? "ON" : "OFF");
  Serial.print("Leakage:               ");
  Serial.println(leakDetected ? "DETECTED" : "NORMAL");
  Serial.print("Consec. count:         ");
  Serial.print(leakConsecutiveCount);
  Serial.print(" / ");
  Serial.println(LEAK_CONFIRM_READINGS);

  // ─── Blynk Update ──────────────────────────────────────
  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_FLOW_RATE_1, flowRate1);
    Blynk.virtualWrite(VPIN_FLOW_RATE_2, flowRate2);
    Blynk.virtualWrite(VPIN_TDS_VALUE, tdsValue);
    Blynk.virtualWrite(VPIN_WATER_LEVEL,
                       waterLevelReached ? "Reached" : "Not Reached");
    Blynk.virtualWrite(VPIN_MOTOR_COMMAND, motorCommandOn ? 1 : 0);
    Blynk.virtualWrite(VPIN_MOTOR_STATE, motorOn ? "ON" : "OFF");
    Blynk.virtualWrite(VPIN_LEAKAGE_STATUS,
                       (leakDetected || leakLatch) ? "Detected" : "Normal");

    // Status message — priority: leak > water level > normal
    const char *statusMsg;
    if (leakDetected)
      statusMsg = "LEAK! Motor OFF";
    else if (leakLatch)
      statusMsg = "LATCHED! Press Reset";
    else if (waterLevelReached)
      statusMsg = "TANK FULL: Motor OFF";
    else if (motorOn)
      statusMsg = "Motor ON: Normal";
    else
      statusMsg = "Motor OFF: Manual";

    Blynk.virtualWrite(VPIN_STATUS_MESSAGE, statusMsg);
  }
}

// ─── Setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Boot OK"); Serial.flush();

  pinMode(FLOW_SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2_PIN, INPUT_PULLUP);
  pinMode(TDS_SENSOR_PIN, INPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("[1] Pins"); Serial.flush();

  // Brownout detector OFF — WiFi RF power-up pe voltage dip se reset hota tha
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.println("[1b] Brownout disabled"); Serial.flush();

  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("[2] Buzzer"); Serial.flush();

  // Power settle karne do buzzer ke baad — WiFi RF init heavy current draw karta hai
  delay(500);

  // Stale WiFi state clear karo before fresh init
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  Serial.println("[3] WiFi mode"); Serial.flush();

  WiFi.begin(ssid, pass);
  Serial.println("[4] WiFi begin"); Serial.flush();

  Blynk.config(auth);
  Serial.println("[5] Blynk cfg"); Serial.flush();

  // WiFi connect hone ka wait — max 10 seconds, non-blocking style
  Serial.print("Waiting for WiFi");
  uint32_t wifiWaitStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiWaitStart) < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println(" Timeout — will retry in loop.");
  }
  Serial.flush();

  // Initial Blynk connection attempt — agar WiFi mila toh
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connecting to Blynk...");
    Blynk.connect(3000); // 3 sec timeout for initial connection
    if (Blynk.connected()) {
      Serial.println("Blynk connected!");
    } else {
      Serial.println("Blynk initial connect failed — will retry in loop.");
    }
  }
  Serial.flush();

  // Ab ISR attach karo — WiFi stable hone ke baad safe hai
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_1_PIN), flowSensor1ISR, FALLING);
  Serial.println("[6] ISR1"); Serial.flush();

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_2_PIN), flowSensor2ISR, FALLING);
  Serial.println("[7] ISR2"); Serial.flush();

  timer.setInterval(SAMPLE_INTERVAL_MS, publishReadings);
  Serial.println("[8] Ready!"); Serial.flush();
}

// ─── Loop ──────────────────────────────────────────────
void loop() {
  buzzerTick();

  // Reconnect throttle: har 5 seconds me ek baar check
  static uint32_t lastReconnectMs = 0;
  if (millis() - lastReconnectMs >= 5000) {
    lastReconnectMs = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi reconnecting...");
      WiFi.begin(ssid, pass);
    } else if (!Blynk.connected()) {
      Serial.println("Blynk reconnecting...");
      Blynk.connect(2000); // 2 sec timeout — enough for handshake
    }
  }

  // Blynk.run() SIRF connected hone pe — warna WDT crash hota tha
  if (Blynk.connected()) {
    Blynk.run();
  }
  timer.run();
}
