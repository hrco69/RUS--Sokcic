#include <Arduino.h>

#define WAKE_UP_INTERVAL_SEC 5
#define uS_TO_S_FACTOR 1000000ULL

#define MAX_RECORDS 10
RTC_DATA_ATTR float temp_data[MAX_RECORDS];
RTC_DATA_ATTR float hum_data[MAX_RECORDS];
RTC_DATA_ATTR int record_count = 0;

void readSensors(float &temp, float &hum) {
    // Simuliramo ocitanja
    temp = 20.0 + ((float)random(-50, 50) / 10.0);
    hum = 50.0 + ((float)random(-100, 100) / 10.0);
}

void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER: 
            Serial.println("Budenje: TIMER."); 
            break;
        default: 
            Serial.println("Budenje: REDOVNI RESET / PRVO PALJENJE."); 
            break;
    }
}

void activePhase() {
    float t, h;
    readSensors(t, h);
    
    temp_data[record_count] = t;
    hum_data[record_count] = h;
    record_count++;
    
    Serial.printf("Mjerenje %d/%d -> Temp: %.1f C, Vlaga: %.1f %%\n", record_count, MAX_RECORDS, t, h);
    
    if (record_count >= MAX_RECORDS) {
        Serial.println("\n--- SPREMNIK PUN -> Ispisujem zadnjih 10 mjerenja ---");
        for (int i = 0; i < MAX_RECORDS; i++) {
            Serial.printf("[%d] Temp: %.1f C, Vlaga: %.1f %%\n", i+1, temp_data[i], hum_data[i]);
        }
        Serial.println("-----------------------------------------------------");
        record_count = 0;
        Serial.println("Spremnik resetiran.");
    }
}

void enterSleep() {
    Serial.println("Ulazak u Deep Sleep mod na 5 sekundi...\n");
    Serial.flush(); // Cekaj da se sav tekst ispise
    esp_sleep_enable_timer_wakeup(WAKE_UP_INTERVAL_SEC * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Stabilizacija
    
    printWakeupReason();
    activePhase();
    enterSleep();
}

void loop() {
    // Nece se izvrsiti zbog Deep Sleepa
}
