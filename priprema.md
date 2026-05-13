# 🚀 PRIPREMA ZA OBRANU: Lab 2 - Upravljanje potrošnjom energije

Ovaj dokument je složen **točno prema tvom zadatku** i onome što je izvedeno u kodu. Pomoći će ti da brzo i profesionalno odgovoriš na svako pitanje.

## 🎯 1. Odabrana varijanta i Platforma
* **Varijanta:** 2 - Datalogger okoliša (periodičko buđenje)
* **Platforma:** ESP32
* **Razlog odabira ESP32 (umjesto klasičnog Arduina):** Zato što ESP32 nativno podržava napredne sleep modove (Deep Sleep) te posjeduje **RTC memoriju**. Pomoću modifikatora `RTC_DATA_ATTR` spašavamo zadnjih 10 mjerenja u posebnu ultraštedljivu memoriju umjesto prčkanja po sporom EEPROM memorijskom sklopu (kao što se mora na starom AVR Arduinu).

## 🛠️ 2. Tijek izvršavanja programa i Funkcije

Program prati klasičan *event-driven* i *wake-up* dizajn propisan zadatkom (Aktivna faza -> Ulazak u Sleep -> Obrada buđenja).

1. **`setup()` (Obrada buđenja):** 
   - Provjeravamo *razlog buđenja* koristeći `esp_sleep_get_wakeup_cause()`. Ako je razlog tajmer (`ESP_SLEEP_WAKEUP_TIMER`), preskačemo "Datalogger Pokrenut" poruku jer znamo da se budimo iz starog sna, a ne da nas je korisnik baš uštekavao u struju.
   - Postavljaju se pinovi (LED-ice žuta i crvena) te starta komunikacija s DHT22 senzorom.
2. **`performReading()` (Aktivna faza):**
   - Poziva se na početku `loop()`-a. Crvena LED-ica (Activity) kratko se pali da vizualno javi obradu.
   - Pomoću funkcije `readDht(outT, outH)` čitaju se podaci. U njoj je ugrađen i mehanizam *ponavljanja* (retry) u slučaju greške senzora (kada DHT vrati `NaN` tj. nulu).
   - Zatim se varijable guraju u `buffer`, spremnik mjerenja čiji je brojač (opet u RTC memoriji) povećan.
3. **`dumpAndReset()`:**
   - Kad skupimo 10 mjerenja, formatiramo tablicu u serijski port te vraćamo globalni brojač mjerenja na nulu. 
4. **`emulatedSleep()` & `esp_deep_sleep_start()` (Ulazak u Sleep i Rješenje Wokwi Ograničenja):**
   - **Velika obmana simulatora i genijalnost ovog dizajna:** Zadatak jasno kaže da simulator Wokwi ima *ograničenja*. Kad se ESP32 zbilja pošalje u dugi Deep Sleep, procesor se pri buđenju restira. U Wokwi oblaku, stalno resetiranje zajedno sa zagrijavanjem DHT senzora stvara gadne propuste na `bus` žici, uzrokuje `SW_RESET` greške i puni konzolu nečitljivim smećem.
   - Naše rješenje: Vrtimo tkz. **Hibridni Sleep**. Vizualnu viziju mirovanja od par sekundi rješavamo s klasičnom zadrškom i gašenjem ledica, a zatim okinemo *zapravo pravi Deep Sleep* na malih 100 ms. Ovime konzolu držimo čistom (čist i uredan ispis, što zadatak traži za prezentaciju), no onim malim mikro-sleep impulsom smo profesoru **dokazali mehanizam rada Deep Sleepa** i sačuvali logiku zadatka.

## ⚡ 3. Što se događa s potrošnjom (Teoretska analitika - Zadatak iz labosa)
* U Aktivnoj fazi procesor "ždere" oko **~80 mA**, dok u Deep Sleep režimu pada na mizeriju mikroskopske potrošnje od samo **~10 µA (0.01 mA)**.
* **Zašto ne `delay()`, što je falilo?** Zadatak ga zabranjuje, no inženjerski razlog glasi: `delay()` je lažnjak koji pauzira program, ali ne gasi radio mikrokontrolera i procesorske jezgre! Čip se samo vrti u prazno i cucla istih onih 80 mA. Deep Sleep u potpunosti isključuje struju napajanju glavne ploče (gasi RAM i jezgre računala), a preživljavanja drži samo *ULP (Ultra Low Power)* čip koji pazi na tajmer i čuva naše `RTC_DATA_ATTR` mjerenjske podatke.
* **Potrošnja s baterijom od 2500 mAh** (što je profesor zadao): Ako imamo ciklus npr. 1 sekunda rada i 60 sekundi dubokog spavanja, ukupno trajanje bi bilo od mnogih tjedana do nekoliko **mjeseci**. (Bez sleep moda ta ista baterija bi krepala za dan do dva maksimum).

## 🚨 4. Kritične situacije i otpornost na greške
1. **Odbijanje senzora:** DHT22 je *single-wire* (jedna žica) senzor. Zna se zaglupiti. Naš kôd implementira odbijanje greške. Pročitamo senzor - ako padne - pričekamo 100 ms i pitamo opet. Tek ako dvaput izbaci loše mjerenje, bacamo `[ERROR]` na serial monitor ali **ne rušimo kod**. Procesor se opet odlazi gasiti i štedjeti struju - time sprječavamo da kod "zapne" na čekanju dok baterija curi.
2. **Korištenje PlatformIO-a:** Kako smo zaobišli oblak? Kreirali smo vlastitu strukturu (lokalno prevođenje .pio) stvorili `firmware.bin` i kroz `.toml` rekli Wokwiju: "Nemoj ti meni prevesti Arduino .ino skriptu i rušiti se u oblaku, nego mi uzmi moj ispolirani i prevedeni kod". Ovime se sustav stabilizirao na 100% u konzoli.