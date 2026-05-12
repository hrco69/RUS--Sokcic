#include <Arduino.h>
#include <DHT.h>

#define WAKE_UP_INTERVAL_SEC 5
#define uS_TO_S_FACTOR 1000000ULL

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_AWAKE 14
#define LED_ACT 26

#define MAX_RECORDS 10
RTC_DATA_ATTR float temp_data[MAX_RECORDS];
RTC_DATA_ATTR float hum_data[MAX_RECORDS];
RTC_DATA_ATTR int record_count = 0;

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_AWAKE, OUTPUT);
    pinMode(LED_ACT, OUTPUT);
    digitalWrite(LED_AWAKE, HIGH);
    
    dht.begin();
    
    digitalWrite(LED_ACT, HIGH);
    delay(2000); 
    
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    digitalWrite(LED_ACT, LOW);
    
    if (isnan(t) || isnan(h)) {
        t = 0.0;
        h = 0.0;
    }
    
    temp_data[record_count] = t;
    hum_data[record_count] = h;
    record_count++;
    
    Serial.printf("\n[Zapis %d/10] -> Temp: %.1f C | Vlaga: %.1f %%\n", record_count, t, h);
    
    if (record_count >= MAX_RECORDS) {
        Serial.println("--- SPREMNIK PUN -> Ocitavam iz memorije ---");
        for (int i = 0; i < MAX_RECORDS; i++) {
            Serial.printf("[%d] Temp: %.1f C | Vlaga: %.1f %%\n", i+1, temp_data[i], hum_data[i]);
        }
        Serial.println("----------------------------------------------");
        record_count = 0;
    }
    
    Serial.flush(); 
    digitalWrite(LED_AWAKE, LOW);
    
    esp_sleep_enable_timer_wakeup(WAKE_UP_INTERVAL_SEC * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop() {
}
