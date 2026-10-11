// SAFE_LINK wearable - firmware skeleton (NOT compiled or tested on hardware).
// Goal: bring subsystems up one at a time (see bring-up plan), then grow this into the state machine.
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "pins.h"

SoftwareSerial gpsSerial(PIN_GPS_RX, 255);   // RX only
SoftwareSerial dbg(255, PIN_DBG_TX);         // TX only

enum State : uint8_t { ST_SAFE, ST_WARNING, ST_CRITICAL };
volatile bool sosFlag = false, motionFlag = false;
struct Telemetry { float lat, lon; bool fix; uint16_t vbat_mV; bool worn; bool usb; uint8_t activity; };

void isrSOS()    { sosFlag = true; }
void isrMotion() { motionFlag = true; }

// ---------- helpers ----------
uint16_t readVbat_mV() {                       // 1.1 V reference, divider 0.248
  analogReference(INTERNAL); delay(5); analogRead(PIN_VBAT_SENS);
  uint32_t a = 0; for (uint8_t i = 0; i < 8; i++) a += analogRead(PIN_VBAT_SENS);
  return (uint16_t)((a / 8) * 1100UL / 1023UL * 1000UL / 248UL);
}
bool strapClosed() { return digitalRead(PIN_INTEG) == LOW; }
void gpsPower(bool on) { digitalWrite(PIN_GPS_EN, on ? HIGH : LOW); }   // active HIGH (NPN+PNP switch)
void simReset() { pinMode(PIN_SIM_RST, OUTPUT); digitalWrite(PIN_SIM_RST, LOW); delay(200); pinMode(PIN_SIM_RST, INPUT); }
void buzz(uint16_t ms) { digitalWrite(PIN_BUZ, HIGH); delay(ms); digitalWrite(PIN_BUZ, LOW); }
void vib(uint16_t ms)  { digitalWrite(PIN_VIB, HIGH); delay(ms); digitalWrite(PIN_VIB, LOW); }

// ---------- SIM800L (hardware UART) ----------
bool at(const char* cmd, const char* expect, uint16_t timeout_ms = 2000) {
  while (Serial.available()) Serial.read();
  Serial.println(cmd);
  uint32_t t = millis(); String r;
  while (millis() - t < timeout_ms) { while (Serial.available()) r += (char)Serial.read(); if (r.indexOf(expect) >= 0) return true; }
  return false;
}
bool modemInit() {
  for (uint8_t i = 0; i < 5; i++) if (at("AT", "OK", 500)) break; else delay(300);   // first bytes wake it from sleep
  return at("ATE0", "OK") && at("AT+IPR=9600", "OK") && at("AT+CSCLK=2", "OK")      // auto-sleep after 5 s UART idle
      && at("AT+CPIN?", "READY") && at("AT+CREG?", ",1") ;                          // ",1"/",5" = registered
}
// Send one report over plain HTTP to YOUR ingestion server (SIM800L TLS 1.2 to Supabase is unreliable).
// Body is signed with HMAC (see docs) because the transport is not TLS.
bool sendReport(const Telemetry& t, bool sos) {
  // TODO: AT+SAPBR=3,1,"Contype","GPRS" ; AT+SAPBR=3,1,"APN","<your APN>" ; AT+SAPBR=1,1
  //       AT+HTTPINIT ; AT+HTTPPARA="CID",1 ; AT+HTTPPARA="URL","http://<server>/api/ingest"
  //       AT+HTTPDATA=<len>,5000 ; <body> ; AT+HTTPACTION=1 ; expect +HTTPACTION: 1,200,...
  //       Retry until the server ACK (HTTP 200) for SOS. Keep the unsent event in EEPROM.
  return false;
}

// ---------- setup / loop ----------
void setup() {
  pinMode(PIN_GPS_EN, OUTPUT); gpsPower(false);              // set before anything else: GPS off
  pinMode(PIN_VIB, OUTPUT); pinMode(PIN_BUZ, OUTPUT); pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SOS, INPUT_PULLUP); pinMode(PIN_INTEG, INPUT); // external 1 Mohm pull-up on INTEG
  pinMode(PIN_SIM_RST, INPUT); pinMode(PIN_IMU_INT, INPUT); pinMode(PIN_HR_INT, INPUT);
  Serial.begin(9600); dbg.begin(9600); gpsSerial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(PIN_SOS), isrSOS, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_IMU_INT), isrMotion, RISING);
  wdt_enable(WDTO_8S);
  // Bring-up order: 1) LED blink  2) Vbat read  3) I2C scan (0x68, 0x57)  4) modem AT  5) GPS NMEA  6) buzzer/vibration  7) SOS path
  digitalWrite(PIN_LED, HIGH); delay(200); digitalWrite(PIN_LED, LOW);
}

void loop() {
  wdt_reset();
  if (sosFlag) {                               // SOS: long-press confirm (1-2 s), then retry until server ACK
    sosFlag = false; uint32_t t0 = millis();
    while (digitalRead(PIN_SOS) == LOW && millis() - t0 < 2000) wdt_reset();
    if (millis() - t0 >= 1500) { vib(300); /* gather last fix, build packet, sendReport(t, true) until ACK, then buzz+vib */ }
  }
  // Periodic: every N s -> read IMU activity, strap, Vbat; power GPS (hot start), read until fix (<=60 s), power off; send report.
  // Alert rules live on the SERVER (thresholds editable mid-event); the device only reports facts + local SOS/fall flags.
  // Sleep: SLEEP_MODE_PWR_DOWN, wake on INT1 (SOS), INT0 (IMU motion), WDT tick.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); sleep_enable(); sleep_cpu(); sleep_disable();
}
