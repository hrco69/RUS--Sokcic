# Lab1: Prekidi u ugradbenim sustavima

## Autor
Hrvoje Sokcic (GitHub: [hrco69](https://github.com/hrco69/RUS--Sokcic))

## Opis rjesenja
Ovaj zadatak rjesava problematiku upravljanja visestrukim prekidima i njihovim prioritetima na mikrokontroleru **ESP32**, uz uporabu Wokwi simulatora i PlatformIO radnog okvira.

U implementaciji su koristena:
1. **Tri tipkala (High, Med, Low):** Vanjski prekidi (External Interrupts) okidani na padajuci brid signala (`FALLING`), povezani na pinove 25, 26 i 27 koristeci interne pull-up otpornike. Postavljena je zastavica u ISR rutini, dok se softverski prioriteti reguliraju u glavnoj petlji, tako da *High* gumb uvijek ima pravo prvog prolaska kroz obradu ako dode do sukoba.
2. **Hardverski Timer:** Ugradeni ESP32 prescaler timer koji okida svake dvije sekunde (`2 000 000` mikrosekundi).
3. **HC-SR04 Senzor udaljenosti:** Implementiran ne-blokirajuci nacin citanja koji se oslanja iskljucivo na `CHANGE` vrste ISR prekida (hvatanje oba brida). 
4. **Upravljanje resursima (Kriticne sekcije):** Buduci da timer i senzor dijele vrijeme procesora, citanje njihovih zapisanih *isr* varijabli unutar `loop()`-a zasticeno je FreeRTOS semaforskim funkcijama `portENTER_CRITICAL()` i `portEXIT_CRITICAL()`.

---

## Control Flow Graph (Tijek programa)

Tijek glavnog programa (`loop`) i asinkroni prekidi (`ISR`) prikazani su koristeci Mermaid.js dijagram:

```mermaid
graph TD
    A([Start: Setup inicijalizacija]) --> B(Deklaracija Pinova, Timer-a i AttachInterrupts)
    B --> C((Glavna Petlja - loop))
    
    C --> D{Trig HC-SR04?}
    D -- Da, proslo 500ms --> E[Pusti 10us HIGH Trig signal]
    D -- Ne --> F
    E --> F{Jel upaljen FLAG High Tipke?}
    
    F -- Da --> G[Obrada PRIORITETA 1]
    F -- Ne --> H{Jel upaljen FLAG Med Tipke?}
    
    G --> L
    H -- Da --> I[Obrada PRIORITETA 2]
    H -- Ne --> J{Jel upaljen FLAG Low Tipke?}
    
    I --> L
    J -- Da --> K[Obrada PRIORITETA 3]
    J -- Ne --> L{Dosao Timer FLAG?}
    
    K --> L
    L -- Da --> M[Citanje uz KRITICNU SEKCIJU] --> N{Dosao Senzor Odgovor?}  
    L -- Ne --> N
    
    N -- Da --> O[Ispis udaljenosti i reset FLAG-a] --> C
    N -- Ne --> C
    
    %% --- Ovdje asinkrono pucaju prekidi ---
    subgraph hardverski_prekidi [Hardverski Prekidi i ISR]
        Z1(Pritisak Gumba) --> Z1_ISR[ISR: Postavi volatile flag]
        Z2(Hardverski Timer) --> Z2_ISR[ISR Timer: Postavi Okinuto]
        Z3(Puls s HC-SR04) --> Z3_ISR[ISR Senzor: Mjerenje vremena]
    end
    
    Z1 -. asinkrono mijenja stanje .-> F
    Z2 -. asinkrono .-> L
    Z3 -. izracunata daljina bez delay .-> N
```

---

## Automatizirana Dokumentacija
Sva dokumentacija koda kreirana je koristeci **Doxygen** norme unutar izvornog koda te je automatizirana putem funkcije GitHub Actions.

