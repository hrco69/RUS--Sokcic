# Lab 2 - Upravljanje potrošnjom energije mikrokontrolera

## Opis rješenja
Uredaj simulira periodicko mjerenje temperature i vlage u svrhu datalogginga. Sustav se budi iz *Deep Sleep* moda putem timera, vrsi ocitanja (simulirana) te ih sprema u ESP32 **RTC memoriju** (RTC_DATA_ATTR) kako bi podaci ostali ocuvani kroz cikluse gašenja.
Nakon maksimalno 10 zapisa, sustav ispisuje sva stara mjerenja, resetira brojac i ponovno odlazi u *Deep sleep*. Održavanje aktivnog rada na minimumu ekstremno smanjuje ukupnu potrošnju energije.

**Wokwi link:** [Ovdje stavite link spremljenog projekta]

---

## Sažetak 

| Stavka | Odgovor |
|---|---|
| Platforma | ESP32 |
| Varijanta | 2 (Datalogger okoliša) |
| Sleep mode | Deep Sleep |
| Budenje | Timer wakeup |
| Cuvanje stanja | RTC memorija (RTC_DATA_ATTR) |
| Debouncing | Nije potrebno (nema vanjskih tipki) |
| Wokwi link | [Ovdje stavite link] |
