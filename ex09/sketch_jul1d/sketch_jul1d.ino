// ==========================================
// 实时传感器Web仪表盘（无JSON库版本）
// 功能：实时采集触摸数据，Web页面动态显示
// ==========================================

#include <WiFi.h>
#include <WebServer.h>

// ===== WiFi配置（AP模式） =====
const char* ap_ssid = "ESP32-Sensor";
const char* ap_password = "12345678";

// ===== 传感器引脚 =====
const int TOUCH_PIN = 4;        // T0触摸引脚
const int LED_PIN = 2;          // 板载LED（可选）

// ===== Web服务器 =====
WebServer server(80);

// ==========================================
// HTML页面（带仪表盘UI）
// ==========================================
String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>实时传感器仪表盘</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: #0a0a1a;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            color: #fff;
        }
        .container {
            background: #1a1a2e;
            border-radius: 30px;
            padding: 40px 35px;
            width: 92%;
            max-width: 450px;
            text-align: center;
            border: 1px solid #2a2a4a;
            box-shadow: 0 20px 60px rgba(0,0,0,0.8);
        }
        .header {
            margin-bottom: 20px;
        }
        .header h1 {
            font-size: 26px;
            background: linear-gradient(135deg, #00d2ff, #3a7bd5);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .header p {
            color: #666;
            font-size: 13px;
            margin-top: 4px;
        }
        .sensor-icon {
            font-size: 50px;
            margin: 10px 0;
        }
        /* ===== 仪表盘数值显示 ===== */
        .gauge-container {
            position: relative;
            margin: 20px auto;
            width: 220px;
            height: 220px;
        }
        .gauge-ring {
            width: 220px;
            height: 220px;
            border-radius: 50%;
            background: conic-gradient(
                #00d2ff 0%,
                #3a7bd5 50%,
                #1a1a2e 50%,
                #1a1a2e 100%
            );
            display: flex;
            justify-content: center;
            align-items: center;
            position: relative;
            transition: background 0.3s;
        }
        .gauge-ring-inner {
            width: 170px;
            height: 170px;
            border-radius: 50%;
            background: #0a0a1a;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
        }
        .gauge-value {
            font-size: 64px;
            font-weight: 700;
            color: #00d2ff;
            line-height: 1;
            transition: color 0.3s;
        }
        .gauge-label {
            font-size: 14px;
            color: #666;
            margin-top: 4px;
        }
        .gauge-unit {
            font-size: 20px;
            color: #888;
            margin-left: 4px;
        }
        /* 数值变化颜色 */
        .value-low { color: #ff4444; }
        .value-mid { color: #ffaa00; }
        .value-high { color: #44ff44; }
        
        /* ===== 状态指示条 ===== */
        .status-bar {
            display: flex;
            justify-content: space-between;
            padding: 12px 20px;
            background: #0a0a1a;
            border-radius: 14px;
            margin: 15px 0;
        }
        .status-item {
            text-align: center;
        }
        .status-item .label {
            font-size: 11px;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .status-item .value {
            font-size: 18px;
            font-weight: bold;
            margin-top: 2px;
        }
        .status-dot {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-left: 6px;
            vertical-align: middle;
            animation: pulse 1s infinite;
        }
        .dot-on { background: #44ff44; }
        .dot-off { background: #ff4444; }
        .dot-touch { background: #ff8800; animation: pulse 0.3s infinite; }
        
        @keyframes pulse {
            0% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.3; transform: scale(0.8); }
            100% { opacity: 1; transform: scale(1); }
        }
        
        /* ===== 历史数据曲线 ===== */
        .chart-container {
            background: #0a0a1a;
            border-radius: 14px;
            padding: 15px;
            margin: 15px 0;
        }
        .chart-container .title {
            font-size: 12px;
            color: #666;
            text-align: left;
            margin-bottom: 8px;
        }
        .chart-bars {
            display: flex;
            align-items: flex-end;
            height: 60px;
            gap: 4px;
        }
        .chart-bar {
            flex: 1;
            background: linear-gradient(to top, #00d2ff, #3a7bd5);
            border-radius: 3px 3px 0 0;
            min-height: 2px;
            transition: height 0.3s;
        }
        .chart-bar.empty { background: #1a1a2e; }
        
        /* ===== 底部信息 ===== */
        .footer {
            font-size: 12px;
            color: #444;
            margin-top: 15px;
        }
        .footer .ip {
            color: #00d2ff;
        }
        .footer .freq {
            color: #888;
        }
        
        @media (max-width: 400px) {
            .gauge-container { width: 180px; height: 180px; }
            .gauge-ring { width: 180px; height: 180px; }
            .gauge-ring-inner { width: 140px; height: 140px; }
            .gauge-value { font-size: 48px; }
            .status-item .value { font-size: 14px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📊 传感器仪表盘</h1>
            <p>实时触摸传感器数据监控</p>
        </div>
        
        <div class="sensor-icon">🖐️</div>
        
        <!-- 仪表盘 -->
        <div class="gauge-container">
            <div class="gauge-ring" id="gaugeRing">
                <div class="gauge-ring-inner">
                    <div class="gauge-value" id="sensorValue">--</div>
                    <div class="gauge-label">
                        触摸值
                        <span class="gauge-unit">T</span>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- 状态信息 -->
        <div class="status-bar">
            <div class="status-item">
                <div class="label">状态</div>
                <div class="value" id="statusText">
                    等待中
                    <span class="status-dot dot-off" id="statusDot"></span>
                </div>
            </div>
            <div class="status-item">
                <div class="label">更新</div>
                <div class="value" id="updateFreq">0 Hz</div>
            </div>
            <div class="status-item">
                <div class="label">IP</div>
                <div class="value" id="ipDisplay" style="font-size:14px;color:#00d2ff;">--</div>
            </div>
        </div>
        
        <!-- 历史曲线 -->
        <div class="chart-container">
            <div class="title">📈 实时数据曲线 (最近20个采样点)</div>
            <div class="chart-bars" id="chartBars"></div>
        </div>
        
        <div class="footer">
            <span class="ip" id="ipFooter">ESP32</span>
            <span> | </span>
            <span class="freq" id="freqFooter">更新频率: 20 Hz</span>
        </div>
    </div>

    <script>
        // DOM元素
        const sensorValueEl = document.getElementById('sensorValue');
        const gaugeRing = document.getElementById('gaugeRing');
        const statusText = document.getElementById('statusText');
        const statusDot = document.getElementById('statusDot');
        const updateFreq = document.getElementById('updateFreq');
        const ipDisplay = document.getElementById('ipDisplay');
        const chartBars = document.getElementById('chartBars');
        
        // 显示IP
        ipDisplay.textContent = window.location.host;
        document.getElementById('ipFooter').textContent = window.location.host;
        
        // 历史数据
        let historyData = [];
        const MAX_HISTORY = 20;
        let updateCount = 0;
        let lastFreqTime = Date.now();
        
        // ===== 获取传感器数据 =====
        function fetchSensorData() {
            fetch('/data')
                .then(response => {
                    if (!response.ok) throw new Error('Network error');
                    return response.text();
                })
                .then(data => {
                    const value = parseInt(data);
                    if (!isNaN(value)) {
                        updateDisplay(value);
                    }
                })
                .catch(error => {
                    console.error('Fetch error:', error);
                    statusText.innerHTML = '连接失败 <span class="status-dot dot-off"></span>';
                });
        }
        
        // ===== 更新界面 =====
        function updateDisplay(value) {
            // 更新数值
            sensorValueEl.textContent = value;
            
            // 根据数值变化颜色
            if (value < 15) {
                sensorValueEl.className = 'gauge-value value-low';
            } else if (value < 30) {
                sensorValueEl.className = 'gauge-value value-mid';
            } else {
                sensorValueEl.className = 'gauge-value value-high';
            }
            
            // 更新仪表盘（环形进度）
            let percent = Math.min(100, (value / 80) * 100);
            if (value < 5) percent = 0;
            if (value > 75) percent = 100;
            
            if (value < 15) {
                gaugeRing.style.background = 
                    'conic-gradient(#ff4444 0%, #ff6644 ' + percent + '%, #1a1a2e ' + percent + '%, #1a1a2e 100%)';
            } else if (value < 30) {
                gaugeRing.style.background = 
                    'conic-gradient(#ffaa00 0%, #ffcc44 ' + percent + '%, #1a1a2e ' + percent + '%, #1a1a2e 100%)';
            } else {
                gaugeRing.style.background = 
                    'conic-gradient(#00d2ff 0%, #44ff88 ' + percent + '%, #1a1a2e ' + percent + '%, #1a1a2e 100%)';
            }
            
            // 更新状态指示
            if (value < 15) {
                statusText.innerHTML = '触摸中 <span class="status-dot dot-touch"></span>';
            } else if (value < 30) {
                statusText.innerHTML = '接近中 <span class="status-dot dot-on" style="background:#ffaa00;"></span>';
            } else {
                statusText.innerHTML = '空闲 <span class="status-dot dot-on"></span>';
            }
            
            // 更新历史数据
            historyData.push(value);
            if (historyData.length > MAX_HISTORY) {
                historyData.shift();
            }
            updateChart();
            
            // 更新频率
            updateCount++;
            const now = Date.now();
            if (now - lastFreqTime >= 1000) {
                const freq = (updateCount / ((now - lastFreqTime) / 1000)).toFixed(1);
                updateFreq.textContent = freq + ' Hz';
                document.getElementById('freqFooter').textContent = '更新频率: ' + freq + ' Hz';
                updateCount = 0;
                lastFreqTime = now;
            }
        }
        
        // ===== 更新历史曲线 =====
        function updateChart() {
            chartBars.innerHTML = '';
            let maxVal = 1;
            for (let v of historyData) {
                if (v > maxVal) maxVal = v;
            }
            if (maxVal < 1) maxVal = 1;
            
            for (let i = 0; i < MAX_HISTORY; i++) {
                const bar = document.createElement('div');
                bar.className = 'chart-bar';
                
                if (i < historyData.length) {
                    const val = historyData[i];
                    const heightPercent = Math.min(100, (val / 80) * 100);
                    bar.style.height = Math.max(5, heightPercent) + '%';
                    
                    if (val < 15) {
                        bar.style.background = 'linear-gradient(to top, #ff4444, #ff6644)';
                    } else if (val < 30) {
                        bar.style.background = 'linear-gradient(to top, #ffaa00, #ffcc44)';
                    } else {
                        bar.style.background = 'linear-gradient(to top, #00d2ff, #44ff88)';
                    }
                } else {
                    bar.className = 'chart-bar empty';
                    bar.style.height = '2px';
                }
                chartBars.appendChild(bar);
            }
        }
        
        // ===== 启动定时拉取 =====
        setInterval(fetchSensorData, 50);
        fetchSensorData();
    </script>
</body>
</html>
)rawliteral";

  return html;
}

// ==========================================
// Web路由处理
// ==========================================

// 根路径 - 返回HTML页面
void handleRoot() {
  server.send(200, "text/html; charset=UTF-8", getHTML());
}

// 数据接口 - 直接返回纯数字（无需JSON库）
void handleData() {
  int touchValue = touchRead(TOUCH_PIN);
  server.send(200, "text/plain", String(touchValue));
}

// 404处理
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// ==========================================
// 主程序
// ==========================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 初始化LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 开启AP热点
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("=================================");
  Serial.println("📊 实时传感器Web仪表盘");
  Serial.println("=================================");
  Serial.print("📶 热点名称: ");
  Serial.println(ap_ssid);
  Serial.print("🔑 密码: ");
  Serial.println(ap_password);
  Serial.print("🌐 访问地址: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("=================================");
  Serial.println("💡 操作提示:");
  Serial.println("  1. 手机连接热点 " + String(ap_ssid));
  Serial.println("  2. 浏览器访问 192.168.4.1");
  Serial.println("  3. 手指靠近GPIO4杜邦线");
  Serial.println("  4. 观察网页数值实时变化");
  Serial.println("=================================");
  
  // 配置Web路由
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handleNotFound);
  
  // 启动服务器
  server.begin();
  Serial.println("✅ Web服务器已启动");
  Serial.println("📡 数据接口: /data (返回纯数字)");
}

void loop() {
  server.handleClient();
  
  // LED闪烁指示运行状态
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}