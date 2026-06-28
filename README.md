# Water Level Monitoring System
**Bare Metal AVR | Arduino Nano (ATmega328P) | avr-gcc | fără Arduino framework**

> Copyright (c) 2026 Bizera Razvan — vezi [LICENSE](LICENSE)

---

## Structura proiectului

```
senzor nivel/
├── bsp/
│   ├── bsp.h               — Board selector (Nano / Uno)
│   └── nano.h              — Pini fizici, clock, parametri periferice
├── drivers/
│   ├── adc/                — ADC bare metal (AVcc ref, prescaler 128)
│   ├── buzzer/             — Buzzer non-blocking (SLOW / FAST / OFF)
│   ├── eeprom/             — EEPROM: persistenta setpoint-uri intre reporniri
│   ├── gpio/               — Abstractie GPIO (macros + init pini)
│   ├── i2c/                — TWI master polling (400kHz)
│   ├── interrupt/          — INT0 cu callback inregistrabil
│   ├── ssd1306/            — Driver OLED SSD1306, font 5x7 PROGMEM, text 2x
│   ├── timer/              — Timer0 CTC 1ms → millis()
│   └── uart/               — USART0: TX polling, RX interrupt + buffer circular
├── src/
│   ├── app_config.h        — Parametri aplicatie (calibrare, setpoint, timing)
│   ├── main.c              — Entry point (~30 linii)
│   ├── water_app.h
│   └── water_app.c         — Logica completa: control, display, serial, EEPROM
├── utils/
│   ├── delay.h / delay.c   — delay_ms() bazat pe millis()
│   └── utils.h             — Macros: BIT_SET, CLAMP, ELAPSED, ARRAY_SIZE
├── .gitignore
├── LICENSE
├── Makefile
└── README.md
```

---

## Hardware

| Componentă | Model | Conexiune |
|---|---|---|
| Microcontroller | Arduino Nano (ATmega328P @ 16MHz) | — |
| Senzor nivel | HW-038 (analogic) | A0 (ADC0) |
| Display | OLED 0.96" SSD1306 | SDA=A4, SCL=A5 (I2C) |
| Releu pompă | 1 canal 5V | D7 (PD7) |
| Buzzer | Activ 5V | D6 (PD6) |
| Buton MODE | Push button | D2 (INT0, falling edge) |
| Buton UP | Push button | D4 |
| Buton DOWN | Push button | D5 |

> Butoanele folosesc pull-up intern. Apăsare = scurtcircuit la GND.

---

## Schema conexiuni

```
Arduino Nano
├── A0          ──── HW-038 (pin S)
├── D7          ──── Releu IN
├── D6          ──── Buzzer +
├── D2 (INT0)   ──── Buton MODE → GND
├── D4          ──── Buton UP   → GND
├── D5          ──── Buton DOWN → GND
├── A4 (SDA)    ──── OLED SDA
├── A5 (SCL)    ──── OLED SCL
├── 5V          ──── VCC comune
└── GND         ──── GND comun
```

---

## Instalare toolchain (Windows)

**1. avr-gcc** — descarcă de la [zakkemble.net/avr-gcc-builds](https://blog.zakkemble.net/avr-gcc-builds/)
   - Dezarhivează în `C:\avr-gcc\`
   - Adaugă `C:\avr-gcc\bin` în PATH (System → Environment Variables)

**2. Verificare:**
```
avr-gcc --version
avrdude --version
```

**3. VS Code Extensions:**
- C/C++ (Microsoft) — IntelliSense

---

## Compilare și flash

```bash
# Modifica portul COM in Makefile (PORT = COMx)
# Gaseste portul: Device Manager → Ports → USB-SERIAL CH340

make          # compileaza
make flash    # upload pe Nano
make size     # consum flash/RAM
make clean    # curata build/
make disasm   # dezasamblare in water_level.lst
```

**Din VS Code:** `Ctrl+Shift+B` → alege Build / Flash / Clean

---

## Calibrare senzor HW-038

Fă asta înainte de primul flash real:

1. Setează în `src/app_config.h`: `SENSOR_DRY 0`, `SENSOR_WET 1023`
2. Flash → deschide Serial Monitor @ 9600 baud
3. Notează valoarea `"raw"` cu senzorul **uscat** → pune în `SENSOR_DRY`
4. Notează valoarea `"raw"` cu senzorul **complet în apă** → pune în `SENSOR_WET`
5. Re-flash

---

## Funcționalități

### Control pompă (histerezis)
```
MOD AUTO:
  nivel ≤ SP_LOW   și pompă oprită  →  pornește pompa
  nivel ≥ SP_HIGH  și pompă pornită →  oprește pompa

MOD MANUAL:
  pompa controlată exclusiv din butoane / serial
```

### Persistență EEPROM
Setpoint-urile (SP_LOW / SP_HIGH) se salvează automat în EEPROM la orice modificare — prin butoane sau comenzi serial. La repornire, valorile sunt restaurate fără intervenție.

### Buzzer (non-blocking)
| Condiție | Mod |
|---|---|
| Nivel ≤ 8% | FAST: 80ms ON / 120ms OFF |
| Nivel ≥ 95% | SLOW: 200ms ON / 800ms OFF |
| Normal | Silențios |

### Display OLED (128×64, page mode)
```
+--------------------------------+
| NIVEL APA          [AUTO]      |  Page 0
|  85%                           |  Page 1-2 (font 2x)
| POMPA: ON          OK          |  Page 3
| ████████████░░░|░░░░░░░░░░░░  |  Page 4 (bară + markeri SP)
| SP:25-80%                      |  Page 5
+--------------------------------+
```

---

## Protocol Serial (9600 baud, 8N1)

### Date trimise de Arduino (la fiecare 500ms):
```json
{"level":45.3,"pump":false,"mode":"AUTO","sp_low":25,"sp_high":80,"raw":512,"alert":"OK"}
```

### Comenzi acceptate de la PC:
| Comandă | Efect |
|---|---|
| `PUMP_ON` | Pornește pompa (mod manual) |
| `PUMP_OFF` | Oprește pompa |
| `MODE_AUTO` | Comutare în mod automat |
| `MODE_MANUAL` | Comutare în mod manual |
| `SP_LOW:25` | Setpoint pornire (5-90%) |
| `SP_HIGH:80` | Setpoint oprire (10-95%) |

---

## Registre AVR utilizate

| Periferic | Registre | Scop |
|---|---|---|
| USART0 | UBRR0, UCSR0A/B/C, UDR0 | UART 9600 baud |
| TWI | TWBR, TWSR, TWCR, TWDR | I2C 400kHz |
| ADC | ADMUX, ADCSRA, ADCL, ADCH | Citire HW-038 |
| Timer0 | TCCR0A/B, OCR0A, TIMSK0 | millis() 1ms |
| INT0 | EICRA, EIMSK | Buton MODE |
| EEPROM | EEAR, EEDR, EECR | Persistență setpoint-uri |
| GPIO | DDRB/C/D, PORT/PIN | Releu, buzzer, butoane |
