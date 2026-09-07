#include <Arduino.h>
#include <IRremote.hpp> // مكتبة IRremote الحديثة

#define IR_RECEIVE_PIN 15 // بن استقبال الإشارة

void setup() {
  Serial.begin(115200);
  
  // بدء الاستقبال مع تفعيل لمبة الـ LED المدمجة
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  Serial.println("==================================");
  Serial.println("   جاهز لاستقبال شفرات IR...    ");
  Serial.println("==================================");
}

void loop() {
  if (IrReceiver.decode()) {
    
    // التاكد من أن الإشارة ليست مجرد ضوضاء أو تكرار
    if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
      Serial.println("----------------------------------");
      Serial.print("البروتوكول : ");
      Serial.println(getProtocolString(IrReceiver.decodedIRData.protocol));

      Serial.print("العنوان    : 0x");
      Serial.println(IrReceiver.decodedIRData.address, HEX);

      Serial.print("الأمر       : 0x");
      Serial.println(IrReceiver.decodedIRData.command, HEX);

      Serial.print("الكود Raw  : 0x");
      Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
      Serial.println("----------------------------------");
    } else {
      Serial.println("⚠️ إشارة غير معروفة أو ضوضاء.");
    }
    
    // إعادة تعيين المستقبل للرسالة التالية
    IrReceiver.resume();
  }
}