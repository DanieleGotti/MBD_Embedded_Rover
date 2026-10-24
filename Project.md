# MBD_Embedded_Rover — documento di progetto

**Rover da mappatura telecomandato via Bluetooth da un'app Android. Firmware C bare-metal su ATmega328P + logica di veicolo sviluppata in Simulink e generata con Embedded Coder.**

L'operatore guida il rover dal telefono. Quando il rover è fermo, su richiesta esegue una scansione con l'ultrasuono montato sul servo e invia i punti all'app, che costruisce una mappa 2D a colori della stanza. Le mappe si salvano e si riaprono.

Il software del rover è organizzato come un'ECU automotive:

- **Basic Software (BSW)** scritto a mano in C, senza IDE né core Arduino: driver a registri, scheduler, protocollo di comunicazione, diagnostica.
- **Application Software (ASW)** sviluppato in Simulink e generato con Embedded Coder: macchina a stati del veicolo, controllo del moto, frenata automatica d'emergenza, stima della posizione, sequenza di scansione.
- Un livello **RTE** separa i due mondi tramite segnali con tipo, unità e range definiti.

Il flusso segue il V-model: requisiti → modello della pianta → progetto della logica → MIL → SIL → PIL → test sul veicolo.

---

## Indice

1. [Architettura di sistema](#1-architettura-di-sistema)
2. [Strumenti](#2-strumenti)
3. [Hardware](#3-hardware)
4. [Elettronica](#4-elettronica)
5. [Procedura di accensione e spegnimento](#5-procedura-di-accensione-e-spegnimento)
6. [Macchina a stati del veicolo](#6-macchina-a-stati-del-veicolo)
7. [Funzioni di guida e scansione](#7-funzioni-di-guida-e-scansione)
8. [Allocazione delle risorse del microcontrollore](#8-allocazione-delle-risorse-del-microcontrollore)
9. [Architettura del firmware](#9-architettura-del-firmware)
10. [Task e scheduling](#10-task-e-scheduling)
11. [Protocollo di comunicazione](#11-protocollo-di-comunicazione)
12. [App Android](#12-app-android)
13. [Workflow Model-Based Design](#13-workflow-model-based-design)
14. [Calibrazione e validazione](#14-calibrazione-e-validazione)
15. [Requisiti](#15-requisiti)
16. [Struttura del repository](#16-struttura-del-repository)
17. [Fasi del progetto](#17-fasi-del-progetto)
18. [Convenzioni](#18-convenzioni)

---

## 1. Architettura di sistema

```
┌──────────────────────────────┐   Bluetooth SPP    ┌───────────────────────────────────┐
│ App Android (Flutter)        │ ◄────────────────► │ Rover — ATmega328P                │
│                              │   frame binari     │                                   │
│ · procedure accensione/spegn.│   + CRC8           │ ASW (Embedded Coder)              │
│ · comandi di guida (10 Hz)   │                    │ · macchina a stati veicolo        │
│ · mappa a griglia a colori   │                    │ · controllo moto + compensazione  │
│ · salvataggio mappe          │                    │ · frenata automatica (AEB)        │
│ · calibrazioni               │                    │ · stima posizione (dead reckoning)│
└──────────────────────────────┘                    │ · sequenza di scansione           │
                                                    │ BSW (C bare-metal)                │
                                                    │ · driver, scheduler, protocollo   │
                                                    └───────────────────────────────────┘
```

| Rover | App |
|---|---|
| Esegue i comandi in sicurezza: decide lui se un comando è accettabile | Interfaccia utente, nessuna logica di sicurezza |
| Stima la propria posizione (x, y, θ) | Costruisce e disegna la mappa |
| Invia posizione, stato e punti di scansione | Invia comandi periodici, che fanno anche da heartbeat |

La mappa vive sul telefono perché l'ATmega328P ha solo 2 KB di RAM.

---

## 2. Strumenti

| Strumento | Uso | Stato |
|---|---|---|
| VS Code + PlatformIO | Build, upload, monitor seriale | Installato |
| Piattaforma PlatformIO `atmelavr` | avr-gcc, avr-libc, avrdude | Da installare: `pio pkg install -g --platform atmelavr` |
| MATLAB R2024b: Simulink, Simulink Coder, Embedded Coder, Simulink Test, System Identification, Control System | Modellazione, generazione codice, test | Installati |
| Stateflow | Macchina a stati del veicolo | Da verificare la licenza; in alternativa Simulink + MATLAB Function |
| Simulink Support Package for Arduino Hardware | PIL sull'ATmega328P | Da installare (Add-On gratuito) |
| Flutter + Android SDK | App | Installato |
| Tinkercad Circuits | Verifica del cablaggio e test rapidi | Online |
| Wokwi (estensione VS Code) | Simulazione del firmware bare-metal, analizzatore logico virtuale | Da installare |
| Multimetro, metro, goniometro di carta | Calibrazione e misure | |
| Analizzatore logico 8 canali + PulseView | Timing di task e segnali | Opzionale, consigliato |

---

## 3. Hardware

### Componenti in uso

| Componente | Modello | Note |
|---|---|---|
| Microcontrollore | Arduino Uno R3 (ATmega328P DIP, 16 MHz, 32 KB flash, 2 KB RAM, 1 KB EEPROM) | Usato senza core Arduino |
| Telaio | Base 2WD acrilica con ruota pivotante | |
| Motori | 2× motoriduttore DC "TT" | Dischi encoder a 20 fori già montati, forcelle assenti |
| Ponte H | TI L293DNE (DIP-16, 600 mA/canale, diodi di ricircolo interni) | Su breadboard |
| Distanza | HC-SR04 | Montato sul servo, inclinato di qualche grado verso il suolo |
| Servo | Micro servo 180° | Scansione dell'ultrasuono |
| Bluetooth | HC-05 (Bluetooth classico, SPP) | Solo Android |
| Alimentazione logica (LV) | Pila 9 V | Su Vin |
| Alimentazione motori (TS) | 4× AA, 6 V nominali | Motori e servo |
| Condensatori | 100 nF film poliestere (µ1J63), 100 µF elettrolitico | |
| Diodo | 1N4007 (o equivalente 1N400x) | Riduce la tensione del servo |
| Resistenze | 1 kΩ, 2 kΩ, 2× 10 kΩ | Partitori |

### Non usati in questa versione
Sensore IR ostacoli (forse guasto), Line Tracker, moduli del kit AZ-Delivery non pertinenti.

### Estensioni future
| Componente | Scopo | Pin riservati |
|---|---|---|
| 2× forcella ottica LM393 | Velocità delle ruote, odometria | D2, D3 |
| MPU6050 (GY-521) | Velocità di imbardata, fusione sensori | A4, A5 (I²C) |

---

## 4. Elettronica

Nessuna saldatura: breadboard, jumper e morsetti.

### 4.1 Domini di alimentazione

Come su un veicolo elettrico, l'alimentazione è separata in due domini:

| Dominio | Sorgente | Alimenta |
|---|---|---|
| **LV** (logica) | Pila 9 V → Vin → regolatore Arduino → 5 V | ATmega328P, logica L293D (pin 16), HC-SR04, HC-05 |
| **TS** (trazione) | 4× AA, 6 V | Motori (L293D pin 8), servo (tramite diodo), partitore di misura |

Le masse dei due domini sono collegate: tutti i segnali hanno lo stesso riferimento.

```
 Pila 9 V (+) ─────────────────────────► Arduino Vin
 Pila 9 V (−) ─────────────────────────► Arduino GND ─┐
                                                       │
 Arduino 5V ──► linea +5V (sopra)                      │
 Arduino GND ─► linea GND (sopra) ──────┬──────────────┘
                                        │ massa comune
 4×AA (−) ────► linea GND (sotto) ──────┘
 4×AA (+) ────► linea +6V (sotto) ──┬──► L293D pin 8 (Vcc2)
                                    ├──► 1N4007 ──► servo (+)      (≈5,7 V)
                                    ├──► 10 kΩ ─┬─ 10 kΩ ─► GND
                                    │           └──► A2 (VBAT_SENSE = VBAT/2)
                                    └──► 100 µF ─► GND
 +5V ──► 100 nF ──► GND   (vicino al pin 16 del L293D)
```

### 4.2 Collegamenti dell'Arduino

| Pin | Funzione | Periferica | Collegato a |
|---|---|---|---|
| D0 | RX | USART0 | HC-05 TXD (da staccare durante l'upload) |
| D1 | TX | USART0 | Partitore 1 kΩ / 2 kΩ → HC-05 RXD |
| D2 | libero | INT0 | Riservato: encoder sinistro |
| D3 | libero | INT1 | Riservato: encoder destro |
| D4 | Direzione motore sx | GPIO | L293D pin 7 (IN2) |
| D5 | PWM motore dx | Timer0 OC0B | L293D pin 9 (EN3,4) |
| D6 | PWM motore sx | Timer0 OC0A | L293D pin 1 (EN1,2) |
| D7 | Direzione motore sx | GPIO | L293D pin 2 (IN1) |
| D8 | Echo ultrasuoni | PCINT0 + TCNT1 | HC-SR04 ECHO |
| D9 | Comando servo | Timer1 OC1A | Servo segnale |
| D10 | Direzione motore dx | GPIO | L293D pin 10 (IN3) |
| D11 | Direzione motore dx | GPIO | L293D pin 15 (IN4) |
| D12 | Trigger ultrasuoni | GPIO | HC-SR04 TRIG |
| D13 | LED di stato | GPIO | LED sulla scheda |
| A2 | Tensione pile AA | ADC2 | Partitore 10 kΩ / 10 kΩ |
| A0, A1, A3 | liberi | | |
| A4, A5 | liberi | I²C | Riservati: IMU |

### 4.3 L293D sulla breadboard
Chip a cavallo della scanalatura centrale, tacca a sinistra.

| Piedino | Nome | Collegato a |
|---|---|---|
| 1 | Enable 1,2 | D6 |
| 2 | Input 1 | D7 |
| 3 | Output 1 | Motore sinistro |
| 4, 5 | GND | Massa |
| 6 | Output 2 | Motore sinistro |
| 7 | Input 2 | D4 |
| 8 | Vcc2 | +6 V (TS) |
| 9 | Enable 3,4 | D5 |
| 10 | Input 3 | D10 |
| 11 | Output 3 | Motore destro |
| 12, 13 | GND | Massa |
| 14 | Output 4 | Motore destro |
| 15 | Input 4 | D11 |
| 16 | Vcc1 | +5 V (LV) |

### 4.4 Moduli

| Modulo | Pin | Collegato a |
|---|---|---|
| HC-SR04 | VCC / TRIG / ECHO / GND | +5 V / D12 / D8 / GND |
| Servo | GND / + / segnale | GND / +6 V tramite 1N4007 (striscia verso il servo) / D9 |
| HC-05 | VCC / GND / TXD / RXD | +5 V / GND / D0 / nodo del partitore (D1 → 1 kΩ → nodo → 2 kΩ → GND) |
| HC-05 | STATE, EN | Non collegati |

### 4.5 Motivazioni di progetto

| Scelta | Motivo |
|---|---|
| Ponte H tra Arduino e motori | Un pin dell'ATmega regge al massimo 40 mA, un motore TT assorbe 200–800 mA |
| Domini LV e TS separati | I picchi di corrente dei motori farebbero cadere la tensione della logica (reset da brown-out). 4× AA su Vin darebbero meno dei 7 V richiesti dal regolatore |
| Servo sul dominio TS | Picchi fino a ~500 mA: sul regolatore dell'Arduino (9 V → 5 V) dissiperebbero circa 2 W |
| Diodo in serie al servo | Pile nuove ≈ 6,4 V, servo specificato fino a 6 V; la caduta di ≈ 0,7 V porta a ≈ 5,7 V |
| 100 µF sulla linea TS | Riserva locale per gli spunti dei motori: con 0,5 A per 1 ms il calo è ΔV = I·Δt/C ≈ 1 V invece di un crollo della linea |
| 100 nF su Vcc1 e sui morsetti dei motori | Filtrano i disturbi ad alta frequenza delle commutazioni e delle spazzole |
| Partitore 1 kΩ / 2 kΩ su HC-05 RXD | L'ingresso dell'HC-05 lavora a 3,3 V: 5 V · 2/3 = 3,33 V |
| Partitore 10 kΩ / 10 kΩ su A2 | Porta 6,4 V a 3,2 V, dentro il range dell'ADC. Serve per la compensazione della velocità e per rilevare la presenza del TS |
| Nessuna resistenza tra Arduino e L293D / HC-SR04 | Stesso livello logico (5 V), ingressi ad alta impedenza: non c'è corrente da limitare |

Lo schema a etichette di rete è in [hardware/wiring.svg](hardware/wiring.svg).

---

## 5. Procedura di accensione e spegnimento

Non c'è un interruttore generale: i domini si collegano e scollegano a mano, nell'ordine indicato dall'app. L'ordine è quello di un veicolo elettrico: prima la logica, poi la trazione; allo spegnimento il contrario.

Per scollegare un dominio si toglie la clip della pila 9 V o **una pila dal portapile AA**, mai un jumper dalla breadboard: un filo sotto tensione lasciato libero può toccare la linea sbagliata.

### Accensione
1. Verificare che le pile AA siano scollegate.
2. **LV ON**: collegare la pila 9 V. Il LED D13 lampeggia: rover in attesa di connessione.
3. Avviare la connessione dall'app. Handshake riuscito: LED D13 acceso fisso.
4. **TS ON**: inserire le pile AA. Il rover rileva la tensione su A2 e la invia all'app.
5. Con STOP attivo, premere **R2D**. Il servo esegue un breve movimento di conferma ("R2D signal") e la guida è abilitata.

### Spegnimento
1. Premere **Spegni**: il rover si ferma, disabilita i motori e centra il servo.
2. **TS OFF**: togliere una pila AA. L'app conferma che la tensione su A2 è scesa a zero.
3. **LV OFF**: scollegare la pila 9 V.
4. L'app chiede se salvare la mappa.

---

## 6. Macchina a stati del veicolo

Modellata in Stateflow (o Simulink) e generata con Embedded Coder.

```
            ┌────────┐ handshake ok  ┌────────┐ VBAT in range ┌────────┐
 reset ───► │ LV_ON  │ ────────────► │ LINKED │ ────────────► │ TS_ON  │
            └────────┘               └────────┘               └────────┘
                 ▲                        ▲                        │ R2D richiesto
                 │ link perso             │ TS perso               │ e STOP attivo
                 │                        │                        ▼
            ┌────────┐               ┌──────────┐  SCAN       ┌────────┐
            │SHUTDOWN│ ◄──────────── │ SCANNING │ ◄────────── │  R2D   │
            └────────┘  richiesta    └──────────┘  fine scan ►└────────┘
                         spegnimento                               │ guasto
                                                                   ▼
                                                              ┌────────┐
                                                              │ FAULT  │
                                                              └────────┘
```

| Stato | Motori | Servo | Uscita verso |
|---|---|---|---|
| `LV_ON` | Disabilitati | Centro | `LINKED` a handshake riuscito |
| `LINKED` | Disabilitati | Centro | `TS_ON` se VBAT > soglia; `LV_ON` se il link cade |
| `TS_ON` | Disabilitati | Centro | `R2D` su richiesta, con STOP attivo |
| `R2D` | Abilitati | Avanti (AEB) | `SCANNING` su SCAN a veicolo fermo; `FAULT` su guasto; `SHUTDOWN` su richiesta |
| `SCANNING` | Disabilitati | Scansione | `R2D` a fine scansione |
| `FAULT` | Disabilitati | Centro | `TS_ON` dopo conferma dall'app e causa rimossa |
| `SHUTDOWN` | Disabilitati | Centro | Attende lo scollegamento del TS |

Cause di `FAULT`: link perso per più di 500 ms, TS scollegato durante la guida, VBAT sotto soglia, overrun ripetuti dello scheduler.

---

## 7. Funzioni di guida e scansione

### Comandi
| Comando | Effetto |
|---|---|
| STOP | Velocità zero (con rampa) |
| LENTO / VELOCE | Velocità lineare costante, due livelli calibrabili (indicativamente 10 e 20 cm/s) |
| RETRO | Retromarcia, solo a velocità lenta |
| SINISTRA / DESTRA (tenuti premuti) | Da fermo: rotazione sul posto. In marcia: curva. Si ruota finché il pulsante è premuto |
| SCAN | Solo a veicolo fermo: scansione da −60° a +60° |

### Controllo del moto
- Il comando (v, ω) viene convertito in velocità delle ruote: v_sx = v − ω·L/2, v_dx = v + ω·L/2.
- Senza sensori di velocità il controllo è in **anello aperto** con mappa calibrata velocità → PWM, compensata con la tensione delle pile: PWM ∝ v_rif / VBAT.
- Rampe di accelerazione per limitare spunti e slittamenti.

### Frenata automatica d'emergenza (AEB)
In marcia avanti l'ultrasuono resta puntato in avanti e misura ogni 60 ms. Se la distanza scende sotto una soglia dipendente dalla velocità (ad esempio 15 cm a LENTO, 25 cm a VELOCE), il rover si ferma e segnala l'intervento all'app. È l'unico anello chiuso su una misura reale della versione base.

### Stima della posizione
Dead reckoning sul modello cinematico dell'uniciclo, integrando le velocità stimate dai comandi:

x ← x + v·cos θ·Δt,  y ← y + v·sin θ·Δt,  θ ← θ + ω·Δt

L'errore cresce nel tempo: la qualità dipende dalla calibrazione (sezione 14).

### Scansione
1. Il rover è fermo; i motori sono disabilitati.
2. Il servo si porta all'angolo iniziale e attende l'assestamento.
3. Per ogni angolo da −60° a +60° a passi di 5°: attesa di assestamento, misura (mediana di 3 letture), invio del punto con la posizione del rover.
4. Fine scansione: servo al centro, ritorno allo stato `R2D`.

Durata indicativa: 25 angoli × ~100 ms ≈ 2,5 s.

---

## 8. Allocazione delle risorse del microcontrollore

| Risorsa | Configurazione | Uso |
|---|---|---|
| Timer0 (8 bit) | PWM phase-correct, prescaler 8 → ≈ 3,9 kHz | PWM dei motori (OC0A, OC0B) |
| Timer1 (16 bit) | Fast PWM modo 14, TOP = ICR1 = 39999, prescaler 8 → 0,5 µs/tick, periodo 20 ms | Impulso servo su OC1A; TCNT1 come base dei tempi per l'eco (letto nell'interrupt PCINT0 di D8) |
| Timer2 (8 bit) | CTC, prescaler 64, OCR2A = 249 → 1 ms | Tick dello scheduler |
| USART0 | 9600 baud 8N1 (default HC-05), poi 38400 configurando l'HC-05 con comandi AT | Protocollo con l'app |
| ADC | Riferimento AVcc, prescaler 128 | Tensione pile su ADC2 |
| EEPROM | 1 KB | Calibrazioni e log degli eventi |
| Watchdog | 60 ms | Reset su blocco del software |

Budget indicativi da misurare: RAM < 1,5 KB, flash < 24 KB, carico CPU < 60 %.

---

## 9. Architettura del firmware

```
┌──────────────────────────────────────────────────────────────┐
│ ASW — generato da Simulink / Embedded Coder                  │
│  VehicleStateMachine · MotionCtrl · Aeb · DeadReckoning      │
│  ScanSequencer                                                │
├──────────────────────────────────────────────────────────────┤
│ RTE — rte.h / rte.c: segnali con tipo, unità, range          │
├──────────────────────────────────────────────────────────────┤
│ Services: Scheduler · Comm (protocollo) · Calib (EEPROM)     │
│           Diag · Logger                                       │
├──────────────────────────────────────────────────────────────┤
│ ECU abstraction: Motor · Servo · Ultrasonic · Battery · Led  │
├──────────────────────────────────────────────────────────────┤
│ MCAL: gpio · timer · pwm · icp · adc · uart · eeprom · wdt   │
├──────────────────────────────────────────────────────────────┤
│ ATmega328P                                                    │
└──────────────────────────────────────────────────────────────┘
```

Principi: ogni livello chiama solo quello sotto; il codice generato non conosce registri né pin; le ISR fanno il minimo; nessuna allocazione dinamica.

### Segnali principali dell'RTE

| Segnale | Dir. ASW | Tipo | Unità |
|---|---|---|---|
| `cmd_speed_mode` | In | `uint8` enum | STOP / LENTO / VELOCE / RETRO |
| `cmd_turn` | In | `int8` | −1 sinistra, 0, +1 destra |
| `cmd_scan_req`, `cmd_r2d_req`, `cmd_shutdown_req` | In | `boolean` | |
| `link_ok` | In | `boolean` | Comando valido ricevuto negli ultimi 500 ms |
| `v_batt` | In | `single` | V |
| `dist_front` | In | `single` | m (valore invalido se nessun eco) |
| `scan_dist` | In | `single` | m |
| `pwm_l`, `pwm_r` | Out | `int16` | −255…+255 |
| `servo_angle_cmd` | Out | `single` | deg |
| `veh_state` | Out | `uint8` enum | |
| `pose_x`, `pose_y`, `pose_theta` | Out | `single` | m, m, rad |
| `aeb_active` | Out | `boolean` | |
| `scan_point_valid` | Out | `boolean` | |

---

## 10. Task e scheduling

Scheduler cooperativo a tick da 1 ms (Timer2).

| Task | Periodo | Contenuto |
|---|---|---|
| ISR tick | 1 ms | Incrementa il tick, segnala i task pronti |
| `Task_10ms` | 10 ms | Ricezione e parsing dei frame, aggiornamento PWM e servo |
| `Task_20ms` | 20 ms | `step()` dei modelli generati |
| `Task_60ms` | 60 ms | Trigger e lettura dell'ultrasuono |
| `Task_100ms` | 100 ms | Invio di stato e posizione, tensione pile, diagnostica, LED |
| Background | – | Invio dei punti di scansione in coda, refresh del watchdog |

Ogni task alza un pin di debug libero all'inizio e lo abbassa alla fine per misurare periodo, jitter e durata.

---

## 11. Protocollo di comunicazione

Frame binari little-endian su UART (Bluetooth SPP):

```
┌──────┬──────┬──────┬──────┬───────────────┬──────┐
│ 0xAA │ 0x55 │  ID  │ LEN  │ PAYLOAD (LEN) │ CRC8 │
└──────┴──────┴──────┴──────┴───────────────┴──────┘
CRC8: polinomio 0x07, init 0x00, calcolato su ID + LEN + PAYLOAD
```

| ID | Direzione | Nome | Payload | Frequenza |
|---|---|---|---|---|
| 0x01 | App → Rover | `HELLO` | versione protocollo | All'avvio |
| 0x02 | Rover → App | `HELLO_ACK` | versione firmware, versione protocollo | Risposta |
| 0x10 | App → Rover | `DRIVE_CMD` | modo velocità, verso di rotazione | 10 Hz (heartbeat) |
| 0x11 | App → Rover | `R2D_REQ` | – | Evento |
| 0x12 | App → Rover | `SCAN_REQ` | angolo min, max, passo | Evento |
| 0x13 | App → Rover | `SHUTDOWN_REQ` | – | Evento |
| 0x14 | App → Rover | `FAULT_ACK` | – | Evento |
| 0x20 | App → Rover | `SET_CALIB` | id parametro, valore | Evento |
| 0x21 | App → Rover | `GET_CALIB` | id parametro | Evento |
| 0x30 | Rover → App | `STATUS` | stato, VBAT, flag (AEB, guasti), overrun | 5 Hz |
| 0x31 | Rover → App | `POSE` | x, y, θ, timestamp | 10 Hz |
| 0x32 | Rover → App | `SCAN_POINT` | id scansione, angolo, distanza, posa | Durante la scansione |
| 0x33 | Rover → App | `SCAN_DONE` | id scansione, numero punti | Fine scansione |
| 0x34 | Rover → App | `CALIB` | id parametro, valore | Risposta |
| 0x7F | Entrambi | `ACK` / `NACK` | id messaggio, codice | Risposta |

Banda stimata: < 300 B/s, contro ~960 B/s disponibili a 9600 baud.

Il parser è una macchina a stati che si risincronizza sui byte di start in caso di frame corrotti. Durante lo sviluppo lo stesso protocollo passa dal cavo USB: un tool Python sul PC sostituisce l'app.

---

## 12. App Android

Flutter, con il minimo di dipendenze esterne.

| Necessità | Soluzione |
|---|---|
| Bluetooth classico (SPP) | Package Flutter per seriale Bluetooth: da scegliere verificando che sia mantenuto e compatibile con i permessi di Android 12+ |
| Mappa | `CustomPainter` + `InteractiveViewer` (inclusi in Flutter): zoom, pan, colori |
| Salvataggio | File JSON nella cartella documenti dell'app |

### Schermate
1. **Home**: elenco delle mappe salvate, "Nuova sessione", "Apri mappa".
2. **Accensione**: checklist della procedura (sezione 5) con lo stato letto dal rover: link, tensione TS. Il pulsante **R2D** si abilita solo quando tutte le condizioni sono soddisfatte.
3. **Guida**: mappa al centro; sotto, i comandi (STOP, LENTO, VELOCE, RETRO, ◀ ▶ da tenere premuti, SCAN); barra di stato con stato del veicolo, tensione delle pile, qualità del link e indicatore AEB.
4. **Spegnimento**: checklist inversa, poi la richiesta di salvare la mappa con un nome.

### Mappa
- **Griglia di occupazione** (celle da 5 cm) con punteggio log-odds per cella:
  - la cella in cui cade l'eco aumenta il punteggio (occupata);
  - le celle attraversate dal fascio prima dell'eco lo diminuiscono (libere).
  
  Gli ostacoli fissi visti più volte si rafforzano; quelli spostati vengono cancellati quando l'area viene riscansionata.
- Colori: libero / sconosciuto / occupato su scala continua; traiettoria del rover; posizione e direzione attuali; raggi dell'ultima scansione.
- Il file salvato contiene sia la griglia sia **le scansioni grezze con le pose**: se le calibrazioni cambiano, la mappa si può ricalcolare.

---

## 13. Workflow Model-Based Design

```
 Requisiti ─► Modello pianta ─► Logica ASW ─► MIL ─► Codice generato ─► SIL ─► PIL ─► Test sul veicolo
                   ▲                                                                         │
                   └──────────────── calibrazione e identificazione ◄──────────────────────┘
```

### Modello della pianta (`model/plant/`)
- **Attuazione**: PWM e VBAT → velocità delle ruote (primo ordine, zona morta, saturazione), identificato dalle prove di calibrazione.
- **Cinematica dell'uniciclo** con disturbi: slittamento, rumore sulla velocità (scanalature del pavimento).
- **Ultrasuono**: cono di apertura, rumore, letture mancanti, inclinazione verso il suolo, portata massima.
- **Servo**: corrispondenza tra comando e angolo reale, tempo di assestamento.
- **Ambiente**: stanza con pareti e ostacoli fissi e mobili.
- **Batteria**: scarica nel tempo.

### Modelli ASW (`model/controller/`)
| Modello | Contenuto | Periodo |
|---|---|---|
| `VehicleStateMachine` | Stati, transizioni, condizioni R2D, gestione guasti | 20 ms |
| `MotionCtrl` | Conversione da comandi a (v, ω), rampe, mixing differenziale, compensazione VBAT | 20 ms |
| `Aeb` | Soglia dipendente dalla velocità, filtro sulla distanza, mantenimento dell'arresto | 20 ms |
| `DeadReckoning` | Integrazione della posa | 20 ms |
| `ScanSequencer` | Sequenza di angoli, attese, mediana delle letture, emissione dei punti | 20 ms |

### Configurazione di Embedded Coder
| Opzione | Valore |
|---|---|
| System target file | `ert.tlc` |
| Solver | Fixed-step discreto, 0,02 s |
| Tipi | `single`, nessun `double` |
| Interfaccia | `initialize()` / `step()`, ingressi e uscite come strutture |
| Main | Non generato |
| Allocazione dinamica | Disabilitata |
| Hardware | AVR 8 bit (`int` a 16 bit) |
| Calibrazioni | Parametri come variabili globali modificabili, inizializzati dalla EEPROM |
| Report | Code generation report e traceability modello ↔ codice |

### Livelli di verifica
| Livello | Cosa gira | Dove |
|---|---|---|
| MIL | Modelli ASW + pianta | Simulink |
| SIL | Codice generato + pianta | PC |
| PIL | Codice generato sull'ATmega328P + pianta | Arduino via seriale |
| Veicolo | Firmware completo + app | Rover |

Gli stessi casi di test vengono rieseguiti a ogni livello.

---

## 14. Calibrazione e validazione

### Parametri di calibrazione
Modificabili dall'app (`SET_CALIB`), salvati in EEPROM con checksum, valori di default nel modello.

| Parametro | Significato |
|---|---|
| `CAL_PWM_SLOW`, `CAL_PWM_FAST` | PWM di riferimento delle due velocità a VBAT nominale |
| `CAL_V_SLOW`, `CAL_V_FAST` | Velocità lineari misurate |
| `CAL_OMEGA_L`, `CAL_OMEGA_R` | Velocità di rotazione sul posto misurate, per verso |
| `CAL_TRIM` | Correzione tra motore sinistro e destro per andare dritti |
| `CAL_US_OFFSET` | Offset della distanza dell'ultrasuono |
| `CAL_US_MAX_RANGE` | Portata oltre la quale le letture non sono affidabili (eco dal pavimento) |
| `CAL_SERVO_ZERO`, `CAL_SERVO_GAIN` | Corrispondenza tra angolo comandato e angolo reale |
| `CAL_AEB_DIST_SLOW`, `CAL_AEB_DIST_FAST` | Distanze di intervento dell'AEB |
| `CAL_VBAT_MIN` | Soglia di batteria scarica |

### Prove

| Prova | Procedura | Calibra / verifica |
|---|---|---|
| Dritto | Marcia LENTA per 5 s, misura della deviazione laterale | `CAL_TRIM` |
| Velocità | Marcia per 5 s a LENTO e VELOCE, misura col metro | `CAL_V_*` |
| Rotazione | Rotazione tenuta per 1, 2, 3 s, misura con goniometro di carta | `CAL_OMEGA_*` |
| Distanza | Ostacolo a 20, 50, 100, 200 cm misurati col metro | `CAL_US_OFFSET` |
| Portata | Scansione in un'area vuota, rilevamento dell'eco dal pavimento | `CAL_US_MAX_RANGE` |
| Servo | Comando a −60°, 0°, +60° e misura dell'angolo reale | `CAL_SERVO_*` |
| AEB | Avvicinamento a un ostacolo a entrambe le velocità | Distanza di arresto |
| Ripetibilità | Scansione, percorso chiuso, ritorno al punto di partenza, nuova scansione | Deriva della posa |
| Ostacoli mobili | Ostacolo rimosso e area riscansionata | Cancellazione dalla mappa |
| Vibrazioni | Scansioni ripetute sullo stesso punto | Dispersione delle misure |

I risultati vanno in `docs/test_reports/`, con data, VBAT e condizioni del pavimento.

---

## 15. Requisiti

| ID | Requisito |
|---|---|
| REQ-SAF-001 | I motori sono abilitati solo nello stato `R2D` |
| REQ-SAF-002 | L'ingresso in `R2D` richiede: link attivo, TS presente, VBAT sopra soglia, comando STOP attivo, nessun guasto |
| REQ-SAF-003 | Se non arriva un `DRIVE_CMD` valido per 500 ms, il rover si ferma entro 100 ms e passa a `FAULT` |
| REQ-SAF-004 | Se il TS scompare durante la guida, il rover passa a `FAULT` |
| REQ-SAF-005 | In marcia avanti, l'AEB ferma il rover prima che la distanza scenda sotto la soglia calibrata |
| REQ-SAF-006 | Su blocco del software, il watchdog resetta il microcontrollore entro 60 ms; dopo il reset i motori restano disabilitati |
| REQ-FUN-001 | Il rover mantiene due velocità costanti selezionabili, con errore < 15 % al variare di VBAT |
| REQ-FUN-002 | La rotazione continua finché il comando di rotazione è attivo |
| REQ-FUN-003 | La scansione è eseguita solo a veicolo fermo; i motori restano disabilitati per tutta la durata |
| REQ-FUN-004 | Ogni punto di scansione è inviato con la posa stimata del rover |
| REQ-FUN-005 | L'app salva e riapre le mappe, insieme alle scansioni grezze |
| REQ-COM-001 | I frame con CRC errato vengono scartati senza bloccare la ricezione |
| REQ-TIM-001 | Nessun task supera il proprio periodo; gli overrun vengono contati e trasmessi |

---

## 16. Struttura del repository

```
MBD_Embedded_Rover/
├── README.md
├── Project.md                   # Questo documento
├── docs/
│   ├── requirements.md
│   ├── interface.md             # Segnali RTE e protocollo
│   ├── calibration.md
│   ├── datasheets/
│   └── test_reports/
├── hardware/
│   ├── wiring.svg
│   └── photos/
├── firmware/                    # PlatformIO, atmelavr, nessun framework
│   ├── platformio.ini
│   ├── src/
│   │   ├── main.c
│   │   ├── mcal/
│   │   ├── ecual/
│   │   ├── services/
│   │   ├── rte/
│   │   └── app/                 # Integrazione del codice generato
│   ├── generated/               # Output di Embedded Coder
│   └── test/
├── model/
│   ├── MBD_Embedded_Rover.prj   # Simulink Project
│   ├── plant/
│   ├── controller/
│   ├── data/                    # Parametri, calibrazioni, bus, tipi
│   ├── ident/
│   └── tests/
├── app/                         # Progetto Flutter
└── tools/                       # Script Python: simulatore dell'app via USB, analisi log
```

---

## 17. Fasi del progetto

| Fase | Contenuto | Completamento |
|---|---|---|
| 0. Setup | PlatformIO `atmelavr`, progetto bare-metal, LED a registri; Simulink Project; progetto Flutter vuoto | Build e upload funzionanti |
| 1. Elettronica | Cablaggio su Tinkercad e sul rover, verifica col multimetro, sketch di test | ✔ cablaggio verificato in Tinkercad |
| 2. MCAL | GPIO, tick 1 ms, UART a interrupt, PWM, ADC, input capture e compare Timer1, EEPROM, watchdog | Ogni periferica verificata |
| 3. ECU abstraction | Motor, Servo, Ultrasonic, Battery, Led in unità fisiche | Valori corretti sul seriale |
| 4. Scheduler e protocollo | Scheduler con misura dei tempi, protocollo con CRC, tool Python via USB | Comandi e telemetria stabili per 10 minuti |
| 5. App v1 | Connessione Bluetooth, handshake, procedure di accensione e spegnimento, comandi | Guida manuale dal telefono |
| 6. Calibrazione e identificazione | Prove della sezione 14, modello della pianta | Modello coerente con le prove |
| 7. Modelli ASW e MIL | Macchina a stati, controllo del moto, AEB, dead reckoning, scansione | Test MIL superati |
| 8. Generazione di codice, SIL, PIL | Embedded Coder, integrazione tramite RTE, PIL sull'ATmega | SIL e PIL equivalenti al MIL; budget rispettati |
| 9. App v2: mappa | Griglia di occupazione, rendering a colori, salvataggio | Mappa coerente con la stanza |
| 10. Validazione | Prove di ripetibilità, ostacoli mobili, AEB, report | Requisiti verificati |
| 11. Estensioni | Encoder, IMU, correzione della posa confrontando le scansioni con la mappa, analisi MISRA | |

---

## 18. Convenzioni

- C99, tipi a larghezza fissa, nessun `float` nelle ISR.
- API pubbliche `Modulo_Funzione()`, variabili statiche di modulo `s_nome`.
- Ogni modulo ha `modulo.h` (interfaccia) e `modulo.c`; gli header non espongono registri.
- Variabili condivise tra ISR e task `volatile`, lette in sezione critica se più larghe di 8 bit.
- Unità SI nei segnali dell'RTE; conversioni da unità grezze nell'ECU abstraction.
- Commit piccoli e tematici; il codice generato è versionato insieme alla versione del modello che lo ha prodotto.
