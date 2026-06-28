/*
 * Sistem de Monitorizare Nivel Apă
 * Hardware: Arduino Nano, HW-038, Releu, OLED SSD1306 0.96", Buzzer, 3 Butoane
 * Autor: Proiect Facultate
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================== CONFIGURARE PINI ====================
#define SENSOR_PIN      A0    // Senzor nivel apă HW-038
#define RELAY_PIN       7     // Releu pompă
#define BUZZER_PIN      6     // Buzzer activ
#define BTN_MODE        2     // Buton MODE (INT0 - interrupt)
#define BTN_UP          4     // Buton UP (setpoint +)
#define BTN_DOWN        5     // Buton DOWN (setpoint -)

// ==================== CALIBRARE SENZOR ====================
// Modifică aceste valori după calibrarea senzorului tău!
#define SENSOR_DRY      50    // Valoare ADC când senzorul e uscat
#define SENSOR_WET      900   // Valoare ADC când senzorul e complet scufundat

// ==================== PARAMETRI CONTROL ====================
#define SP_LOW_DEFAULT  25    // % - pornire pompă (umplere)
#define SP_HIGH_DEFAULT 80    // % - oprire pompă (umplere)
#define LEVEL_CRITICAL_LOW  8   // % - alertă nivel prea mic
#define LEVEL_CRITICAL_HIGH 95  // % - alertă nivel prea mare

// ==================== OLED ====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== VARIABILE SISTEM ====================
int sensorRaw = 0;
float levelPercent = 0.0;
bool pumpOn = false;
bool modeAuto = true;        // true = AUTO, false = MANUAL
int setpointLow  = SP_LOW_DEFAULT;
int setpointHigh = SP_HIGH_DEFAULT;

// Timing
unsigned long lastSensorRead = 0;
unsigned long lastSerialSend  = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBuzzerToggle  = 0;
unsigned long lastBtnPress = 0;

const unsigned long SENSOR_INTERVAL  = 500;   // ms
const unsigned long SERIAL_INTERVAL  = 500;   // ms
const unsigned long DISPLAY_INTERVAL = 250;   // ms
const unsigned long DEBOUNCE_DELAY   = 200;   // ms

// Buzzer state
bool buzzerActive = false;
bool buzzerState  = false;
unsigned long buzzerOnTime  = 100;
unsigned long buzzerOffTime = 400;

// Mode button interrupt flag
volatile bool modePressed = false;

// Mediere senzor (8 mostre)
#define NUM_SAMPLES 8
int sensorSamples[NUM_SAMPLES];
int sampleIndex = 0;

// ==================== SETUP ====================
void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_MODE,   INPUT_PULLUP);
  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);

  digitalWrite(RELAY_PIN,  LOW);   // Pompă oprită la start
  digitalWrite(BUZZER_PIN, LOW);

  // Interrupt pentru butonul MODE (falling edge)
  attachInterrupt(digitalPinToInterrupt(BTN_MODE), isr_mode, FALLING);

  // Inițializare buffer mediere
  for (int i = 0; i < NUM_SAMPLES; i++) sensorSamples[i] = 0;

  // Inițializare OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    // Dacă OLED-ul nu e găsit, continuă fără el
    while (1); // sau comentează această linie dacă vrei să meargă fără display
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Ecran de start
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Water Level System");
  display.setCursor(30, 40);
  display.println("Initializing...");
  display.display();
  delay(1500);

  Serial.println("{\"status\":\"ready\"}");
}

// ==================== INTERRUPT MODE ====================
void isr_mode() {
  modePressed = true;
}

// ==================== CITIRE SENZOR CU MEDIERE ====================
float readLevelPercent() {
  sensorSamples[sampleIndex] = analogRead(SENSOR_PIN);
  sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) sum += sensorSamples[i];
  int averaged = sum / NUM_SAMPLES;

  // Mapare la procente cu limitare
  float pct = map(averaged, SENSOR_DRY, SENSOR_WET, 0, 100);
  pct = constrain(pct, 0, 100);
  return pct;
}

// ==================== CONTROL POMPĂ (HISTEREZIS) ====================
void controlPump() {
  if (!modeAuto) return;  // În MANUAL, pompa e controlată de butoane / serial

  if (!pumpOn && levelPercent <= setpointLow) {
    pumpOn = true;
    digitalWrite(RELAY_PIN, HIGH);
  } else if (pumpOn && levelPercent >= setpointHigh) {
    pumpOn = false;
    digitalWrite(RELAY_PIN, LOW);
  }
}

// ==================== ALERTĂ BUZZER ====================
void updateBuzzer() {
  bool criticalLow  = (levelPercent <= LEVEL_CRITICAL_LOW);
  bool criticalHigh = (levelPercent >= LEVEL_CRITICAL_HIGH);
  buzzerActive = (criticalLow || criticalHigh);

  // Beep rapid la nivel critic
  if (buzzerActive) {
    buzzerOnTime  = criticalLow ? 80  : 200;
    buzzerOffTime = criticalLow ? 120 : 800;

    unsigned long now = millis();
    if (!buzzerState && (now - lastBuzzerToggle >= buzzerOffTime)) {
      buzzerState = true;
      digitalWrite(BUZZER_PIN, HIGH);
      lastBuzzerToggle = now;
    } else if (buzzerState && (now - lastBuzzerToggle >= buzzerOnTime)) {
      buzzerState = false;
      digitalWrite(BUZZER_PIN, LOW);
      lastBuzzerToggle = now;
    }
  } else {
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ==================== BUTOANE UP/DOWN ====================
void handleButtons() {
  unsigned long now = millis();
  if (now - lastBtnPress < DEBOUNCE_DELAY) return;

  if (digitalRead(BTN_UP) == LOW) {
    setpointLow  = constrain(setpointLow  + 5, 5,  setpointHigh - 10);
    setpointHigh = constrain(setpointHigh + 5, setpointLow + 10, 95);
    lastBtnPress = now;
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    setpointLow  = constrain(setpointLow  - 5, 5,  setpointHigh - 10);
    setpointHigh = constrain(setpointHigh - 5, setpointLow + 10, 95);
    lastBtnPress = now;
  }
}

// ==================== SERIAL JSON SEND ====================
void sendSerial() {
  Serial.print("{");
  Serial.print("\"level\":"); Serial.print(levelPercent, 1);
  Serial.print(",\"pump\":");  Serial.print(pumpOn ? "true" : "false");
  Serial.print(",\"mode\":\""); Serial.print(modeAuto ? "AUTO" : "MANUAL"); Serial.print("\"");
  Serial.print(",\"sp_low\":"); Serial.print(setpointLow);
  Serial.print(",\"sp_high\":"); Serial.print(setpointHigh);
  Serial.print(",\"raw\":"); Serial.print(sensorRaw);
  Serial.print(",\"alert\":\"");
  if (levelPercent <= LEVEL_CRITICAL_LOW)  Serial.print("CRITICAL_LOW");
  else if (levelPercent >= LEVEL_CRITICAL_HIGH) Serial.print("CRITICAL_HIGH");
  else Serial.print("OK");
  Serial.print("\"");
  Serial.println("}");
}

// ==================== SERIAL RECEIVE (comenzi de la PC) ====================
void receiveSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "PUMP_ON") {
    pumpOn = true;
    digitalWrite(RELAY_PIN, HIGH);
  } else if (cmd == "PUMP_OFF") {
    pumpOn = false;
    digitalWrite(RELAY_PIN, LOW);
  } else if (cmd == "MODE_AUTO") {
    modeAuto = true;
  } else if (cmd == "MODE_MANUAL") {
    modeAuto = false;
  } else if (cmd.startsWith("SP_LOW:")) {
    setpointLow = constrain(cmd.substring(7).toInt(), 5, 90);
  } else if (cmd.startsWith("SP_HIGH:")) {
    setpointHigh = constrain(cmd.substring(8).toInt(), 10, 95);
  }
}

// ==================== DISPLAY OLED ====================
void updateDisplay() {
  display.clearDisplay();

  // --- Linie titlu ---
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("NIVEL APA");
  display.setCursor(78, 0);
  display.print(modeAuto ? "[AUTO]" : "[MAN] ");

  // --- Procent mare ---
  display.setTextSize(2);
  display.setCursor(0, 14);
  if (levelPercent < 10) display.print(" ");
  display.print((int)levelPercent);
  display.print("%");

  // --- Status pompă ---
  display.setTextSize(1);
  display.setCursor(72, 14);
  display.print("POMPA:");
  display.setCursor(72, 24);
  display.print(pumpOn ? " ON " : " OFF");

  // --- Bara nivel ---
  int barWidth = map((int)levelPercent, 0, 100, 0, 128);
  display.drawRect(0, 36, 128, 12, SSD1306_WHITE);
  display.fillRect(0, 36, barWidth, 12, SSD1306_WHITE);

  // --- Setpoint markers pe bară ---
  int spLowX  = map(setpointLow,  0, 100, 0, 128);
  int spHighX = map(setpointHigh, 0, 100, 0, 128);
  display.drawFastVLine(spLowX,  34, 16, SSD1306_WHITE);
  display.drawFastVLine(spHighX, 34, 16, SSD1306_WHITE);

  // --- Setpoint values ---
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print("SP:");
  display.print(setpointLow);
  display.print("-");
  display.print(setpointHigh);
  display.print("%");

  // --- Alert ---
  if (levelPercent <= LEVEL_CRITICAL_LOW) {
    display.setCursor(72, 52);
    display.print("! LOW !");
  } else if (levelPercent >= LEVEL_CRITICAL_HIGH) {
    display.setCursor(68, 52);
    display.print("! HIGH !");
  }

  display.display();
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  unsigned long now = millis();

  // Gestionare interrupt MODE
  if (modePressed) {
    modePressed = false;
    modeAuto = !modeAuto;
    if (!modeAuto) {
      pumpOn = false;
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  // Citire senzor
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    sensorRaw    = analogRead(SENSOR_PIN);
    levelPercent = readLevelPercent();
    lastSensorRead = now;
    controlPump();
  }

  // Butoane
  handleButtons();

  // Buzzer
  updateBuzzer();

  // Display
  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = now;
  }

  // Serial send
  if (now - lastSerialSend >= SERIAL_INTERVAL) {
    sendSerial();
    lastSerialSend = now;
  }

  // Serial receive
  receiveSerial();
}
