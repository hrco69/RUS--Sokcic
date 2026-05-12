# Laboratorijska vježba 2 - Upravljanje potrošnjom i stanje mirovanja (Deep Sleep)

**Kolegij:** Računalni upravljački sustavi (RUS)
**Platforma:** ESP32 DOIT DevKit V1 (PlatformIO + Arduino Framework)
**Simulator:** Wokwi
**Varijanta:** 2 - Datalogger okoliša sa periodičkim buđenjem

---

## 1. Kratki opis projekta

Cilj ove vježbe je bio napraviti ESP32 senzor (datalogger) koji provodi većinu svog vremena u stanju mirovanja (Deep Sleep) kako bi štedio bateriju, a povremeno se budi pomoću ugrađenog RTC timera da bi očitao podatke (temperaturu i vlagu) sa DHT22 senzora. 

Zapisi se pohranjuju u `RTC_DATA_ATTR` memoriju u obliku ring buffera (kapaciteta 10 zapisa). Budući da standardna RAM memorija ne preživljava Deep Sleep stanje, korištenje `RTC_DATA_ATTR` omogućuje da podaci ostanu netaknuti i kroz resete. Kada se spremnik napuni, svih 10 zapisa se ispisuje na serijskom monitoru, nakon čega se spremnik prazni.

---

## 2. Povezivanje komponenti i shema (Wokwi)

Simulator je konfiguriran preko `wokwi.toml` datoteke da kompajlira izvorni `.pio` projekt (`firmware.bin`) kako bi osigurao identično izvođenje kao na pravoj opremi i uklonio bugove oblaka.

| Komponenta | ESP32 Pin | Smjer | Opis |
| :--- | :---: | :--- | :--- |
| **DHT22** | GPIO 15 | Ulaz | Senzor za temperaturu i vlagu |
| **LED ACT (Crvena)** | GPIO 26 | Izlaz | Signalizira trenutak samog očitavanja podatka |
| **LED AWAKE (Žuta)** | GPIO 14 | Izlaz | Pokazuje općenitu budnost MCU-a |

> Napomena: U Wokwi `diagram.json` je TX/RX linija ESP32 modula eksplicitno povezana s `$serialMonitor:RX` / `$serialMonitor:TX` vezama kako se spriječio nestanak terminalnog zapisa uslijed simulacijskih propusta kod soft-restarta.

---

## 3. Implementacija Hibridnog 'Sleep' Mehanizma

U realnim uvjetima, MCU koristi funkciju `esp_deep_sleep_start()`. Wokwi simulator pri svakom Deep Sleep izlasku ispisuje dugi *bootloader banner* i zbog DHT senzora (kojem treba vremena da se stabilizira) upada u problem s konstantnim restiranjima (SW_RESET) i pretrpava Serijski monitor sa boot informacijama.

Da bi rješenje bilo savršeno čitljivo na simulatoru, a logički ispravno, implementiran je **Hibridni mehanizam preuzet iz Bencic repozitorija**:
1. **Emulacija sna:** Simulator prvo napravi obično čekanje s ugašenim LED indikatorima kako bi se vizualno dočaralo vrijeme mirovanja bez rušenja konzole.
2. **Kratki pravi Deep Sleep:** Odmah iza toga započinje stvarni ESP32 Deep sleep poziv na samo 100ms. To jamči simulaciju pravog hardware reseta bez prekomjernog zasljepljivanja ekrana. Ovo istovremeno predstavlja strogi tehnički dokaz da je projektirani `RTC_DATA_ATTR` memorijski princip za očuvanje *Reading zapisa* implementiran potpuno ispravno jer memorija efektivno preživljava pravi ponovni start.

---

## 4. Očekivano ponašanje terminala

Kako se uređaj ponaša iz perspektive konzole:
- Na prvom pokretanju ispiše se ukrasni *"Datalogger Pokrenut"* blok.
- Na svakom buđenju prikupi se broj, temperatura, vlaga i ispiše skraćena informacija: `[Zapis X/10] -> Temp: 24.0 C | Vlaga: 40.0 %`
- Budi se na redovnoj bazi i inkrementira counter koji je globalna varijabla u sigurnoj sporo-propusnoj SRAM memoriji.
- Nakon 10 buđenja, pali se `dumpAndReset()` kontroler koji ispiše urednu tablicu na terminalu te nuli brojčanike. 

---

## 5. Teoretski izračun trajanja baterije

Pretpostavimo bateriju od **2000 mAh** (npr. klasična Li-Po) s nominalnim naponom od 3.3V sustava. 

* **Struja u Deep Sleepu ($I_{sleep}$):** ~10 uA (0.01 mA)
* **Vrijeme u Sleepu ($T_{sleep}$):** 59 sekundi
* **Struja u Aktivnom Radu ($I_{active}$):** ~80 mA (uz WiFi ugašen)
* **Vrijeme u Aktivnom Radu ($T_{active}$):** 1 sekunda (zbroj senzorskog očitavanja i ostalog kalkuliranja)

Vrijeme jednog ciklusa je t=60 s.  
**Prosječna struja:**
I_avg = (80 * 1 + 0.01 * 59) / 60 = (80 + 0.59)/60 = 1.343 mA

**Trajanje baterije:**
Trajanje (sati) = 2000 mAh / 1.343 mA = 1489 sati = 62 dana

Time dokazujemo da se višemjesečni rad dataloggera može omogućiti kvalitetnim Power Management pristupom, što se značajno kosi sa situacijom trajanja od jedva 1 dan bez Deep Sleep režima rada. Uz gašenje power-ledica na ESP modulu (hardverska prepravka), može se doseći iznos od preko godine dana.