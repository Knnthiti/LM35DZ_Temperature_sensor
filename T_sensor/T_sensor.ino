#include <driver/adc.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

uint32_t raw_value_1 = 0;
uint32_t raw_value_avg = 0;
float V = 0;

#define I2C_SDA 4
#define I2C_SCL 5

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  // setup_ADC
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_BIT_12);

  Wire.begin(I2C_SDA, I2C_SCL);
  
  // เช็คการเชื่อมต่อ OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // วนลูปค้างไว้ถ้าหาจอไม่เจอ
  }

  // กำหนดสีตัวอักษรเป็นสีขาว (เปิดหลอด LED) ให้จอ OLED ตั้งแต่ Setup
  display.setTextColor(SSD1306_WHITE);

  Serial.begin(115200);
}

void loop() {
  for (uint16_t i = 0; i < 1000; i++) {
    raw_value_avg += adc1_get_raw(ADC1_CHANNEL_5);
    delay(1);
  }

  raw_value_1 = raw_value_avg / 1000;
  raw_value_avg = 0;

  // bit_to_V (เปลี่ยนมาใช้สมการแทน map เพื่อรักษาทศนิยม และใส่ offset ชดเชย)
  // (raw_value * (950 / 4096)) + 4 แล้วนำทั้งหมดไปหาร 1000
  V = ((raw_value_1 * (950.0 / 4096.0)) + 4.0) / 1000.0;

  // print value ลง Serial Monitor
  Serial.print(raw_value_1);
  Serial.print(" , ");
  Serial.print(V, 4);
  Serial.println(" V");

  // แสดงผลบนจอ OLED
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(V, 2); // แสดงผลแค่ 2 ตำแหน่งบนจอตามที่คุณเขียนไว้
  display.print(" V");
  display.display();

  delay(10);
}