/**
 * @file Lab1.ino
 * @brief Obrada visestrukih prekida i prioriteta (Tipkala)
 * 
 * Demonstracija detekcije pritisaka tipkala s razlicitim prioritetima.
 * U ISR funkcijama prebacuju se volatile zastavice (flags).
 */

#include <Arduino.h>

/** Definicije pinova za tipkala */
const int PIN_BTN_HIGH = 25;
const int PIN_BTN_MED  = 26;
const int PIN_BTN_LOW  = 27;

/** 
 * Volatile zastavice (flags) za komunikaciju ISR rutine i glavnog koda.
 * Zastavice moraju biti volatile kako bi programer sugerirao
 * prevoditelju da se njihova vrijednost moze asinkrono promijeniti.
 */
volatile bool flagHigh = false;
volatile bool flagMed  = false;
volatile bool flagLow  = false;

/**
 * @brief ISR za tipkalo VISOKOG prioriteta
 * @note IRAM_ATTR smjesta ovu funkciju u RAM radi najbrzeg izvrsavanja.
 */
void IRAM_ATTR isrHigh() {
  flagHigh = true;
}

void IRAM_ATTR isrMed() {
  flagMed = true;
}

void IRAM_ATTR isrLow() {
  flagLow = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- Sustav pokrenut: Cekam prekide ---");

  // Konfiguracija kao ulazi (INPUT_PULLUP)
  pinMode(PIN_BTN_HIGH, INPUT_PULLUP);
  pinMode(PIN_BTN_MED,  INPUT_PULLUP);
  pinMode(PIN_BTN_LOW,  INPUT_PULLUP);

  // Dodavanje prekida za svako tipkalo (okida na padajuci brid)
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_HIGH), isrHigh, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_MED),  isrMed,  FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN_LOW),  isrLow,  FALLING);
}

void loop() {
  // Ovdje softverski simuliramo prioritete citanjem zastavica (flags).
  // Najvazniji uvjet ide prvi.
  
  if (flagHigh) {
    Serial.println("[PRIORITET 1] -> Visoki prioritet izvrsen!");
    flagHigh = false;
  } 
  else if (flagMed) {
    Serial.println("[PRIORITET 2] -> Srednji prioritet izvrsen!");
    flagMed = false;
  } 
  else if (flagLow) {
    Serial.println("[PRIORITET 3] -> Niski prioritet izvrsen!");
    flagLow = false;
  }

  // De-bouncing pauza
  delay(100);
}
