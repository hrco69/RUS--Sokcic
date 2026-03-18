/**
 * @file main.cpp
 * @brief Obrada višestrukih prekida (Tipkala, Timer, Senzor udaljenosti)
 * 
 * Ovaj program demonstrira korištenje prekida s tipkalima,
 * mjerenje udaljenosti HC-SR04 senzorom pomoću prekida na promjenu stanja (CHANGE)
 * te korištenje hardverskog timera uz ugrađenu zaštitu kritičnih sekcija.
 */

#include <Arduino.h>

// --- PINOVI ---
const int PIN_BTN_HIGH = 25;
const int PIN_BTN_MED  = 26;
const int PIN_BTN_LOW  = 27;

const int PIN_TRIG = 5;
const int PIN_ECHO = 18;

// --- ZASTAVICE (Flags) ---
volatile bool flagHigh = false;
volatile bool flagMed  = false;
volatile bool flagLow  = false;
volatile bool flagTimer = false;
volatile bool distanceReady = false;

// --- PODACI SENZORA I TIMERA ---
volatile unsigned long echoStart = 0;
volatile float distance = 0.0;
volatile int sharedCounter = 0; // Dijeljeni resurs koji će se inkrementirati

// --- TIMERS I MUTEX (Kritična sekcija) ---
hw_timer_t * timer = NULL;
// Mutex (zastavica / semafor) sprječava da prekid proglasi pristup varijabli dok ju procesor čita
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; 

// ISR: Tipkala
void IRAM_ATTR isrHigh() { flagHigh = true; }
void IRAM_ATTR isrMed()  { flagMed = true; }
void IRAM_ATTR isrLow()  { flagLow = true; }

/**
 * @brief ISR za senzor HC-SR04
 * Reagira na obje promjene (CHANGE) - s LOW na HIGH i obrnuto.
 * Na taj način precizno mjerimo širinu ECHO pulsa koristeći mikrosekunde.
 */
void IRAM_ATTR isrEcho() {
  if (digitalRead(PIN_ECHO) == HIGH) {
    // Početak pulse-a - bilježimo vrijeme
    echoStart = micros();
  } else {
    // Kraj pulse-a
    unsigned long echoEnd = micros();
    unsigned long echoDuration = echoEnd - echoStart;
    
    // ULAZAK U KRITIČNU SEKCIJU
    // Sprječavamo sudaranje procesa ako se ovdje dogodi neki drugi jači prekid (npr. timer)
    portENTER_CRITICAL_ISR(&mux);
    distance = echoDuration * 0.034 / 2.0;
    portEXIT_CRITICAL_ISR(&mux);
    
    distanceReady = true;
  }
}

/**
 * @brief ISR za Hardverski Timer
 * Okida se zadanim intervalom.
 */
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&mux);
  sharedCounter++; // Inkrementiranje dijeljenog resursa unutar sigurne zone
  portEXIT_CRITICAL_ISR(&mux);
  
  flagTimer = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- Sustav pokrenut: ESP32 Višestruki Prekidi ---");

  // Postavljanje tipkala
  pinMode(PIN_BTN_HIGH, INPUT_PULLUP);
  pinMode(PIN_BTN_MED,  INPUT_PULLUP);
  pinMode(PIN_BTN_LOW,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_HIGH), isrHigh, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_MED),  isrMed,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_LOW),  isrLow,  FALLING);

  // Postavljanje HC-SR04
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  // Postavljamo "CHANGE" što znači da funkcija hvata svaku promjenu signala
  attachInterrupt(digitalPinToInterrupt(PIN_ECHO), isrEcho, CHANGE);

  // Postavljanje hardverskog Timera 0
  // Pretpojačalo (prescaler) na 80 znači da je svaki "tick" = 1 mikrosekunda (jer je ESP32 na 80 MHz osnovnog takta)
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  // Okidanje svake 2 sekunde (2 000 000 mikrosekundi)
  timerAlarmWrite(timer, 2000000, true);
  timerAlarmEnable(timer);
}

// Varijabla za ne-blokirajući trigger senzora udaljenosti
unsigned long lastTrigMillis = 0;

void loop() {
  // === 1. PERIODIČNO OKIDANJE SENZORA (svakih 500 ms) ===
  // Ne koristimo delay() jer bi delay blokirao obradu softverskih zastavica!
  if (millis() - lastTrigMillis > 500) {
    lastTrigMillis = millis();
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
  }

  // === 2. PRIORITETNA OBRADA ZASTAVICA (FLAGS) ===
  
  // (A) Tipkala imaju apsolutni softverski prioritet pri ispisu
  if (flagHigh) {
    Serial.println("[PRIORITET 1] -> Visoki prekid obraden!");
    flagHigh = false;
  } 
  else if (flagMed) {
    Serial.println("[PRIORITET 2] -> Srednji prekid obraden!");
    flagMed = false;
  } 
  else if (flagLow) {
    Serial.println("[PRIORITET 3] -> Niski prekid obraden!");
    flagLow = false;
  }
  
  // (B) Hardverski timer
  if (flagTimer) {
    // Sigurno čitanje varijable podložne promjeni u ISR-u
    portENTER_CRITICAL(&mux);
    int currentCounter = sharedCounter;
    portEXIT_CRITICAL(&mux);
    
    Serial.printf("[TIMER] -> Timer ISR okinuo. Trenutni Counter: %d\r\n", currentCounter);
    flagTimer = false;
  }

  // (C) Senzor udaljenosti ispis
  if (distanceReady) {
    portENTER_CRITICAL(&mux);
    float currentDist = distance;
    portEXIT_CRITICAL(&mux);
    
    Serial.printf("[SENZOR HC-SR04] -> Ocitanje s prekida: %.2f cm\r\n", currentDist);
    distanceReady = false;
  }
}
