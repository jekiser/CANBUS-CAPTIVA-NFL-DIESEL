# CANBUS-CAPTIVA-NFL-DIESEL 2008
Proyek ini menampilkan data CAN bus (RPM, suhu, gear, fuel, AC pressure, fan speed, dll) dari kendaraan ke layar **OLED 128x64 SH1106** menggunakan **ESP32** dan **TWAI driver (CAN)** bawaan.

---

## 🔧 Fitur
- Baca data dari beberapa CAN ID:
  - RPM (0x0C9)
  - Coolant Temp (0x4C1)
  - Gear Position (0x1F5)
  - Transmission Oil Temp (0x4C9)
  - Fuel Level (0x4D1)
  - Fuel Injected, AC Pressure, Fan Speed (0x3F9)
- Tampilkan di layar OLED SH1106 128x64 (U8G2)
- Startup animation
- Update layar setiap 300 ms

---

## ⚙️ Hardware
| Komponen | Keterangan |
|-----------|-------------|
| **ESP32** | Modul utama |
| **CAN Transceiver** | SN65HVD230 / TJA1050 |
| **OLED 128x64 SH1106** | I2C interface |
| **CAN TX Pin** | GPIO 5 |
| **CAN RX Pin** | GPIO 4 |

---

## 🧰 Library yang digunakan
- `U8g2` (OLED Display)
- `driver/twai.h` (CAN interface bawaan ESP32)
- `Wire.h`
- `algorithm` (untuk `std::max`)
