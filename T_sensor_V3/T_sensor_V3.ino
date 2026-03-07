#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/adc.h>

// --- Config Pins ---
#define I2C_SDA 4
#define I2C_SCL 5
#define ADC_SCOPE_PIN ADC1_CHANNEL_5  // GPIO 6
#define BTN_WIFI 3
#define BTN_TEMP 7     // ปุ่มดูอุณหภูมิ
#define BTN_VOLTAGE 8  // ปุ่มดูแรงดัน

// --- Objects ---
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// --- Variables ---
enum DeviceMode { IDLE,
                  MODE_WIFI,
                  MODE_TEMP,
                  MODE_VOLTAGE };
DeviceMode currentMode = IDLE;

// === นำเข้าไฟล์หน้าเว็บ HTML ===
#include "html_page.h" 

uint32_t raw_value_1 = 0;
uint32_t raw_value_avg = 0;
float V = 0;

// --- 1. ฟังก์ชันจัดการหน้าจอ OLED ---
void updateDisplay(String title, String line1, String line2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(line1);
  display.setCursor(0, 35);
  display.println(line2);
  display.display();
}

// --- 2. ฟังก์ชัน WiFiManager Callback ---
void configModeCallback(WiFiManager *myWiFiManager) {
  updateDisplay("WIFI CONFIG",
                "SSID: " + myWiFiManager->getConfigPortalSSID(),
                "PASS: 12345678");
}

// --- 3. ฟังก์ชันเริ่มระบบ Network ---
void startNetwork() {
  server.on("/", []() {
    // ส่งข้อมูล webpageCode ที่ถูกดึงมาจาก html_page.h
    server.send(200, "text/html", webpageCode);
  });
  server.begin();
  webSocket.begin();
  Serial.println("Network Services Started");
}

// --- 4. ฟังก์ชันจัดการ WiFi ---
void runWiFiManager(bool forceConfig) {
  currentMode = MODE_WIFI;
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);

  bool res;
  if (forceConfig) {
    res = wm.startConfigPortal("AutoConnectAP", "12345678");
  } else {
    res = wm.autoConnect("AutoConnectAP", "12345678");
  }

  if (res) {
    updateDisplay("CONNECTED", "http://" + WiFi.localIP().toString(), " ");
    delay(2000);
    startNetwork();
  }
}

// --- 5. ฟังก์ชันคำนวณแรงดันและอุณหภูมิ ---
float Get_Voltage() {
  for (uint16_t i = 0; i < 1000; i++) {
    raw_value_avg += adc1_get_raw(ADC_SCOPE_PIN);
    delay(1);
  }

  raw_value_1 = raw_value_avg / 1000;
  raw_value_avg = 0;

  // bit_to_V
  V = (float)(map(raw_value_1, 0, 4096, 0, 950) + 4) / 1000.00;
  V += 0.04;
  return V;
}

float Get_Temperature(float v) {
  // สมการ C = 14.286V + 5.7143
  return (14.286 * v) + 5.7143;
}

// --- 6. ฟังก์ชันแสดงผลบนจอ OLED ---
void Print_Voltage() {
  float voltage = Get_Voltage();
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(voltage, 2);
  display.print(" V");
  display.display();
}

void Print_Temperature() {
  float voltage = Get_Voltage();
  float temp = Get_Temperature(voltage);
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(temp, 1);
  display.print(" C");
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_WIFI, INPUT_PULLUP);
  pinMode(BTN_TEMP, INPUT_PULLUP);
  pinMode(BTN_VOLTAGE, INPUT_PULLUP);
  pinMode(1, INPUT); // ปุ่มสำหรับสลับโหมดหลัก/พักจอ

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;) ;
  }

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_SCOPE_PIN, ADC_ATTEN_DB_11);

  updateDisplay("STARTING...", "Connecting WiFi...", "Please wait");
  runWiFiManager(false);
}

void loop() {
  if (digitalRead(1) == 0) {
    // โหมดตั้งค่า WiFi (GPIO 3)
    if (digitalRead(BTN_WIFI) == LOW) {
      delay(10);
      ESP.restart();
      runWiFiManager(true);
      currentMode = IDLE;
    }

    // โหมดแสดงอุณหภูมิ (GPIO 7)
    if (digitalRead(BTN_TEMP) == LOW) {
      delay(10);
      currentMode = MODE_TEMP;
    }

    // โหมดแสดงแรงดัน (GPIO 8)
    if (digitalRead(BTN_VOLTAGE) == LOW) {
      delay(10);
      currentMode = MODE_VOLTAGE;
    }

    // --- อัปเดตการทำงานตามโหมด ---
    if (currentMode == MODE_VOLTAGE) {
      Print_Voltage();
    } 
    else if (currentMode == MODE_TEMP) {
      Print_Temperature();
    } 
    else if (currentMode == MODE_WIFI) {
      static unsigned long lastWS = 0;
      if (millis() - lastWS > 100) {
        float v = Get_Voltage();
        float t = Get_Temperature(v);

        // ส่งข้อมูลเป็น JSON: {"v": 1.23, "t": 23.45}
        String json = "{\"v\":" + String(v, 3) + ",\"t\":" + String(t, 2) + "}";
        webSocket.broadcastTXT(json);
        lastWS = millis();
      }
      webSocket.loop();
      server.handleClient();
    }
  } else {
    // กรณีที่ GPIO 1 เป็น High (พักหน้าจอ)
    display.clearDisplay();
    display.fillTriangle(44+22, 22+3,  44+12, 22+13, 44+22, 22+13, SSD1306_WHITE);
    display.fillTriangle(44+22, 22+12, 44+32, 22+12, 44+22, 22+22, SSD1306_WHITE);
    display.display();
  }
}