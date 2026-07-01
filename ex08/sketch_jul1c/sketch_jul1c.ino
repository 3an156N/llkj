// ==========================================
// 物联网安防报警器模拟实验
// 功能：Web布防/撤防 + 触摸触发报警
// ==========================================

#include <WiFi.h>
#include <WebServer.h>

// ===== WiFi配置 =====
const char* ap_ssid = "ESP32-Security";
const char* ap_password = "12345678";

// ===== 硬件引脚 =====
const int LED_PIN = 2;
const int TOUCH_PIN = 4;

// ===== 系统状态 =====
bool systemArmed = false;
bool alarmTriggered = false;
bool lastTouchState = false;

const int TOUCH_THRESHOLD = 25;
const int ALARM_FLASH_INTERVAL = 100;

WebServer server(80);

// ==========================================
// HTML页面（使用字符串拼接方式）
// ==========================================
String getHTML() {
  String statusText = systemArmed ? "🔴 已布防" : "🟢 已撤防";
  String alarmText = alarmTriggered ? "🚨 报警中！" : "✅ 系统正常";
  String statusClass = systemArmed ? "armed" : "disarmed";
  String alarmClass = alarmTriggered ? "alarm" : "normal";
  String armDisabled = systemArmed ? "disabled" : "";
  String disarmDisabled = systemArmed ? "" : "disabled";
  
  String html = "";
  html += "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "    <meta charset=\"UTF-8\">\n";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "    <title>安防报警器</title>\n";
  html += "    <style>\n";
  html += "        body { font-family: Arial; text-align: center; padding: 30px; background: #0a0a1a; color: #fff; }\n";
  html += "        .container { max-width: 400px; margin: 0 auto; background: #1a1a2e; padding: 30px; border-radius: 20px; }\n";
  html += "        h1 { color: #00d2ff; }\n";
  html += "        .status { font-size: 20px; padding: 15px; border-radius: 10px; margin: 15px 0; }\n";
  html += "        .armed { background: #441111; color: #ff4444; }\n";
  html += "        .disarmed { background: #112211; color: #44ff44; }\n";
  html += "        .alarm { background: #ff0000; color: #fff; animation: blink 0.5s infinite; }\n";
  html += "        .normal { background: #112211; color: #44ff44; }\n";
  html += "        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.3; } 100% { opacity: 1; } }\n";
  html += "        .btn { padding: 15px 30px; margin: 10px; border: none; border-radius: 10px; font-size: 16px; font-weight: bold; cursor: pointer; color: #fff; }\n";
  html += "        .btn-arm { background: #ff4444; }\n";
  html += "        .btn-disarm { background: #44ff44; color: #000; }\n";
  html += "        .btn:disabled { opacity: 0.4; cursor: not-allowed; }\n";
  html += "        .info { font-size: 12px; color: #666; margin-top: 20px; }\n";
  html += "    </style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "    <div class=\"container\">\n";
  html += "        <h1>🔐 安防报警器</h1>\n";
  html += "        <div class=\"status " + statusClass + "\">" + statusText + "</div>\n";
  html += "        <div class=\"status " + alarmClass + "\">" + alarmText + "</div>\n";
  html += "        <button class=\"btn btn-arm\" onclick=\"arm()\" " + armDisabled + ">🔒 布防</button>\n";
  html += "        <button class=\"btn btn-disarm\" onclick=\"disarm()\" " + disarmDisabled + ">🔓 撤防</button>\n";
  html += "        <div class=\"info\">触摸GPIO4触发报警 | IP: " + WiFi.softAPIP().toString() + "</div>\n";
  html += "    </div>\n";
  html += "    <script>\n";
  html += "        function arm() { fetch('/arm').then(()=>location.reload()); }\n";
  html += "        function disarm() { fetch('/disarm').then(()=>location.reload()); }\n";
  html += "        setInterval(()=>{ fetch('/status').then(r=>r.text()).then(()=>location.reload()); }, 3000);\n";
  html += "    </script>\n";
  html += "</body>\n";
  html += "</html>\n";
  
  return html;
}

// ==========================================
// Web路由
// ==========================================
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", getHTML());
}

void handleArm() {
  systemArmed = true;
  alarmTriggered = false;
  Serial.println("🔒 系统已布防");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDisarm() {
  systemArmed = false;
  alarmTriggered = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("🔓 系统已撤防");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  server.send(200, "text/plain", "ok");
}

void handleNotFound() {
  server.send(404, "text/plain", "404");
}

// ==========================================
// 触摸检测
// ==========================================
void checkTouch() {
  int touchValue = touchRead(TOUCH_PIN);
  bool isTouched = (touchValue < TOUCH_THRESHOLD);
  
  // 每2秒打印一次
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.print("触摸值: ");
    Serial.print(touchValue);
    Serial.print(" | 状态: ");
    Serial.print(isTouched ? "触摸" : "未触摸");
    Serial.print(" | 布防: ");
    Serial.print(systemArmed ? "是" : "否");
    Serial.print(" | 报警: ");
    Serial.println(alarmTriggered ? "是" : "否");
  }
  
  // 边缘检测
  if (isTouched && !lastTouchState) {
    delay(100);
    int confirmValue = touchRead(TOUCH_PIN);
    if (confirmValue < TOUCH_THRESHOLD) {
      Serial.println("⚠️ 触摸事件检测到！");
      if (systemArmed && !alarmTriggered) {
        alarmTriggered = true;
        Serial.println("🚨🚨🚨 报警触发！LED开始闪烁！");
      } else if (!systemArmed) {
        Serial.println("⏸️ 系统未布防，忽略触摸");
      } else if (alarmTriggered) {
        Serial.println("🔁 已报警，忽略重复触发");
      }
    }
  }
  
  lastTouchState = isTouched;
}

// ==========================================
// LED报警闪烁
// ==========================================
void handleAlarmLED() {
  static unsigned long lastToggle = 0;
  static bool ledState = false;
  
  if (alarmTriggered) {
    if (millis() - lastToggle >= ALARM_FLASH_INTERVAL) {
      lastToggle = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
  }
}

// ==========================================
// 主程序
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("=================================");
  Serial.println("🔐 物联网安防报警器");
  Serial.println("=================================");
  Serial.print("📶 热点: ");
  Serial.println(ap_ssid);
  Serial.print("🔑 密码: ");
  Serial.println(ap_password);
  Serial.print("🌐 访问: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("=================================");
  Serial.println("💡 操作流程:");
  Serial.println("  1. 手机连接热点 " + String(ap_ssid));
  Serial.println("  2. 浏览器访问 192.168.4.1");
  Serial.println("  3. 点击「布防」");
  Serial.println("  4. 触摸GPIO4上的杜邦线");
  Serial.println("  5. LED应该开始闪烁");
  Serial.println("=================================");
  
  server.on("/", handleRoot);
  server.on("/arm", handleArm);
  server.on("/disarm", handleDisarm);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("✅ 服务器已启动");
}

void loop() {
  server.handleClient();
  checkTouch();
  handleAlarmLED();
  delay(20);
}