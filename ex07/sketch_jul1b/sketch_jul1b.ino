// ==========================================
// Web网页端无极调光器（AP热点模式）
// 无需WiFi路由器，手机直连ESP32
// ==========================================

#include <WiFi.h>
#include <WebServer.h>

// ===== AP热点配置 =====
const char* ap_ssid = "ESP32-Light";      // 热点名称（可自定义）
const char* ap_password = "12345678";     // 热点密码（至少8位）

// ===== LED配置 =====
const int LED_PIN = 2;                    // GPIO2 (板载LED)
const int PWM_FREQ = 5000;                // PWM频率
const int PWM_RESOLUTION = 8;             // 8位分辨率

WebServer server(80);
int currentBrightness = 128;

// ==========================================
// HTML页面
// ==========================================
String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 调光器</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            padding: 30px;
            background: #0a0a1a;
            color: #fff;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background: #1a1a2e;
            padding: 40px;
            border-radius: 20px;
        }
        h1 {
            color: #00d2ff;
            font-size: 28px;
        }
        .value {
            font-size: 60px;
            font-weight: bold;
            margin: 20px 0;
            color: #00d2ff;
        }
        .value span {
            font-size: 24px;
            color: #888;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: linear-gradient(to right, #1a1a2e, #00d2ff);
            outline: none;
            -webkit-appearance: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 28px;
            height: 28px;
            border-radius: 50%;
            background: #00d2ff;
            cursor: pointer;
        }
        .buttons {
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
            margin-top: 20px;
        }
        .buttons button {
            padding: 10px 20px;
            border: none;
            border-radius: 10px;
            background: #2a2a4a;
            color: #fff;
            font-size: 14px;
            cursor: pointer;
        }
        .buttons button:hover {
            background: #00d2ff;
            color: #000;
        }
        .status {
            margin-top: 20px;
            font-size: 14px;
            color: #666;
        }
        .status .ip {
            color: #00d2ff;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>💡 无极调光器</h1>
        <div class="value" id="display">50<span>%</span></div>
        
        <input type="range" id="slider" min="0" max="255" value="128"
               oninput="update(this.value)">
        
        <div class="buttons">
            <button onclick="setVal(0)">熄灭</button>
            <button onclick="setVal(64)">25%</button>
            <button onclick="setVal(128)">50%</button>
            <button onclick="setVal(192)">75%</button>
            <button onclick="setVal(255)">最亮</button>
        </div>
        
        <div class="status">
            <span class="ip" id="ipDisplay">ESP32</span>
            <span> | 状态: <span id="statusText">在线</span></span>
        </div>
    </div>

    <script>
        const slider = document.getElementById('slider');
        const display = document.getElementById('display');
        const statusText = document.getElementById('statusText');
        document.getElementById('ipDisplay').textContent = window.location.host;

        function update(value) {
            const percent = Math.round((value / 255) * 100);
            display.innerHTML = percent + '<span>%</span>';
            
            fetch('/set?value=' + value)
                .then(() => {
                    statusText.textContent = '已更新';
                    statusText.style.color = '#00d2ff';
                    setTimeout(() => {
                        statusText.textContent = '在线';
                        statusText.style.color = '#666';
                    }, 800);
                })
                .catch(() => {
                    statusText.textContent = '连接失败';
                    statusText.style.color = '#ff4444';
                });
        }

        function setVal(value) {
            slider.value = value;
            update(value);
        }

        // 键盘控制
        document.addEventListener('keydown', function(e) {
            if (e.key === 'ArrowUp' || e.key === 'ArrowRight') {
                e.preventDefault();
                let val = Math.min(255, parseInt(slider.value) + 10);
                slider.value = val;
                update(val);
            } else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') {
                e.preventDefault();
                let val = Math.max(0, parseInt(slider.value) - 10);
                slider.value = val;
                update(val);
            }
        });
    </script>
</body>
</html>
)rawliteral";

  return html;
}

// ===== Web路由 =====
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", getHTML());
}

void handleSet() {
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    
    currentBrightness = brightness;
    ledcWrite(LED_PIN, brightness);
    
    Serial.print("亮度: ");
    Serial.println(brightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "ERROR");
  }
}

void handleStatus() {
  server.send(200, "text/plain", String(currentBrightness));
}

void handleNotFound() {
  server.send(404, "text/plain", "404");
}

// ==========================================
// 主程序
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 初始化LED和PWM
  pinMode(LED_PIN, OUTPUT);
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(LED_PIN, 128);
  
  // 开启AP热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("=================================");
  Serial.println("📶 AP热点已开启");
  Serial.print("📡 热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("🔑 密码: ");
  Serial.println(ap_password);
  Serial.print("🌐 访问地址: http://192.168.4.1");
  Serial.println(WiFi.softAPIP());  // 通常是 192.168.4.1
  Serial.println("=================================");
  Serial.println("请在手机WiFi中连接上述热点");
  Serial.println("然后在浏览器访问上述IP地址");
  
  // 配置路由
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("🚀 Web服务器已启动");
}

void loop() {
  server.handleClient();
}