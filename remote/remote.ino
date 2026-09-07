#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRsend.h>

// ================= إعدادات الشبكة =================
const char* ssid = "001";          // اسم شبكة الواي فاي
const char* password = "MYYEMEN12";  // كلمة سر الواي فاي

// إعدادات الـ IP الثابت
IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// ================= إعدادات الـ IR =================
const uint16_t kIrLed = 4; // منفذ توصيل الـ IR LED (GPIO 4)
IRsend irsend(kIrLed);

// ================= أكواد شاشة سامسونج (Samsung32) =================
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

// ================= أكواد رسيفر StarSat SR-550HD (NEC Protocol) =================
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

#define STARSAT_VOL_UP     0x60D5A857  // يتشارك نفس زر السهم الأيمن في معظم موديلات 550
#define STARSAT_VOL_DOWN   0x60D538C7  // يتشارك نفس زر السهم الأيسر
#define STARSAT_CH_UP      0x60D5F807  // يتشارك نفس زر السهم الأعلى
#define STARSAT_CH_DOWN    0x60D57887  // يتشارك نفس زر السهم الأسفل

// أكواد الأرقام (0-9) لرسيفر StarSat SR-550HD
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
// خادم الويب
AsyncWebServer server(80);

// صفحة الويب التفاعلية المزدوجة (HTML + CSS + JS)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Universal Smart Remote</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #0f0f13;
            color: #ffffff;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            padding: 15px;
        }
        .tab-buttons {
            display: flex;
            width: 100%;
            max-width: 330px;
            margin-bottom: 15px;
            background: #1a1a22;
            border-radius: 20px;
            padding: 5px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.5);
        }
        .tab-btn {
            flex: 1;
            padding: 12px;
            border: none;
            background: transparent;
            color: #a0a0b0;
            font-size: 15px;
            font-weight: bold;
            border-radius: 15px;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        .tab-btn.active {
            background: linear-gradient(135deg, #1e90ff, #00bfff);
            color: #ffffff;
            box-shadow: 0 4px 10px rgba(30, 144, 255, 0.4);
        }
        .remote-card {
            background: linear-gradient(145deg, #1e1e24, #141418);
            border-radius: 30px;
            padding: 25px 20px;
            box-shadow: 0 15px 35px rgba(0, 0, 0, 0.7), inset 0 1px 1px rgba(255, 255, 255, 0.1);
            width: 100%;
            max-width: 330px;
            display: none;
            flex-direction: column;
            gap: 18px;
            border: 1px solid #2a2a35;
        }
        .remote-card.active {
            display: flex;
        }
        .header {
            text-align: center;
            color: #00bfff;
            font-size: 13px;
            letter-spacing: 2px;
            text-transform: uppercase;
            font-weight: 600;
        }
        .row {
            display: flex;
            justify-content: space-between;
            gap: 10px;
        }
        .btn {
            background: #252530;
            color: #e0e0e0;
            border: none;
            height: 50px;
            border-radius: 14px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.15s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            flex: 1;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.3);
            user-select: none;
            -webkit-tap-highlight-color: transparent;
        }
        .btn:active {
            transform: scale(0.92);
            background: #323242;
            box-shadow: inset 0 2px 5px rgba(0,0,0,0.5);
        }
        .btn-power {
            background: linear-gradient(135deg, #ff416c, #ff4b2b);
            color: #ffffff;
        }
        .dpad {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 8px;
            background: #181820;
            padding: 12px;
            border-radius: 50%;
            width: 210px;
            height: 210px;
            margin: 0 auto;
            align-items: center;
            justify-items: center;
            box-shadow: inset 0 4px 10px rgba(0,0,0,0.5);
        }
        .dpad .btn {
            width: 55px;
            height: 55px;
            border-radius: 50%;
        }
        .btn-ok {
            background: linear-gradient(135deg, #11998e, #38ef7d);
            color: #ffffff;
            font-size: 15px;
            font-weight: 800;
        }
        .numpad {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 8px;
            margin-top: 5px;
        }
        .numpad .btn {
            height: 42px;
            font-size: 15px;
            background: #1a1a22;
        }
        .empty { visibility: hidden; }
        .icon { display: inline-block; line-height: 1; }
    </style>
</head>
<body>

    <!-- أزرار التبديل بين الأجهزة -->
    <div class="tab-buttons">
        <button class="tab-btn active" onclick="switchTab('samsung')">تلفزيون Samsung</button>
        <button class="tab-btn" onclick="switchTab('starsat')">رسيفر StarSat</button>
    </div>

    <!-- ================= ريموت سامسونج ================= -->
    <div id="samsung-remote" class="remote-card active">
        <div class="header">SAMSUNG TV REMOTE</div>

        <div class="row">
            <button class="btn btn-power" onclick="sendCmd('sam_power')"><span class="icon">⏻</span></button>
            <button class="btn" onclick="sendCmd('sam_mute')"><span class="icon">🔇</span></button>
            <button class="btn" onclick="sendCmd('sam_source')"><span class="icon">🔌</span></button>
        </div>

        <div class="row">
            <button class="btn" onclick="sendCmd('sam_home')"><span class="icon">🏠</span></button>
            <button class="btn" onclick="sendCmd('sam_menu')"><span class="icon">⚙️</span></button>
            <button class="btn" onclick="sendCmd('sam_exit')"><span class="icon">✖</span></button>
        </div>

        <div class="dpad">
            <div class="empty"></div>
            <button class="btn" onclick="sendCmd('sam_up')"><span class="icon">▲</span></button>
            <div class="empty"></div>

            <button class="btn" onclick="sendCmd('sam_left')"><span class="icon">◄</span></button>
            <button class="btn btn-ok" onclick="sendCmd('sam_enter')">OK</button>
            <button class="btn" onclick="sendCmd('sam_right')"><span class="icon">►</span></button>

            <div class="empty"></div>
            <button class="btn" onclick="sendCmd('sam_down')"><span class="icon">▼</span></button>
            <div class="empty"></div>
        </div>

        <div class="row">
            <button class="btn" style="flex: 2;" onclick="sendCmd('sam_return')"><span class="icon">↩ عودة</span></button>
        </div>

        <div class="row">
            <button class="btn" onclick="sendCmd('sam_volup')"><span class="icon">🔊 +</span></button>
            <button class="btn" onclick="sendCmd('sam_chup')"><span class="icon">📺 +</span></button>
        </div>
        <div class="row">
            <button class="btn" onclick="sendCmd('sam_voldown')"><span class="icon">🔉 -</span></button>
            <button class="btn" onclick="sendCmd('sam_chdown')"><span class="icon">📺 -</span></button>
        </div>
    </div>

    <!-- ================= ريموت ستارسات ================= -->
    <div id="starsat-remote" class="remote-card">
        <div class="header">STARSAT RECEIVER</div>

        <div class="row">
            <button class="btn btn-power" onclick="sendCmd('sat_power')"><span class="icon">⏻</span></button>
            <button class="btn" onclick="sendCmd('sat_mute')"><span class="icon">🔇</span></button>
            <button class="btn" onclick="sendCmd('sat_sat')">SAT</button>
        </div>

        <div class="row">
            <button class="btn" onclick="sendCmd('sat_menu')"><span class="icon">📋 القائمة</span></button>
            <button class="btn" onclick="sendCmd('sat_info')"><span class="icon">ℹ️ معلومات</span></button>
            <button class="btn" onclick="sendCmd('sat_exit')"><span class="icon">✖ خروج</span></button>
        </div>

        <div class="dpad">
            <div class="empty"></div>
            <button class="btn" onclick="sendCmd('sat_up')"><span class="icon">▲</span></button>
            <div class="empty"></div>

            <button class="btn" onclick="sendCmd('sat_left')"><span class="icon">◄</span></button>
            <button class="btn btn-ok" onclick="sendCmd('sat_ok')">OK</button>
            <button class="btn" onclick="sendCmd('sat_right')"><span class="icon">►</span></button>

            <div class="empty"></div>
            <button class="btn" onclick="sendCmd('sat_down')"><span class="icon">▼</span></button>
            <div class="empty"></div>
        </div>

        <div class="row">
            <button class="btn" onclick="sendCmd('sat_volup')"><span class="icon">🔊 +</span></button>
            <button class="btn" onclick="sendCmd('sat_chup')"><span class="icon">📺 +</span></button>
        </div>
        <div class="row">
            <button class="btn" onclick="sendCmd('sat_voldown')"><span class="icon">🔉 -</span></button>
            <button class="btn" onclick="sendCmd('sat_chdown')"><span class="icon">📺 -</span></button>
        </div>

        <!-- لوحة الأرقام للقنوات -->
        <div class="numpad">
            <button class="btn" onclick="sendCmd('sat_1')">1</button>
            <button class="btn" onclick="sendCmd('sat_2')">2</button>
            <button class="btn" onclick="sendCmd('sat_3')">3</button>
            <button class="btn" onclick="sendCmd('sat_4')">4</button>
            <button class="btn" onclick="sendCmd('sat_5')">5</button>
            <button class="btn" onclick="sendCmd('sat_6')">6</button>
            <button class="btn" onclick="sendCmd('sat_7')">7</button>
            <button class="btn" onclick="sendCmd('sat_8')">8</button>
            <button class="btn" onclick="sendCmd('sat_9')">9</button>
            <div class="empty"></div>
            <button class="btn" onclick="sendCmd('sat_0')">0</button>
            <div class="empty"></div>
        </div>
    </div>

    <script>
        function switchTab(tab) {
            document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
            document.querySelectorAll('.remote-card').forEach(card => card.classList.remove('active'));

            if (tab === 'samsung') {
                document.querySelectorAll('.tab-btn')[0].classList.add('active');
                document.getElementById('samsung-remote').classList.add('active');
            } else {
                document.querySelectorAll('.tab-btn')[1].classList.add('active');
                document.getElementById('starsat-remote').classList.add('active');
            }
        }

        function sendCmd(cmd) {
            fetch('/cmd?code=' + cmd)
            .then(response => response.text())
            .then(data => console.log(data))
            .catch(error => console.error('Error:', error));
        }
    </script>
</body>
</html>
)rawliteral";

// دالة معالجة الأوامر وإرسال بروتوكول IR المناسب
void handleCommand(String cmd) {
    // ================= معالجة أوامر سامسونج (Samsung32 Protocol) =================
    if (cmd.startsWith("sam_")) {
        uint32_t codeToSend = 0;
        bool doubleSend = false;

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
        else if (cmd == "sam_enter") { codeToSend = SAMSUNG_ENTER; doubleSend = true; }
        else if (cmd == "sam_return") { codeToSend = SAMSUNG_RETURN; doubleSend = true; }
        else if (cmd == "sam_exit") codeToSend = SAMSUNG_EXIT;
        else if (cmd == "sam_home") codeToSend = SAMSUNG_HOME;
        else if (cmd == "sam_source") codeToSend = SAMSUNG_SOURCE;
        else if (cmd == "sam_menu") codeToSend = SAMSUNG_MENU;

        if (codeToSend != 0) {
            irsend.sendSAMSUNG(codeToSend, 32);
            if (doubleSend) {
                delay(40);
                irsend.sendSAMSUNG(codeToSend, 32);
            }
            Serial.print("Sent SAMSUNG Code: ");
            Serial.println(cmd);
        }
    } 
    // ================= معالجة أوامر ستارسات (NEC Protocol) =================
    else if (cmd.startsWith("sat_")) {
        uint32_t codeToSend = 0;

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
            // إرسال كود NEC ذو الـ 32 بت لرسيفر ستارسات
            irsend.sendNEC(codeToSend, 32);
            Serial.print("Sent STARSAT NEC Code: ");
            Serial.println(cmd);
        }
    }
}

void setup() {
    Serial.begin(115200);

    // بدء تشغيل وحدة الـ IR
    irsend.begin();

    // ضبط الـ IP الثابت
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("STA Failed to configure Static IP");
    }

    // الاتصال بالواي فاي
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected! IP Address: ");
    Serial.println(WiFi.localIP());

    // مسارات خادم الويب
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("code")) {
            String cmd = request->getParam("code")->value();
            handleCommand(cmd);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });

    server.begin();
}

void loop() {
    // AsyncWebServer ينفذ المهمات بالخلفية
}