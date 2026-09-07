
# 🚀 ESP32 Smart Home Ecosystem (Smart Lock, IR Remote API & IR Decoder)

مشروع مدمج متكامل يجمع بين التحكم بالقفل الذكي، إرسال واستقبال إشارات الأشعة تحت الحمراء (IR)، والتحكم بالأجهزة المنزلية عبر خادم REST API والتحديث اللاسلكي (OTA).

---

## 📂 المجلدات والمكونات الرئيسية للمشروع

### 1️⃣ `Fan_and_Remote_and_lock_and_apis`
الوحدة المركزية الرئيسية وتتضمن:
* **Smart Lock API:** التحكم بنبضة فتح القفل (Relay) مع غلق تلقائي (250ms).
* **Universal IR Transmitter:** إرسال أوامر التحكم لشاشات Samsung ورسيفرات StarSat والمراوح.
* **Fan Auth System:** نظام مصادقة وحماية يتطلب توكن أمان (`Token`) لتشغيل المروحة.
* **ArduinoOTA Update:** إمكانية تحديث الشفرة البرمجية لاسلكياً عبر الشبكة.

### 2️⃣ `IR_Receiver_Test`
أداة اختبار وفحص إشارات الـ IR (Decoder):
* الاستماع للإشارات عبر مستقبل IR في `GPIO 15`.
* طباعة نوع البروتوكول، العنوان (Address)، رمز الأمر (Command)، والكود الخام (Raw Data) بصيغة HEX عبر الـ Serial Monitor لتسهيل استخراج أكواد الريموتات الجديدة.

---

## 🛠️ العتاد والتوصيل (Hardware & Pinout)

| المكون | المنفذ (GPIO) | الملاحظات |
| :--- | :--- | :--- |
| **Relay Module** | `GPIO 27` | التحكم بالقفل الكهربائي (High = فتح، Low = إغلاق) |
| **IR LED Transmitter** | `GPIO 4` | مرسل الأشعة تحت الحمراء (عبر ترانزستور 2N2222) |
| **IR Receiver Module** | `GPIO 15` | مستقبل الأشعة تحت الحمراء (وحدة الاختبار) |

---

## 📦 المكتبات المطلوبة (Arduino IDE)

تأكد من تثبيت المكتبات التالية:
* `WiFi` (مدمجة)
* `ESPAsyncWebServer`
* `AsyncTCP`
* `IRremoteESP8266` / `IRremote`
* `ArduinoOTA`

---

## 📡 دليل استخدام الـ REST API

* **العنوان الرئيسي (Base URL):** `http://192.168.1.200`

### 🔒 1. القفل الذكي (Smart Lock)
* **فحص حالة النظام:** `GET /api/status`
* **فتح القفل:** `POST /api/unlock`

### 🔑 2. مصادقة المروحة (Fan Auth)
* **تسجيل الدخول:** `GET /api/login?pass=12345`
* **الاستجابة:** تُرجع التوكن الخاص بالحماية (`ESP32_FAN_AUTH_TOKEN_998877`).

### 📺 3. أوامر الـ IR
* **الصيغة العامة:** `GET /api/cmd?code=COMMAND_NAME`
* **شاشات Samsung:** مثل `sam_power`, `sam_volup`, `sam_voldown`, `sam_enter`, `sam_home`
* **رسيفر StarSat:** مثل `sat_power`, `sat_volup`, `sat_chup`, `sat_ok`, `sat_0` إلى `sat_9`
* **المراوح (تتطلب توكن):** `GET /api/cmd?code=fan_on_speed&token=ESP32_FAN_AUTH_TOKEN_998877`
  *(الأوامر المتاحة: `fan_on_speed`, `fan_off`, `fan_mode`, `fan_timer`, `fan_swing`, `fan_lamp`)*

---

## 🔄 التحديث اللاسلكي (ArduinoOTA)

* **اسم الجهاز (Hostname):** `esp32-smartlock-remote`
* **كلمة المرور:** `admin123`
* **المنفذ:** `3232`

```
