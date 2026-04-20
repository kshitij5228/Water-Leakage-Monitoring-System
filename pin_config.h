#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ─── Hardware Pin Configuration ────────────────────────────
#define FLOW_SENSOR_1_PIN 27
#define FLOW_SENSOR_2_PIN 26
#define RELAY_PIN         25
#define TDS_SENSOR_PIN    34
#define WATER_LEVEL_PIN   33
#define BUZZER_PIN        32

// ─── Blynk Virtual Pin Configuration ──────────────────────
#define VPIN_FLOW_RATE_1    V0
#define VPIN_FLOW_RATE_2    V1
#define VPIN_TDS_VALUE      V2
#define VPIN_LEAKAGE_STATUS V3
#define VPIN_STATUS_MESSAGE V4
#define VPIN_MOTOR_COMMAND  V5
#define VPIN_MOTOR_STATE    V6
#define VPIN_WATER_LEVEL    V7
#define VPIN_LEAK_RESET     V8

#endif // PIN_CONFIG_H
