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

// المروحة (NEC Standard)
#define FAN_ON_SPEED  0x000001F2
#define FAN_OFF       0x85649F80
#define FAN_MODE      0x000002F2
#define FAN_TIMER     0x00000372
#define FAN_SWING     0x000003B2
#define FAN_LAMP      0xE258A0CF

// ==========================================
// 3. خادم الويب ومتغيرات التحكم
// ==========================================
AsyncWebServer server(80);

bool isUnlocked = false;
unsigned long unlockStartTime = 0;

// ==========================================
// 4. دالتا المعالجة الأساسية
// ==========================================

// فتح القفل الفيزيائي
void triggerUnlockMechanism() {
    if (!isUnlocked) {
        digitalWrite(RELAY_PIN, HIGH);
        isUnlocked = true;
        unlockStartTime = millis();
        Serial.println("تم فتح القفل!");
    }
}

// معالجة أوامر الـ IR
bool handleCommand(String cmd) {
    uint32_t codeToSend = 0;
    bool doubleSend = false;

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

// ضبط رؤوس CORS
void setCORS(AsyncWebServerResponse *response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ==========================================
// 5. التهيئة والتشغيل (Setup)
// ==========================================
void setup() {
    Serial.begin(115200);

    // تهيئة عتاد القفل والـ IR
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    irsend.begin();

    // الاتصال بالشبكة
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

    // إعداد التحديث اللاسلكي ArduinoOTA
    ArduinoOTA.setHostname("esp32-smartlock-remote");
    ArduinoOTA.setPassword("admin123");

    ArduinoOTA.onStart([]() { Serial.println("Start updating OTA..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd OTA Update!"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
    });

    ArduinoOTA.begin();

    // ================= API Routes =================

    // HTTP OPTIONS لمعالجة CORS Preflight Requests
    server.onRequest([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(204);
            setCORS(response);
            request->send(response);
        }
    });

    // الجذر الأساسي للإشارة لوجود الخادم
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "ESP32 API Server Active");
        setCORS(response);
        request->send(response);
    });

    // 1. فحص الحالة الشامل للقطعتين (Lock + IR Server)
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

    // 2. فتح القفل الذكي (Smart Lock Unlock)
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

    // 3. تسجيل دخول المروحة (Auth Login)
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

    // 4. تنفيذ أوامر الـ IR
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
// 6. الحلقة التكرارية (Loop)
// ==========================================
void loop() {
    ArduinoOTA.handle(); // استقبال تحديثات الكود لاسلكياً

    // إغلاق القفل تلقائياً بعد انقضاء المدة المحددة (250ms)
    if (isUnlocked && ((unsigned long)(millis() - unlockStartTime) >= LOCK_OPEN_DURATION)) {
        digitalWrite(RELAY_PIN, LOW);
        isUnlocked = false;
        Serial.println("تم إعادة إغلاق القفل تلقائياً.");
    }
}