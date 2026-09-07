# ESP32 Smart Lock & Universal IR Remote API

نظام شامل وموحد للتحكم بالقفل الذكي والأجهزة المنزلية عبر الأشعة تحت الحمراء (IR) والتحديث اللاسلكي (OTA) باستعمال ESP32 وواجهة برمجية REST API.

---

## 🌐 Base URL
`http://192.168.1.200`

---

## 🔒 1. القفل الذكي (Smart Lock API)

### فحص الحالة الشاملة (System Status)
* **المسار:** `/api/status`
* **الطريقة:** `GET`
* **الوصف:** يعرض حالة السيرفر وحالة القفل الحالية (مغلق/مفتوح) ومدة التشغيل.
* **مثال:**
  ```http
  GET [http://192.168.1.200/api/status](http://192.168.1.200/api/status)

```

* **الاستجابة:**
```json
{
  "status": "success",
  "server": "online",
  "lock_state": "locked",
  "uptime_ms": 12450
}

```



---

### فتح القفل الذكي (Unlock Lock)

* **المسار:** `/api/unlock`
* **الطريقة:** `POST`
* **الوصف:** يُفعل الريليه (GPIO 27) لفتح القفل لمدة 250ms ثم يُغلق تلقائياً.
* **مثال:**
```http
POST [http://192.168.1.200/api/unlock](http://192.168.1.200/api/unlock)

```


* **الاستجابة الناجحة:**
```json
{
  "status": "success",
  "message": "Unlocked",
  "state": "unlocked"
}

```



---

## 📡 2. مصادقة المروحة (Fan Auth API)

### تسجيل الدخول وحصول على التوكن

* **المسار:** `/api/login`
* **الطريقة:** `GET`
* **المعاملات (Params):** `pass` (كلمة المرور الإفتراضية: `12345`)
* **الوصف:** الحصول على التوكن لتشغيل أوامر المروحة المحمية.
* **مثال:**
```http
GET [http://192.168.1.200/api/login?pass=12345](http://192.168.1.200/api/login?pass=12345)

```


* **الاستجابة الناجحة:**
```json
{
  "success": true,
  "token": "ESP32_FAN_AUTH_TOKEN_998877"
}

```



---

## 📺 3. أوامر الـ IR (Command Codes)

### طريقة الاستخدام العامة

* **المسار:** `/api/cmd`
* **الطريقة:** `GET`
* **المعاملات:**
* `code` *(مطلوب)*: كود الجهاز.
* `token` *(مطلوب فقط لأوامر المروحة)*: التوكن الناتج من تسجيل الدخول.



---

### 📺 شاشة سامسونج (Samsung TV)

لا تطلب توكن حماية.

| الأمر | الوصف | الرابط المباشر |
| --- | --- | --- |
| `sam_power` | تشغيل / إيقاف | `http://192.168.1.200/api/cmd?code=sam_power` |
| `sam_volup` | رفع الصوت | `http://192.168.1.200/api/cmd?code=sam_volup` |
| `sam_voldown` | خفض الصوت | `http://192.168.1.200/api/cmd?code=sam_voldown` |
| `sam_mute` | كتم الصوت | `http://192.168.1.200/api/cmd?code=sam_mute` |
| `sam_chup` | القناة التالية | `http://192.168.1.200/api/cmd?code=sam_chup` |
| `sam_chdown` | القناة السابقة | `http://192.168.1.200/api/cmd?code=sam_chdown` |
| `sam_up` | سهم لأعلى | `http://192.168.1.200/api/cmd?code=sam_up` |
| `sam_down` | سهم لأسفل | `http://192.168.1.200/api/cmd?code=sam_down` |
| `sam_left` | سهم لليسار | `http://192.168.1.200/api/cmd?code=sam_left` |
| `sam_right` | سهم لليمين | `http://192.168.1.200/api/cmd?code=sam_right` |
| `sam_enter` | موافق / OK | `http://192.168.1.200/api/cmd?code=sam_enter` |
| `sam_return` | العودة | `http://192.168.1.200/api/cmd?code=sam_return` |
| `sam_exit` | خروج | `http://192.168.1.200/api/cmd?code=sam_exit` |
| `sam_home` | الرئيسية | `http://192.168.1.200/api/cmd?code=sam_home` |
| `sam_source` | المصدر | `http://192.168.1.200/api/cmd?code=sam_source` |
| `sam_menu` | القائمة | `http://192.168.1.200/api/cmd?code=sam_menu` |

---

### 📻 رسيفر ستارسات (StarSat Receiver)

لا تطلب توكن حماية.

| الأمر | الوصف | الرابط المباشر |
| --- | --- | --- |
| `sat_power` | تشغيل / إيقاف | `http://192.168.1.200/api/cmd?code=sat_power` |
| `sat_mute` | كتم الصوت | `http://192.168.1.200/api/cmd?code=sat_mute` |
| `sat_volup` | رفع الصوت | `http://192.168.1.200/api/cmd?code=sat_volup` |
| `sat_voldown` | خفض الصوت | `http://192.168.1.200/api/cmd?code=sat_voldown` |
| `sat_chup` | القناة التالية | `http://192.168.1.200/api/cmd?code=sat_chup` |
| `sat_chdown` | القناة السابقة | `http://192.168.1.200/api/cmd?code=sat_chdown` |
| `sat_up` | سهم لأعلى | `http://192.168.1.200/api/cmd?code=sat_up` |
| `sat_down` | سهم لأسفل | `http://192.168.1.200/api/cmd?code=sat_down` |
| `sat_left` | سهم لليسار | `http://192.168.1.200/api/cmd?code=sat_left` |
| `sat_right` | سهم لليمين | `http://192.168.1.200/api/cmd?code=sat_right` |
| `sat_ok` | موافق | `http://192.168.1.200/api/cmd?code=sat_ok` |
| `sat_menu` | القائمة | `http://192.168.1.200/api/cmd?code=sat_menu` |
| `sat_exit` | خروج | `http://192.168.1.200/api/cmd?code=sat_exit` |
| `sat_sat` | قائمة الأقمار | `http://192.168.1.200/api/cmd?code=sat_sat` |
| `sat_info` | معلومات القناة | `http://192.168.1.200/api/cmd?code=sat_info` |
| `sat_0` إلى `sat_9` | أرقام القنوات (0-9) | `http://192.168.1.200/api/cmd?code=sat_1` |

---

### 🌀 المروحة (Fan - Protected)

**تتطلب توكن الحماية:** `&token=ESP32_FAN_AUTH_TOKEN_998877`

| الأمر | الوصف | الرابط المباشر |
| --- | --- | --- |
| `fan_on_speed` | تشغيل / السرعة | `http://192.168.1.200/api/cmd?code=fan_on_speed&token=ESP32_FAN_AUTH_TOKEN_998877` |
| `fan_off` | إيقاف التشغيل | `http://192.168.1.200/api/cmd?code=fan_off&token=ESP32_FAN_AUTH_TOKEN_998877` |
| `fan_mode` | وضع الهبوب | `http://192.168.1.200/api/cmd?code=fan_mode&token=ESP32_FAN_AUTH_TOKEN_998877` |
| `fan_timer` | المؤقت الزمني | `http://192.168.1.200/api/cmd?code=fan_timer&token=ESP32_FAN_AUTH_TOKEN_998877` |
| `fan_swing` | الدوران الأفقي | `http://192.168.1.200/api/cmd?code=fan_swing&token=ESP32_FAN_AUTH_TOKEN_998877` |
| `fan_lamp` | الإضاءة | `http://192.168.1.200/api/cmd?code=fan_lamp&token=ESP32_FAN_AUTH_TOKEN_998877` |

---

## 🔄 4. التحديث اللاسلكي (ArduinoOTA)

* **Hostname:** `esp32-smartlock-remote`
* **Password:** `admin123`
* **Port:** `3232` (تلقائي)

```

```
