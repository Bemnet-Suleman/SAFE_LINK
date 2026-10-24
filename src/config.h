// SAFE_LINK firmware configuration - edit this file per device / per deployment.
#pragma once
#include <stdint.h>

#define FW_VERSION      "0.2.0"
#define DEVICE_ID       "WG-023"                 // unique per wearable (matches the dashboard's device ID)

// ---- cellular / server (SIM800L is 2G only: plain HTTP to YOUR ingestion server, payload is HMAC-signed) ----
#define APN             "ethiotelecom.et"        // check with your SIM (Safaricom Ethiopia: "safaricom")
#define SERVER_HOST     "your-server.example.com"
#define SERVER_PATH     "/api/device/ingest"
#define SERVER_URL      "http://" SERVER_HOST SERVER_PATH

// 32-byte HMAC-SHA256 key, different for every device. Generate with: openssl rand -hex 32
#define HMAC_KEY_BYTES { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, \
                         0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f }

// ---- reporting ----
#define REPORT_MOVING_S     60UL      // report interval while MOVING
#define REPORT_STILL_S      300UL     // report interval while STATIONARY (also the heartbeat)
#define GPS_TIMEOUT_S       45        // give up on a GPS fix after this long
#define GPS_MIN_SATS        4
#define SOS_ACTIVE_MIN      15        // keep sending SOS-flagged reports for this long after an SOS
#define SOS_REPORT_S        60UL
#define SEND_RETRY_GAP_S    5
#define SOS_RETRY_MAX_S     600UL     // keep retrying a failed SOS for up to 10 minutes

// ---- sensing ----
#define ACTIVITY_POLL_S     5
#define MOVING_MAD_MG       90        // mean absolute deviation of |a| above this => MOVING
#define INACTIVITY_S        900UL     // STATIONARY this long => PROLONGED_INACTIVITY (tune per activity)
#define MOT_THR_32MG        62        // MPU6050 impact threshold, 32 mg per LSB (62 = about 2 g)
#define FALL_ORIENT_COS_X100 50       // cos(60 deg) = 0.50: orientation change needed
#define FALL_SETTLE_MS      2000
#define FALL_STILL_S        10
#define FALL_STILL_MAD_MG   80
#define HR_PERIOD_S         300UL
#define HR_WINDOW_S         10
#define IR_SKIN_MIN         20000UL   // MAX30102 IR level that means "on skin" (calibrate on your board)

// ---- power ----
#define LOW_BATT_MV         3650
#define CRIT_BATT_MV        3450      // SIM800L browns out below about 3.4 V
#define SOS_HOLD_MS         1500
#define STRAP_DEBOUNCE_S    3
#define WDT_STALL_LIMIT_S   180       // main loop must call wdtKick() at least this often
