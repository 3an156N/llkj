/**
 * 多档位触摸调速呼吸灯
 * 结合实验3 (PWM呼吸灯) 和 实验4 (触摸引脚)
 * 
 * 功能：LED持续呼吸灯效果，触摸切换速度档位
 * 档位：1档(慢速) -> 2档(中速) -> 3档(快速) -> 1档...
 */

// ==================== 引脚定义 ====================
#define TOUCH_PIN T4      // GPIO4, 触摸引脚
#define LED_PIN 2         // GPIO2, 板载LED

// ==================== 触摸参数 ====================
#define THRESHOLD 20      // 触摸阈值（需根据实际测试调整）
#define DEBOUNCE_MS 50    // 防抖时间(ms)

// ==================== PWM参数 ====================
#define PWM_FREQ 5000     // PWM频率 5kHz
#define PWM_RES 8         // 8位分辨率 (0-255)

// ==================== 速度档位定义 ====================
#define SPEED_LEVELS 3    // 3个速度档位

// 每个档位的参数配置 {延迟时间(ms), 步进值}
// 延迟越小、步进越大 -> 呼吸越快
const struct SpeedConfig {
  int delayTime;    // 每步延迟(ms)
  int stepSize;     // 占空比步进值
} speedConfigs[SPEED_LEVELS] = {
  {20, 1},   // 1档: 慢速呼吸 (20ms * 255 = 5.1s 一个周期)
  {10, 2},   // 2档: 中速呼吸 (10ms * 128 = 1.28s 一个周期)
  {5, 4}     // 3档: 快速呼吸 (5ms * 64 = 0.32s 一个周期)
};

// ==================== 全局变量 ====================
int currentLevel = 0;           // 当前档位 (0,1,2 对应 1,2,3档)
bool ledState = false;          // LED状态（用于自锁）
bool lastTouchState = false;    // 上一次触摸状态（边缘检测）
unsigned long lastDebounceTime = 0;  // 防抖计时

// ==================== 功能函数 ====================

/**
 * 切换档位：1->2->3->1...
 */
void switchSpeedLevel() {
  currentLevel = (currentLevel + 1) % SPEED_LEVELS;
  
  // 串口输出当前档位信息
  Serial.print(">>> Speed switched to Level ");
  Serial.print(currentLevel + 1);
  Serial.print(" (Delay: ");
  Serial.print(speedConfigs[currentLevel].delayTime);
  Serial.print("ms, Step: ");
  Serial.print(speedConfigs[currentLevel].stepSize);
  Serial.println(")");
}

/**
 * 检测触摸边缘（上升沿检测 + 防抖）
 * @return true: 检测到有效触摸
 */
bool detectTouchEdge() {
  // 读取当前触摸值
  int touchValue = touchRead(TOUCH_PIN);
  bool currentTouchState = (touchValue < THRESHOLD);
  
  // 边缘检测：未触摸 -> 触摸 (上升沿)
  if (currentTouchState && !lastTouchState) {
    // 防抖处理：检查时间间隔
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > DEBOUNCE_MS) {
      lastDebounceTime = currentTime;
      lastTouchState = currentTouchState;
      return true;  // 检测到有效触摸
    }
  }
  
  // 更新状态
  lastTouchState = currentTouchState;
  return false;
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 配置LED引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 配置PWM
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(LED_PIN, 0);
  
  // 显示启动信息
  Serial.println("========================================");
  Serial.println("Multi-Level Touch Speed Control Breathing LED");
  Serial.println("========================================");
  Serial.print("Initial Level: 1 (Delay: ");
  Serial.print(speedConfigs[currentLevel].delayTime);
  Serial.print("ms, Step: ");
  Serial.print(speedConfigs[currentLevel].stepSize);
  Serial.println(")");
  Serial.println("Touch the pad to change speed level");
  Serial.println("----------------------------------------");
}

// ==================== 主循环 ====================
void loop() {
  // 1. 检测触摸事件（边缘检测）
  if (detectTouchEdge()) {
    switchSpeedLevel();  // 切换速度档位
  }
  
  // 2. 执行呼吸灯效果（根据当前档位）
  breatheCycle();
  
  // 3. 小延迟避免看门狗问题
  // 实际呼吸周期由 breatheCycle() 内部控制
}

/**
 * 执行一个完整的呼吸周期（变亮 + 变暗）
 * 速度由 currentLevel 控制
 */
void breatheCycle() {
  // 获取当前档位配置
  int delayTime = speedConfigs[currentLevel].delayTime;
  int stepSize = speedConfigs[currentLevel].stepSize;
  
  // ----- 阶段1: 逐渐变亮 (0 -> 255) -----
  for (int duty = 0; duty <= 255; duty += stepSize) {
    // 在每次循环中检测触摸（保持响应性）
    if (detectTouchEdge()) {
      switchSpeedLevel();
      // 注意：切换档位后，延迟和步进会变化
      // 重新获取当前档位配置
      delayTime = speedConfigs[currentLevel].delayTime;
      stepSize = speedConfigs[currentLevel].stepSize;
    }
    
    ledcWrite(LED_PIN, duty);
    delay(delayTime);
  }
  
  // ----- 阶段2: 逐渐变暗 (255 -> 0) -----
  for (int duty = 255; duty >= 0; duty -= stepSize) {
    // 在每次循环中检测触摸（保持响应性）
    if (detectTouchEdge()) {
      switchSpeedLevel();
      delayTime = speedConfigs[currentLevel].delayTime;
      stepSize = speedConfigs[currentLevel].stepSize;
    }
    
    ledcWrite(LED_PIN, duty);
    delay(delayTime);
  }
}