#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Preferences.h> 

// ===== PIN DRIVE BTS7960 =====
#define RPWM_PIN 25  
#define LPWM_PIN 26  
#define R_EN_PIN 32  
#define L_EN_PIN 33  

// ===== PIN STEERING =====
#define RELAY_STEER 14
#define MOSFET_STEER 13

// ===== PIN MANUAL CONTROL =====
#define INPUT_FORWARD 4
#define INPUT_BACKWARD 27

// ===== VARIABEL PARAMETER (NVS) =====
int speedPWM, belokPWM;      
int limitKiri, limitKanan;    
int startDrivePWM; 
int adcTengah, deadzone;
int autoCenter; 
int accelTime;  
int manualMode; 
int potKiri, potKanan; 
float vbatCalib; 

// Parameter Setting Manual 3-Zone
int dzMan;     
int stirGap;   

String currentDrive = "stop";
String currentSteer = "lurus";

Adafruit_ADS1115 ads;
WebServer server(80);
Preferences preferences; 

// ===== VARIABEL TIMING & NON-BLOCKING =====
unsigned long lastLimitCheck = 0;
unsigned long lastWebCmdTime = 0; 

int drivePhase = 0; 
unsigned long driveTimer = 0;
String pendingDrive = "";
int currentPWM = 0;
int activeDriveChannel = 0;

int steerPhase = 0; 
unsigned long steerTimer = 0;
String pendingSteer = "lurus";
int activeSteerPWM = 0;

// ===== VARIABEL FILTER & ZONA =====
float smoothedRoda = 0;
float smoothedStir = 0;
int currentZone = 2; 

// ==========================================
// ===== HALAMAN 1: MAIN MENU DASHBOARD =====
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>RC Control</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; text-align: center; background-color: #1a1a1a; color: white; user-select: none; margin: 0; padding: 0;}
    .header { padding: 10px 15px; background: #222; border-bottom: 2px solid #444; display: flex; justify-content: space-between; align-items: center; }
    .btn-settings { background: #555; color: white; padding: 8px 15px; text-decoration: none; border-radius: 5px; font-weight: bold; font-size: 14px;}
    .live-data { text-align: left; font-family: monospace; font-size: 12px; color: #0f0; }
    .telemetry-box { background: #000; padding: 5px 10px; border-radius: 5px; border: 1px solid #333;}
    .container { display: flex; justify-content: space-around; padding: 40px 10px; align-items: center; height: 60vh; }
    .btn-circle { width: 90px; height: 90px; border-radius: 50%; font-size: 16px; font-weight: bold; background: #007bff; color: white; border: 4px solid #0056b3; box-shadow: 0 5px 0 #004494; touch-action: manipulation; }
    .btn-circle:active, .btn-circle[data-pressed="true"] { transform: translateY(5px); box-shadow: none; background: #0056b3; }
  </style>
</head>
<body>
  <div class="header">
    <div class="telemetry-box">
      <div class="live-data">Baterai: <span id="tVbat">0.0</span>V</div>
      <div class="live-data">Roda ADC: <span id="tRoda">0</span></div>
      <div class="live-data">Stir ADC: <span id="tStir">0</span></div>
    </div>
    <div style="font-size: 18px; font-weight: bold; color: #ffcc00;">RC DASHBOARD</div>
    <a href="/settings" class="btn-settings">⚙️ Setup</a>
  </div>
  
  <div class="container">
    <div style="display: flex; flex-direction: column; gap: 25px;">
      <button class="btn-circle" id="btnMaju">MAJU</button>
      <button class="btn-circle" id="btnMundur">MUNDUR</button>
    </div>
    <div style="display: flex; gap: 20px;">
      <button class="btn-circle" style="background:#dc3545; border-color:#a71d2a;" id="btnKiri">KIRI</button>
      <button class="btn-circle" style="background:#dc3545; border-color:#a71d2a;" id="btnKanan">KANAN</button>
    </div>
  </div>

  <script>
    let pingTimer = null;

    function sendCmd(action) { fetch('/action?cmd=' + action); }

    function bindBtn(id, cmdPress, cmdRelease) {
      const btn = document.getElementById(id);

      const press = (e) => {
        e.preventDefault();
        if(!btn.dataset.pressed) {
          btn.dataset.pressed = "true";
          sendCmd(cmdPress);
          if(!pingTimer) pingTimer = setInterval(() => fetch('/ping'), 1000); // Heartbeat tiap 1 detik
        }
      };

      const release = (e) => {
        e.preventDefault();
        if(btn.dataset.pressed) {
          btn.dataset.pressed = "";
          sendCmd(cmdRelease);
          // Kalau ga ada tombol lain yang lagi dipencet, stop heartbeat
          if(!document.querySelector('[data-pressed="true"]')) {
             clearInterval(pingTimer);
             pingTimer = null;
          }
        }
      };

      // Handle semua kemungkinan jari mencet atau kepeleset
      btn.addEventListener('touchstart', press);
      btn.addEventListener('mousedown', press);
      btn.addEventListener('touchend', release);
      btn.addEventListener('mouseup', release);
      btn.addEventListener('mouseleave', release); // Kalo mouse geser keluar
      btn.addEventListener('touchcancel', release); // Kalo layar error/ada notif masuk
    }

    bindBtn('btnMaju', 'maju', 'release_drive');
    bindBtn('btnMundur', 'mundur', 'release_drive');
    bindBtn('btnKiri', 'kiri', 'release_steer');
    bindBtn('btnKanan', 'kanan', 'release_steer');

    setInterval(() => {
      fetch('/telemetry').then(res => res.json()).then(data => {
        document.getElementById('tVbat').innerText = data.vbat.toFixed(2);
        document.getElementById('tRoda').innerText = data.roda;
        document.getElementById('tStir').innerText = data.stir;
      });
    }, 500);
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// ===== HALAMAN 2: SETTINGS PARAMETER ======
// ==========================================
const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Tuning Settings</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #1a1a1a; color: white; padding: 20px; text-align: center;}
    .card { background: #2c2c2c; padding: 20px; border-radius: 12px; max-width: 400px; margin: auto; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; text-align: left; margin-bottom: 15px;}
    input[type=number], input[type=text] { width: 100%; box-sizing: border-box; padding: 8px; border-radius: 5px; border: 1px solid #444; background: #333; color: #0f0; font-weight: bold; }
    select { width: 100%; padding: 8px; border-radius: 5px; background: #333; color: white; border: 1px solid #444; }
    .btn-small { background: #666; color: white; border: none; padding: 5px; border-radius: 3px; cursor: pointer; font-size: 10px; width: 100%; margin-top: 5px;}
    .btn-save { width: 100%; padding: 15px; background: #28a745; color: white; border: none; border-radius: 8px; font-weight: bold; font-size: 16px; margin-bottom: 10px;}
    .btn-back { display: block; margin-bottom: 20px; color: #ffcc00; text-decoration: none; font-weight: bold; text-align: left; }
    .section-title { color: #00bfff; font-size: 14px; border-bottom: 1px solid #444; padding-bottom: 5px; margin-bottom: 10px; text-align: left; margin-top:20px;}
  </style>
</head>
<body>
  <div class="card">
    <a href="/" class="btn-back">⬅ Kembali ke Kontrol</a>
    <h3 style="color: #ffcc00; border-bottom: 1px solid #444; padding-bottom: 10px;">⚙️ TUNING PARAMETER</h3>
    <p style="font-size: 12px; color: #ccc;">Roda ADC: <b id="liveADC" style="color:#0f0;">0</b> | Setir ADC: <b id="livePot" style="color:#0f0;">0</b></p>
    
    <div class="section-title">Drive & Auto Steer</div>
    <div class="grid">
      <div><label>Limit Kiri</label><input type="number" id="valLK"><button class="btn-small" onclick="getADC('valLK', 1)">GET ADC KIRI</button></div>
      <div><label>Limit Kanan</label><input type="number" id="valRK"><button class="btn-small" onclick="getADC('valRK', 1)">GET ADC KANAN</button></div>
      <div><label>Center Lurus</label><input type="number" id="valMid"><button class="btn-small" onclick="getADC('valMid', 1)">GET ADC TENGAH</button></div>
      <div><label>Deadzone Web</label><input type="number" id="valDz"></div>
      <div><label>Gas Max (PWM)</label><input type="number" id="valSpeed"></div>
      <div><label>Hentakan Awal</label><input type="number" id="valStart"></div>
      <div><label>Tenaga Belok Max</label><input type="number" id="valBelok"></div>
      <div><label>Auto Center</label>
        <select id="valAuto"><option value="1">Aktif</option><option value="0">Mati</option></select>
      </div>
      <div style="grid-column: span 2;"><label>Waktu Akselerasi (ms)</label><input type="number" id="valAccel"></div>
    </div>

    <div class="section-title" style="color: #ff6666;">Manual Control (3-Zone)</div>
    <div class="grid">
      <div><label>Mode Manual</label>
        <select id="valMan"><option value="1">Aktif</option><option value="0">Mati</option></select>
      </div>
      <div></div>
      <div><label>Stir Kiri Max</label><input type="number" id="valPK"><button class="btn-small" onclick="getADC('valPK', 0)">GET STIR KIRI</button></div>
      <div><label>Stir Kanan Max</label><input type="number" id="valPR"><button class="btn-small" onclick="getADC('valPR', 0)">GET STIR KANAN</button></div>
      
      <div style="grid-column: span 2; border-top: 1px dashed #555; margin-top: 5px; padding-top: 10px;">
        <i style="font-size:11px; color:#aaa;">Khusus tuning setir manual (Kiri - Tengah - Kanan)</i>
      </div>
      <div><label>Gap Setir (%)</label><input type="number" id="valZgp" placeholder="10"></div>
      <div><label>Deadzone Roda</label><input type="number" id="valDzM" placeholder="250"></div>
    </div>

    <div class="section-title">Kalibrasi Baterai (3S Li-ion)</div>
    <div class="grid" style="grid-template-columns: 2fr 1fr;">
      <div><label>Tegangan Aktual (V)</label><input type="text" id="valCalVbat" placeholder="Cth: 12.4"></div>
      <div style="display: flex; align-items: flex-end;"><button class="btn-save" style="padding: 10px; font-size:12px;" onclick="calibrateVbat()">KALIBRASI</button></div>
    </div>

    <button class="btn-save" onclick="saveParams()">💾 SIMPAN SETTING</button>
  </div>

  <script>
    function getADC(targetId, ch) { fetch('/get_adc?ch='+ch).then(res => res.text()).then(val => document.getElementById(targetId).value = val); }
    
    setInterval(() => { 
      fetch('/telemetry').then(res => res.json()).then(data => {
        document.getElementById('liveADC').innerText = data.roda;
        document.getElementById('livePot').innerText = data.stir;
      });
    }, 800);

    fetch('/get_params').then(res => res.json()).then(data => {
      document.getElementById('valLK').value = data.lk; document.getElementById('valRK').value = data.rk;
      document.getElementById('valMid').value = data.mid; document.getElementById('valDz').value = data.dz;
      document.getElementById('valSpeed').value = data.spm; document.getElementById('valStart').value = data.sta;
      document.getElementById('valBelok').value = data.bel; document.getElementById('valAuto').value = data.ac;
      document.getElementById('valAccel').value = data.acc; document.getElementById('valMan').value = data.man;
      document.getElementById('valPK').value = data.pk; document.getElementById('valPR').value = data.pr;
      
      document.getElementById('valZgp').value = data.zgp; document.getElementById('valDzM').value = data.dzm;
    });

    function saveParams() {
      let q = `lk=${document.getElementById('valLK').value}&rk=${document.getElementById('valRK').value}&mid=${document.getElementById('valMid').value}&dz=${document.getElementById('valDz').value}&spm=${document.getElementById('valSpeed').value}&sta=${document.getElementById('valStart').value}&bel=${document.getElementById('valBelok').value}&ac=${document.getElementById('valAuto').value}&acc=${document.getElementById('valAccel').value}&man=${document.getElementById('valMan').value}&pk=${document.getElementById('valPK').value}&pr=${document.getElementById('valPR').value}&zgp=${document.getElementById('valZgp').value}&dzm=${document.getElementById('valDzM').value}`;
      fetch('/set_params?' + q).then(() => alert('Setting Tersimpan!'));
    }

    function calibrateVbat() {
      let v = document.getElementById('valCalVbat').value;
      if(v) fetch('/cal_vbat?v=' + v).then(() => alert('Baterai Dikalibrasi!'));
    }
  </script>
</body>
</html>
)rawliteral";

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(INPUT_FORWARD, INPUT_PULLUP);
  pinMode(INPUT_BACKWARD, INPUT_PULLUP);

  preferences.begin("rc-config", false);
  limitKiri     = preferences.getInt("lk", 3600);
  limitKanan    = preferences.getInt("rk", 8400);
  adcTengah     = preferences.getInt("mid", 6200); 
  deadzone      = preferences.getInt("dz", 150); 
  speedPWM      = preferences.getInt("spm", 80);
  startDrivePWM = preferences.getInt("sta", 5);
  belokPWM      = preferences.getInt("bel", 150);
  autoCenter    = preferences.getInt("ac", 1); 
  accelTime     = preferences.getInt("acc", 1500); 
  manualMode    = preferences.getInt("man", 0);
  potKiri       = preferences.getInt("pk", 12385);
  potKanan      = preferences.getInt("pr", 4733);
  vbatCalib     = preferences.getFloat("vbc", 0.000937); 
  
  stirGap       = preferences.getInt("zgp", 10);
  dzMan         = preferences.getInt("dzm", 250);

  pinMode(R_EN_PIN, OUTPUT); pinMode(L_EN_PIN, OUTPUT);
  digitalWrite(R_EN_PIN, LOW); digitalWrite(L_EN_PIN, LOW);
  pinMode(RELAY_STEER, OUTPUT);

  ledcSetup(0, 1500, 8); ledcAttachPin(RPWM_PIN, 0);
  ledcSetup(2, 1500, 8); ledcAttachPin(LPWM_PIN, 2);
  ledcSetup(1, 5000, 8); ledcAttachPin(MOSFET_STEER, 1);

  if (!ads.begin()) { Serial.println("ADS Gagal!"); while(1); }

  stopAll();
  WiFi.softAP("Mobil_RC_ESP32", "12345678");
  
  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/settings", []() { server.send(200, "text/html", settings_html); });
  
  server.on("/action", handleAction);
  server.on("/get_params", handleGetParams);
  server.on("/set_params", handleSetParams);
  server.on("/telemetry", handleTelemetry);
  
  server.on("/get_adc", [](){ 
    int ch = server.hasArg("ch") ? server.arg("ch").toInt() : 1;
    server.send(200, "text/plain", String(ads.readADC_SingleEnded(ch))); 
  });

  server.on("/cal_vbat", []() {
    if (server.hasArg("v")) {
      float actualV = server.arg("v").toFloat();
      int16_t rawAdc = ads.readADC_SingleEnded(2);
      if (rawAdc > 0) {
        vbatCalib = actualV / rawAdc;
        preferences.putFloat("vbc", vbatCalib);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  // FUNGSI HEARTBEAT SAFETY
  server.on("/ping", []() {
    lastWebCmdTime = millis(); // Reset timer safety selama ping masuk
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

// ==========================================
// ===== FUNGSI SENTRAL ANTI-JITTER RELAY ===
// ==========================================

void steerTo(String arah, int pwm) {
  activeSteerPWM = pwm;
  
  if (pendingSteer != arah) {
    if (steerPhase == 0) {
      pendingSteer = arah; 
      steerPhase = 1; 
      steerTimer = millis(); 
      ledcWrite(1, 0);
    }
  } else {
    if (steerPhase == 0) {
      ledcWrite(1, activeSteerPWM);
    }
  }
}

void stopSteer() {
  ledcWrite(1, 0); 
  currentSteer = "lurus"; 
  steerPhase = 0; 
}


// ===== LOOP =====
void loop() {
  server.handleClient();
  
  int16_t rawAdcRoda = ads.readADC_SingleEnded(1);
  int16_t rawAdcStir = ads.readADC_SingleEnded(0);

  if (smoothedRoda == 0) smoothedRoda = rawAdcRoda;
  if (smoothedStir == 0) smoothedStir = rawAdcStir;

  smoothedRoda = (0.3 * rawAdcRoda) + (0.7 * smoothedRoda);
  smoothedStir = (0.05 * rawAdcStir) + (0.95 * smoothedStir);

  int16_t adcRoda = (int16_t)smoothedRoda;
  int16_t adcStir = (int16_t)smoothedStir;
  
  bool pedalFwd = (digitalRead(INPUT_FORWARD) == LOW);
  bool pedalBwd = (digitalRead(INPUT_BACKWARD) == LOW);
  
  // Timer Safety sekarang dipandu oleh update dari /action ATAU /ping
  bool webOverride = (millis() - lastWebCmdTime < 3000); 

  // TIMEOUT SAFETY AKTIF (Jika webOverride false / udah > 3 detik ga dapet kabar)
  if (!webOverride) {
    if (currentDrive != "stop" && !pedalFwd && !pedalBwd) stopDrive();
  }

  // LIMIT CHECK AMAN
  if (millis() - lastLimitCheck >= 30) {
    lastLimitCheck = millis();
    if (currentSteer != "lurus" && currentSteer != "centering") {
      if (pendingSteer == "kiri" && adcRoda <= limitKiri) stopSteer();
      else if (pendingSteer == "kanan" && adcRoda >= limitKanan) stopSteer();
    }
  }

  // ====================================================
  // ===== 4. MANUAL CONTROL: LOGIKA 3 ZONE STEER =====
  // ====================================================
  if (manualMode == 1 && !webOverride) {
    
    // ---- MANUAL DRIVE ----
    if (pedalFwd && !pedalBwd && pendingDrive != "maju") {
       stopDrive(); pendingDrive = "maju"; drivePhase = (currentDrive == "mundur") ? 1 : 0;
       if(drivePhase==0) executeMaju(); else driveTimer=millis();
    } else if (pedalBwd && !pedalFwd && pendingDrive != "mundur") {
       stopDrive(); pendingDrive = "mundur"; drivePhase = (currentDrive == "maju") ? 1 : 0;
       if(drivePhase==0) executeMundur(); else driveTimer=millis();
    } else if (!pedalFwd && !pedalBwd && currentDrive != "stop") {
       stopDrive();
    }

    // ---- MANUAL STEER (Logika Saklar 3-Kaki + Gap) ----
    int persenStir = map(adcStir, potKiri, potKanan, 0, 100);
    persenStir = constrain(persenStir, 0, 100);

    if (persenStir < (33 - stirGap)) {
      currentZone = 1; // KIRI
    } else if (persenStir > (33 + stirGap) && persenStir < (66 - stirGap)) {
      currentZone = 2; // TENGAH
    } else if (persenStir > (66 + stirGap)) {
      currentZone = 3; // KANAN
    }
    
    int targetADC = adcTengah;
    if (currentZone == 1) targetADC = limitKiri;
    else if (currentZone == 2) targetADC = adcTengah;
    else if (currentZone == 3) targetADC = limitKanan;

    int errorRoda = abs(adcRoda - targetADC);

    // Eksekusi Roda Mengejar Target
    if (errorRoda > dzMan) {
      String arahManual = (adcRoda < targetADC) ? "kanan" : "kiri";
      currentSteer = "manual"; 
      steerTo(arahManual, belokPWM);
    } else {
      if (currentSteer != "lurus") stopSteer();
    }
  }

  // 5. AUTO CENTER LOGIC
  if (currentSteer == "centering" && (manualMode == 0 || webOverride)) {
    int errorCenter = abs(adcRoda - adcTengah);
    if (errorCenter <= deadzone) { 
      stopSteer(); 
    } else {
      String arahBalik = (adcRoda < adcTengah) ? "kanan" : "kiri";
      int pwmBalik = belokPWM;
      if (errorCenter <= 500) {
        pwmBalik = map(errorCenter, deadzone, 500, startDrivePWM, belokPWM);
      }
      steerTo(arahBalik, pwmBalik);
    }
  }

  // 6. STEER STATE MACHINE
  if (steerPhase == 1 && millis() - steerTimer >= 50) {
    digitalWrite(RELAY_STEER, (pendingSteer == "kanan") ? HIGH : LOW);
    steerPhase = 2; steerTimer = millis();
  }
  else if (steerPhase == 2 && millis() - steerTimer >= 50) {
    ledcWrite(1, activeSteerPWM);
    steerPhase = 0; 
  }

  // 7. DRIVE STATE MACHINE
  if (drivePhase == 1 && millis() - driveTimer >= 300) {
    if (pendingDrive == "maju") executeMaju();
    else if (pendingDrive == "mundur") executeMundur();
  }
  else if (drivePhase == 2) {
    unsigned long elapsed = millis() - driveTimer;
    if (elapsed >= accelTime) {
      currentPWM = speedPWM;
      drivePhase = 0; 
    } else {
      currentPWM = startDrivePWM + ((speedPWM - startDrivePWM) * elapsed / accelTime);
    }
    ledcWrite(activeDriveChannel, currentPWM);
  }
}

// ===== HANDLERS =====
void handleTelemetry() {
  lastWebCmdTime = millis(); // <--- TAMBAHIN BARIS INI untuk remote tetap check remote terhubung
  
  float vbat = ads.readADC_SingleEnded(2) * vbatCalib;
  int16_t adcRoda = (int16_t)smoothedRoda; 
  int16_t adcStir = (int16_t)smoothedStir; 
  
  String json = "{";
  json += "\"vbat\":" + String(vbat, 2) + ",";
  json += "\"roda\":" + String(adcRoda) + ",";
  json += "\"stir\":" + String(adcStir);
  json += "}";
  server.send(200, "application/json", json);
}

void handleGetParams() {
  String json = "{";
  json += "\"lk\":" + String(limitKiri) + ",\"rk\":" + String(limitKanan) + ",";
  json += "\"mid\":" + String(adcTengah) + ",\"dz\":" + String(deadzone) + ",";
  json += "\"spm\":" + String(speedPWM) + ",\"sta\":" + String(startDrivePWM) + ",";
  json += "\"bel\":" + String(belokPWM) + ",\"ac\":" + String(autoCenter) + ",";
  json += "\"acc\":" + String(accelTime) + ",\"man\":" + String(manualMode) + ",";
  json += "\"pk\":" + String(potKiri) + ",\"pr\":" + String(potKanan) + ",";
  json += "\"zgp\":" + String(stirGap) + ",\"dzm\":" + String(dzMan);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetParams() {
  if (server.hasArg("lk"))  limitKiri = server.arg("lk").toInt();
  if (server.hasArg("rk"))  limitKanan = server.arg("rk").toInt();
  if (server.hasArg("mid")) adcTengah = server.arg("mid").toInt();
  if (server.hasArg("dz"))  deadzone = server.arg("dz").toInt();
  if (server.hasArg("spm")) speedPWM = server.arg("spm").toInt();
  if (server.hasArg("sta")) startDrivePWM = server.arg("sta").toInt();
  if (server.hasArg("bel")) belokPWM = server.arg("bel").toInt();
  if (server.hasArg("ac"))  autoCenter = server.arg("ac").toInt();
  if (server.hasArg("acc")) accelTime = server.arg("acc").toInt();
  if (server.hasArg("man")) manualMode = server.arg("man").toInt();
  if (server.hasArg("pk"))  potKiri = server.arg("pk").toInt();
  if (server.hasArg("pr"))  potKanan = server.arg("pr").toInt();
  if (server.hasArg("zgp")) stirGap = server.arg("zgp").toInt();
  if (server.hasArg("dzm")) dzMan   = server.arg("dzm").toInt();

  preferences.putInt("lk", limitKiri); preferences.putInt("rk", limitKanan);
  preferences.putInt("mid", adcTengah); preferences.putInt("dz", deadzone);
  preferences.putInt("spm", speedPWM); preferences.putInt("sta", startDrivePWM);
  preferences.putInt("bel", belokPWM); preferences.putInt("ac", autoCenter);
  preferences.putInt("acc", accelTime); preferences.putInt("man", manualMode);
  preferences.putInt("pk", potKiri); preferences.putInt("pr", potKanan);
  preferences.putInt("zgp", stirGap); preferences.putInt("dzm", dzMan);

  server.send(200, "text/plain", "OK");
}

void handleAction() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    lastWebCmdTime = millis(); 
    
    // Ditambahkan perlindungan ekstra biar nggak restart accelTime kalo tombol dipencet berulang kali
    if (cmd == "maju") { 
      if (pendingDrive != "maju") { stopDrive(); pendingDrive = "maju"; drivePhase = (currentDrive == "mundur") ? 1 : 0; if(drivePhase==0) executeMaju(); else driveTimer=millis(); }
    }
    else if (cmd == "mundur") { 
      if (pendingDrive != "mundur") { stopDrive(); pendingDrive = "mundur"; drivePhase = (currentDrive == "maju") ? 1 : 0; if(drivePhase==0) executeMundur(); else driveTimer=millis(); }
    }
    else if (cmd == "release_drive") stopDrive();
    
    else if (cmd == "kiri") { 
      if(smoothedRoda > limitKiri) { currentSteer = "manual"; steerTo("kiri", belokPWM); } 
    }
    else if (cmd == "kanan") { 
      if(smoothedRoda < limitKanan) { currentSteer = "manual"; steerTo("kanan", belokPWM); } 
    }
    else if (cmd == "release_steer") {
      if (autoCenter == 1) currentSteer = "centering"; 
      else stopSteer(); 
    }
  }
  server.send(200, "text/plain", "OK");
}

void stopDrive() {
  ledcWrite(0, 0); ledcWrite(2, 0);
  digitalWrite(R_EN_PIN, LOW); digitalWrite(L_EN_PIN, LOW);
  if (drivePhase != 1) { currentDrive = "stop"; pendingDrive = ""; }
}

void executeMaju() {
  currentDrive = "maju";
  digitalWrite(R_EN_PIN, HIGH); digitalWrite(L_EN_PIN, HIGH);
  activeDriveChannel = 0; currentPWM = startDrivePWM;
  ledcWrite(0, currentPWM); ledcWrite(2, 0);
  drivePhase = 2; driveTimer = millis();
}

void executeMundur() {
  currentDrive = "mundur";
  digitalWrite(R_EN_PIN, HIGH); digitalWrite(L_EN_PIN, HIGH);
  activeDriveChannel = 2; currentPWM = startDrivePWM;
  ledcWrite(2, currentPWM); ledcWrite(0, 0);
  drivePhase = 2; driveTimer = millis();
}

void stopAll() { stopDrive(); stopSteer(); }
