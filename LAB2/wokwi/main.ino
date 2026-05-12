#include <Arduino.h>
#include <DHT.h>

#define WAKE_UP_INTERVAL_SEC 5
#define uS_TO_S_FACTOR 1000000ULL

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pinovi za LED indikatore
#define LED_AWAKE 14
#define LED_ACT 26

#define MAX_RECORDS 10
RTC_DATA_ATTR float temp_data[MAX_RECORDS];
RTC_DATA_ATTR float hum_data[MAX_RECORDS];
RTC_DATA_ATTR int record_count = 0;

void readSensors(float &temp, float &hum) {
    // Palimo crvenu LEDicu za indikaciju mjerenja
    digitalWrite(LED_ACT, HIGH);
    delay(1500); // Kratka pauza da mjerenje bude vizualno primjetno
    
    // Čitanje stvarnih vrijednosti sa senzora
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Provjera
    if (isnan(t) || isnan(h)) {
        temp = 0.0;
        hum = 0.0;
    } else {
        temp = t;
        hum = h;
    }
    
    // Gasimo indikator mjerenja
    digitalWrite(LED_ACT, LOW);
}

void activePhase() {
    float t, h;
    readSensors(t, h);
    
    // Spremanje u RTC polja
    temp_data[record_count] = t;
    hum_data[record_count] = h;
    record_count++;
    
    // 2-in-1 formatiran ispis temperature i vlage kako si trazio, bez spama!
    Serial.printf("[Zapis %d/10] -> Temp: %.1f C | Vlaga: %.1f %%\n", record_count, t, h);
    
    if (record_count >= MAX_RECORDS) {
        Serial.println("\n--- SPREMNIK PUN -> Ocitavam iz RTC memorije ---");
        for (int i = 0; i < MAX_RECORDS; i++) {
            Serial.printf("[%d] Temp: %.1f C | Vlaga: %.1f %%\n", i+1, temp_data[i], hum_data[i]);
        }
        Serial.println("------------------------------------------------");
        record_count = 0;
    }
}

void enterSleep() {
    Serial.println("-> Ulazak u Deep Sleep mod na 5s...\n");
    Serial.flush(); 
    
    // Gasimo zutu 'BUDAN' LEDicu netom prije spavanja
    digitalWrite(LED_AWAKE, LOW);
    
    esp_sleep_enable_timer_wakeup(WAKE_UP_INTERVAL_SEC * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void setup() {
    // Postavljanje izlaza
    pinMode(LED_AWAKE, OUTPUT);
    pinMode(LED_ACT, OUTPUT);
    
    // Odmah palimo 'BUDAN' indikator
    digitalWrite(LED_AWAKE, HIGH);
    
    Serial.begin(115200);
    delay(1000); 
    
    // Inicijalizacija senzora
    dht.begin();
    
    // Pokreni posao
    activePhase();
    enterSleep();
}

void loop() {
    // Prazno zbog Deep Sleepa
}