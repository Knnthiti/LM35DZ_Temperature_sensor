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
#define BTN_SCOPE 7

// --- Objects ---
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// --- Variables ---
enum DeviceMode { IDLE,
                  MODE_WIFI,
                  MODE_SCOPE };
DeviceMode currentMode = IDLE;

#include "html_page.h"  // อย่าลืมไฟล์นี้ต้องอยู่ในโฟลเดอร์เดียวกัน

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
    server.send(200, "text/html", webpageCode);
  });
  server.begin();
  webSocket.begin();
  Serial.println("Network Services Started");
}

// --- 4. ฟังก์ชันจัดการ WiFi (เรียกใช้เมื่อต้องการล้างค่า/ตั้งใหม่) ---
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

// --- 5. ฟังก์ชันอ่านค่า ADC 6 และแสดงผล ---
void Print_ADC() {
  float voltage = ADC_2_Temperature();

  display.clearDisplay();
  // display.setTextSize(1);
  // display.setCursor(0, 0);
  // display.print("IP: ");
  // display.println(WiFi.localIP());

  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(voltage, 2);
  display.print(" V");
  display.display();
}

float ADC_2_Temperature() {
  for (uint16_t i = 0; i < 1000; i++) {
    raw_value_avg += adc1_get_raw(ADC_SCOPE_PIN);
    delay(1);
  }

  raw_value_1 = raw_value_avg/1000;
  raw_value_avg = 0;

  //bit_to_V
  V = (float)(map(raw_value_1, 0, 4096, 0, 950)+4) / 1000.00;
  V += 0.04;
  return V;
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_WIFI, INPUT_PULLUP);
  pinMode(BTN_SCOPE, INPUT_PULLUP);

  pinMode(1, INPUT);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    for (;;)
      ;

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_SCOPE_PIN, ADC_ATTEN_DB_0);

  updateDisplay("STARTING...", "Connecting WiFi...", "Please wait");

  // พยายามเชื่อมต่อ WiFi เดิมที่มีอยู่ก่อน
  runWiFiManager(false);
}

void loop() {
  if (digitalRead(1) == 0) {
    display.clearDisplay();
    // กดปุ่ม GPIO 3 เพื่อเข้าโหมดตั้งค่า WiFi ใหม่
    if (digitalRead(BTN_WIFI) == LOW) {
      delay(10);
      ESP.restart();
      runWiFiManager(true);
      currentMode = IDLE;
    }

    // กดปุ่ม GPIO 7 เพื่อกลับเข้าโหมด Scope (กรณีหลุดไปโหมดอื่น)
    if (digitalRead(BTN_SCOPE) == LOW) {
      delay(10);
      currentMode = MODE_SCOPE;
    }

    if (currentMode == MODE_SCOPE) {
      Print_ADC();
    } else if (currentMode == MODE_WIFI) {
      static unsigned long lastWS = 0;
      if (millis() - lastWS > 100) {
        float voltage = ADC_2_Temperature();

        String json = "{\"adc\":" + String(voltage, 2) + "}";
        webSocket.broadcastTXT(json);
        lastWS = millis();
      }

      webSocket.loop();
      server.handleClient();
    }
  } else {
    display.clearDisplay();
    display.fillTriangle(44+22, 22+3,  44+12, 22+13, 44+22, 22+13, SSD1306_WHITE);
    display.fillTriangle(44+22, 22+12, 44+32, 22+12, 44+22, 22+22, SSD1306_WHITE);
    display.display();
  }
}