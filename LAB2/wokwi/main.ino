#include <Arduino.h>
#include <esp_sleep.h>
#include <DHT.h>

#define DHT_PIN               15     
#define DHT_TYPE              DHT22 
#define LED_AWAKE_PIN         14     
#define LED_ACT_PIN           26     

#define BUFFER_CAPACITY       10     
#define SLEEP_INTERVAL_MS     4000   
#define REAL_DEEP_SLEEP_MS    100    
#define DHT_RETRY_DELAY_MS    100    

struct Reading {
    uint32_t timestampMs;    
    float    temperatureC;
    float    humidityPct;
};

DHT dht(DHT_PIN, DHT_TYPE);

RTC_DATA_ATTR static Reading  buffer[BUFFER_CAPACITY];
RTC_DATA_ATTR static uint8_t  bufferCount      = 0;   
RTC_DATA_ATTR static uint32_t totalReadings    = 0;   
RTC_DATA_ATTR static uint32_t dumpsCompleted   = 0;   
RTC_DATA_ATTR static uint32_t bootCount        = 0;   
RTC_DATA_ATTR static uint32_t accumulatedMs    = 0;   

static bool readDht(float& outT, float& outH) {
    outT = dht.readTemperature();
    outH = dht.readHumidity();
    if (!isnan(outT) && !isnan(outH)) return true;

    delay(DHT_RETRY_DELAY_MS);
    outT = dht.readTemperature();
    outH = dht.readHumidity();
    return !isnan(outT) && !isnan(outH);
}

static void dumpAndReset() {
    Serial.println();
    Serial.println("============================== SPREMNIK PUN -> ANALIZA ZAPISA ==============================");
    
    // Traženje min i max indeksa
    uint8_t minT = 0, maxT = 0, minH = 0, maxH = 0;
    for (uint8_t i = 1; i < BUFFER_CAPACITY; i++) {
        if (buffer[i].temperatureC < buffer[minT].temperatureC) minT = i;
        if (buffer[i].temperatureC > buffer[maxT].temperatureC) maxT = i;
        if (buffer[i].humidityPct < buffer[minH].humidityPct) minH = i;
        if (buffer[i].humidityPct > buffer[maxH].humidityPct) maxH = i;
    }

    Serial.println(" zapis | vrijeme (ms) | temperatura (C) | vlaga (%) | Oznake ekstrema       ");
    Serial.println("-------+--------------+-----------------+-----------+-----------------------");
    for (uint8_t i = 0; i < BUFFER_CAPACITY; i++) {
        String flags = "";
        if (i == maxT) flags += "[MAX TEMP] ";
        if (i == minT) flags += "[MIN TEMP] ";
        if (i == maxH) flags += "[MAX VLAGA] ";
        if (i == minH) flags += "[MIN VLAGA] ";
        
        Serial.printf(" %5u | %12u | %15.1f | %9.1f | %s\r\n",
                      (unsigned)(i+1),
                      (unsigned)buffer[i].timestampMs,
                      buffer[i].temperatureC,
                      buffer[i].humidityPct,
                      flags.c_str());
    }
    dumpsCompleted++;
    Serial.printf(" Ukupno mjerenja otkad je upaljen: %u   |   Praznjenja: %u\r\n",
                  (unsigned)totalReadings, (unsigned)dumpsCompleted);
    Serial.println("============================================================================================");
    Serial.println();
    bufferCount = 0;
}

static void performReading() {
    digitalWrite(LED_ACT_PIN, HIGH);

    float t, h;
    bool success = readDht(t, h);

    if (!success) {
        Serial.println("[ERROR] Ne mogu ocitati DHT22 senzor!");
    } else {
        buffer[bufferCount].timestampMs  = accumulatedMs;
        buffer[bufferCount].temperatureC = t;
        buffer[bufferCount].humidityPct  = h;
        bufferCount++;
        totalReadings++;
        
        // Formatiran ispis umjesto Bencicevog
        Serial.printf("[Zapis %d/10] -> Temp: %.1f C | Vlaga: %.1f %%\r\n", bufferCount, t, h);

        if (bufferCount >= BUFFER_CAPACITY) {
            dumpAndReset();
        }
    }

    digitalWrite(LED_ACT_PIN, LOW);
}

static void emulatedSleep(uint32_t ms) {
    Serial.flush();
    digitalWrite(LED_AWAKE_PIN, LOW);
    delay(ms);   // Wokwi simulirano spavanje da se vizualno vidi na ekranu bez crasha
    digitalWrite(LED_AWAKE_PIN, HIGH);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    bootCount++;
    const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    const bool fromDeepSleep = (cause == ESP_SLEEP_WAKEUP_TIMER);

    pinMode(LED_AWAKE_PIN, OUTPUT);
    pinMode(LED_ACT_PIN,   OUTPUT);
    digitalWrite(LED_AWAKE_PIN, HIGH);
    digitalWrite(LED_ACT_PIN,   LOW);

    dht.begin();

    if (!fromDeepSleep) {
        accumulatedMs = 0;
        Serial.println("");
        Serial.println("=====================================================================");
        Serial.println(" Datalogger Pokrenut (Prvo paljenje)");
        Serial.println("=====================================================================");
    }
}

void loop() {
    performReading();
    emulatedSleep(SLEEP_INTERVAL_MS);

    accumulatedMs += SLEEP_INTERVAL_MS + REAL_DEEP_SLEEP_MS;

    Serial.flush();
    digitalWrite(LED_AWAKE_PIN, LOW);

    esp_sleep_enable_timer_wakeup((uint64_t)REAL_DEEP_SLEEP_MS * 1000ULL);
    esp_deep_sleep_start();  
}

