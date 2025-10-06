#include <driver/twai.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <algorithm>

#define CAN_TX_PIN GPIO_NUM_5
#define CAN_RX_PIN GPIO_NUM_4

// === CAN ID Setup ===
uint32_t ID_RPM = 0x0C9;
int BYTE_RPM_H = 1;
int BYTE_RPM_L = 2;

uint32_t ID_TEMP = 0x4C1;
int BYTE_TEMP = 2;

uint32_t ID_TRANS = 0x1F5;       // Transmission_General_Status_2
uint32_t ID_TRANS_OILTEMP = 0x4C9; // Transmission_General_Status_3
uint32_t ID_FUEL = 0x4D1;
int BYTE_FUEL = 5;
uint32_t ID_FUEL_INJECTED = 0x3F9;
uint8_t latestTransLen = 0;
uint8_t latestTransData[8] = {0};


// === Data Variable ===
float rpm = 0;
float temperature = 0;
float transOilTemp = 0;
int gear = 0;
float fuelLevel = 0;
float fuelInjected = 0;
float acPressure = 0;
float fanSpeed = 0;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#define MAX_CAN_IDS 100
uint32_t canIDs[MAX_CAN_IDS];
uint8_t canIDCount = 0;

// === Tambah ID unik ke daftar ===
void addCANID(uint32_t id) {
  for (uint8_t i = 0; i < canIDCount; i++) if (canIDs[i] == id) return;
  if (canIDCount < MAX_CAN_IDS) canIDs[canIDCount++] = id;
}

// === Setup TWAI (CAN) ===
void setupCAN() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) Serial.println("TWAI driver installed");
  else { Serial.println("Failed to install TWAI driver"); return; }

  if (twai_start() == ESP_OK) Serial.println("TWAI started");
  else Serial.println("Failed to start TWAI");
}

// === Nama Gear ===
String gearName(int g) {
  switch (g) {
    case 1: return "1st";
    case 2: return "2nd";
    case 3: return "3rd";
    case 4: return "4th";
    case 5: return "5th";
    case 6: return "6th";
    case 12: return "CVT";
    case 13: return "N";
    case 14: return "R";
    case 15: return "P";
    default: return "-";
  }
}

// === Baca data CAN ===
void readCANMessages() {
  twai_message_t message;
  if (twai_receive(&message, 0) == ESP_OK) {
    addCANID(message.identifier);

    // RPM
    if (message.identifier == ID_RPM && message.data_length_code >= std::max(BYTE_RPM_H, BYTE_RPM_L) + 1) {
      uint16_t val = (message.data[BYTE_RPM_H] << 8) | message.data[BYTE_RPM_L];
      rpm = val * 0.25;
    }

    // Coolant Temp
    if (message.identifier == ID_TEMP && message.data_length_code >= BYTE_TEMP + 1) {
      temperature = message.data[BYTE_TEMP] - 40;
    }

    // Gear Position (bit 3..6 dari byte[0])
    if (message.identifier == ID_TRANS && message.data_length_code >= 1) {
      uint8_t raw = message.data[0] & 0x0F;
      gear = raw;

      // Simpan DATA CANBUS untuk display
    latestTransLen = message.data_length_code;
    for (uint8_t i = 0; i < message.data_length_code && i < 8; i++) {
        latestTransData[i] = message.data[i];
    }
    }

    // Transmission Oil Temp (byte[1], offset -40)
    if (message.identifier == ID_TRANS_OILTEMP && message.data_length_code >= 2) {
      uint8_t raw = message.data[1];
      transOilTemp = raw - 40;
    }
    if (message.identifier == ID_FUEL && message.data_length_code >= BYTE_FUEL + 1) {
  uint8_t raw = message.data[BYTE_FUEL];
  fuelLevel = raw * 0.3921; // sesuai DBC
  if (fuelLevel > 100) fuelLevel = 100;
    }
    if (message.identifier == ID_FUEL_INJECTED && message.data_length_code >= 8) {
  // Fuel_Injected_Rolling_Count (bit 15..0 → byte[0], byte[1])
  uint16_t rawFuelInjected = (message.data[1] << 8) | message.data[2];
  fuelInjected = rawFuelInjected * 0.0000305176; // hasil dalam liter

  // AC_High_Side_Pressure (byte[7])
  uint8_t rawPressure = message.data[7];
  acPressure = rawPressure * 14; // kPa

    // Engine_Cooling_Fan_Speed (byte[5])
  uint8_t rawFan = message.data[5];
  fanSpeed = rawFan * 0.392157; // %
   }
  }
}

// === OLED Setup ===
void setupOLED() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "CAN Display Init...");
  u8g2.sendBuffer();
}

// === Gambar layar utama ===
void drawMainScreen(float rpmVal, float tempVal, float oilTempVal) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_mf);

  // Baris 1: RPM dan Temp
  u8g2.drawStr(0, 12, "RPM :");
  char rpmStr[10];
  snprintf(rpmStr, sizeof(rpmStr), "%4d", (int)rpmVal);
  u8g2.drawStr(34, 12, rpmStr);

  u8g2.drawStr(64, 12, "Temp:");
  char tempStr[10];
  snprintf(tempStr, sizeof(tempStr), "%3dC", (int)tempVal);
  u8g2.drawStr(94, 12, tempStr);

  // Baris 2: Gear dan Oil Temp
  char gearStr[12];
  snprintf(gearStr, sizeof(gearStr), "Gear: %s", gearName(gear));
  u8g2.drawStr(0, 28, gearStr);

  char oilStr[12];
  snprintf(oilStr, sizeof(oilStr), "Oil :%3dC", (int)oilTempVal);
  u8g2.drawStr(64, 28, oilStr);
// Baris 3 (Fuel)
  char fuelStr[16];
  snprintf(fuelStr, sizeof(fuelStr), "Fuel:%3.0f%%", fuelLevel);
  u8g2.drawStr(0, 44, fuelStr);

    char injStr[12];
  snprintf(injStr, sizeof(injStr), "Inj :%.2fL", fuelInjected);
  u8g2.drawStr(64, 44, injStr);

  // Baris 4: AC Pressure
 char acStr[16];
  snprintf(acStr, sizeof(acStr), "AC  : %.0f", acPressure);
  u8g2.drawStr(0, 60, acStr);

  char fanStr[12];
  snprintf(fanStr, sizeof(fanStr), "Fan:%3.0f%%", fanSpeed);
  u8g2.drawStr(70, 60, fanStr);

  u8g2.sendBuffer();
}

// === Animasi Startup ===
void animateStartup() {
  const int steps = 60;
  const int delayMs = 30;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / steps;
    float tempVal = t * 120.0;
    float rpmVal = t * 9000.0;
    float oilVal = t * 120.0;
    drawMainScreen(rpmVal, tempVal, oilVal);
    delay(delayMs);
  }
  rpm = 0;
  temperature = 0;
  transOilTemp = 0;
  drawMainScreen(rpm, temperature, transOilTemp);
}

// === Setup utama ===
void setup() {
  Serial.begin(115200);
  delay(1000);

  setupOLED();
  setupCAN();

  Serial.println("CAN Display started (with startup animation)");
  animateStartup();
}

// === Update tampilan ===
void updateOLED() {
  drawMainScreen(rpm, temperature, transOilTemp);
}

// === Loop utama ===
void loop() {
  readCANMessages();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 300) {
    updateOLED();
    lastUpdate = millis();
  }
  yield();
}
