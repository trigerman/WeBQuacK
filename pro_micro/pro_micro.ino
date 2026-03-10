/*
 * WebQuack Clone - Pro Micro Firmware
 * Board: Arduino Pro Micro (ATmega32U4) 5V/16MHz
 * 
 * Wiring to D1 Mini Pro:
 *   Pro Micro RX (pin 0) <-- D1 Mini TX (D7 / GPIO13)  [via logic level shifter]
 *   Pro Micro TX (pin 1) --> D1 Mini RX (D8 / GPIO15)  [via logic level shifter]
 *   GND <--> GND
 *
 * Command protocol (newline terminated, sent from D1 Mini over serial):
 *   KEY:<text>            - Type text as keystrokes
 *   ENTER                 - Press Enter key
 *   TAB                   - Press Tab key
 *   ESC                   - Press Escape key
 *   BACKSPACE             - Press Backspace
 *   SPACE                 - Press Space
 *   GUI_R                 - Win + R  (open Run dialog)
 *   GUI_X                 - Win + X  (admin menu)
 *   ALT_F4                - Alt + F4
 *   CTRL_ALT_DEL          - Ctrl + Alt + Del
 *   CTRL_SHIFT_ESC        - Ctrl + Shift + Esc (Task Manager)
 *   COMBO:<mod>+<key>     - Generic combo e.g. COMBO:CTRL+c
 *   DELAY:<ms>            - Wait N milliseconds
 *   MOUSE_MOVE:<x>,<y>   - Relative mouse move
 *   MOUSE_CLICK:<btn>     - Click: L, R, or M
 *   ACK                   - Ping/pong test
 */

#include <Keyboard.h>
#include <Mouse.h>

#define BAUD_RATE 9600

String inputBuffer = "";

void setup() {
  Serial1.begin(BAUD_RATE);   // UART to D1 Mini Pro
  Keyboard.begin();
  Mouse.begin();
  delay(1000);                 // Let USB enumerate on host
  Serial1.println("READY");   // Tell D1 Mini we're up
}

void loop() {
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (c == '\n') {
      inputBuffer.trim();
      processCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

void processCommand(String cmd) {
  if (cmd.length() == 0) return;

  // ── ACK ──────────────────────────────────────────────────────────
  if (cmd == "ACK") {
    Serial1.println("ACK_OK");

  // ── KEY:<text> ────────────────────────────────────────────────────
  } else if (cmd.startsWith("KEY:")) {
    String text = cmd.substring(4);
    Keyboard.print(text);
    Serial1.println("OK");

  // ── Simple keys ───────────────────────────────────────────────────
  } else if (cmd == "ENTER") {
    Keyboard.press(KEY_RETURN);
    delay(50);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "TAB") {
    Keyboard.press(KEY_TAB);
    delay(50);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "ESC") {
    Keyboard.press(KEY_ESC);
    delay(50);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "BACKSPACE") {
    Keyboard.press(KEY_BACKSPACE);
    delay(50);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "SPACE") {
    Keyboard.press(' ');
    delay(50);
    Keyboard.releaseAll();
    Serial1.println("OK");

  // ── Key combos ────────────────────────────────────────────────────
  } else if (cmd == "GUI_R") {
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('r');
    delay(100);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "GUI_X") {
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('x');
    delay(100);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "ALT_F4") {
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_F4);
    delay(100);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "CTRL_ALT_DEL") {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_DELETE);
    delay(100);
    Keyboard.releaseAll();
    Serial1.println("OK");

  } else if (cmd == "CTRL_SHIFT_ESC") {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_LEFT_SHIFT);
    Keyboard.press(KEY_ESC);
    delay(100);
    Keyboard.releaseAll();
    Serial1.println("OK");

  // ── COMBO:<MOD>+<key>  e.g. COMBO:CTRL+c ─────────────────────────
  } else if (cmd.startsWith("COMBO:")) {
    String combo = cmd.substring(6);
    combo.toUpperCase();
    int plusIdx = combo.indexOf('+');
    if (plusIdx > 0) {
      String mod = combo.substring(0, plusIdx);
      String key = combo.substring(plusIdx + 1);
      if (mod == "CTRL")  Keyboard.press(KEY_LEFT_CTRL);
      if (mod == "ALT")   Keyboard.press(KEY_LEFT_ALT);
      if (mod == "SHIFT") Keyboard.press(KEY_LEFT_SHIFT);
      if (mod == "GUI")   Keyboard.press(KEY_LEFT_GUI);
      delay(50);
      if (key.length() == 1) Keyboard.press((char)key[0]);
      delay(100);
      Keyboard.releaseAll();
      Serial1.println("OK");
    }

  // ── DELAY:<ms> ────────────────────────────────────────────────────
  } else if (cmd.startsWith("DELAY:")) {
    int ms = cmd.substring(6).toInt();
    delay(ms);
    Serial1.println("OK");

  // ── MOUSE_MOVE:<x>,<y> ────────────────────────────────────────────
  } else if (cmd.startsWith("MOUSE_MOVE:")) {
    String args = cmd.substring(11);
    int comma = args.indexOf(',');
    if (comma > 0) {
      int x = args.substring(0, comma).toInt();
      int y = args.substring(comma + 1).toInt();
      Mouse.move(x, y, 0);
      Serial1.println("OK");
    }

  // ── MOUSE_CLICK:<L|R|M> ───────────────────────────────────────────
  } else if (cmd.startsWith("MOUSE_CLICK:")) {
    String btn = cmd.substring(12);
    if (btn == "L") Mouse.click(MOUSE_LEFT);
    else if (btn == "R") Mouse.click(MOUSE_RIGHT);
    else if (btn == "M") Mouse.click(MOUSE_MIDDLE);
    Serial1.println("OK");

  } else {
    Serial1.println("ERR:UNKNOWN_CMD");
  }
}
