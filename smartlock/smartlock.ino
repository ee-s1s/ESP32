#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoOTA.h> // مكتبة التحديث لاسلكياً

// ==========================================
// 1. إعدادات الشبكة والعتاد
// ==========================================
const char *WIFI_SSID = "001";
const char *WIFI_PASSWORD = "MYYEMEN12";

IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);

const int RELAY_PIN = 27;                       // منفذ الريليه
const unsigned long LOCK_OPEN_DURATION = 250;  // 1.5 ثانية

// ==========================================
// 2. متغيرات التحكم في الحالة
// ==========================================
WebServer server(80);
bool isUnlocked = false;
unsigned long unlockStartTime = 0;

// ==========================================
// 3. واجهة الويب (HTML + JS + CSS)
// ==========================================
const char HTML_INTERFACE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>نظام التحكم بالقفل الذكي</title>
    <style>
        :root {
            --bg: #0f172a;
            --card-bg: #1e293b;
            --primary: #10b981;
            --primary-hover: #059669;
            --text: #f8fafc;
            --subtext: #94a3b8;
        }
        body {
            font-family: system-ui, -apple-system, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
        }
        .container {
            background: var(--card-bg);
            padding: 2.5rem;
            border-radius: 1.5rem;
            box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.5);
            text-align: center;
            width: 100%;
            max-width: 360px;
        }
        .status-icon { font-size: 4rem; margin-bottom: 1rem; }
        h2 { margin: 0 0 0.5rem 0; font-size: 1.5rem; }
        p { color: var(--subtext); margin-bottom: 2rem; font-size: 0.9rem; }
        .btn {
            background-color: var(--primary);
            color: white;
            border: none;
            padding: 1rem 2rem;
            font-size: 1.1rem;
            font-weight: 600;
            border-radius: 0.75rem;
            cursor: pointer;
            width: 100%;
            transition: all 0.2s ease;
        }
        .btn:hover { background-color: var(--primary-hover); }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    </style>
</head>
<body>
    <div class="container">
        <div id="icon" class="status-icon">🔒</div>
        <h2 id="title">القفل مغلق</h2>
        <p id="desc">اضغط على الزر لفتح الباب تلقائياً</p>
        <button id="unlockBtn" class="btn" onclick="triggerUnlock()">فتح القفل</button>
    </div>

    <script>
        async function triggerUnlock() {
            const btn = document.getElementById('unlockBtn');
            const icon = document.getElementById('icon');
            const title = document.getElementById('title');
            const desc = document.getElementById('desc');

            btn.disabled = true;
            icon.textContent = '🔓';
            title.textContent = 'تم فتح القفل';
            desc.textContent = 'جاري إعادة الإغلاق تلقائياً...';

            try {
                await fetch('/api/unlock', { method: 'POST' });
                
                let timeLeft = 1.5;
                const interval = setInterval(() => {
                    timeLeft -= 0.5;
                    if (timeLeft <= 0) {
                        clearInterval(interval);
                        icon.textContent = '🔒';
                        title.textContent = 'القفل مغلق';
                        desc.textContent = 'اضغط على الزر لفتح الباب تلقائياً';
                        btn.disabled = false;
                    }
                }, 500);
            } catch (err) {
                alert('فشل الاتصال باللوحة!');
                btn.disabled = false;
            }
        }
    </script>
</body>
</html>
)rawliteral";

// ==========================================
// 4. وظائف الدعم ودعم CORS
// ==========================================
void triggerUnlockMechanism() {
  if (!isUnlocked) {
    digitalWrite(RELAY_PIN, HIGH);
    isUnlocked = true;
    unlockStartTime = millis();
    Serial.println("تم فتح القفل!");
  }
}

void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  setCORSHeaders();
  server.send(204);
}

// ==========================================
// 5. مسارات الخدمة (Routes)
// ==========================================
void handleRoot() { server.send(200, "text/html", HTML_INTERFACE); }

void handleUnlockAPI() {
  setCORSHeaders();
  if (!isUnlocked) {
    triggerUnlockMechanism();
    server.send(200, "application/json",
                "{\"status\":\"success\",\"message\":\"Unlocked\",\"state\":"
                "\"unlocked\"}");
  } else {
    server.send(400, "application/json",
                "{\"status\":\"error\",\"message\":\"Already "
                "unlocked\",\"state\":\"unlocked\"}");
  }
}

void handleStatusAPI() {
  setCORSHeaders();
  String json = "{\"status\":\"success\",\"state\":\"";
  json += isUnlocked ? "unlocked" : "locked";
  json += "\",\"uptime_ms\":";
  json += String(millis());
  json += "}";

  server.send(200, "application/json", json);
}

// ==========================================
// 6. التهيئة والإعداد (Setup)
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("فشل إعداد Static IP!");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println("\nتم الاتصال بنجاح!");
  Serial.print("واجهة الويب (Root): http://");
  Serial.println(WiFi.localIP());

  // إعداد وتفعيل التحديث لاسلكياً (OTA)
  ArduinoOTA.setHostname("esp32-smartlock");
  ArduinoOTA.setPassword("admin123"); // كلمة مرور اختيارية لحماية الرفع اللاسلكي

  ArduinoOTA.onStart([]() { Serial.println("بدء التحديث لاسلكياً..."); });
  ArduinoOTA.onEnd([]() { Serial.println("\nانتهى التحديث!"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("التقدم: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("خطأ [%u]: ", error);
  });

  ArduinoOTA.begin();

  // تسجيل مسارات الويب
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/unlock", HTTP_POST, handleUnlockAPI);
  server.on("/api/unlock", HTTP_OPTIONS, handleOptions);
  server.on("/api/status", HTTP_GET, handleStatusAPI);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);

  server.begin();
}

// ==========================================
// 7. الحلقة التكرارية (Loop)
// ==========================================
void loop() {
  ArduinoOTA.handle(); // الاستماع لطلبات التحديث اللاسلكي
  server.handleClient();

  if (isUnlocked && (millis() - unlockStartTime >= LOCK_OPEN_DURATION)) {
    digitalWrite(RELAY_PIN, LOW);
    isUnlocked = false;
    Serial.println("تم إعادة إغلاق القفل تلقائياً.");
  }
}