/*
 * WebQuack Clone - Wemos D1 Mini Pro Firmware
 * Board: LOLIN(WEMOS) D1 mini Pro
 *
 * Required Libraries (install via Arduino Library Manager):
 *   - ESPAsyncWebServer  (c:\Users\dhrup\Downloads\arduino-littlefs-upload-1.6.3.vsixby Me-No-Dev)
 *   - ESPAsyncTCP        (by Me-No-Dev)
 *   - LittleFS           (built-in with ESP8266 core >= 3.x)
 *
 * WIRING (from your actual board pin labels):
 *
 *   D1 Mini Pro [right col]        Level Shifter    Pro Micro [left col]
 *   ───────────────────────        ─────────────    ────────────────────
 *   TX  (col 2, pin 1 top)  ─────► HV1 ─► LV1 ───► RXI (col 1, pin 2)
 *   RX  (col 2, pin 2)      ◄───── HV2 ◄─ LV2 ◄─── TXO (col 1, pin 1)
 *   GND (col 2, pin 7)      ─────────────────────── GND (col 1, pin 3)
 *   3V3 (col 1, pin 8 bot)  ─────── LV ref input
 *   [Pro Micro VCC]         ─────── HV ref input
 *
 *   POWER: Plug EACH board into its OWN USB port. Do NOT share power rails.
 *
 *   WHY LEVEL SHIFTER:
 *   Pro Micro TXO = 5V logic. D1 Mini RX max = 3.3V. No shifter = fried ESP!
 *   D1 Mini TX = 3.3V -> Pro Micro RXI is safe (3.3V is valid HIGH for 5V MCU)
 *
 * After flashing:
 *   1. Run upload_littlefs.ps1 to upload web UI
 *   2. Connect phone to WiFi "WebQuack" password "hacker123"
 *   3. Open http://192.168.4.1 in browser
 */

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "html_page.h"   // HTML embedded in PROGMEM


// ──────────────────────────────────────────────────────────────────────────────
// Config
// ──────────────────────────────────────────────────────────────────────────────
const char* AP_SSID     = "WebQuack";
const char* AP_PASS     = "hacker123";   // min 8 chars for WPA2
const IPAddress LOCAL_IP(192, 168, 4, 1);
const IPAddress GATEWAY(192, 168, 4, 1);
const IPAddress SUBNET(255, 255, 255, 0);

#define SERIAL_BAUD       9600
#define MAX_LOG_LINES     50

// ──────────────────────────────────────────────────────────────────────────────
// Globals
// ──────────────────────────────────────────────────────────────────────────────
AsyncWebServer server(80);
String logBuffer = "";
int    logLineCount = 0;
bool   proMicroReady = false;

// Payload State Machine
String payloadScript = "";
int    payloadPos = 0;
unsigned long payloadNextTime = 0;
bool   payloadActive = false;

// Serial Buffer
String serialBuffer = "";

// ──────────────────────────────────────────────────────────────────────────────
// Helpers

// ──────────────────────────────────────────────────────────────────────────────
void appendLog(const String& line) {
  logBuffer += line + "\n";
  logLineCount++;
  // Keep only last MAX_LOG_LINES lines
  if (logLineCount > MAX_LOG_LINES) {
    int idx = logBuffer.indexOf('\n');
    if (idx >= 0) logBuffer = logBuffer.substring(idx + 1);
    logLineCount--;
  }
}

// Send command to Pro Micro - fire and forget (NO blocking wait!)
void sendToProMicro(const String& cmd) {
  Serial.println(cmd);          // Send and immediately return
  appendLog(">> " + cmd);      // Log it
  // NOTE: We do NOT wait for a reply - waiting blocks the WiFi stack
  //       and triggers a watchdog reset (WiFi disconnect/reconnect)
}

// Start a multi-line payload script for processing in the loop
void runPayloadScript(const String& script) {
  payloadScript = script;
  payloadPos = 0;
  payloadNextTime = millis();
  payloadActive = true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Setup
// ──────────────────────────────────────────────────────────────────────────────
void setup() {
  // UART to Pro Micro (uses Arduino Serial = UART0, D1 Mini pins TX/RX)
  // NOTE: D1 Mini Pro uses alternate pins for Serial via SoftwareSerial
  // We use the hardware Serial here on GPIO1(TX) / GPIO3(RX)
  Serial.begin(SERIAL_BAUD);

  // ── WiFi AP ──────────────────────────────────────────────────────────────
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(LOCAL_IP, GATEWAY, SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS);

  // ── LittleFS ──────────────────────────────────────────────────────────────
  if (!LittleFS.begin()) {
    appendLog("ERROR: LittleFS mount failed");
  } else {
    appendLog("LittleFS OK");
  }

  // ── Routes ────────────────────────────────────────────────────────────────

  // Serve UI from PROGMEM (bypasses LittleFS path issues)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", index_html);
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", index_html);
  });


  // GET /exec?cmd=GUI_R — execute a single command
  server.on("/exec", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("cmd")) {
      String cmd = req->getParam("cmd")->value();
      appendLog("EXEC: " + cmd);
      sendToProMicro(cmd);
      req->send(200, "application/json",
        "{\"status\":\"ok\",\"cmd\":\"" + cmd + "\"}");
    } else {
      req->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"missing cmd\"}");
    }
  });

  // GET /payload?name=wifi_dump — run a named payload
  server.on("/payload", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name")) {
      String name = req->getParam("name")->value();
      String path = "/payloads/" + name + ".txt";
      appendLog("PAYLOAD: " + name);
      if (LittleFS.exists(path)) {
        File f = LittleFS.open(path, "r");
        String script = f.readString();
        f.close();
        runPayloadScript(script);
        req->send(200, "application/json",
          "{\"status\":\"ok\",\"payload\":\"" + name + "\"}");
      } else {
        req->send(404, "application/json",
          "{\"status\":\"error\",\"msg\":\"payload not found\"}");
      }
    } else {
      req->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"missing name\"}");
    }
  });

  // GET /payloads — list available payloads and filesystem info
  server.on("/payloads", HTTP_GET, [](AsyncWebServerRequest* req) {
    FSInfo fs_info;
    LittleFS.info(fs_info);
    
    String json = "{\"total\":" + String(fs_info.totalBytes) + 
                  ",\"used\":" + String(fs_info.usedBytes) + ",\"scripts\":[";
    
    Dir dir = LittleFS.openDir("/payloads");
    bool first = true;
    while (dir.next()) {
      String fname = dir.fileName();
      if (fname.endsWith(".txt")) {
        if (!first) json += ",";
        fname = fname.substring(0, fname.length() - 4); // strip .txt
        json += "{\"name\":\"" + fname + "\",\"size\":" + String(dir.fileSize()) + "}";
        first = false;
      }
    }
    json += "]}";
    req->send(200, "application/json", json);
  });

  // GET /delete?name=xxx - delete a payload script
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name")) {
      String name = req->getParam("name")->value();
      String path = "/payloads/" + name + ".txt";
      if (LittleFS.exists(path)) {
        LittleFS.remove(path);
        req->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        req->send(404, "application/json", "{\"status\":\"error\",\"msg\":\"payload not found\"}");
      }
    } else {
      req->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"missing name\"}");
    }
  });

  // POST /save — save a new payload
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (req->hasParam("name", true) && req->hasParam("script", true)) {
      String name   = req->getParam("name", true)->value();
      String script = req->getParam("script", true)->value();
      String path   = "/payloads/" + name + ".txt";
      File f = LittleFS.open(path, "w");
      if (f) {
        f.print(script);
        f.close();
        req->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        req->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"write failed\"}");
      }
    } else {
      req->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"missing params\"}");
    }
  });

  // GET /log — return current log
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/plain", logBuffer);
  });

  // GET /status — ping
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json",
      "{\"ap\":\"" + String(AP_SSID) + "\",\"ip\":\"192.168.4.1\",\"ready\":true}");
  });

  server.begin();
  appendLog("Server started. AP: " + String(AP_SSID));
}

// ──────────────────────────────────────────────────────────────────────────────
// Loop
// ──────────────────────────────────────────────────────────────────────────────
void loop() {
  // Read any unsolicited output from Pro Micro (non-blocking)
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        appendLog("[PM] " + serialBuffer);
        if (serialBuffer == "READY") proMicroReady = true;
      }
      serialBuffer = "";
    } else {
      // Prevent buffer from growing infinitely on line noise
      if (serialBuffer.length() < 200) {
        serialBuffer += c;
      }
    }
  }

  // Non-blocking payload execution state machine
  if (payloadActive && millis() >= payloadNextTime) {
    if (payloadPos >= (int)payloadScript.length()) {
      payloadActive = false;
      return;
    }

    int end = payloadScript.indexOf('\n', payloadPos);
    if (end < 0) end = payloadScript.length();
    
    String line = payloadScript.substring(payloadPos, end);
    line.trim();
    payloadPos = end + 1;

    if (line.length() > 0 && !line.startsWith("#")) {
      if (line.startsWith("DELAY:")) {
        payloadNextTime = millis() + line.substring(6).toInt();
      } else {
        sendToProMicro(line);
        payloadNextTime = millis() + 50; // default safe gap between commands
      }
    }
  }
}
