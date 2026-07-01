#define TOUCH_PIN 4
#define LED_PIN 2
#define THRESHOLD 20 // 需要根据实际测试修改此阈值

bool ledState = false;

// 中断服务函数 (ISR)
void gotTouch() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  
  // 绑定中断函数
  touchAttachInterrupt(TOUCH_PIN, gotTouch, THRESHOLD);
}

void loop() {
  // 主循环可以处理其他任务，或者置空
  // 注意：在某些ESP32核心版本中，loop中可能需要保留简单的代码以保持看门狗复位
  delay(100);
}