#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>      
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/adc.h>

// --- การตั้งค่าพิน ---
#define I2C_SDA 2
#define I2C_SCL 3
#define ADC_CHAN ADC1_CHANNEL_5 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
int values[SCREEN_WIDTH];
String JSONtxt;

#include "html_page.h"

// ฟังก์ชัน Callback: แสดงข้อมูลเมื่อ ESP32 เข้าโหมด Config
void configModeCallback (WiFiManager *myWiFiManager) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("--- WIFI CONFIG ---");
  
  display.setCursor(0, 15);
  display.print("SSID: ");
  display.println("AutoConnectAP");
  
  display.print("PASS: ");
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // ไฮไลท์รหัสผ่านเพื่อให้สังเกตง่าย
  display.println("12345678");
  
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 40);
  display.println("Web Address:");
  display.setTextSize(1);
  display.setCursor(15, 52);
  display.println("192.168.4.1");
  
  display.display();
  
  Serial.println("Entered config mode");
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for(;;);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("System Booting...");
    display.display();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHAN, ADC_ATTEN_DB_11);

    WiFi.mode(WIFI_STA); 
    WiFiManager wm;

    // ตั้งค่า Callback เพื่อโชว์รหัสผ่านบนจอ
    wm.setAPCallback(configModeCallback);

    bool res;
    // ปรับเปลี่ยน SSID และ Password ตามที่คุณต้องการ
    res = wm.autoConnect("AutoConnectAP", "12345678"); 

    if(!res) {
        display.clearDisplay();
        display.println("Connect Failed!");
        display.display();
    } 
    else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WiFi Connected!");
        display.println("");
        display.setTextSize(1);
        display.println("SERVER IP:");
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        display.println(WiFi.localIP().toString()); 
        display.display();
        delay(3000);
    }

    for(int i=0; i<SCREEN_WIDTH; i++) values[i] = SCREEN_HEIGHT - 1;
    server.on("/", []() { server.send(200, "text/html", webpageCode); });
    server.begin();
    webSocket.begin();
}

void loop() {
    webSocket.loop();
    server.handleClient();

    // อ่านค่า Analog
    uint32_t raw = adc1_get_raw(ADC_CHAN);
    float voltage = (float)((map(raw, 0, 4095, 0, 2800)) + 134) / 1000.00;

    // อัปเดตข้อมูลกราฟ
    int y_val = map(raw, 0, 4095, SCREEN_HEIGHT - 1, 18);
    for(int i = 0; i < SCREEN_WIDTH - 1; i++) values[i] = values[i+1];
    values[SCREEN_WIDTH - 1] = y_val;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("IP: "); 
    display.println(WiFi.localIP()); 
    display.print("V : ");
    display.print(voltage, 2);
    display.println(" V");
    display.drawFastHLine(0, 16, 128, SSD1306_WHITE);

    for(int x = 0; x < SCREEN_WIDTH - 1; x++) {
        display.drawLine(x, values[x], x + 1, values[x+1], SSD1306_WHITE);
    }
    display.display();

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 50) {
        JSONtxt = "{\"val\":" + String(voltage, 3) + ",\"raw\":" + String(raw) + "}";
        webSocket.broadcastTXT(JSONtxt);
        lastSend = millis();
    }
}