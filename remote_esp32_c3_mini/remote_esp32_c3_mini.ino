#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ===== CONFIG =====
const char* ssid = "Mobil_RC_ESP32";
const char* password = "12345678";
const String serverIP = "http://192.168.4.1";

#define BTN_MAJU   2
#define BTN_MUNDUR 3
#define BTN_KIRI   4
#define BTN_KANAN  5

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// ===== TELEMETRY DATA =====
float carVbat = 0.0;
int carRoda = 0;
int carStir = 0;
String statusMobil = "SEARCHING";
unsigned long lastUpdate = 0;

void sendAction(String cmd) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverIP + "/action?cmd=" + cmd);
    http.GET();
    http.end();
  }
}

// Fungsi utama mengambil data telemetri (sekaligus sebagai Heartbeat)
void fetchTelemetry() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setConnectTimeout(800);
    http.begin(serverIP + "/telemetry");
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        carVbat = doc["vbat"];
        carRoda = doc["roda"];
        carStir = doc["stir"];
        statusMobil = "CONNECTED";
      }
    } else {
      statusMobil = "LOST CONN";
    }
    http.end();
  }
}

void drawUI() {
  u8g2.clearBuffer();
  
  // Header
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "RC MASTER V2");
  u8g2.drawStr(80, 10, statusMobil.c_str());
  u8g2.drawHLine(0, 13, 128);

  // Bagian Telemetri (Kiri)
  u8g2.setFont(u8g2_font_7x14_tf);
  u8g2.setCursor(0, 30);
  u8g2.print("BAT: "); u8g2.print(carVbat, 1); u8g2.print(" V");

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 48);
  u8g2.print("Roda: "); u8g2.print(carRoda);
  u8g2.setCursor(0, 60);
  u8g2.print("Stir: "); u8g2.print(carStir);

  // Visualisasi Setir (Kanan)
  u8g2.drawFrame(85, 35, 40, 15); // Box indikator
  int barPos = map(carRoda, 3000, 9000, 0, 36); // Mapping posisi roda ke bar
  u8g2.drawBox(87 + barPos, 37, 4, 11);

  u8g2.sendBuffer();
}

void setup() {
  pinMode(BTN_MAJU, INPUT_PULLUP);
  pinMode(BTN_MUNDUR, INPUT_PULLUP);
  pinMode(BTN_KIRI, INPUT_PULLUP);
  pinMode(BTN_KANAN, INPUT_PULLUP);

  u8g2.begin();
  WiFi.begin(ssid, password);
}

void loop() {
  static String lastDrive = "stop";
  static String lastSteer = "lurus";

  // Baca Input Tombol
  bool bMaju = !digitalRead(BTN_MAJU);
  bool bMundur = !digitalRead(BTN_MUNDUR);
  bool bKiri = !digitalRead(BTN_KIRI);
  bool bKanan = !digitalRead(BTN_KANAN);

  // Logic Drive
  if (bMaju && lastDrive != "maju") { sendAction("maju"); lastDrive = "maju"; }
  else if (bMundur && lastDrive != "mundur") { sendAction("mundur"); lastDrive = "mundur"; }
  else if (!bMaju && !bMundur && lastDrive != "stop") { sendAction("release_drive"); lastDrive = "stop"; }

  // Logic Steer
  if (bKiri && lastSteer != "kiri") { sendAction("kiri"); lastSteer = "kiri"; }
  else if (bKanan && lastSteer != "kanan") { sendAction("kanan"); lastSteer = "kanan"; }
  else if (!bKiri && !bKanan && lastSteer != "lurus") { sendAction("release_steer"); lastSteer = "lurus"; }

  // Update Telemetri & Layar setiap 500ms
  if (millis() - lastUpdate > 500) {
    fetchTelemetry();
    drawUI();
    lastUpdate = millis();
  }
}
