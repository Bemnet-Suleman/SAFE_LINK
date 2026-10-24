# SAFE_LINK firmware (v0.2.0)

ATmega328P at 8 MHz / 3.3 V, PlatformIO + Arduino core. No external libraries (Wire and SoftwareSerial are part of the core).

## Files

| File | Purpose |
|---|---|
| `platformio.ini` (repo root) | board, USBasp upload, fuse values; env `safelink_debug` prints debug text on TP1 |
| `src/config.h` | **edit this first**: device ID, APN, server host, HMAC key, intervals, thresholds |
| `src/pins.h` | pin map of the carrier board |
| `src/main.cpp` | state machine: SOS, fall, strap, battery, reports, retries, sleep |
| `src/power.*` | watchdog tick (1 s), battery and USB sensing, sleep, wake interrupts |
| `src/imu.*` | MPU6050: impact interrupt, MOVING/STILL, fall check |
| `src/hr.*` | MAX30102: heart rate, on-skin check |
| `src/gps.*` | NEO-6M power switch, NMEA GGA parser, fix wait |
| `src/modem.*` | SIM800L AT driver: register, bearer, HTTP POST |
| `src/store.*` | EEPROM: report sequence counter and queue of unsent events |
| `src/report.*`, `src/sha256.*` | JSON report + HMAC-SHA256 signature |
| `tools/mock_ingest_server.js` | bench server: checks the signature, prints a decoded line, answers 200 |
| `test/host/test_host.cpp`, `test/stubs/` | PC tests for the logic (see below) |

## Build and flash

```bash
pio run                  # build
pio run -t upload        # flash through USBasp (jumper at 3.3 V!)
pio run -e safelink_debug -t upload   # with debug text on TP1 (9600 baud)
```
Fuses (set by `pio run -t fuses` or avrdude): `lfuse=0xFF hfuse=0xD9 efuse=0xFD` (external 8 MHz crystal, no clock divide, brown-out 2.7 V). Check them in a fuse calculator before burning.

## Before the first flash

1. `config.h`: set `DEVICE_ID`, `APN`, `SERVER_HOST`/`SERVER_PATH`, and a **new 32-byte `HMAC_KEY_BYTES`** per device (`openssl rand -hex 32`). Put the same key (hex) on the server.
2. The SIM800L cannot do modern TLS, so the device speaks **plain HTTP**. The body is signed, so the server can trust it even though the channel is not encrypted. Do not put secrets in the body.

## Report format

One JSON object per HTTP POST (`Content-Type: application/json`), ending with a signature. Every field is an integer unless noted.

| Key | Meaning |
|---|---|
| `id` | device ID (string) |
| `n` | report sequence number, strictly increasing, survives resets. The server must ignore `n` less than or equal to the last one seen per device (replay protection) and may answer 200 to duplicates |
| `up` | seconds since the device booted |
| `f` | event flags: 1 SOS, 2 possible FALL, 4 prolonged INACTIVITY, 8 STRAP_OPEN, 16 LOW_BATT, 32 CRIT_BATT, 64 BOOT, 128 LATE (resent from the queue) |
| `fx` | 1 if a GPS fix is included |
| `la`, `lo` | latitude, longitude in millionths of a degree (divide by 1e6); last known fix |
| `fa` | age of that fix in seconds |
| `sa` | satellites used |
| `mv`, `bp` | battery millivolts, percent (estimate from the open-circuit curve) |
| `hr`, `hq` | heart rate in bpm (0 = none) and quality 0 none, 1 poor, 2 good. Supporting signal only |
| `ac` | activity: 0 unknown, 1 still, 2 moving |
| `sw` | 1 strap closed, 0 strap open |
| `us` | 1 if USB power is present (charging) |
| `rs` | GSM signal 0 to 31, -1 unknown |
| `fw` | firmware version (string) |
| `sig` | first 16 bytes (32 hex characters) of HMAC-SHA256 over every byte before `,"sig":"` |

Verifying on the server (Node): split the body at `,"sig":"`, compute `crypto.createHmac('sha256', key).update(prefix).digest('hex').slice(0,32)` and compare. `tools/mock_ingest_server.js` does exactly this.

Example:
```
{"id":"WG-023","n":42,"up":1234,"f":1,"fx":1,"la":9035700,"lo":38765100,"fa":234,"sa":9,"mv":3987,"bp":71,"hr":78,"hq":2,"ac":2,"sw":1,"us":0,"rs":17,"fw":"0.2.0","sig":"468081898ce14250452727084ca37654"}
```
Mapping to the dashboard states: SAFE, WARNING and CRITICAL are decided **on the server** from these fields (geofence, separation, inactivity thresholds stay editable mid-event). A missing heartbeat is UNKNOWN, never SAFE. The device sends a report at least every 5 minutes (60 s while moving).

## Behaviour

- **SOS**: hold the button 1.5 s (vibration + beep). The event is written to EEPROM, then sent until the server answers HTTP 200 (up to 10 minutes of retries, modem reset after 3 failures). A second vibration + beep means acknowledged. SOS-flagged reports with a fresh GPS fix follow every 60 s for 15 minutes.
- **Fall**: an impact above about 2 g wakes the MCU. If gravity then changes direction by 60 degrees or more and the wearer stays still for 10 s, a FALL event is stored and sent. Always "possible", never confirmed.
- **Inactivity**: STILL for 15 minutes (`INACTIVITY_S`) raises one INACTIVITY event per episode.
- **Strap**: opening the strap loop for 3 s raises STRAP_OPEN (ignored while USB power is present). Closing it sends a report that clears the alert.
- **Battery**: LOW_BATT at 3.65 V, and below 3.45 V a final report is sent, the modem is powered down and the device sleeps until USB is plugged in.
- **Offline**: critical events are queued in EEPROM (20 records) and sent oldest first when the network returns.
- **Watchdog**: a 1 s watchdog tick keeps time while asleep; if the main loop stalls for 3 minutes the device resets.
- Heart rate is measured every 5 minutes for 10 s while the strap is closed. Sleep current is dominated by the SIM800L in auto-sleep (`AT+CSCLK=2`).

## Tests on a PC (no hardware)

```bash
g++ -std=gnu++11 -Itest/stubs -Isrc test/host/test_host.cpp src/sha256.cpp src/gps.cpp src/report.cpp src/store.cpp -o /tmp/safelink_tests && /tmp/safelink_tests
```
Covers SHA-256 and HMAC-SHA256 against the standard test vectors, the NMEA parser (including Addis Ababa coordinates, south/west, corrupted checksum), the signed payload, and the EEPROM queue and sequence counter. The same signed payload is accepted by `tools/mock_ingest_server.js` and a tampered one is rejected.

## Bring-up order on the real board

1. Power the board from a bench supply or battery without modules; check 3V3, then flash. The LED blinks once at boot.
2. Plug in the IMU and HR modules. Debug build: confirm no init failures (`imuInit`, `hrInit` return false when the chip does not answer).
3. SIM800L alone: check `AT`, `AT+CPIN?`, `AT+CREG?`, `AT+CSQ` from a USB-serial adapter first, then let the firmware register.
4. Run `node tools/mock_ingest_server.js` on a machine reachable from the internet (or a tunnel), set `SERVER_HOST`, and watch reports arrive.
5. GPS: take the board outside; first fix can take a minute on a cold start.
6. Tune on real data: `MOVING_MAD_MG`, `MOT_THR_32MG`, `FALL_*`, `IR_SKIN_MIN`, battery divider constant in `batteryMv()`.

## Not verified on hardware

This firmware has been compiled for syntax against host stubs and its logic tested on a PC, but it has **not** been built with the AVR toolchain or run on the board. Expect to adjust:

- SIM800L firmware versions differ in AT behaviour (bearer and HTTP commands, response text).
- MPU6050 motion threshold units, interrupt polarity and the effect of the high-pass filter on readings.
- MAX30102 beat detection thresholds and the on-skin level (`IR_SKIN_MIN`).
- SoftwareSerial (GPS) sharing the CPU with the hardware UART (modem): the firmware never uses them at the same time.
- Flash and RAM use (the target is under 20 KB flash and under 1.2 KB RAM).
