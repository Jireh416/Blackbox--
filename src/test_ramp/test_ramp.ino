/*
 * 15³ Emotional Black Box - Hardware Integration Test
 * 測試項目：觸控 → 伺服馬達漸升 + 震動馬達漸強 → 頂點 → 漸降歸零
 * 
 * 接線摘要：
 *   MPR121   : SDA=A4, SCL=A5, VCC=5V, GND=GND (I2C addr 0x5A)
 *   PCA9685  : SDA=A4, SCL=A5 (並聯), V+=5V, GND=GND
 *              Servo CH0 & CH1 (棕=GND, 紅=V+, 橘=PWM)
 *   震動馬達 : SIG=D5(PWM), VCC=5V, GND=GND
 */

#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_PWMServoDriver.h>

// --- PIN ---
#define VIB_MOTOR_PIN 5  // Grove 震動馬達 SIG (PWM)

// --- SERVO CONFIG ---
#define SERVO_MIN 150    // PCA9685 pulse: 0 度
#define SERVO_MAX 600    // PCA9685 pulse: 180 度
#define SERVO_MAX_ANGLE 45.0  // 安全上限角度 (防止頂蓋過推)

// --- RAMP CONFIG ---
#define RAMP_STEP_MS 30       // 每步間隔 (ms)，越小越滑順
#define RAMP_STEPS   60       // 總共幾步到頂點
#define HOLD_AT_PEAK_MS 2000  // 頂點維持時間 (ms)

// --- INSTANCES ---
Adafruit_MPR121 cap = Adafruit_MPR121();
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- STATE ---
bool isRunning = false;

void setServoAngle(uint8_t channel, float angle) {
  angle = constrain(angle, 0, SERVO_MAX_ANGLE);
  uint16_t pulse = (uint16_t)map((long)(angle * 10), 0, 1800, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(channel, 0, pulse);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // 震動馬達
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  analogWrite(VIB_MOTOR_PIN, 0);

  // MPR121
  if (!cap.begin(0x5A)) {
    Serial.println("[ERROR] MPR121 not found! Check I2C wiring.");
    while (1); // 停在這裡，不繼續
  }
  Serial.println("[OK] MPR121 initialized.");

  // PCA9685
  pwm.begin();
  pwm.setPWMFreq(50);
  setServoAngle(0, 0);
  setServoAngle(1, 0);
  Serial.println("[OK] PCA9685 initialized. Servos at 0 deg.");

  Serial.println("=====================================");
  Serial.println(" Touch Test Ready!");
  Serial.println(" Touch any pad to start ramp cycle.");
  Serial.println("=====================================");
}

void loop() {
  uint16_t touched = cap.touched();

  // 偵測任意觸控點被觸碰 & 目前沒有在執行動作
  if (touched && !isRunning) {
    Serial.print("[TOUCH] Pads detected: 0b");
    Serial.println(touched, BIN);
    runRampCycle();
  }

  delay(50);
}

void runRampCycle() {
  isRunning = true;

  // ========== Phase 1: 漸升 (Ramp Up) ==========
  Serial.println("[RAMP UP] Starting...");
  for (int step = 0; step <= RAMP_STEPS; step++) {
    float progress = (float)step / (float)RAMP_STEPS; // 0.0 ~ 1.0

    // 伺服馬達：線性漸升到最大角度
    float angle = SERVO_MAX_ANGLE * progress;
    setServoAngle(0, angle);
    setServoAngle(1, angle);

    // 震動馬達：PWM 從 0 漸升到 200 (約 78% duty cycle，安全值)
    int vibPower = (int)(200.0f * progress);
    analogWrite(VIB_MOTOR_PIN, vibPower);

    // 每 15 步列印一次進度
    if (step % 15 == 0) {
      Serial.print("  Angle: ");
      Serial.print(angle, 1);
      Serial.print(" deg | Vib: ");
      Serial.println(vibPower);
    }

    delay(RAMP_STEP_MS);
  }

  // ========== Phase 2: 頂點維持 (Hold at Peak) ==========
  Serial.print("[PEAK] Holding for ");
  Serial.print(HOLD_AT_PEAK_MS);
  Serial.println(" ms...");
  delay(HOLD_AT_PEAK_MS);

  // ========== Phase 3: 漸降 (Ramp Down) ==========
  Serial.println("[RAMP DOWN] Starting...");
  for (int step = RAMP_STEPS; step >= 0; step--) {
    float progress = (float)step / (float)RAMP_STEPS;

    float angle = SERVO_MAX_ANGLE * progress;
    setServoAngle(0, angle);
    setServoAngle(1, angle);

    int vibPower = (int)(200.0f * progress);
    analogWrite(VIB_MOTOR_PIN, vibPower);

    if (step % 15 == 0) {
      Serial.print("  Angle: ");
      Serial.print(angle, 1);
      Serial.print(" deg | Vib: ");
      Serial.println(vibPower);
    }

    delay(RAMP_STEP_MS);
  }

  // ========== 完全歸零 ==========
  setServoAngle(0, 0);
  setServoAngle(1, 0);
  analogWrite(VIB_MOTOR_PIN, 0);

  Serial.println("[DONE] Cycle complete. Touch again to repeat.");
  Serial.println("-------------------------------------");

  // 等 500ms 防止連續觸發
  delay(500);
  isRunning = false;
}
