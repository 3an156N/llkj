// ==========================================
// 警车双闪灯效 - ESP32 PWM 双通道呼吸灯
// 适用于 ESP32 Arduino 核心库 3.0+
// ==========================================

const int ledPinA = 16;    // LED_A (建议红色)
const int ledPinB = 17;    // LED_B (建议蓝色)

const int freq = 5000;     // PWM 频率
const int resolution = 8;  // 分辨率 8位 (0-255)

void setup() {
  Serial.begin(115200);
  
  // 初始化两个PWM通道（新版一行搞定）
  ledcAttach(ledPinA, freq, resolution);
  ledcAttach(ledPinB, freq, resolution);
  
  Serial.println("🚨 警车双闪灯效启动！");
}

void loop() {
  // 上升阶段：A渐亮，B渐暗
  for(int duty = 0; duty <= 255; duty++) {
    ledcWrite(ledPinA, duty);
    ledcWrite(ledPinB, 255 - duty);
    delay(10);
  }
  
  // 下降阶段：A渐暗，B渐亮
  for(int duty = 255; duty >= 0; duty--) {
    ledcWrite(ledPinA, duty);
    ledcWrite(ledPinB, 255 - duty);
    delay(10);
  }
  
  Serial.println("⏱️ 完成一个闪烁周期");
}