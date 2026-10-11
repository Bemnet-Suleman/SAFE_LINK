// SAFE_LINK THT carrier - pin map (ATmega328P DIP-28, 8 MHz crystal, 3.3 V)
#pragma once
#define F_CPU_HZ        8000000UL   // 16 MHz is out of spec at 3.3 V
#define PIN_SIM_RXD_MCU 0   // PD0 (pin 2)  <- SIM800L TXD
#define PIN_SIM_TXD_MCU 1   // PD1 (pin 3)  -> 1k/2.2k divider -> SIM800L RXD
#define PIN_IMU_INT     2   // PD2 (pin 4)  INT0
#define PIN_SOS         3   // PD3 (pin 5)  INT1, active LOW
#define PIN_SIM_RST     4   // PD4 (pin 6)  OUTPUT LOW = reset, INPUT = released. NEVER drive HIGH
#define PIN_GPS_RX      5   // PD5 (pin 11) SoftwareSerial RX <- GPS TXD
#define PIN_GPS_EN      6   // PD6 (pin 12) HIGH = GPS ON (NPN+PNP switch on VBAT), LOW = off
#define PIN_HR_INT      7   // PD7 (pin 13) MAX30102 INT (optional)
#define PIN_VIB         8   // PB0 (pin 14)
#define PIN_BUZ         9   // PB1 (pin 15)
#define PIN_LED         10  // PB2 (pin 16)
// PB3/PB4/PB5 = D11/D12/D13 = ISP (pins 17/18/19)
#define PIN_VBAT_SENS   A0  // PC0 (pin 23)  VBAT*0.248, use 1.1 V reference
#define PIN_VBUS_SENS   A1  // PC1 (pin 24)  VBUS*0.545, use AVCC reference (USB present if > ~1.5 V)
#define PIN_INTEG       A2  // PC2 (pin 25)  LOW = strap closed, HIGH = removed (1 Mohm pull-up)
#define PIN_DBG_TX      A3  // PC3 (pin 26)  SoftwareSerial TX debug
// PC4/PC5 = I2C SDA/SCL (pins 27/28): MPU6050 0x68, MAX30102 0x57
