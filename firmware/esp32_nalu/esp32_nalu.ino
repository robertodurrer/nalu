#include <Arduino.h>
#include <ArduinoJson.h>

#define MOTOR_LEFT_PWM    18
#define MOTOR_LEFT_DIR    19
#define MOTOR_RIGHT_PWM   22
#define MOTOR_RIGHT_DIR   23
#define BATTERY_PIN       34
#define ESTOP_PIN         25

#define SERIAL_BAUD           115200
#define TELEMETRY_INTERVAL_MS  50
#define CMD_TIMEOUT_MS        500
#define MAX_PWM               255

float g_linear = 0.0f, g_angular = 0.0f;
int g_pwm_left = 0, g_pwm_right = 0;
bool g_estop = false;
unsigned long g_last_cmd_ms = 0;
String g_serial_buffer = "";

void stopMotors() {
  analogWrite(MOTOR_LEFT_PWM, 0);
  analogWrite(MOTOR_RIGHT_PWM, 0);
  g_pwm_left = g_pwm_right = 0;
}

void setMotor(int pwm_pin, int dir_pin, int pwm_value) {
  digitalWrite(dir_pin, pwm_value >= 0 ? HIGH : LOW);
  analogWrite(pwm_pin, constrain(abs(pwm_value), 0, MAX_PWM));
}

void applyVelocity(float linear, float angular) {
  float v_left  = linear - angular * 0.10f;
  float v_right = linear + angular * 0.10f;
  float max_v = max(abs(v_left), abs(v_right));
  if (max_v > 1.0f) { v_left /= max_v; v_right /= max_v; }
  g_pwm_left  = (int)(v_left  * MAX_PWM);
  g_pwm_right = (int)(v_right * MAX_PWM);
  setMotor(MOTOR_LEFT_PWM,  MOTOR_LEFT_DIR,  g_pwm_left);
  setMotor(MOTOR_RIGHT_PWM, MOTOR_RIGHT_DIR, g_pwm_right);
}

void processCommand(const String & json_str) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, json_str)) return;
  if (strcmp(doc["type"] | "", "cmd") != 0) return;
  g_linear  = doc["linear"]  | 0.0f;
  g_angular = doc["angular"] | 0.0f;
  g_last_cmd_ms = millis();
  if (!g_estop) applyVelocity(g_linear, g_angular);
}

float readBatteryMv() {
  return (analogRead(BATTERY_PIN) / 4095.0f) * 3300.0f * 4.0f;
}

void sendTelemetry() {
  StaticJsonDocument<200> doc;
  doc["type"]           = "telemetry";
  doc["battery_mv"]     = (int)readBatteryMv();
  doc["emergency_stop"] = g_estop;
  doc["motor_left"]     = g_pwm_left;
  doc["motor_right"]    = g_pwm_right;
  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(MOTOR_LEFT_PWM,  OUTPUT);
  pinMode(MOTOR_LEFT_DIR,  OUTPUT);
  pinMode(MOTOR_RIGHT_PWM, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR, OUTPUT);
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  stopMotors();
  Serial.println("{\"type\":\"info\",\"msg\":\"Nalu ESP32 ready\"}");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') { processCommand(g_serial_buffer); g_serial_buffer = ""; }
    else g_serial_buffer += c;
  }
  if (millis() - g_last_cmd_ms > CMD_TIMEOUT_MS) {
    g_linear = g_angular = 0.0f;
    stopMotors();
  }
  g_estop = (digitalRead(ESTOP_PIN) == LOW);
  if (g_estop) stopMotors();
  static unsigned long last_tel = 0;
  if (millis() - last_tel >= TELEMETRY_INTERVAL_MS) {
    last_tel = millis();
    sendTelemetry();
  }
}
