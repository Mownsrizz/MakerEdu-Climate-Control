/*
 * ============================================================
 *  Hệ Thống Điều Hòa Thông Minh Ứng Dụng Công Nghệ Peltier 
 *  Tác giả: Võ Thành Đạt & Nguyễn Ngọc Thiện & Lê Trương Minh Nguyệt
 *  Mạch chủ: MakerEdu Creator (Arduino Uno Compatible)
 * ============================================================
 *
 *  PHẦN CỨNG:
 *  - MakerEdu Creator (ATmega328P)
 *  - DHT11           → A2  (nhiệt độ + độ ẩm)
 *  - Buzzer MKE-M03   → A1
 *  - LED MKE-M01      → A3
 *  - LCD 1602 I2C     → I2C (0x27)
 *  - BMP180           → I2C
 *  - Relay Module 2 kênh (10A, relay đen):
 *      Kênh 1 → D10, Kênh 2 → D11 (H-Bridge sò Peltier)
 *  - L298N Module:
 *      ENA → D3 (PWM quạt), ENB → D12 (bơm)
 *      IN1=HIGH, IN2=LOW (jumper cố định trên L298N)
 *      IN3=HIGH, IN4=LOW (jumper cố định trên L298N)
 *      12V từ nguồn tổ ong → VMS trên L298N
 *
 *  SƠ ĐỒ NỐI RELAY H-BRIDGE CHO SÒ PELTIER (AN TOÀN KHI BOOT):
 *    Relay1: COM → Dây sò A | NC → GND(PSU) | NO → +12V(PSU)
 *    Relay2: COM → Dây sò B | NC → GND(PSU) | NO → +12V(PSU)
 *
 *    Cả 2 OFF (NC) → Cùng GND (+0V)      → SÒ TẮT (An toàn tuyệt đối khi cúp điện)
 *    Cả 2 ON  (NO) → Cùng +12V           → SÒ TẮT
 *    R1 ON, R2 OFF → Dòng chạy A sang B  → LÀM LẠNH
 *    R1 OFF, R2 ON → Dòng chạy B sang A  → LÀM NÓNG
 *
 *  L298N ĐẤU NỐI:
 *    VMS  → +12V(PSU)
 *    GND  → GND(PSU) + GND(Arduino)
 *    Motor A (OUT1, OUT2) → 2 Quạt (song song)
 *    Motor B (OUT3, OUT4) → 2 Bơm (song song)
 */

#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <Adafruit_BMP085.h>
#include <avr/wdt.h>

// ==================== CẤU HÌNH CHÂN ====================
#define DHT_PIN       A2
#define BUZZER_PIN    A1
#define LED_PIN       A3
#define DHTTYPE       DHT11

#define RELAY_PELTIER_1  10   // D10 - H-Bridge kênh 1
#define RELAY_PELTIER_2  11   // D11 - H-Bridge kênh 2
#define L298N_ENA        3    // D3  - Quạt (PWM tốc độ)
#define L298N_ENB        12   // D12 - Bơm (ON/OFF)

// ==================== NGƯỠNG TÙY CHỈNH ====================
// Thay đổi các giá trị này tùy điều kiện môi trường
const float COOL_THRESHOLD   = 35.0;   // °C - trên ngưỡng → làm lạnh
const float HEAT_THRESHOLD   = 18.0;   // °C - dưới ngưỡng → làm nóng
const float COOL_HYSTERESIS  = 2.0;    // °C - tắt lạnh khi temp < (COOL_THRESHOLD - 2)
const float HEAT_HYSTERESIS  = 2.0;    // °C - tắt nóng khi temp > (HEAT_THRESHOLD + 2)
const float HIGH_PRESSURE    = 1100.0; // hPa - áp suất cao → cảnh báo
const float LOW_PRESSURE     = 900.0;  // hPa - áp suất thấp → cảnh báo
const float PRESSURE_HYSTERESIS = 20.0;// hPa - vùng chết áp suất

// ==================== CẤU HÌNH RELAY ====================
// true  = HIGH bật relay (jumper ở vị trí H)
// false = LOW bật relay  (jumper ở vị trí L)
#define RELAY_ACTIVE_HIGH true

#if RELAY_ACTIVE_HIGH
  #define RELAY_ON  HIGH
  #define RELAY_OFF LOW
#else
  #define RELAY_ON  LOW
  #define RELAY_OFF HIGH
#endif

// ==================== THỜI GIAN AN TOÀN ====================
const unsigned long PUMP_WARMUP_MS  = 2000;  // 2s chờ bơm ổn định
const unsigned long COOLDOWN_MS     = 30000; // 30s chờ tản nhiệt
const unsigned long SENSOR_READ_MS  = 1000;  // 1s giữa mỗi lần đọc
const float SENSOR_FILTER = 0.3; // Bộ lọc: 0.3 = 30% giá trị mới + 70% giá trị cũ

// Tốc độ quạt (PWM 0-255)
// Lưu ý: L298N sụt áp ~2-3V, quạt 12V nhận ~9-10V
// FAN_SPEED_MIN quá thấp → quạt không đủ lực quay
const uint8_t FAN_SPEED_MIN  = 160; // Tốc độ thấp nhất (~63%, đảm bảo quay được)
const uint8_t FAN_SPEED_MAX  = 255; // Tốc độ tối đa

// ==================== TRẠNG THÁI ====================
enum SystemState {
  STATE_IDLE,         // Bình thường - tất cả tắt
  STATE_PUMP_WARMUP,  // Bơm đang khởi động
  STATE_COOLING,      // Đang làm lạnh
  STATE_HEATING,      // Đang làm nóng
  STATE_COOLDOWN      // Chờ tản nhiệt trước khi tắt
};

enum Action {
  ACTION_NONE,
  ACTION_COOL,
  ACTION_HEAT
};

// Chiều hoạt động của sò Peltier (dùng để chống sốc nhiệt)
enum PeltierDir {
  DIR_NONE,
  DIR_COOL,
  DIR_HEAT
};

// ==================== KHỞI TẠO ĐỐI TƯỢNG ====================
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHTTYPE);
Adafruit_BMP085 bmp;

// ==================== BIẾN TOÀN CỤC ====================
SystemState currentState = STATE_IDLE;
float temperature = 25.0;  // Giá trị an toàn (giữa 2 ngưỡng)
float humidity    = 50.0;
float pressure    = 1013.0; // Áp suất khí quyển chuẩn
unsigned long lastSensorRead = 0;
unsigned long stateChangeTime = 0;
bool sensorError = true;    // true cho đến khi đọc được lần đầu
uint8_t sensorFailCount = 0; // Đếm số lần lỗi liên tiếp
const uint8_t MAX_SENSOR_FAILS = 5; // Cho phép 5 lần lỗi trước khi shutdown
bool firstValidRead = true; // Lần đọc hợp lệ đầu tiên (không lọc)
PeltierDir lastPeltierDir = DIR_NONE; // Lưu chiều hoạt động cuối cùng

// ==================== ĐIỀU KHIỂN RELAY + L298N ====================              
// Tắt sò Peltier (cả 2 OFF → cùng GND → an toàn tuyệt đối khi boot)
void peltierOff() {
  digitalWrite(RELAY_PELTIER_1, RELAY_OFF);
  digitalWrite(RELAY_PELTIER_2, RELAY_OFF);
}

// Sò chiều lạnh (R1 ON, R2 OFF)
void peltierCool() {
  digitalWrite(RELAY_PELTIER_1, RELAY_ON);
  digitalWrite(RELAY_PELTIER_2, RELAY_OFF);
}

// Sò chiều nóng (R1 OFF, R2 ON)
void peltierHeat() {
  digitalWrite(RELAY_PELTIER_1, RELAY_OFF);
  digitalWrite(RELAY_PELTIER_2, RELAY_ON);
}

void fansOn()    { analogWrite(L298N_ENA, FAN_SPEED_MAX); }
void fansOff()   { analogWrite(L298N_ENA, 0); }
// D12 không phải chân PWM → dùng digitalWrite (ON/OFF)
void pumpsOn()   { digitalWrite(L298N_ENB, HIGH); }
void pumpsOff()  { digitalWrite(L298N_ENB, LOW); }

// Điều chỉnh tốc độ quạt theo nhiệt độ (càng nóng càng nhanh)
// Chỉ gọi trong STATE_COOLING và STATE_HEATING
void fansAutoSpeed() {
  float diff, range = 10.0;

  if (currentState == STATE_COOLING) {
    diff = temperature - (COOL_THRESHOLD - COOL_HYSTERESIS);
  } else {
    // STATE_HEATING: lạnh quá → quạt cũng nhanh để tản nhiệt
    diff = (HEAT_THRESHOLD + HEAT_HYSTERESIS) - temperature;
  }

  float ratio = constrain(diff / range, 0.0, 1.0);
  uint8_t speed = FAN_SPEED_MIN + (uint8_t)((FAN_SPEED_MAX - FAN_SPEED_MIN) * ratio);
  analogWrite(L298N_ENA, speed);
}

// Tắt tất cả (gọi khi khẩn cấp hoặc khởi động)
void allOff() {
  peltierOff();
  fansOff();
  pumpsOff();
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

// ==================== CẢNH BÁO ====================
void alertBlink() {
  // Giảm delay để tránh trễ watchdog khi kết hợp delay khác
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    wdt_reset(); // Reset watchdog giữa các lần nháy
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}

// ==================== ĐỌC CẢM BIẾN ====================
bool readSensors() {
  bool ok = true;

  // Đọc DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    ok = false;
    // Giữ giá trị cũ, không ghi đè
  } else {
    if (firstValidRead) {
      // Lần đầu: gán trực tiếp (không lọc)
      temperature = t;
      humidity    = h;
    } else {
      // Bộ lọc trung bình giảm nhiễu, chống nhảy số LCD
      temperature = temperature * (1.0 - SENSOR_FILTER) + t * SENSOR_FILTER;
      humidity    = humidity    * (1.0 - SENSOR_FILTER) + h * SENSOR_FILTER;
    }
  }

  // Luôn đọc BMP180 (độc lập với DHT)
  float p = bmp.readPressure() / 100.0; // Pa → hPa
  // Kiểm tra giá trị hợp lệ (300-1200 hPa là phạm vi thực tế)
  if (p >= 300.0 && p <= 1200.0) {
    if (firstValidRead) {
      pressure = p;
    } else {
      pressure = pressure * (1.0 - SENSOR_FILTER) + p * SENSOR_FILTER;
    }
  } else {
    ok = false; // Báo lỗi nếu BMP180 trả giá trị sai (mất kết nối)
  }

  sensorError = !ok;
  if (!ok) {
    // Cap tại MAX để tránh tràn uint8_t (255 → 0)
    if (sensorFailCount < MAX_SENSOR_FAILS) {
      sensorFailCount++;
    }
  } else {
    sensorFailCount = 0;
    firstValidRead = false; // Đã có dữ liệu hợp lệ
  }
  return ok;
}

// ==================== CẬP NHẬT LCD ====================
void updateLCD() {
  // Ghi đè trực tiếp với ký tự padding, không xóa màn hình
  // → Tránh nhấp nháy LCD

  // Dòng 1: Nhiệt độ + Độ ẩm
  char line1[17];
  // dtostrf: chuyển float → string (Arduino không hỗ trợ sprintf %f)
  char tempStr[7], humStr[5];
  dtostrf(temperature, 4, 1, tempStr); // ví dụ: "35.2"
  dtostrf(humidity, 3, 0, humStr);     // ví dụ: " 65"
  snprintf(line1, 17, "%sC %s%%", tempStr, humStr);
  // Đệm padding cho đủ 16 ký tự
  int len1 = strlen(line1);
  for (int i = len1; i < 16; i++) line1[i] = ' ';
  line1[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(line1);

  // Dòng 2: Áp suất + Trạng thái
  char line2[17];
  char pressStr[7];
  dtostrf(pressure, 4, 0, pressStr); // ví dụ: "1013"

  const char* stateStr;
  switch (currentState) {
    case STATE_IDLE:        stateStr = "IDLE"; break;
    case STATE_PUMP_WARMUP: stateStr = "PUMP"; break;
    case STATE_COOLING:     stateStr = "COOL"; break;
    case STATE_HEATING:     stateStr = "HEAT"; break;
    case STATE_COOLDOWN:    stateStr = "WAIT"; break;
    default:                stateStr = "????"; break;
  }
  snprintf(line2, 17, "%shPa %s", pressStr, stateStr);
  int len2 = strlen(line2);
  for (int i = len2; i < 16; i++) line2[i] = ' ';
  line2[16] = '\0';
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ==================== XÁC ĐỊNH HÀNH ĐỘNG ====================
Action determineAction() {
  // Hysteresis: khi đang làm lạnh, chỉ tắt khi temp < (COOL_THRESHOLD - HYSTERESIS)
  // Ngăn relay đóng/mở liên tục khi temp dao động quanh ngưỡng

  // Ưu tiên 1: Nhiệt độ
  if (currentState == STATE_COOLING || currentState == STATE_COOLDOWN) {
    // Đang làm lạnh: chỉ dừng khi xuống dưới (ngưỡng - hysteresis)
    if (temperature > (COOL_THRESHOLD - COOL_HYSTERESIS)) return ACTION_COOL;
  } else if (currentState == STATE_HEATING) {
    // Đang làm nóng: chỉ dừng khi lên trên (ngưỡng + hysteresis)
    if (temperature < (HEAT_THRESHOLD + HEAT_HYSTERESIS)) return ACTION_HEAT;
  }

  // Kiểm tra ngưỡng bình thường (khi IDLE hoặc PUMP_WARMUP)
  if (temperature > COOL_THRESHOLD) return ACTION_COOL;
  if (temperature < HEAT_THRESHOLD) return ACTION_HEAT;

  // Ưu tiên 2: Áp suất (chỉ xét khi nhiệt độ bình thường)
  if (currentState == STATE_COOLING || currentState == STATE_COOLDOWN) {
    if (pressure > (HIGH_PRESSURE - PRESSURE_HYSTERESIS)) return ACTION_COOL;
  } else if (currentState == STATE_HEATING) {
    if (pressure < (LOW_PRESSURE + PRESSURE_HYSTERESIS)) return ACTION_HEAT;
  }
  if (pressure > HIGH_PRESSURE) return ACTION_COOL;
  if (pressure < LOW_PRESSURE)  return ACTION_HEAT;

  return ACTION_NONE;
}

// ==================== IN SERIAL DEBUG ====================
void printDebug() {
  Serial.print("T=");
  Serial.print(temperature, 1);
  Serial.print("C | H=");
  Serial.print(humidity, 0);
  Serial.print("% | P=");
  Serial.print(pressure, 0);
  Serial.print("hPa | ");

  switch (currentState) {
    case STATE_IDLE:        Serial.println("IDLE"); break;
    case STATE_PUMP_WARMUP: Serial.println("PUMP_WARMUP"); break;
    case STATE_COOLING:     Serial.println("COOLING"); break;
    case STATE_HEATING:     Serial.println("HEATING"); break;
    case STATE_COOLDOWN:    Serial.println("COOLDOWN"); break;
  }
}

// ==================== SETUP ====================
void setup() {
  // === BẢO VỆ KHI BOOT ===
  // Với sơ đồ đấu dây mới (Cả 2 NC nối GND), khi Arduino khởi động 
  // chân digital bị floating (LOW), relay sẽ nhả về NC.
  // Nhờ vậy, sò Peltier sẽ tự động nhận 0V (GND-GND) và TẮT an toàn.
  pinMode(RELAY_PELTIER_1, OUTPUT);
  pinMode(RELAY_PELTIER_2, OUTPUT);
  pinMode(L298N_ENA, OUTPUT);
  pinMode(L298N_ENB, OUTPUT);
  // Đảm bảo tắt mọi thứ bằng phần mềm
  peltierOff(); 
  fansOff();
  pumpsOff();

  // Cấu hình chân cảnh báo
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(9600);
  Serial.println(F("=== HE THONG DIEU HOA KHONG KHI V2 ==="));

  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Dieu Hoa KK V2");
  lcd.setCursor(0, 1);
  lcd.print("Khoi dong...");

  // Khởi tạo cảm biến
  dht.begin();
  if (!bmp.begin()) {
    lcd.clear();
    lcd.print("LOI BMP180!");
    Serial.println(F("LOI: BMP180 khong phan hoi!"));
    while (1) { delay(1000); } // Dừng luôn, không chạy tiếp
  }

  delay(2000);
  lcd.clear();

  // Bật Watchdog SAU KHI mọi thứ đã init xong
  // Tránh reset liên tục nếu LCD/BMP khởi động chậm
  wdt_enable(WDTO_8S);

  Serial.println(F("Khoi dong thanh cong!"));
  Serial.print(F("Nguong lam lanh: "));
  Serial.print(COOL_THRESHOLD);
  Serial.println(F("C"));
  Serial.print(F("Nguong lam nong: "));
  Serial.print(HEAT_THRESHOLD);
  Serial.println(F("C"));
}

// ==================== MAIN LOOP ====================
void loop() {
  wdt_reset(); // Reset watchdog mỗi vòng

  unsigned long now = millis();

  // Đọc cảm biến định kỳ
  if (now - lastSensorRead >= SENSOR_READ_MS) {
    lastSensorRead = now;

    if (readSensors()) {
      updateLCD();
      printDebug();
    } else {
      lcd.setCursor(0, 0);
      lcd.print("LOI CAM BIEN!   ");
      Serial.println(F("LOI: Doc cam bien that bai!"));
    }
  }

  // Nếu cảm biến lỗi liên tục >= 5 lần → tắt hết, an toàn
  if (sensorError && sensorFailCount >= MAX_SENSOR_FAILS) {
    if (currentState != STATE_IDLE) {
      allOff();
      currentState = STATE_IDLE;
      alertBlink();
      Serial.println(F("!! SHUTDOWN: Cam bien loi lien tuc!"));
    }
    // Hiển thị lỗi trên cả 2 dòng LCD
    lcd.setCursor(0, 0);
    lcd.print("LOI CAM BIEN!   ");
    lcd.setCursor(0, 1);
    lcd.print("Kiem tra DHT11  ");
    return;
  }

  // === STATE MACHINE ===
  Action action = determineAction();

  switch (currentState) {

    // --- IDLE: Chờ, mọi thứ tắt ---
    case STATE_IDLE:
      if (action != ACTION_NONE) {
        pumpsOn();
        stateChangeTime = now;
        currentState = STATE_PUMP_WARMUP;
        Serial.println(F(">> Bat bom, cho 2s..."));
        alertBlink();
      }
      break;

    // --- PUMP WARMUP: Bơm đang chạy, chờ ổn định ---
    case STATE_PUMP_WARMUP:
      if (now - stateChangeTime >= PUMP_WARMUP_MS) {
        fansOn();
        action = determineAction(); // Kiểm tra lại

        if (action == ACTION_COOL) {
          peltierCool();
          lastPeltierDir = DIR_COOL; // Cập nhật chiều
          currentState = STATE_COOLING;
          Serial.println(F(">> LAM LANH"));
        } else if (action == ACTION_HEAT) {
          peltierHeat();
          lastPeltierDir = DIR_HEAT; // Cập nhật chiều
          currentState = STATE_HEATING;
          Serial.println(F(">> LAM NONG"));
        } else {
          // Đã ổn trong lúc chờ
          allOff();
          currentState = STATE_IDLE;
          Serial.println(F(">> Da on, tat het"));
        }
      }
      break;

    // --- COOLING: Đang làm lạnh ---
    case STATE_COOLING:
      fansAutoSpeed(); // Điều chỉnh tốc độ quạt theo nhiệt độ
      if (action == ACTION_NONE) {
        // Ổn rồi → tắt sò, vào cooldown
        peltierOff();
        stateChangeTime = now;
        currentState = STATE_COOLDOWN;
        Serial.println(F(">> Tat so, cho tan nhiet 30s..."));
      } else if (action == ACTION_HEAT) {
        // Đổi chiều (CHỐNG SỐC NHIỆT): Ép vào COOLDOWN 30s trước khi đảo cực
        peltierOff();
        stateChangeTime = now;
        currentState = STATE_COOLDOWN;
        Serial.println(F(">> Can LAM NONG. Cho tan nhiet 30s chong soc nhiet!"));
      }
      break;

    // --- HEATING: Đang làm nóng ---
    case STATE_HEATING:
      fansAutoSpeed(); // Điều chỉnh tốc độ quạt theo nhiệt độ
      if (action == ACTION_NONE) {
        peltierOff();
        stateChangeTime = now;
        currentState = STATE_COOLDOWN;
        Serial.println(F(">> Tat so, cho tan nhiet 30s..."));
      } else if (action == ACTION_COOL) {
        // Đổi chiều (CHỐNG SỐC NHIỆT): Ép vào COOLDOWN 30s trước khi đảo cực
        peltierOff();
        stateChangeTime = now;
        currentState = STATE_COOLDOWN;
        Serial.println(F(">> Can LAM LANH. Cho tan nhiet 30s chong soc nhiet!"));
      }
      break;

    // --- COOLDOWN: Sò đã tắt, chờ tản nhiệt ---
    case STATE_COOLDOWN:
      fansOn(); // Quạt chạy MAX để tản nhiệt nhanh nhất
      
      // Chỉ cho phép bật lại ngay nếu CÙNG CHIỀU (tránh sốc nhiệt)
      if (action == ACTION_COOL && lastPeltierDir != DIR_HEAT) {
        peltierCool();
        currentState = STATE_COOLING;
        Serial.println(F(">> Bat lai LAM LANH"));
      } else if (action == ACTION_HEAT && lastPeltierDir != DIR_COOL) {
        peltierHeat();
        currentState = STATE_HEATING;
        Serial.println(F(">> Bat lai LAM NONG"));
      } else if (now - stateChangeTime >= COOLDOWN_MS) {
        // Đã chờ đủ 30s → tắt hết, xóa bộ nhớ chiều
        fansOff();
        pumpsOff();
        lastPeltierDir = DIR_NONE;
        currentState = STATE_IDLE;
        Serial.println(F(">> IDLE - Tat het"));
      }
      break;
  }
}
