# Smart Climate Control System using Peltier Technology 🌬️❄️

*Read in another language: [Tiếng Việt](#-phần-tiếng-việt)*

**Author:** Nguyen Ngoc Thien (Mownsrizz - HaMaCon)  
**Co-Authors:** Vo Thanh Dat, Le Truong Minh Nguyet

## 📖 Introduction
The "Smart Climate Control System using Peltier Technology" is an intuitive STEM educational model inspired by the need for practical learning in secondary education. In the context of complex climate change, air conditioning is essential, yet most students only know it as a household appliance without understanding the underlying mechanics. This project accurately simulates the entire cooling, heat dissipation, and automation process of an industrial HVAC system. Moving beyond simple on/off switches, it is engineered with a "Fail-Safe Automation" mindset, utilizing a **State Machine** architecture to protect the hardware and optimize energy efficiency.

## 🛠️ Key Features
1. **Actual Heating / Cooling:** Uses the thermoelectric effect of the TEC1-12706 Peltier module combined with a water-cooling circulation system.
2. **Safe Polarity Reversal (H-Bridge):** Implements a 2-channel Relay module configured as an H-Bridge, allowing flexible switching between cooling and heating modes. The hardware wiring forces the current to 0V upon booting to prevent short circuits.
3. **Energy Efficiency (Inverter Simulation):** The `IDLE` mode cuts off all power consumption when the target temperature is reached. It also features automatic PWM (Pulse Width Modulation) to dynamically adjust fan speeds based on temperature differentials.
4. **Thermal Shock Prevention (Cooldown):** The system automatically forces the water pumps and fans to run for an additional 30 seconds after the Peltier module is turned off. This dissipates residual heat and prevents the ceramic semiconductor from cracking.
5. **Anti-Hang Protection (Watchdog Timer):** Automatically detects system hangs caused by electromagnetic interference and reboots the microcontroller within 8 seconds.

## 🧰 Hardware Requirements
- **Microcontroller:** MakerEdu Creator (Arduino Uno Compatible - ATmega328P).
- **Sensors:** DHT11 (Temperature & Humidity), BMP180 (Atmospheric Pressure).
- **Power Modules:** L298N Motor Driver (Controls Fans & Pumps), 2-Channel Relay (Controls Peltier module).
- **Actuators:** TEC1-12706 Peltier module, 12V Cooling Fans, 12V Mini Water Pumps.
- **Power Supply:** 12V-10A Switching Power Supply (for actuators) and 5V USB (for logic control).

## 🚀 Libraries Installation
This project uses the Arduino IDE. You need to install the following libraries via the Library Manager:
- `LiquidCrystal_I2C` (For the LCD display)
- `DHT sensor library` by Adafruit (For the DHT11 sensor)
- `Adafruit BMP085 Library` (For the pressure sensor)

## ⚙️ State Machine Architecture
The control algorithm is divided into a 5-state loop:
- `STATE_IDLE`: Hibernation mode, all actuators are off to save power.
- `STATE_PUMP_WARMUP`: Primes the water pumps for 2 seconds to establish coolant flow before powering the Peltier.
- `STATE_COOLING` / `STATE_HEATING`: Activates the Peltier module and automatically adjusts fan speed (PWM) based on real-time temperature.
- `STATE_COOLDOWN`: Cuts power to the Peltier but maintains Pump + Fan operation for 30s to safely dissipate residual temperature.

## ⚠️ Safety Precautions
- **MANDATORY:** Always plug in the 12V power supply FIRST, then the 5V USB power for the microcontroller. This prevents the Relay module from receiving floating signals during boot.
- Do not let the water pumps run dry for extended periods.

---
*This project was built for educational purposes, aiming to enhance logical thinking and ignite a passion for STEM among secondary school students.*

<br><br>

---

# 🇻🇳 Phần Tiếng Việt
# Hệ Thống Điều Hòa Thông Minh Ứng Dụng Công Nghệ Peltier 🌬️❄️

**Tác giả:** Nguyễn Ngọc Thiện (Mownsrizz - HaMaCon)  
**Đồng tác giả:** Võ Thành Đạt, Lê Trương Minh Nguyệt

## 📖 Giới thiệu
Ý tưởng của sản phẩm “Hệ thống điều hòa thông minh ứng dụng công nghệ Peltier” xuất phát từ thực tiễn đời sống và nhu cầu học tập của học sinh trung học cơ sở trong các hoạt động giáo dục STEM. Nhằm giúp học sinh thấu hiểu nguyên lý làm lạnh, cơ chế truyền nhiệt và các thuật toán tự động, mô hình giáo dục trực quan này thu nhỏ toàn bộ quy trình vận hành của một chiếc máy lạnh công nghiệp. Dự án không chỉ bật/tắt thiết bị đơn thuần mà được thiết kế với tư duy "Tự động hóa an toàn" (Fail-Safe), sử dụng thuật toán **Máy trạng thái (State Machine)** để bảo vệ phần cứng, mô phỏng công nghệ Inverter thân thiện với môi trường và tối ưu hóa năng lượng.

## 🛠️ Tính năng nổi bật
1. **Làm lạnh / Làm nóng thực tế:** Sử dụng hiệu ứng nhiệt điện của sò Peltier TEC1-12706 kết hợp hệ thống bơm tản nhiệt nước tuần hoàn.
2. **Đảo chiều an toàn (H-Bridge):** Ứng dụng Relay 2 kênh tạo thành mạch Cầu H, cho phép đảo cực sò Peltier linh hoạt. Cấu hình phần cứng ép dòng điện về 0V khi khởi động để chống đoản mạch.
3. **Tiết kiệm điện (Mô phỏng Inverter):** Chế độ `IDLE` ngắt toàn bộ điện năng khi đạt nhiệt độ chuẩn. Tính năng tự động băm xung (PWM) để điều tốc quạt gió linh hoạt dựa trên sự chênh lệch nhiệt độ.
4. **Cơ chế chống sốc nhiệt (Cooldown):** Hệ thống tự động ép máy bơm và quạt chạy thêm 30 giây sau khi ngắt điện sò Peltier để xả nhiệt tồn đọng, bảo vệ gốm bán dẫn không bị nứt vỡ do biến thiên nhiệt quá nhanh.
5. **Chống treo (Watchdog Timer):** Tự động phát hiện lỗi và khởi động lại vi điều khiển trong vòng 8 giây nếu hệ thống bị treo do nhiễu điện từ.

## 🧰 Phần cứng sử dụng
- **Vi mạch trung tâm:** MakerEdu Creator (Tương thích Arduino Uno - ATmega328P).
- **Cảm biến:** DHT11 (Nhiệt độ & Độ ẩm), BMP180 (Áp suất không khí).
- **Module Công suất:** L298N (Điều khiển Quạt & Bơm), Relay 2 kênh (Điều khiển Sò Peltier).
- **Cơ cấu chấp hành:** Sò nóng lạnh TEC1-12706, Quạt tản nhiệt 12V, Máy bơm nước mini 12V.
- **Năng lượng:** Nguồn tổ ong 12V-10A (Cho phần công suất) và cáp USB 5V (Cho mạch điều khiển).

## 🚀 Cài đặt thư viện
Dự án sử dụng Arduino IDE. Bạn cần cài đặt các thư viện sau thông qua Library Manager:
- `LiquidCrystal_I2C` (Hiển thị màn hình LCD)
- `DHT sensor library` by Adafruit (Cho cảm biến nhiệt độ)
- `Adafruit BMP085 Library` (Cho cảm biến áp suất)

## ⚙️ Kiến trúc Máy trạng thái (State Machine)
Thuật toán chia hoạt động thành 5 trạng thái quản lý khép kín:
- `STATE_IDLE`: Trạng thái ngủ đông, ngắt toàn bộ thiết bị để tiết kiệm điện.
- `STATE_PUMP_WARMUP`: Khởi động mồi máy bơm nước trước 2 giây để tạo dòng tản nhiệt trước khi cấp điện cho sò.
- `STATE_COOLING` / `STATE_HEATING`: Kích hoạt sò Peltier làm lạnh/nóng, tự động điều chỉnh tốc độ quạt (PWM) bám sát nhiệt độ thực tế.
- `STATE_COOLDOWN`: Ngắt điện sò Peltier, duy trì Bơm + Quạt thêm 30s để xả lượng nhiệt dư thừa.

## ⚠️ Lưu ý an toàn
- **BẮT BUỘC** cấp nguồn 12V trước, sau đó mới cấp nguồn USB 5V cho vi điều khiển để mạch Relay không bị nhiễu tín hiệu (floating) lúc khởi động.
- Tuyệt đối không để máy bơm chạy khan (không có nước) quá lâu để tránh cháy động cơ.

---
*Dự án được xây dựng với mục tiêu giáo dục, nâng cao tư duy logic thuật toán và lan tỏa ngọn lửa đam mê khoa học công nghệ (STEM) đến học sinh trung học.*
