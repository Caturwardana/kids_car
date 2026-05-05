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

// ===== VARIABEL PARAMETER (NVS) =====
int speedPWM, belokPWM;      
int limitKiri, limitKanan;    
int startDrivePWM; 
int adcTengah, deadzone;
int autoCenter; // 1 = Enable, 0 = Disable

String currentDrive = "stop";
String currentSteer = "lurus";

Adafruit_ADS1115 ads;
WebServer server(80);
Preferences preferences; 
unsigned long lastLimitCheck = 0;

// ===== VARIABEL NON-BLOCKING =====
int drivePhase = 0; 
unsigned long driveTimer = 0;
String pendingDrive = "";
int currentPWM = 0;
int activeDriveChannel = 0;

int steerPhase = 0; 
unsigned long steerTimer = 0;
String pendingSteer = "";

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
    .header { padding: 15px; background: #222; border-bottom: 2px solid #444; display: flex; justify-content: space-between; align-items: center; }
    .btn-settings { background: #555; color: white; padding: 8px 15px; text-decoration: none; border-radius: 5px; font-weight: bold; font-size: 14px;}
    .live-adc { background: #000; padding: 8px 15px; border-radius: 5px; color: #0f0; font-family: monospace; font-size: 18px; font-weight: bold;}
    .container { display: flex; justify-content: space-around; padding: 40px 10px; align-items: center; height: 60vh; }
    .btn-circle { width: 90px; height: 90px; border-radius: 50%; font-size: 16px; font-weight: bold; background: #007bff; color: white; border: 4px solid #0056b3; box-shadow: 0 5px 0 #004494; touch-action: manipulation; }
    .btn-circle:active { transform: translateY(5px); box-shadow: none; background: #0056b3; }
  </style>
</head>
<body>
  <div class="header">
    <div class="live-adc">ADC: <span id="liveADC">0</span></div>
    <div style="font-size: 20px; font-weight: bold; color: #ffcc00;">RC DASHBOARD</div>
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
    function sendCmd(action) { fetch('/action?cmd=' + action); }

    function bindBtn(id, cmdPress, cmdRelease) {
      const btn = document.getElementById(id);
      ['touchstart', 'mousedown'].forEach(evt => btn.addEventListener(evt, (e) => { e.preventDefault(); sendCmd(cmdPress); }));
      ['touchend', 'mouseup'].forEach(evt => btn.addEventListener(evt, (e) => { e.preventDefault(); sendCmd(cmdRelease); }));
    }

    bindBtn('btnMaju', 'maju', 'release_drive');
    bindBtn('btnMundur', 'mundur', 'release_drive');
    bindBtn('btnKiri', 'kiri', 'release_steer');
    bindBtn('btnKanan', 'kanan', 'release_steer');

    setInterval(() => {
      fetch('/get_adc').then(res => res.text()).then(val => {
        document.getElementById('liveADC').innerText = val;
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
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; text-align: left; }
    input[type=number] { width: 100%; box-sizing: border-box; padding: 8px; border-radius: 5px; border: 1px solid #444; background: #333; color: #0f0; font-weight: bold; }
    select { width: 100%; padding: 8px; border-radius: 5px; background: #333; color: white; border: 1px solid #444; }
    .btn-small { background: #666; color: white; border: none; padding: 5px; border-radius: 3px; cursor: pointer; font-size: 10px; width: 100%; margin-top: 5px;}
    .btn-save { width: 100%; padding: 15px; background: #28a745; color: white; border: none; border-radius: 8px; font-weight: bold; margin-top: 20px; font-size: 16px; }
    .btn-back { display: block; margin-bottom: 20px; color: #ffcc00; text-decoration: none; font-weight: bold; text-align: left; }
  </style>
</head>
<body>
  <div class="card">
    <a href="/" class="btn-back">⬅ Kembali ke Kontrol</a>
    <h3 style="color: #ffcc00; border-bottom: 1px solid #444; padding-bottom: 10px;">⚙️ TUNING PARAMETER</h3>
    <p>ADC Live: <b id="liveADC" style="color: #0f0;">0</b></p>
    <div class="grid">
      <div><label>Limit Kiri</label><input type="number" id="valLK"><button class="btn-small" onclick="getADC('valLK')">GET ADC KIRI</button></div>
      <div><label>Limit Kanan</label><input type="number" id="valRK"><button class="btn-small" onclick="getADC('valRK')">GET ADC KANAN</button></div>
      <div><label>Center Lurus</label><input type="number" id="valMid"><button class="btn-small" onclick="getADC('valMid')">GET ADC TENGAH</button></div>
      <div><label>Deadzone ADC</label><input type="number" id="valDz"></div>
      <div><label>Gas Max (PWM)</label><input type="number" id="valSpeed"></div>
      <div><label>Hentakan Awal</label><input type="number" id="valStart"></div>
      <div><label>Tenaga Belok Max</label><input type="number" id="valBelok"></div>
      <div><label>Auto Center</label>
        <select id="valAuto">
          <option value="1">Aktif</option>
          <option value="0">Mati</option>
        </select>
      </div>
    </div>
    <button class="btn-save" onclick="saveParams()">💾 SIMPAN SETTING</button>
  </div>

  <script>
    function getADC(targetId) { fetch('/get_adc').then(res => res.text()).then(val => document.getElementById(targetId).value = val); }
    setInterval(() => { fetch('/get_adc').then(res => res.text()).then(val => document.getElementById('liveADC').innerText = val); }, 800);

    fetch('/get_params').then(res => res.json()).then(data => {
      document.getElementById('valLK').value = data.lk;
      document.getElementById('valRK').value = data.rk;
      document.getElementById('valMid').value = data.mid;
      document.getElementById('valDz').value = data.dz;
      document.getElementById('valSpeed').value = data.spm;
      document.getElementById('valStart').value = data.sta;
      document.getElementById('valBelok').value = data.bel;
      document.getElementById('valAuto').value = data.ac;
    });

    function saveParams() {
      let q = `lk=${document.getElementById('valLK').value}&rk=${document.getElementById('valRK').value}&mid=${document.getElementById('valMid').value}&dz=${document.getElementById('valDz').value}&spm=${document.getElementById('valSpeed').value}&sta=${document.getElementById('valStart').value}&bel=${document.getElementById('valBelok').value}&ac=${document.getElementById('valAuto').value}`;
      fetch('/set_params?' + q).then(() => alert('Tersimpan!'));
    }
  </script>
</body>
</html>
)rawliteral";

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  // Load Parameters NVS
  preferences.begin("rc-config", false);
  limitKiri     = preferences.getInt("lk", 4150);
  limitKanan    = preferences.getInt("rk", 8200);
  adcTengah     = preferences.getInt("mid", 6175); 
  deadzone      = preferences.getInt("dz", 150); 
  speedPWM      = preferences.getInt("spm", 100);
  startDrivePWM = preferences.getInt("sta", 60);
  belokPWM      = preferences.getInt("bel", 150);
  autoCenter    = preferences.getInt("ac", 1); 

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
  server.on("/get_adc", [](){ server.send(200, "text/plain", String(ads.readADC_SingleEnded(1))); });

  server.begin();
}

// ===== LOOP =====
void loop() {
  server.handleClient();
  int16_t adcRoda = ads.readADC_SingleEnded(1);

  // 1. LIMIT CHECK (Emergency Stop)
  if (millis() - lastLimitCheck >= 30) {
    lastLimitCheck = millis();
    if (currentSteer != "centering") {
      if ((currentSteer == "kiri" || pendingSteer == "kiri") && adcRoda <= limitKiri) lurus();
      else if ((currentSteer == "kanan" || pendingSteer == "kanan") && adcRoda >= limitKanan) lurus();
    }
  }

  // 2. AUTO CENTER LOGIC
  if (currentSteer == "centering") {
    // Kalau sudah masuk deadzone, stop steering.
    if (abs(adcRoda - adcTengah) <= deadzone) {
      lurus();
    } else {
      // Tentukan arah kembali (Asumsi: ADC kecil = Kiri, ADC besar = Kanan)
      String arahBalik = (adcRoda < adcTengah) ? "kanan" : "kiri";
      
      // Hitung kecepatan balik natural (sesuai mobil jalan)
      int returnPWM = startDrivePWM;
      if (currentDrive != "stop") {
        // Semakin kencang mobil melaju (currentPWM), tarikan setir ke tengah semakin kuat
        returnPWM = map(currentPWM, 0, 255, startDrivePWM, belokPWM); 
      } else {
        // Kalau mobil berhenti, balik pelan aja
        returnPWM = belokPWM / 2;
      }

      // Eksekusi belok ke arah tengah
      if (pendingSteer != arahBalik && steerPhase == 0) {
        pendingSteer = arahBalik;
        steerPhase = 1;
        steerTimer = millis();
        ledcWrite(1, 0); // Matikan putaran stir sebentar buat ganti relay
      } else if (steerPhase == 0) {
        ledcWrite(1, returnPWM); // Jalankan PWM
      }
    }
  }

  // 3. STEER STATE MACHINE
  if (steerPhase == 1 && millis() - steerTimer >= 50) {
    digitalWrite(RELAY_STEER, (pendingSteer == "kanan") ? HIGH : LOW);
    steerPhase = 2; steerTimer = millis();
  }
  else if (steerPhase == 2 && millis() - steerTimer >= 50) {
    // Kalau sedang auto-center, biarkan logika centering yang ngatur PWM-nya di atas
    if (currentSteer != "centering") {
      ledcWrite(1, belokPWM);
      currentSteer = pendingSteer; 
    }
    steerPhase = 0;
  }

  // 4. DRIVE STATE MACHINE
  if (drivePhase == 1 && millis() - driveTimer >= 300) {
    if (pendingDrive == "maju") executeMaju();
    else if (pendingDrive == "mundur") executeMundur();
  }
  else if (drivePhase == 2 && millis() - driveTimer >= 20) {
    currentPWM += 5;
    if (currentPWM >= speedPWM) { currentPWM = speedPWM; drivePhase = 0; }
    ledcWrite(activeDriveChannel, currentPWM);
    driveTimer = millis();
  }
}

// ===== HANDLERS =====
void handleGetParams() {
  String json = "{";
  json += "\"lk\":" + String(limitKiri) + ",\"rk\":" + String(limitKanan) + ",";
  json += "\"mid\":" + String(adcTengah) + ",\"dz\":" + String(deadzone) + ",";
  json += "\"spm\":" + String(speedPWM) + ",\"sta\":" + String(startDrivePWM) + ",";
  json += "\"bel\":" + String(belokPWM) + ",\"ac\":" + String(autoCenter);
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

  preferences.putInt("lk", limitKiri); preferences.putInt("rk", limitKanan);
  preferences.putInt("mid", adcTengah); preferences.putInt("dz", deadzone);
  preferences.putInt("spm", speedPWM); preferences.putInt("sta", startDrivePWM);
  preferences.putInt("bel", belokPWM); preferences.putInt("ac", autoCenter);

  server.send(200, "text/plain", "OK");
}

void handleAction() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "maju") { stopDrive(); pendingDrive = "maju"; drivePhase = (currentDrive == "mundur") ? 1 : 0; if(drivePhase==0) executeMaju(); else driveTimer=millis(); }
    else if (cmd == "mundur") { stopDrive(); pendingDrive = "mundur"; drivePhase = (currentDrive == "maju") ? 1 : 0; if(drivePhase==0) executeMundur(); else driveTimer=millis(); }
    else if (cmd == "release_drive") stopDrive();
    
    // Steering Commands
    else if (cmd == "kiri") { if(ads.readADC_SingleEnded(1) > limitKiri) { currentSteer = "manual"; pendingSteer = "kiri"; steerPhase = 1; steerTimer = millis(); ledcWrite(1, 0); } }
    else if (cmd == "kanan") { if(ads.readADC_SingleEnded(1) < limitKanan) { currentSteer = "manual"; pendingSteer = "kanan"; steerPhase = 1; steerTimer = millis(); ledcWrite(1, 0); } }
    else if (cmd == "release_steer") {
      if (autoCenter == 1) currentSteer = "centering"; 
      else lurus(); 
    }
  }
  server.send(200, "text/plain", "OK");
}

void stopDrive() {
  ledcWrite(0, 0); ledcWrite(2, 0);
  digitalWrite(R_EN_PIN, LOW); digitalWrite(L_EN_PIN, LOW);
  if (drivePhase != 1) currentDrive = "stop";
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

void lurus() { steerPhase = 0; currentSteer = "lurus"; pendingSteer = "lurus"; ledcWrite(1, 0); }
void stopAll() { stopDrive(); lurus(); }