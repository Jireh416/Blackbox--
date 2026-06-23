/*
 * Project: 15³ Emotional Black Box
 * Architecture: Arduino UNO Q (STM32U585 Cortex-M33, Zephyr RTOS platform)
 * Features: MPR121 (Touch), PCA9685 (Servos), DFPlayer Mini (Audio), FastLED (WS2812B), Atomizer, Fan, Vib Motor
 */

#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_PWMServoDriver.h>
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>

// --- PIN DEFINITIONS ---
// DFPlayer Mini uses hardware Serial1 (D0=RX, D1=TX on UNO Q)
// Connect DFPlayer TX -> Arduino D0(RX1), DFPlayer RX -> 1K resistor -> Arduino D1(TX1)
#define LED_PIN 4       // WS2812B DIN
#define VIB_MOTOR_PIN 5 // Vibration motor (Require MOSFET/Transistor for safety)
#define FAN_PIN 6       // 3cm Fan (Require MOSFET if 12V or high current)
#define ATOMIZER_PIN 7  // Ultrasonic Atomizer EN/SIG

// --- HARDWARE CONFIG ---
#define NUM_LEDS 30     // Adjust based on actual LED strip length
#define SERVO_MIN 150   // PCA9685 Min pulse length (0 deg)
#define SERVO_MAX 600   // PCA9685 Max pulse length (180 deg)

// --- STATE MACHINE ---
enum State {
  PHASE_0_DORMANT,
  PHASE_1_AWAKENING,
  PHASE_2_AGITATION,
  PHASE_3_CLIMAX,
  PHASE_4_EMPTINESS
};
State currentState = PHASE_0_DORMANT;
State lastState = PHASE_0_DORMANT;

// --- HABITUATION VARS ---
float excitement = 0.0;
const float MAX_EXCITEMENT = 100.0;
unsigned long lastTouchTime = 0;
uint16_t lastTouchedPads = 0;
float touchDiversityScore = 0;
unsigned long stateTimer = 0;
unsigned long lastHeartbeatTime = 0;

// --- INSTANCES ---
DFRobotDFPlayerMini myDFPlayer;
Adafruit_MPR121 cap = Adafruit_MPR121();
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
CRGB leds[NUM_LEDS];

// --- AUDIO TRACK MAPPING (SD Card 01~05.mp3) ---
#define TRK_HEARTBEAT 1
#define TRK_AWAKEN 2
#define TRK_AGITATION 3
#define TRK_CLIMAX 4
#define TRK_AFTERMATH 5

void setServoAngle(uint8_t n, double angle) {
  // Safe angle constraint (0 to 60) to prevent mechanical damage
  angle = constrain(angle, 0, 60);
  double pulse = map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(n, 0, pulse);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // 1. Initialize Pins
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(ATOMIZER_PIN, OUTPUT);
  digitalWrite(VIB_MOTOR_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(ATOMIZER_PIN, LOW);

  // 2. Initialize FastLED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); // Set to moderate brightness to protect from current draw
  FastLED.clear();
  FastLED.show();

  // 3. Initialize MPR121 (Touch)
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check I2C wiring.");
  }
  
  // 4. Initialize PCA9685 (Servo)
  pwm.begin();
  pwm.setPWMFreq(50); // 50Hz for standard servos
  setServoAngle(0, 0); // Left cover servo
  setServoAngle(1, 0); // Right cover servo

  // 5. Initialize DFPlayer Mini (via hardware Serial1)
  Serial1.begin(9600);
  if (!myDFPlayer.begin(Serial1)) {
    Serial.println("DFPlayer error, check wiring or SD card.");
  } else {
    myDFPlayer.volume(20);  // Volume 0~30
  }
  
  Serial.println("System Initialized. Entering Phase 0.");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read touch input
  uint16_t touched = cap.touched();
  
  // --- HABITUATION ALGORITHM ---
  // Allow excitement build only outside of Climax and Cooldown
  if (currentState != PHASE_3_CLIMAX && currentState != PHASE_4_EMPTINESS) {
    if (touched) {
      // Calculate diversity: Check if touched pads are different from last frame
      uint16_t changes = touched ^ lastTouchedPads;
      if (changes > 0) {
        touchDiversityScore = (touchDiversityScore + 2.0f < 15.0f) ? touchDiversityScore + 2.0f : 15.0f; // Reward dynamic touch
      } else {
        touchDiversityScore = (touchDiversityScore - 0.5f > 0.0f) ? touchDiversityScore - 0.5f : 0.0f;  // Penalize static hold (fatigue)
      }
      
      // Calculate Excitement Increment
      float increment = 0.05f + (touchDiversityScore * 0.05f);
      if (touchDiversityScore < 1.0f) increment = 0.01f; // Fatigue state: very slow growth
      
      excitement = (excitement + increment < MAX_EXCITEMENT) ? excitement + increment : MAX_EXCITEMENT;
      lastTouchTime = currentMillis;
    } else {
      // Decay over time if not touched
      if (currentMillis - lastTouchTime > 3000) {
        excitement = (excitement - 0.1f > 0.0f) ? excitement - 0.1f : 0.0f; // Slow decay
      }
      touchDiversityScore = (touchDiversityScore - 0.1f > 0.0f) ? touchDiversityScore - 0.1f : 0.0f;
    }
    lastTouchedPads = touched;
  }

  // --- STATE TRANSITION LOGIC ---
  switch (currentState) {
    case PHASE_0_DORMANT:
      if (excitement > 5.0) {
        currentState = PHASE_1_AWAKENING;
        Serial.println("-> PHASE 1: Awakening");
      }
      break;
      
    case PHASE_1_AWAKENING:
      if (excitement > 40.0) {
        currentState = PHASE_2_AGITATION;
        Serial.println("-> PHASE 2: Agitation");
      } else if (excitement <= 0.0) {
        currentState = PHASE_0_DORMANT;
        Serial.println("-> PHASE 0: Dormant");
      }
      break;
      
    case PHASE_2_AGITATION:
      if (excitement >= MAX_EXCITEMENT) {
        currentState = PHASE_3_CLIMAX;
        stateTimer = currentMillis;
        Serial.println("-> PHASE 3: Climax");
      } else if (excitement < 15.0) {
        currentState = PHASE_1_AWAKENING;
        Serial.println("-> PHASE 1: Awakening");
      }
      break;
      
    case PHASE_3_CLIMAX:
      // Climax lasts for 8 seconds
      if (currentMillis - stateTimer > 8000) { 
        currentState = PHASE_4_EMPTINESS;
        stateTimer = currentMillis;
        excitement = 0; // Reset excitement abruptly
        Serial.println("-> PHASE 4: Emptiness");
      }
      break;
      
    case PHASE_4_EMPTINESS:
      // Cooldown & Atomizer lasts for 15 seconds
      if (currentMillis - stateTimer > 15000) {
        currentState = PHASE_0_DORMANT;
        Serial.println("-> PHASE 0: Dormant");
      }
      break;
  }

  // Audio State change trigger (Play tracks once per state transition)
  if (currentState != lastState) {
    switch (currentState) {
      case PHASE_1_AWAKENING: myDFPlayer.loop(TRK_AWAKEN); break;
      case PHASE_2_AGITATION: myDFPlayer.loop(TRK_AGITATION); break;
      case PHASE_3_CLIMAX:    myDFPlayer.loop(TRK_CLIMAX); break;
      case PHASE_4_EMPTINESS: myDFPlayer.loop(TRK_AFTERMATH); break;
      case PHASE_0_DORMANT:   myDFPlayer.pause(); break; // Heartbeat handled manually
    }
    lastState = currentState;
  }

  // --- OUTPUT EXECUTION (Physical Feedback) ---
  executeStateOutputs(currentMillis);
  
  delay(20); // 50Hz loop rate
}

void executeStateOutputs(unsigned long timeMs) {
  switch (currentState) {
    case PHASE_0_DORMANT: {
      // 1. Kinetic: Closed
      setServoAngle(0, 0);
      setServoAngle(1, 0);
      
      // 2. Light: Faint, slow dark halo (breathe every 10s)
      uint8_t bright = (sin(timeMs / 1500.0) > 0.95) ? 10 : 0;
      fill_solid(leds, NUM_LEDS, CRGB(bright, 0, 0));
      FastLED.show();
      
      // 3. Sound: Deep heartbeat every 10s
      if (timeMs - lastHeartbeatTime > 10000) {
        myDFPlayer.play(TRK_HEARTBEAT);
        lastHeartbeatTime = timeMs;
      }
      
      // 4. Peripherals: OFF
      analogWrite(VIB_MOTOR_PIN, 0);
      digitalWrite(FAN_PIN, LOW);
      digitalWrite(ATOMIZER_PIN, LOW);
      break;
    }
    
    case PHASE_1_AWAKENING: {
      // 1. Kinetic: 1mm slit (approx 3 degrees)
      setServoAngle(0, 3);
      setServoAngle(1, 3);
      
      // 2. Light: Soft dark red breath
      uint8_t breath = (sin(timeMs / 800.0) + 1.0) * 15.0; // 0 to 30
      fill_solid(leds, NUM_LEDS, CRGB(breath + 5, 0, 0));
      FastLED.show();
      
      // 3. Peripherals: OFF
      analogWrite(VIB_MOTOR_PIN, 0);
      digitalWrite(FAN_PIN, LOW);
      digitalWrite(ATOMIZER_PIN, LOW);
      break;
    }
    
    case PHASE_2_AGITATION: {
      // 1. Kinetic: Irregular flutter based on excitement
      double flutter = 5 + (sin(timeMs / 200.0) * (excitement / 10.0));
      setServoAngle(0, flutter);
      setServoAngle(1, flutter);
      
      // 2. Light: Warm orange-red, breathing gets faster and brighter
      float freq = (400.0f - (excitement * 2.0f) > 100.0f) ? 400.0f - (excitement * 2.0f) : 100.0f;
      uint8_t breath = (sin(timeMs / freq) + 1.0) * (20.0 + excitement/2.0); 
      fill_solid(leds, NUM_LEDS, CRGB(breath + 20, breath / 4, 0));
      FastLED.show();
      
      // 3. Vibration: Gentle tingling
      int vibPower = 30 + (sin(timeMs / 100.0) * 15);
      analogWrite(VIB_MOTOR_PIN, vibPower);
      
      // 4. Peripherals
      digitalWrite(FAN_PIN, LOW);
      digitalWrite(ATOMIZER_PIN, LOW);
      break;
    }
    
    case PHASE_3_CLIMAX: {
      // 1. Kinetic: Fully open
      setServoAngle(0, 45); 
      setServoAngle(1, 45);
      
      // 2. Light: Saturated deep red/amber pulsing rapidly
      uint8_t pulse = (sin(timeMs / 50.0) + 1.0) * 127.0; // 0 to 254
      fill_solid(leds, NUM_LEDS, CRGB(255, pulse/3, 0));
      FastLED.show();
      
      // 3. Vibration: Continuous resonance
      analogWrite(VIB_MOTOR_PIN, 180); // 70% duty cycle for safety
      
      // 4. Peripherals
      digitalWrite(FAN_PIN, LOW);
      digitalWrite(ATOMIZER_PIN, LOW);
      break;
    }
    
    case PHASE_4_EMPTINESS: {
      // 1. Kinetic: Dropped down, 2 degree slit
      setServoAngle(0, 2);
      setServoAngle(1, 2);
      
      // 2. Light: Faint cold blue
      fill_solid(leds, NUM_LEDS, CRGB(0, 10, 40));
      FastLED.show();
      
      // 3. Vibration: Aftershock / exhaustion tremors
      if (timeMs % 3000 > 2800) {
        analogWrite(VIB_MOTOR_PIN, 60);
      } else {
        analogWrite(VIB_MOTOR_PIN, 0);
      }
      
      // 4. Peripherals: Mist & Fan ON!
      digitalWrite(ATOMIZER_PIN, HIGH);
      digitalWrite(FAN_PIN, HIGH);
      break;
    }
  }
}
