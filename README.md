# Sistem de Monitorizare Nivel Apă — Bare Metal AVR
**ATmega328P (Arduino Nano) | avr-gcc | fără Arduino framework**

---

## Structura proiectului

```
senzor nivel/
├── src/
│   ├── main.c          — Logica principala, loop, control histerezis
│   ├── uart.c          — Driver USART0 (TX polling, RX interrupt + buffer)
│   ├── i2c.c           — Driver TWI/I2C master (polling)
│   ├── adc.c           — Driver ADC (single shot + mediere)
│   ├── timer.c         — Timer0 CTC 1ms (millis)
│   └── ssd1306.c       — Driver OLED SSD1306, font 5x7 PROGMEM, 2x scale
├── include/
│   ├── config.h        — Pini, constante, calibrare (editeaza aici)
│   ├── uart.h
│   ├── i2c.h
│   ├── adc.h
│   ├── timer.h
│   └── ssd1306.h
├── .vscode/
│   ├── c_cpp_properties.json   — IntelliSense AVR
│   ├── tasks.json               — Build/Flash/Clean din VS Code
│   └── settings.json
├── Makefile
└── README.md
```

---

## 1. Instalare toolchain (Windows)

### Optiunea A — avr-gcc de la Zak Kemble (recomandat)
1. Descarca de la: https://blog.zakkemble.net/avr-gcc-builds/
   → alege ultima versiune `avr-gcc-XX.X.X-x64-windows.zip`
2. Dezarchiveaza in `C:\avr-gcc\`
3. Adauga `C:\avr-gcc\bin` in PATH:
   - Win+R → `sysdm.cpl` → Advanced → Environment Variables
   - System variables → `Path` → Edit → New → `C:\avr-gcc\bin`
4. Verifica in terminal nou:
   ```
   avr-gcc --version
   avrdude --version
   ```

### Optiunea B — Scoop (package manager)
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
irm get.scoop.sh | iex
scoop install avr-gcc
```

### Extensii VS Code necesare
- **C/C++** (Microsoft) — IntelliSense
- **Makefile Tools** (Microsoft) — optional dar util

---

## 2. Schema conexiuni

```
Arduino Nano              Periferic
─────────────────────────────────────
A0  (PC0)          ───── HW-038 S (semnal analogic)
D7  (PD7)          ───── Releu IN
D6  (PD6)          ───── Buzzer + (activ)
D2  (PD2 / INT0)   ───── Buton MODE  → GND
D4  (PD4)          ───── Buton UP    → GND
D5  (PD5)          ───── Buton DOWN  → GND
A4  (PC4 / SDA)    ───── OLED SDA
A5  (PC5 / SCL)    ───── OLED SCL
5V                 ───── HW-038 VCC, Releu VCC, Buzzer VCC, OLED VCC
GND                ───── Toate GND-urile comune

Pompa (12V DC recomandat):
  Sursa 12V (+) → COM releu → NO releu → Pompa (+) → Pompa (-) → Sursa 12V (-)
```

> Butoanele folosesc pull-up intern (INPUT_PULLUP echivalent in registre).
> Apasare buton = scurt-circuit la GND.

---

## 3. Calibrare senzor HW-038

**Inainte de primul upload:**

1. Deschide `include/config.h`
2. Seteaza valori temporare: `SENSOR_DRY 0`, `SENSOR_WET 1023`
3. Compileaza + flash → deschide Serial Monitor @ 9600 baud
4. Noteaza valoarea `"raw"` din JSON cand senzorul e **uscat** → `SENSOR_DRY`
5. Noteaza valoarea `"raw"` cand senzorul e **complet scufundat** → `SENSOR_WET`
6. Pune valorile reale in `config.h` si re-flash

---

## 4. Compilare si Flash

### Din VS Code
- **Ctrl+Shift+B** → `Build` (compileaza)
- **Ctrl+Shift+B** → `Flash` (upload pe Nano)
- **Ctrl+Shift+B** → `Clean` (sterge fisierele build)

### Din terminal
```bash
# Modifica portul in Makefile (PORT = COMx)
make              # compileaza
make flash        # upload
make size         # afiseaza dimensiuni flash/RAM
make clean        # curatare
make disasm       # dezasamblare in water_level.lst
```

### Gasire port COM
Device Manager → Ports (COM & LPT) → `USB-SERIAL CH340 (COMx)`

---

## 5. Consum resurse ATmega328P

Estimat dupa compilare (`make size`):

| Sectiune | Utilizare estimata | Disponibil |
|---|---|---|
| Flash (program) | ~6-8 KB | 32 KB |
| SRAM (variabile) | ~300-400 B | 2 KB |
| EEPROM | 0 | 1 KB |

---

## 6. Protocol Serial (9600 baud, 8N1)

### Date trimise de Arduino (la fiecare 500ms):
```json
{"level":45.3,"pump":false,"mode":"AUTO","sp_low":25,"sp_high":80,"raw":512,"alert":"OK"}
```

| Camp | Tip | Descriere |
|---|---|---|
| `level` | float 1 dec | Nivel % (0.0 – 100.0) |
| `pump` | bool | Stare releu pompa |
| `mode` | string | "AUTO" sau "MANUAL" |
| `sp_low` | int | Setpoint pornire pompa |
| `sp_high` | int | Setpoint oprire pompa |
| `raw` | uint | Valoare ADC bruta (0-1023) |
| `alert` | string | "OK", "CRIT_LOW", "CRIT_HIGH" |

### Comenzi acceptate de Arduino (trimise de PC, terminate cu `\n`):
```
PUMP_ON          — pornire manuala pompa
PUMP_OFF         — oprire manuala pompa
MODE_AUTO        — comutare in mod automat
MODE_MANUAL      — comutare in mod manual (opreste pompa)
SP_LOW:25        — seteaza setpoint pornire (valoare 5-90)
SP_HIGH:80       — seteaza setpoint oprire (valoare 10-95)
```

---

## 7. Display OLED (layout)

```
+--------------------------------+
| NIVEL APA          [AUTO]      |  <- Page 0
|                                |
|  85%                           |  <- Page 1-2 (font 2x, 14px inaltime)
|                                |
| POMPA: ON          OK          |  <- Page 3
| ████████████░░░░|░░░░░░░░░░░  |  <- Page 4 (bara + markeri setpoint)
| SP:25-80%                      |  <- Page 5
+--------------------------------+
```

---

## 8. Logica de control (histerezis)

```
MOD AUTO:
  daca nivel <= SP_LOW  si pompa oprita  →  porneste pompa
  daca nivel >= SP_HIGH si pompa pornita →  opreste pompa

MOD MANUAL:
  pompa controlata exclusiv din butoane hardware sau comenzi serial
  comutare AUTO→MANUAL opreste automat pompa

ALERTE BUZZER (non-blocking, pe Timer0 millis):
  nivel <= 8%   → beep rapid (80ms ON / 120ms OFF)
  nivel >= 95%  → beep lent  (200ms ON / 800ms OFF)
```

---

## 9. Registre AVR utilizate (referinta)

| Periferic | Registre | Scop |
|---|---|---|
| USART0 | UBRR0, UCSR0A/B/C, UDR0 | UART 9600 baud |
| TWI | TWBR, TWSR, TWCR, TWDR | I2C 400kHz |
| ADC | ADMUX, ADCSRA, ADCL, ADCH | Citire HW-038 |
| Timer0 | TCCR0A/B, OCR0A, TIMSK0 | millis() 1ms tick |
| INT0 | EICRA, EIMSK | Buton MODE falling edge |
| GPIO | DDRB/C/D, PORT/PIN | Releu, Buzzer, Butoane |
