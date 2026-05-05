#🚗 RC Car ESP32

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
