#include <Arduino.h>
#include <DHT.h>
#include <esp_sleep.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

#define LED_AWAKE_PIN 14
#define LED_ACT_PIN 26

#define SLEEP_INTERVAL_MS 4000
#define REAL_DEEP_SLEEP_MS 100
#define BUFFER_CAPACITY 10

// Struktura za spremanje 
struct Reading {
    float temperatureC;
    float humidityPct;
};

// Spremanje u RTC memoriju mikrokontrolera kako bi prezivjelo Deep Sleep resete
RTC_DATA_ATTR static Reading  buffer[BUFFER_CAPACITY];
RTC_DATA_ATTR static uint8_t  bufferCount = 0;   
RTC_DATA_ATTR static uint32_t bootCount = 0;   

void performReading() {
    digitalWrite(LED_ACT_PIN, HIGH);
    
    // DHT22 zna biti spor pa cekamo bar 100ms ako zatreba
    float outT = dht.readTemperature();
    float outH = dht.readHumidity();
    
    if (isnan(outT) || isnan(outH)) {
        delay(150);
        outT = dht.readTemperature();
        outH = dht.readHumidity();
    }
    
    digitalWrite(LED_ACT_PIN, LOW);
    
    if (isnan(outT) || isnan(outH)) {
        outT = 0.0;
        outH = 0.0;
    }
    
    // Spremi u RTC_DATA_ATTR spremnik
    buffer[bufferCount].temperatureC = outT;
    buffer[bufferCount].humidityPct = outH;
    bufferCount++;
    
    Serial.printf(" [Zapis %u/%u] Ocitano -> Temp: %.1f C | Vlaga: %.1f %%\r\n", 
                  (unsigned)bufferCount, (unsigned)BUFFER_CAPACITY, outT, outH);
                  
    if (bufferCount >= BUFFER_CAPACITY) {
        Serial.println("\r\n================== SPREMNIK PUN -> ISPIS 10 ZAPISA ==================");
        Serial.println(" redni broj | temperatura (C) | vlaga (%)  ");
        Serial.println("------------+-----------------+------------");
        for (uint8_t i = 0; i < BUFFER_CAPACITY; i++) {
            Serial.printf(" %10u | %15.1f | %10.1f\r\n", 
                          (unsigned)(i+1), buffer[i].temperatureC, buffer[i].humidityPct);
        }
        Serial.println("=====================================================================\r\n");
        bufferCount = 0;
    }
}

void emulatedSleep(uint32_t ms) {
    Serial.printf("[SLEEP]  Simulirano cekanje izmedju mjerenja na %u ms...\r\n", (unsigned)ms);
    Serial.flush();
    digitalWrite(LED_AWAKE_PIN, LOW);
    delay(ms);
    digitalWrite(LED_AWAKE_PIN, HIGH);
    Serial.println("[WAKE]   Cekanje zavrseno.");
}

void setup() {
    Serial.begin(115200);
    delay(200);
    
    bootCount++;
    const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    const bool fromDeepSleep = (cause == ESP_SLEEP_WAKEUP_TIMER);

    pinMode(LED_AWAKE_PIN, OUTPUT);
    pinMode(LED_ACT_PIN, OUTPUT);
    digitalWrite(LED_AWAKE_PIN, HIGH);
    digitalWrite(LED_ACT_PIN, LOW);

    dht.begin();
    
    if (fromDeepSleep) {
        Serial.printf("\r\n[BOOT #%u] Budenje iz Deep Sleepa\r\n", (unsigned)bootCount);
    } else {
        Serial.println();
        Serial.println("=====================================================================");
        Serial.println(" [RUS] Datalogger ESP32 - Hibridni Sleep Mod za cisti Wokwi prikaz");
        Serial.println("=====================================================================");
    }
}

void loop() {
    // 1. Ocitaj i zapisi
    performReading();
    
    // 2. Simulirano Wokwi spavanje kako terminal ne bi stalno ispisivao 'boot spam'
    emulatedSleep(SLEEP_INTERVAL_MS);
    
    // 3. Pravi *Kratki* Deep Sleep kako bismo demonstrirali RTC cuvanje
    Serial.flush();
    digitalWrite(LED_AWAKE_PIN, LOW);
    
    esp_sleep_enable_timer_wakeup((uint64_t)REAL_DEEP_SLEEP_MS * 1000ULL);
    esp_deep_sleep_start();  
}
