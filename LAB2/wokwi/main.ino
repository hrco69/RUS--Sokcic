#include <Arduino.h>
#include <DHT.h>

#define WAKE_UP_INTERVAL_SEC 5
#define uS_TO_S_FACTOR 1000000ULL

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define MAX_RECORDS 10
RTC_DATA_ATTR float temp_data[MAX_RECORDS];
RTC_DATA_ATTR float hum_data[MAX_RECORDS];
RTC_DATA_ATTR int record_count = 0;

void readSensors(float &temp, float &hum) {
    // Čitanje stvarnih vrijednosti sa senzora
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Provjera jesmo li uspješno pročitali
    if (isnan(t) || isnan(h)) {
        Serial.println("Greska pri citanju sa DHT22 senzora!");
        temp = 0.0;
        hum = 0.0;
    } else {
        temp = t;
        hum = h;
    }
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
    
    // Spremanje u RTC polja
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
    Serial.flush(); // Cekaj da se sav tekst ispise prije gasenja periferija
    esp_sleep_enable_timer_wakeup(WAKE_UP_INTERVAL_SEC * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Kratka stabilizacija senzora i serijske veze
    
    Serial.println("\n=== Ocitavanje zapoceto ===");
    
    // Inicijalizacija senzora
    dht.begin();
    
    printWakeupReason();
    activePhase();
    enterSleep();
}

void loop() {
    // Nece se izvrsiti zbog Deep Sleepa
}