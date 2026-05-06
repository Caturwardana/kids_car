# 🚗 RC Car ESP32

## ✨ Fitur Utama

Remote control mobil anak berbasis **ESP32 + Web Control (WiFi AP)**.
Kontrol dilakukan via browser (HP / laptop), jadi ga perlu remote fisik — tinggal connect WiFi, gas langsung 🚀

---
* Kontrol **maju / mundur** (PWM smooth ramp)
* Kontrol **belok kiri / kanan** (relay + MOSFET)
* **Auto Center Steering** (opsional)
* **Limit steering berbasis ADC** (biar ga jebol stir 😅)
* Web dashboard realtime (ADC live)
* Halaman tuning parameter (disimpan di NVS)
* Non-blocking state machine (anti delay, anti ngelag)

---
<details>
  <summary>CLICK to Expand -->🚗 RC Car ESP32 - Version 1</summary>
 
## 📡 Mode Koneksi

ESP32 membuat Access Point:

```
SSID     : Mobil_RC_ESP32
Password : 12345678
```

Akses via browser:

```
http://192.168.4.1
```

---

## 🔌 Konfigurasi Pin

### 🔋 Motor Driver (BTS7960)

| Fungsi        | Pin ESP32 |
| ------------- | --------- |
| RPWM (Maju)   | 25        |
| LPWM (Mundur) | 26        |
| R_EN          | 32        |
| L_EN          | 33        |

---

### 🛞 Steering

| Fungsi          | Pin ESP32 |
| --------------- | --------- |
| Relay Arah Stir | 14        |
| MOSFET Steering | 13        |

---

### 📊 Sensor ADC (ADS1115)

| Fungsi                | Channel |
| -----------           | ------- |
| Posisi Steering Wheel | A0      | (belum di gunakan untuk versi 1)
| Posisi Stir           | A1      |
| Kosong                | A2      |
| Kosong                | A3      |

Interface: I2C (default ESP32)

---

## ⚙️ Parameter Tuning (Web Settings)

| Parameter     | Fungsi                   |
| ------------- | ------------------------ |
| Limit Kiri    | Batas kiri steering      |
| Limit Kanan   | Batas kanan steering     |
| Center Lurus  | Posisi tengah stir       |
| Deadzone      | Area toleransi tengah    |
| Gas Max (PWM) | Kecepatan maksimum       |
| Hentakan Awal | PWM awal biar ga letoy   |
| Tenaga Belok  | Kekuatan steering        |
| Auto Center   | Balik ke tengah otomatis |

Semua parameter disimpan di **NVS (non-volatile)** → ga hilang walau mati listrik 👍

---

## 🧠 Logic Sistem

### 🚦 Drive (Motor DC)

* Menggunakan **state machine**
* Ada delay aman saat ganti arah (biar gearbox ga ngamuk)
* PWM naik bertahap → smooth acceleration

---

### 🛞 Steering

* Relay menentukan arah (kiri/kanan)
* MOSFET untuk kontrol tenaga (PWM)
* Ada:

  * Limit kiri/kanan
  * Auto center berbasis ADC

---

### 🤖 Auto Center

Kalau dilepas:

* Akan balik ke tengah otomatis
* Kecepatan balik tergantung kecepatan mobil
  👉 makin kenceng = makin agresif baliknya

---

## 🌐 Endpoint API

| Endpoint      | Fungsi            |
| ------------- | ----------------- |
| `/`           | Dashboard utama   |
| `/settings`   | Halaman tuning    |
| `/action`     | Kontrol mobil     |
| `/get_adc`    | Baca ADC realtime |
| `/get_params` | Ambil parameter   |
| `/set_params` | Simpan parameter  |

---

## 🎮 Command Control

| Command         | Fungsi             |
| --------------- | ------------------ |
| `maju`          | Jalan maju         |
| `mundur`        | Jalan mundur       |
| `release_drive` | Stop motor         |
| `kiri`          | Belok kiri         |
| `kanan`         | Belok kanan        |
| `release_steer` | Stop / auto center |

---

## ⚠️ Pin yang BELUM Digunakan

Biar lu gampang upgrade ke versi 2 nanti 😏

| Pin ESP32 | Status                                 |
| --------- | -------------------------------------- |
| 2         | Free                                   |
| 4         | INPUT_FORWARD (belum digunakan di v1)  |
| 5         | Free                                   |
| 12        | Free                                   |
| 15        | Free                                   |
| 16        | Free                                   |
| 17        | Free                                   |
| 18        | Free                                   |
| 19        | Free                                   |
| 21        | I2C SDA (dipakai ADS1115)              |
| 22        | I2C SCL (dipakai ADS1115)              |
| 23        | Free                                   |
| 27        | INPUT_BACKWARD (belum digunakan di v1) |
| 34-39     | Input only (bisa buat sensor tambahan) |

---

## ⚡ Catatan Penting

* Jangan langsung bolak-balik maju ↔ mundur → sistem sudah kasih delay, tapi fisik tetap harus waras 😅
* Pastikan supply BTS7960 kuat (arus gede bro, ini bukan kipas angin)
* Steering wajib ada limit → kalau engga:
  👉 siap-siap suara "KREEEEK" terakhir 😭

---

## 🚀 Next Upgrade Versi 2

* 🔋 Monitoring baterai (ADC)
* 📱 WebSocket realtime control (lebih smooth)

---

## 🏁 Penutup

Versi 1 ini sudah:
✔ Stabil
✔ Bisa dipakai harian
✔ Aman dari steering overlimit

Sisanya tinggal lu upgrade sesuai ambisi 😎

</details>

<details>
  <summary>CLICK to Expand -->🚗 RC Car ESP32 - Version 1</summary>

## 🚀 Upgrade Versi 2

* 🔋 Monitoring baterai (ADC)
* 📱 WebSocket realtime control (lebih smooth)
***

## 🚀 Changelog V2: Major Upgrade & Fitur Baru

Versi 2.0 membawa banyak peningkatan signifikan dibandingkan V1, terutama fokus pada **Keamanan (Failsafe)**, **Kontrol Hybrid (Fisik + Web)**, dan **Stabilitas Pergerakan**. Berikut adalah rincian lengkap peningkatannya:

### 1. 🛡️ Sistem Failsafe & Heartbeat (Anti-Runaway)
Pada V1, jika koneksi WiFi terputus saat tombol gas sedang ditekan di web, mobil akan terus melaju (Runaway). Di V2, masalah ini diatasi dengan:
*   **Web Heartbeat (Ping Timer):** Dashboard web sekarang mengirimkan sinyal "ping" setiap 1 detik selama tombol ditekan.
*   **Auto-Cutoff:** ESP32 akan memonitor sinyal ini (`lastWebCmdTime`). Jika tidak ada sinyal atau perintah dari web selama **3 detik** (misal koneksi HP putus), mobil akan otomatis melakukan **Emergency Stop**.

### 2. 🎮 Kontrol Hybrid (Manual & WiFi)
V2 sekarang mendukung penggunaan sebagai mobil RC murni atau mobil dorong anak konvensional:
*   **Pin Pedal Fisik:** Penambahan pin `INPUT_FORWARD` (Pin 4) dan `INPUT_BACKWARD` (Pin 27) untuk membaca injakan pedal gas fisik pada mobil.
*   **Dukungan Setir Manual (Potensio):** Penambahan pembacaan ADC untuk posisi setir fisik anak (`rawAdcStir`), sehingga setir fisik dan motor *steering* bisa saling tersinkronisasi.
*   **Mode Manual 3-Zone:** Logika baru untuk mengkonversi putaran setir fisik anak menjadi pergerakan motor *steering* dengan zona aman (Deadzone & Stir Gap) agar motor tidak gampang panas.

### 3. ⚙️ Filter Sinyal & Pergerakan Halus (Anti-Jitter)
*   **EMA Filter (Exponential Moving Average):** Sensor ADC (potensio roda dan setir) sering mengalami "noise" yang membuat relay setir bergetar (*jitter*). V2 mengimplementasikan filter matematika (`smoothedRoda` dan `smoothedStir`) untuk menstabilkan pembacaan sensor.
*   **Centralized Steer Logic:** Fungsi `steerTo()` dibuat khusus untuk mengatur transisi relay dan MOSFET setir dengan *delay phase* yang aman, mencegah lonjakan arus (*spike*) yang bisa merusak relay.
*   **Soft-Start Acceleration:** Penambahan parameter `accelTime` (Waktu Akselerasi) agar tarikan awal gas tidak menghentak, lebih nyaman dan aman untuk anak.

### 4. 📊 Live Telemetry Dashboard
Halaman antarmuka web (UI) dirombak untuk memberikan informasi *real-time* kepada operator:
*   **Monitor Baterai:** Menampilkan tegangan aktual baterai (contoh: 12.4V untuk 3S Li-ion) secara langsung di dashboard.
*   **Dual ADC Display:** Memisahkan pantauan nilai sensor roda bawah (`tRoda`) dan setir atas (`tStir`) untuk mempermudah *troubleshooting*.
*   **Touch Optimization:** Tombol web sekarang mendukung *multi-touch handling* yang lebih baik (`touchstart`, `touchend`, `mouseleave`), mencegah gas tersangkut jika jari operator meleset dari tombol di layar HP.

### 5. 🛠️ Menu Tuning & Kalibrasi Lanjutan
Menu `/settings` sekarang jauh lebih komprehensif tanpa perlu *hardcode* ulang program:
*   **Kalibrasi Baterai Otomatis:** Fitur untuk mengkalibrasi pembacaan voltase. Cukup ukur aki/baterai pakai multitester, masukkan nilainya ke web, dan ESP32 akan otomatis menghitung faktor kalibrasinya (`vbatCalib`).
*   **Tuning Setir Manual:** Fitur `GET STIR KIRI` dan `GET STIR KANAN` untuk menyimpan batas putaran setir anak ke dalam memori NVS (Non-Volatile Storage).
*   **Grouping Parameter:** Tampilan menu di web sekarang dikelompokkan secara rapi (Drive, Manual Control, Kalibrasi) agar lebih mudah diatur di lapangan.

Ini dia detail lengkap untuk dokumentasi Pin Mapping dan API HTTP-nya, bro. Gw susun rapi supaya gampang lo masukkan ke Wiki atau `README.md` repositori lo. 

Karena di V2 ini ada fitur Failsafe (Heartbeat), gw juga tambahin panduan khusus gimana caranya ESP32 "Master/Remote" lo bisa nge-remote ESP32 "Mobil" ini tanpa kena *Emergency Stop*.

---

### 🔌 1. Hardware Pin Mapping (Versi 2.0)

Berikut adalah konfigurasi wiring untuk ESP32 (Microcontroller Utama). Pastikan semua *ground* (GND) terhubung menjadi satu (Common Ground).

**Motor Drive (BTS7960)**
| Pin ESP32 | Pin Komponen | Keterangan / Fungsi |
| :--- | :--- | :--- |
| `GPIO 25` | `R_PWM` | PWM Sisi Kanan (Maju) |
| `GPIO 26` | `L_PWM` | PWM Sisi Kiri (Mundur) |
| `GPIO 32` | `R_EN` | Enable Kanan (HIGH = Aktif) |
| `GPIO 33` | `L_EN` | Enable Kiri (HIGH = Aktif) |

**Steering Control**
| Pin ESP32 | Pin Komponen | Keterangan / Fungsi |
| :--- | :--- | :--- |
| `GPIO 14` | Modul Relay | Kontrol arah setir (Kiri / Kanan) |
| `GPIO 13` | Gate MOSFET | PWM untuk mengatur kecepatan/tenaga setir |

**Manual Control (Pedal Anak)**
*Pin ini menggunakan internal pull-up (`INPUT_PULLUP`), jadi pedal/tombol dihubungkan antara pin ESP32 dan GND.*
| Pin ESP32 | Keterangan / Fungsi |
| :--- | :--- |
| `GPIO 4` | Pedal Maju (Aktif LOW saat diinjak) |
| `GPIO 27` | Pedal Mundur (Aktif LOW saat diinjak) |

**Sensor I2C (ADS1115)**
*Menggunakan pin I2C default ESP32.*
| Pin ESP32 | Pin ADS1115 | Keterangan |
| :--- | :--- | :--- |
| `GPIO 21` | `SDA` | Data I2C |
| `GPIO 22` | `SCL` | Clock I2C |
| `3.3V` / `5V`| `VDD` | Power (sesuaikan dengan logic level) |

**Channel ADS1115 Mapping**
* `A0`: Potensiometer Setir Atas (Posisi tangan anak)
* `A1`: Potensiometer Roda Bawah (Posisi steering rack)
* `A2`: Sensor Tegangan / Voltage Divider (Baterai)

---

### 📡 2. HTTP API Documentation

Mobil berjalan sebagai *Access Point* (SoftAP) dengan parameter bawaan:
*   **SSID:** `Mobil_RC_ESP32`
*   **Password:** `12345678`
*   **Base IP:** `[http://192.168.4.1](http://192.168.4.1)`

Semua endpoint menggunakan *method* **HTTP GET**.

#### A. Kontrol Pergerakan (`/action`)
Endpoint utama untuk menggerakkan mobil.
*   **URL:** `/action`
*   **Parameter:** `cmd`
*   **Values:**
    *   `maju` : Mobil melaju ke depan.
    *   `mundur` : Mobil melaju ke belakang.
    *   `release_drive` : Menghentikan pergerakan (Stop).
    *   `kiri` : Membelokkan roda ke kiri.
    *   `kanan` : Membelokkan roda ke kanan.
    *   `release_steer` : Melepas setir (Akan lurus otomatis jika *Auto Center* aktif).
*   **Contoh:** `[http://192.168.4.1/action?cmd=maju](http://192.168.4.1/action?cmd=maju)`

#### B. Heartbeat Failsafe (`/ping`) ⚠️ PENTING
Karena V2 memiliki sistem *anti-runaway*, kontroler eksternal (ESP32 lain atau aplikasi) **wajib** mengirimkan ping secara berkala saat mobil sedang ditekan gasnya. Jika mobil tidak menerima `/ping` atau `/action` selama 3 detik, mobil akan otomatis berhenti.
*   **URL:** `/ping`
*   **Respon:** `OK`
*   *Note: Panggil endpoint ini setiap 1 detik selama tombol di *remote* ditekan.*

#### C. Live Telemetry (`/telemetry`)
Mengambil data sensor secara *real-time*.
*   **URL:** `/telemetry`
*   **Respon JSON:**
    ```json
    {
      "vbat": 12.45,
      "roda": 6200,
      "stir": 8500
    }
    ```

#### D. Pengaturan Parameter NVS
*   **Membaca Parameter:** `/get_params` (Mengembalikan JSON berisi seluruh konfigurasi `lk`, `rk`, `mid`, `spm`, dll).
*   **Menyimpan Parameter:** `/set_params` (Menyimpan konfigurasi ke memori permanen).
    *   *Contoh Update Deadzone:* `[http://192.168.4.1/set_params?dz=200&spm=100](http://192.168.4.1/set_params?dz=200&spm=100)`

#### E. Utilitas ADC & Baterai
*   **Get Raw ADC:** `/get_adc?ch=0` (Bisa pilih channel `0`, `1`, atau `2`).
*   **Kalibrasi Baterai:** `/cal_vbat?v=12.5` (Masukkan angka tegangan asli dari multitester untuk mengkalibrasi faktor pembagi).

---
</details>

<details>
  <summary>CLICK to Expand -->🎮 Remote Version 1</summary>

# Remote V1 Contoh Implementasi untuk "ESP32 Remote Controller"

Kalau lo bikin remote pakai ESP32 lain, begini logika dasar code di ESP32 Remote-nya pakai library `HTTPClient`.
```cpp
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Mobil_RC_ESP32";
const char* password = "12345678";
String baseURL = "http://192.168.4.1";

unsigned long lastPingTime = 0;
bool isMoving = false;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected to Car!");
}

void sendCommand(String cmd) {
  HTTPClient http;
  http.begin(baseURL + "/action?cmd=" + cmd);
  int httpCode = http.GET();
  http.end();
}

void sendHeartbeat() {
  HTTPClient http;
  http.begin(baseURL + "/ping");
  http.GET();
  http.end();
}

void loop() {
  // Contoh: Jika tombol di pin 5 ditekan
  if (digitalRead(5) == LOW) {
    if (!isMoving) {
      sendCommand("maju");
      isMoving = true;
    }
    
    // Kirim heartbeat setiap 1 detik selama ditahan
    if (millis() - lastPingTime > 1000) {
      sendHeartbeat();
      lastPingTime = millis();
    }
  } else {
    if (isMoving) {
      sendCommand("release_drive");
      isMoving = false;
    }
  }
}
```
---
Ini adalah upgrade besar untuk firmware **Remote Control (Master)**. Sekarang remote nggak cuma ngirim perintah, tapi juga **narik data (Telemetri)** dari mobil lewat JSON. 

Lo bakal bisa lihat **Voltase Baterai Aki** mobil dan **Posisi Roda** langsung di layar OLED remote lo.

### 🛠️ Kebutuhan Library (Arduino IDE)
1. **U8g2** (Untuk OLED)
2. **ArduinoJson** (PENTING: Untuk membaca data telemetri dari mobil)

---

### 🔌 1. Pin Mapping ESP32-C3 Super Mini
| Komponen | Pin ESP32-C3 | Keterangan |
| :--- | :--- | :--- |
| **OLED SDA** | `GPIO 8` | Data I2C |
| **OLED SCL** | `GPIO 9` | Clock I2C |
| **Tombol MAJU** | `GPIO 2` | Hubungkan ke GND |
| **Tombol MUNDUR** | `GPIO 3` | Hubungkan ke GND |
| **Tombol KIRI** | `GPIO 4` | Hubungkan ke GND |
| **Tombol KANAN** | `GPIO 5` | Hubungkan ke GND |

---

### 💻 2. Full Source Code Remote V2 (With Telemetry)

```cpp
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
```

---

### 📝 Dokumentasi Fitur Baru

#### 1. Real-Time Telemetry Parsing
Remote kini bertindak sebagai klien yang cerdas. Setiap 500ms, ia memanggil endpoint `[http://192.168.4.1/telemetry](http://192.168.4.1/telemetry)`. Data JSON yang diterima dari mobil dipisahkan (parsing) menjadi variabel:
*   `vbat`: Mengetahui sisa baterai aki mobil anak agar tidak drop.
*   `roda`: Mengetahui posisi real-time steering rack (untuk kalibrasi).

#### 2. Visual Feedback (Steering Bar)
Gue tambahin fitur visual di layar OLED berupa **Bar Horizontal**. 
*   Jika roda mobil belok kiri, kotak kecil di dalam bar akan geser ke kiri. 
*   Ini berguna banget buat mastiin *Auto-Center* mobil bekerja dengan benar tanpa lo harus ngintip ke bawah mobil.

#### 3. Dual-Purpose Heartbeat
Fungsi `fetchTelemetry()` secara otomatis mereset timer failsafe pada mobil. Jadi, selama remote ini nyala dan terhubung, mobil nggak akan terkena *Emergency Stop*. Kalau remote mati/putus sinyal, telemetri gagal ditarik, dan mobil otomatis berhenti dalam 3 detik.

#### 4. State-Change Optimization
Program tetap menggunakan sistem *State-Change*. Perintah HTTP `/action` hanya dikirim **satu kali** saat tombol ditekan atau dilepas. Ini sangat penting supaya koneksi WiFi ESP32-C3 tidak *crash* karena terlalu banyak menangani request.

### 💡 Tips Instalasi
Gunakan **Case 3D Print** kecil untuk C3 Mini ini. Karena ini remote anak, pastikan tombolnya empuk. Dengan adanya info voltase baterai di layar, lo jadi tahu kapan harus nge-charge aki mobil sebelum bener-bener habis di tengah jalan. 

</details>
