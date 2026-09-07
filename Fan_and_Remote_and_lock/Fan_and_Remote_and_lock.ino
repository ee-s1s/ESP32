#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRsend.h>
#include <ArduinoOTA.h>

// ==========================================
// 1. إعدادات الشبكة والعتاد
// ==========================================
const char* ssid = "001";
const char* password = "MYYEMEN12";

IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// منافذ وعتاد القفل الذكي
const int RELAY_PIN = 27;                      // منفذ الريليه
const unsigned long LOCK_OPEN_DURATION = 250;  // مدة فتح القفل بالميللي ثانية

// إعدادات حماية المروحة
const char* www_password = "12345";
String sessionToken = "ESP32_FAN_AUTH_TOKEN_998877";

// منفذ الـ IR LED
const uint16_t kIrLed = 4;
IRsend irsend(kIrLed);

// ==========================================
// 2. أكواد الـ IR (سامسونج، ستارسات، المروحة)
// ==========================================
// شاشة سامسونج (Samsung32)
#define SAMSUNG_POWER      0xE0E040BF
#define SAMSUNG_VOL_UP     0xE0E0E01F
#define SAMSUNG_VOL_DOWN   0xE0E0D02F
#define SAMSUNG_MUTE       0xE0E0F00F
#define SAMSUNG_CH_UP      0xE0E048B7
#define SAMSUNG_CH_DOWN    0xE0E008F7
#define SAMSUNG_UP         0xE0E006F9
#define SAMSUNG_DOWN       0xE0E08679
#define SAMSUNG_LEFT       0xE0E0A659
#define SAMSUNG_RIGHT      0xE0E046B9
#define SAMSUNG_ENTER      0xE0E016E9
#define SAMSUNG_RETURN     0xE0E01AE5
#define SAMSUNG_EXIT       0xE0E0B44B
#define SAMSUNG_HOME       0xE0E09E61
#define SAMSUNG_SOURCE     0xE0E0807F
#define SAMSUNG_MENU       0xE0E058A7

// رسيفر StarSat SR-550HD (NEC)
#define STARSAT_POWER      0x60D550AF 
#define STARSAT_MUTE       0x60D5F00F 
#define STARSAT_UP         0x60D5D02F 
#define STARSAT_DOWN       0x60D5708F 
#define STARSAT_LEFT       0x60D508F7 
#define STARSAT_RIGHT      0x60D58877 
#define STARSAT_OK         0x60D5B04F 
#define STARSAT_MENU       0x60D502FD 
#define STARSAT_EXIT       0x60D542BD 
#define STARSAT_SAT        0x60D5A25D 
#define STARSAT_INFO       0x60D5827D 
#define STARSAT_VOL_UP     0x60D5A857
#define STARSAT_VOL_DOWN   0x60D538C7
#define STARSAT_CH_UP      0x60D5F807
#define STARSAT_CH_DOWN    0x60D57887
#define STARSAT_0          0x60D500FF 
#define STARSAT_1          0x60D5807F 
#define STARSAT_2          0x60D540BF 
#define STARSAT_3          0x60D5C03F 
#define STARSAT_4          0x60D520DF 
#define STARSAT_5          0x60D5A05F 
#define STARSAT_6          0x60D5609F 
#define STARSAT_7          0x60D5E01F 
#define STARSAT_8          0x60D510EF 
#define STARSAT_9          0x60D5906F 

// المروحة (NEC / Raw Standard Format)
#define FAN_ON_SPEED  0x000001F2
#define FAN_OFF       0x85649F80
#define FAN_MODE      0x000002F2
#define FAN_TIMER     0x00000372
#define FAN_SWING     0x000003B2
#define FAN_LAMP      0xE258A0CF

// ==========================================
// 3. واجهة الويب الاحترافية
// ==========================================
const char HTML_INTERFACE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP32 Smart Controller</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css">
    <style>
        :root {
            --bg-gradient: linear-gradient(135deg, #0b1329 0%, #17153b 50%, #0d1117 100%);
            --card-bg: rgba(23, 31, 52, 0.75);
            --card-border: rgba(255, 255, 255, 0.12);
            --primary: #10b981;
            --primary-glow: rgba(16, 185, 129, 0.4);
            --danger: #ef4444;
            --danger-glow: rgba(239, 68, 68, 0.4);
            --accent: #6366f1;
            --accent-glow: rgba(99, 102, 241, 0.4);
            --text-main: #f8fafc;
            --text-sub: #94a3b8;
            --btn-bg: rgba(255, 255, 255, 0.05);
            --btn-hover: rgba(255, 255, 255, 0.12);
            --btn-active: rgba(255, 255, 255, 0.2);
        }

        * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; font-family: system-ui, -apple-system, sans-serif; }
        body { background: var(--bg-gradient); background-attachment: fixed; color: var(--text-main); min-height: 100vh; margin: 0; padding: 1rem; display: flex; justify-content: center; align-items: flex-start; }
        .app-container { width: 100%; max-width: 480px; background: var(--card-bg); backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px); border: 1px solid var(--card-border); border-radius: 1.75rem; padding: 1.25rem; box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.6); }
        header { text-align: center; margin-bottom: 1.25rem; }
        header h1 { font-size: 1.35rem; margin: 0; color: var(--text-main); font-weight: 700; display: flex; align-items: center; justify-content: center; gap: 0.5rem; }
        header p { font-size: 0.8rem; color: var(--text-sub); margin: 0.35rem 0 0 0; }
        .tabs { display: flex; background: rgba(10, 15, 30, 0.7); padding: 0.35rem; border-radius: 1.2rem; gap: 0.25rem; margin-bottom: 1.25rem; border: 1px solid var(--card-border); }
        .tab-btn { flex: 1; padding: 0.65rem 0.2rem; font-size: 0.82rem; font-weight: 600; border: none; background: transparent; color: var(--text-sub); border-radius: 0.85rem; cursor: pointer; transition: all 0.3s ease; display: flex; flex-direction: column; align-items: center; gap: 0.35rem; }
        .tab-btn.active { background: var(--accent); color: #ffffff; box-shadow: 0 4px 15px var(--accent-glow); }
        .tab-content { display: none; opacity: 0; transition: all 0.3s ease; }
        .tab-content.active { display: block; opacity: 1; }
        .lock-box { text-align: center; padding: 1.75rem 1rem; background: rgba(10, 15, 30, 0.4); border-radius: 1.25rem; border: 1px solid var(--card-border); }
        .lock-icon-wrapper { width: 100px; height: 100px; margin: 0 auto 1.25rem auto; border-radius: 50%; background: rgba(16, 185, 129, 0.1); border: 2px solid var(--primary); display: flex; align-items: center; justify-content: center; font-size: 2.75rem; color: var(--primary); transition: all 0.4s ease; }
        .lock-icon-wrapper.unlocked { background: rgba(239, 68, 68, 0.15); border-color: var(--danger); color: var(--danger); }
        .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 0.6rem; }
        .btn { background: var(--btn-bg); border: 1px solid var(--card-border); color: var(--text-main); padding: 0.85rem 0.25rem; font-size: 0.82rem; font-weight: 600; border-radius: 0.85rem; cursor: pointer; transition: all 0.2s ease; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 0.4rem; user-select: none; }
        .btn:hover { background: var(--btn-hover); transform: translateY(-2px); }
        .btn:active { transform: scale(0.93); }
        .btn-main { background: linear-gradient(135deg, #10b981 0%, #059669 100%); color: #ffffff; font-size: 1.05rem; padding: 1rem; width: 100%; border: none; flex-direction: row; gap: 0.6rem; }
        .btn-danger { background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%); border: none; }
        .btn-wide { grid-column: span 3; flex-direction: row; gap: 0.5rem; }
        input[type="password"] { width: 100%; padding: 0.85rem; border-radius: 0.85rem; border: 1px solid var(--card-border); background: rgba(10, 15, 30, 0.6); color: #ffffff; font-size: 0.95rem; text-align: center; margin-bottom: 0.75rem; outline: none; }
    </style>
</head>
<body>
    <div class="app-container">
        <header>
            <h1><i class="fa-solid fa-microchip"></i> التحكم الذكي ESP32</h1>
            <p>نظام القفل وأجهزة التحكم عبر الـ IR</p>
        </header>

        <div class="tabs">
            <button class="tab-btn active" onclick="switchTab('tab-lock', this)"><i class="fa-solid fa-lock"></i> القفل</button>
            <button class="tab-btn" onclick="switchTab('tab-sam', this)"><i class="fa-solid fa-tv"></i> سامسونج</button>
            <button class="tab-btn" onclick="switchTab('tab-sat', this)"><i class="fa-solid fa-satellite-dish"></i> ستارسات</button>
            <button class="tab-btn" onclick="switchTab('tab-fan', this)"><i class="fa-solid fa-fan"></i> المروحة</button>
        </div>

        <div id="tab-lock" class="tab-content active">
            <div class="lock-box">
                <div id="lockIconWrapper" class="lock-icon-wrapper"><i id="lockIcon" class="fa-solid fa-lock"></i></div>
                <button id="unlockBtn" class="btn btn-main" onclick="triggerUnlock()"><i class="fa-solid fa-key"></i> فتح القفل الذكي</button>
            </div>
        </div>

        <div id="tab-sam" class="tab-content">
            <div class="grid">
                <button class="btn btn-danger" onclick="sendCmd('sam_power')"><i class="fa-solid fa-power-off"></i>تشغيل / إيقاف</button>
                <button class="btn" onclick="sendCmd('sam_source')"><i class="fa-solid fa-sliders"></i>المصدر</button>
                <button class="btn" onclick="sendCmd('sam_mute')"><i class="fa-solid fa-volume-xmark"></i>كتم</button>
                <button class="btn" onclick="sendCmd('sam_volup')"><i class="fa-solid fa-volume-high"></i>صوت +</button>
                <button class="btn" onclick="sendCmd('sam_up')"><i class="fa-solid fa-chevron-up"></i>▲</button>
                <button class="btn" onclick="sendCmd('sam_chup')"><i class="fa-solid fa-angle-up"></i>قناة +</button>
                <button class="btn" onclick="sendCmd('sam_left')"><i class="fa-solid fa-chevron-right"></i>◄</button>
                <button class="btn" onclick="sendCmd('sam_enter')"><i class="fa-solid fa-circle-dot"></i>موافق</button>
                <button class="btn" onclick="sendCmd('sam_right')"><i class="fa-solid fa-chevron-left"></i>►</button>
                <button class="btn" onclick="sendCmd('sam_voldown')"><i class="fa-solid fa-volume-low"></i>صوت -</button>
                <button class="btn" onclick="sendCmd('sam_down')"><i class="fa-solid fa-chevron-down"></i>▼</button>
                <button class="btn" onclick="sendCmd('sam_chdown')"><i class="fa-solid fa-angle-down"></i>قناة -</button>
                <button class="btn" onclick="sendCmd('sam_home')"><i class="fa-solid fa-house"></i>الرئيسية</button>
                <button class="btn" onclick="sendCmd('sam_menu')"><i class="fa-solid fa-bars"></i>القائمة</button>
                <button class="btn" onclick="sendCmd('sam_return')"><i class="fa-solid fa-rotate-left"></i>رجوع</button>
                <button class="btn btn-wide" onclick="sendCmd('sam_exit')"><i class="fa-solid fa-right-from-bracket"></i>خروج (Exit)</button>
            </div>
        </div>

        <div id="tab-sat" class="tab-content">
            <div class="grid">
                <button class="btn btn-danger" onclick="sendCmd('sat_power')"><i class="fa-solid fa-power-off"></i>تشغيل / إيقاف</button>
                <button class="btn" onclick="sendCmd('sat_sat')"><i class="fa-solid fa-satellite"></i>SAT</button>
                <button class="btn" onclick="sendCmd('sat_mute')"><i class="fa-solid fa-volume-xmark"></i>كتم</button>
                <button class="btn" onclick="sendCmd('sat_volup')"><i class="fa-solid fa-volume-high"></i>صوت +</button>
                <button class="btn" onclick="sendCmd('sat_up')"><i class="fa-solid fa-chevron-up"></i>▲</button>
                <button class="btn" onclick="sendCmd('sat_chup')"><i class="fa-solid fa-angle-up"></i>قناة +</button>
                <button class="btn" onclick="sendCmd('sat_left')"><i class="fa-solid fa-chevron-right"></i>◄</button>
                <button class="btn" onclick="sendCmd('sat_ok')"><i class="fa-solid fa-circle-dot"></i>OK</button>
                <button class="btn" onclick="sendCmd('sat_right')"><i class="fa-solid fa-chevron-left"></i>►</button>
                <button class="btn" onclick="sendCmd('sat_voldown')"><i class="fa-solid fa-volume-low"></i>صوت -</button>
                <button class="btn" onclick="sendCmd('sat_down')"><i class="fa-solid fa-chevron-down"></i>▼</button>
                <button class="btn" onclick="sendCmd('sat_chdown')"><i class="fa-solid fa-angle-down"></i>قناة -</button>
                <button class="btn" onclick="sendCmd('sat_menu')"><i class="fa-solid fa-bars"></i>القائمة</button>
                <button class="btn" onclick="sendCmd('sat_info')"><i class="fa-solid fa-circle-info"></i>INFO</button>
                <button class="btn" onclick="sendCmd('sat_exit')"><i class="fa-solid fa-right-from-bracket"></i>خروج</button>
                <button class="btn" onclick="sendCmd('sat_1')">1</button>
                <button class="btn" onclick="sendCmd('sat_2')">2</button>
                <button class="btn" onclick="sendCmd('sat_3')">3</button>
                <button class="btn" onclick="sendCmd('sat_4')">4</button>
                <button class="btn" onclick="sendCmd('sat_5')">5</button>
                <button class="btn" onclick="sendCmd('sat_6')">6</button>
                <button class="btn" onclick="sendCmd('sat_7')">7</button>
                <button class="btn" onclick="sendCmd('sat_8')">8</button>
                <button class="btn" onclick="sendCmd('sat_9')">9</button>
                <button class="btn btn-wide" onclick="sendCmd('sat_0')">0</button>
            </div>
        </div>

        <div id="tab-fan" class="tab-content">
            <div id="fanAuthSection">
                <input type="password" id="fanPass" placeholder="كلمة المرور لتفعيل المروحة">
                <button class="btn btn-main" onclick="loginFan()"><i class="fa-solid fa-lock-open"></i> تسجيل الدخول</button>
            </div>
            <div id="fanControls" class="grid" style="display: none;">
                <button class="btn" onclick="sendFanCmd('fan_on_speed')"><i class="fa-solid fa-gauge-high"></i>تشغيل/سرعة</button>
                <button class="btn btn-danger" onclick="sendFanCmd('fan_off')"><i class="fa-solid fa-power-off"></i>إيقاف</button>
                <button class="btn" onclick="sendFanCmd('fan_mode')"><i class="fa-solid fa-wind"></i>الوضع</button>
                <button class="btn" onclick="sendFanCmd('fan_timer')"><i class="fa-solid fa-clock"></i>المؤقت</button>
                <button class="btn" onclick="sendFanCmd('fan_swing')"><i class="fa-solid fa-arrows-left-right"></i>الدوران</button>
                <button class="btn" onclick="sendFanCmd('fan_lamp')"><i class="fa-solid fa-lightbulb"></i>الإضاءة</button>
            </div>
        </div>
    </div>

    <script>
        let fanToken = "";
        function switchTab(tabId, btnElement) {
            document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
            btnElement.classList.add('active');
            document.getElementById(tabId).classList.add('active');
        }
        async function triggerUnlock() {
            const btn = document.getElementById('unlockBtn');
            const iconWrapper = document.getElementById('lockIconWrapper');
            const icon = document.getElementById('lockIcon');
            btn.disabled = true;
            iconWrapper.classList.add('unlocked');
            icon.className = 'fa-solid fa-lock-open';
            try {
                await fetch('/api/unlock', { method: 'POST' });
                setTimeout(() => {
                    iconWrapper.classList.remove('unlocked');
                    icon.className = 'fa-solid fa-lock';
                    btn.disabled = false;
                }, 1500);
            } catch (e) {
                alert('فشل الاتصال بالخادم!');
                iconWrapper.classList.remove('unlocked');
                icon.className = 'fa-solid fa-lock';
                btn.disabled = false;
            }
        }
        async function sendCmd(code) {
            try { await fetch('/api/cmd?code=' + code); } catch (e) { console.error(e); }
        }
        async function loginFan() {
            const pass = document.getElementById('fanPass').value;
            try {
                const res = await fetch('/api/login?pass=' + encodeURIComponent(pass));
                const data = await res.json();
                if (data.success) {
                    fanToken = data.token;
                    document.getElementById('fanAuthSection').style.display = 'none';
                    document.getElementById('fanControls').style.display = 'grid';
                } else { alert('كلمة المرور غير صحيحة!'); }
            } catch (e) { alert('خطأ أثناء تسجيل الدخول'); }
        }
        async function sendFanCmd(code) {
            if (!fanToken) return;
            try { await fetch(`/api/cmd?code=${code}&token=${fanToken}`); } catch (e) { console.error(e); }
        }
    </script>
</body>
</html>
)rawliteral";

// ==========================================
// 4. خادم الويب ومتغيرات التحكم
// ==========================================
AsyncWebServer server(80);

bool isUnlocked = false;
unsigned long unlockStartTime = 0;

// ==========================================
// 5. دالتا المعالجة الأساسية
// ==========================================

void triggerUnlockMechanism() {
    if (!isUnlocked) {
        digitalWrite(RELAY_PIN, HIGH);
        isUnlocked = true;
        unlockStartTime = millis();
        Serial.println("تم فتح القفل!");
    }
}

bool handleCommand(String cmd) {
    uint32_t codeToSend = 0;

    // --- سامسونج ---
    if (cmd == "sam_power") codeToSend = SAMSUNG_POWER;
    else if (cmd == "sam_volup") codeToSend = SAMSUNG_VOL_UP;
    else if (cmd == "sam_voldown") codeToSend = SAMSUNG_VOL_DOWN;
    else if (cmd == "sam_mute") codeToSend = SAMSUNG_MUTE;
    else if (cmd == "sam_chup") codeToSend = SAMSUNG_CH_UP;
    else if (cmd == "sam_chdown") codeToSend = SAMSUNG_CH_DOWN;
    else if (cmd == "sam_up") codeToSend = SAMSUNG_UP;
    else if (cmd == "sam_down") codeToSend = SAMSUNG_DOWN;
    else if (cmd == "sam_left") codeToSend = SAMSUNG_LEFT;
    else if (cmd == "sam_right") codeToSend = SAMSUNG_RIGHT;
    else if (cmd == "sam_enter") codeToSend = SAMSUNG_ENTER;
    else if (cmd == "sam_return") codeToSend = SAMSUNG_RETURN;
    else if (cmd == "sam_exit") codeToSend = SAMSUNG_EXIT;
    else if (cmd == "sam_home") codeToSend = SAMSUNG_HOME;
    else if (cmd == "sam_source") codeToSend = SAMSUNG_SOURCE;
    else if (cmd == "sam_menu") codeToSend = SAMSUNG_MENU;

    if (codeToSend != 0) {
        irsend.sendSAMSUNG(codeToSend, 32);
        return true;
    }

    // --- ستارسات ---
    if (cmd == "sat_power") codeToSend = STARSAT_POWER;
    else if (cmd == "sat_mute") codeToSend = STARSAT_MUTE;
    else if (cmd == "sat_up") codeToSend = STARSAT_UP;
    else if (cmd == "sat_down") codeToSend = STARSAT_DOWN;
    else if (cmd == "sat_left") codeToSend = STARSAT_LEFT;
    else if (cmd == "sat_right") codeToSend = STARSAT_RIGHT;
    else if (cmd == "sat_ok") codeToSend = STARSAT_OK;
    else if (cmd == "sat_menu") codeToSend = STARSAT_MENU;
    else if (cmd == "sat_exit") codeToSend = STARSAT_EXIT;
    else if (cmd == "sat_sat") codeToSend = STARSAT_SAT;
    else if (cmd == "sat_info") codeToSend = STARSAT_INFO;
    else if (cmd == "sat_volup") codeToSend = STARSAT_VOL_UP;
    else if (cmd == "sat_voldown") codeToSend = STARSAT_VOL_DOWN;
    else if (cmd == "sat_chup") codeToSend = STARSAT_CH_UP;
    else if (cmd == "sat_chdown") codeToSend = STARSAT_CH_DOWN;
    else if (cmd == "sat_0") codeToSend = STARSAT_0;
    else if (cmd == "sat_1") codeToSend = STARSAT_1;
    else if (cmd == "sat_2") codeToSend = STARSAT_2;
    else if (cmd == "sat_3") codeToSend = STARSAT_3;
    else if (cmd == "sat_4") codeToSend = STARSAT_4;
    else if (cmd == "sat_5") codeToSend = STARSAT_5;
    else if (cmd == "sat_6") codeToSend = STARSAT_6;
    else if (cmd == "sat_7") codeToSend = STARSAT_7;
    else if (cmd == "sat_8") codeToSend = STARSAT_8;
    else if (cmd == "sat_9") codeToSend = STARSAT_9;

    if (codeToSend != 0) {
        irsend.sendNEC(codeToSend, 32);
        return true;
    }

    // --- المروحة ---
    if (cmd == "fan_on_speed") codeToSend = FAN_ON_SPEED;
    else if (cmd == "fan_off") codeToSend = FAN_OFF;
    else if (cmd == "fan_mode") codeToSend = FAN_MODE;
    else if (cmd == "fan_timer") codeToSend = FAN_TIMER;
    else if (cmd == "fan_swing") codeToSend = FAN_SWING;
    else if (cmd == "fan_lamp") codeToSend = FAN_LAMP;

    if (codeToSend != 0) {
        irsend.sendNEC(codeToSend, 32);
        return true;
    }

    return false;
}

void setCORS(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ==========================================
// 6. التهيئة والتشغيل (Setup)
// ==========================================
void setup() {
    Serial.begin(115200);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    irsend.begin();

    WiFi.mode(WIFI_STA);
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure Static IP");
    }

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print(".");
    }
    Serial.println("\nConnected! IP Address: " + WiFi.localIP().toString());

    ArduinoOTA.setHostname("esp32-smartlock-remote");
    ArduinoOTA.setPassword("admin123");
    ArduinoOTA.begin();

    // Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", HTML_INTERFACE);
    });

    server.onRequest([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(204);
            setCORS(response);
            request->send(response);
        }
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"status\":\"success\",\"server\":\"online\",\"lock_state\":\"";
        json += isUnlocked ? "unlocked" : "locked";
        json += "\",\"uptime_ms\":";
        json += String(millis());
        json += "}";

        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        setCORS(response);
        request->send(response);
    });

    server.on("/api/unlock", HTTP_POST, [](AsyncWebServerRequest *request){
        String jsonResponse;
        int httpCode = 200;

        if (!isUnlocked) {
            triggerUnlockMechanism();
            jsonResponse = "{\"status\":\"success\",\"message\":\"Unlocked\",\"state\":\"unlocked\"}";
        } else {
            httpCode = 400;
            jsonResponse = "{\"status\":\"error\",\"message\":\"Already unlocked\",\"state\":\"unlocked\"}";
        }

        AsyncWebServerResponse *response = request->beginResponse(httpCode, "application/json", jsonResponse);
        setCORS(response);
        request->send(response);
    });

    server.on("/api/login", HTTP_GET, [](AsyncWebServerRequest *request){
        String jsonResponse;
        int httpCode = 200;

        if (request->hasParam("pass")) {
            if (request->getParam("pass")->value() == www_password) {
                jsonResponse = "{\"success\":true,\"token\":\"" + sessionToken + "\"}";
            } else {
                httpCode = 401;
                jsonResponse = "{\"success\":false,\"error\":\"Invalid password\"}";
            }
        } else {
            httpCode = 400;
            jsonResponse = "{\"success\":false,\"error\":\"Missing pass parameter\"}";
        }

        AsyncWebServerResponse *response = request->beginResponse(httpCode, "application/json", jsonResponse);
        setCORS(response);
        request->send(response);
    });

    server.on("/api/cmd", HTTP_GET, [](AsyncWebServerRequest *request){
        String jsonResponse;
        int httpCode = 200;

        if (request->hasParam("code")) {
            String cmd = request->getParam("code")->value();

            if (cmd.startsWith("fan_")) {
                bool authenticated = request->hasParam("token") && (request->getParam("token")->value() == sessionToken);
                if (!authenticated) {
                    AsyncWebServerResponse *response = request->beginResponse(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
                    setCORS(response);
                    request->send(response);
                    return;
                }
            }

            if (handleCommand(cmd)) {
                jsonResponse = "{\"success\":true,\"command\":\"" + cmd + "\"}";
            } else {
                httpCode = 400;
                jsonResponse = "{\"success\":false,\"error\":\"Unknown command\"}";
            }
        } else {
            httpCode = 400;
            jsonResponse = "{\"success\":false,\"error\":\"Missing code parameter\"}";
        }

        AsyncWebServerResponse *response = request->beginResponse(httpCode, "application/json", jsonResponse);
        setCORS(response);
        request->send(response);
    });

    server.begin();
}

// ==========================================
// 7. الحلقة التكرارية (Loop)
// ==========================================
void loop() {
    ArduinoOTA.handle();

    if (isUnlocked && ((unsigned long)(millis() - unlockStartTime) >= LOCK_OPEN_DURATION)) {
        digitalWrite(RELAY_PIN, LOW);
        isUnlocked = false;
        Serial.println("تم إعادة إغلاق القفل تلقائياً.");
    }
}