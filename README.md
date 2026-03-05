# Zero and Span Adjustment Circuit for LM35 Temperature Sensor

โปรเจกต์นี้เป็นการออกแบบและสร้างวงจรปรับแต่งสัญญาณ (Signal Conditioning) เพื่อแปลงแรงดันจากเซนเซอร์ **LM35** ให้มีช่วงแรงดันขาออกที่เหมาะสมกับ ADC ของ Microcontroller โดยเฉพาะช่วง **1V – 3.3V**

## 📐 รายละเอียดการคำนวณ (Calculation)

เป้าหมายคือการแปลงช่วงอุณหภูมิ **20°C – 50°C** ให้เป็นแรงดันขาออกในช่วง **1V – 3.3V** เพื่อความละเอียดสูงสุดในการอ่านค่า

### 1. วิเคราะห์สัญญาณขาเข้า (Input Analysis)
[cite_start]เซนเซอร์ LM35 มีความสัมพันธ์ของแรงดันคือ $10mV/°C$[cite: 5]:
* ที่ $20°C$: $V_{in} = 20 \times 10mV = 200mV$ (0.2V)
* ที่ $50°C$: $V_{in} = 50 \times 10mV = 500mV$ (0.5V)

### 2. สมการเส้นตรง (Transfer Function)
ใช้สูตรสมการเส้นตรง $y = mx + c$ เพื่อหาค่า Gain และ Offset:

* **หาค่าความชัน (Span/Gain):**
  $$m = \frac{\Delta V_{out}}{\Delta V_{in}} = \frac{3.3V - 1.0V}{0.5V - 0.2V} = \frac{2.3}{0.3} \approx 7.6667$$

* **หาค่าจุดตัดแกน Y (Zero/Offset):**
  แทนค่าใน $y = mx + c$ ด้วยจุด $(0.2, 1.0)$:
  $$1.0 = (7.6667 \times 0.2) + c$$
  $$1.0 = 1.5333 + c$$
  $$c = 1.0 - 1.5333 = -0.5333$$

**สมการความสัมพันธ์สุดท้าย:**
$$V_{out} = 7.67(V_{in}) - 0.533$$

---

## 🛠 โครงสร้างและการทำงานของวงจร

วงจรนี้ใช้ Op-Amp **LM358** ในการจัดการสัญญาณ แบ่งออกเป็นส่วนต่างๆ ดังนี้:

**Sensor Input:** รับสัญญาณจาก LM35-LP ที่ไฟเลี้ยง 3.3V
**Input Buffer (U2A):** ทำหน้าที่เป็น Voltage Follower เพื่อแยกโหลดออกจากตัวเซนเซอร์
**Zero Adjustment (U3A):** ใช้ Potentiometer (RV1) เพื่อปรับแรงดันอ้างอิง (Reference Voltage) สำหรับชดเชยค่า Offset
**Span Adjustment (U3B):** ปรับอัตราขยาย (Gain) เพื่อให้ได้ช่วงแรงดันที่ต้องการ (Span)
**Output:** สัญญาณ $V_{out}$ จะถูกส่งออกทาง Connector J1



[Image of operational amplifier differential amplifier circuit diagram]


## 📊 ระบบบันทึกข้อมูล (Data Logging System)

ข้อมูลจะถูกประมวลผลผ่าน Microcontroller และส่งต่อดังนี้:
1. **ADC/uC:** แปลงสัญญาณ Analog เป็น Digital
2. **Terminal:** แสดงค่าพารามิเตอร์แบบ Text ผ่าน Serial Port
3. **Chart/Log:** พล็อตกราฟและบันทึกข้อมูลลงไฟล์ `.elxs`

---
