// SAFE_LINK wearable firmware - ATmega328P, 8 MHz, 3.3 V
// Reports state to the backend over SIM800L (GPRS, plain HTTP with an HMAC-signed JSON body).
#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "types.h"
#include "power.h"
#include "imu.h"
#include "hr.h"
#include "gps.h"
#include "modem.h"
#include "store.h"
#include "report.h"
#include "debug.h"

static uint8_t g_act=ACT_UNKNOWN, g_hr=0, g_hq=0;
static uint32_t lastReport=0,lastAct=0,lastHr=0,lastMotion=0,lastBatt=0,lastTick=0xFFFFFFFFUL;
static uint32_t strapOpenSince=0,sosUntil=0,lastSosReport=0,motionRearmAt=0;
static bool strapClosed=true,strapOpenReported=false,inactivityReported=false,lowBattReported=false,modemUp=false,motionArmed=true;
static uint8_t failStreak=0;
static char payload[256];

// ---------- local feedback ----------
static void beep(uint16_t ms){ digitalWrite(PIN_BUZ,HIGH); delay(ms); digitalWrite(PIN_BUZ,LOW); }
static void vib(uint16_t ms){ digitalWrite(PIN_VIB,HIGH); delay(ms); digitalWrite(PIN_VIB,LOW); }
static void led(uint16_t ms){ digitalWrite(PIN_LED,HIGH); delay(ms); digitalWrite(PIN_LED,LOW); }

// ---------- telemetry ----------
static void fillTelemetry(Telemetry& t,uint8_t flags){
  t.mv=batteryMv(); t.pct=batteryPercent(t.mv); t.usb=usbPresent();
  if(t.mv<LOW_BATT_MV && !t.usb) flags|=FLAG_LOW_BATT;
  if(t.mv<CRIT_BATT_MV && !t.usb) flags|=FLAG_CRIT_BATT;
  t.flags=flags; t.fix=g_lastFix; t.hr=g_hr; t.hq=g_hq; t.act=g_act;
  t.strapClosed=strapClosed; t.rssi=modemRssi(); t.upSec=g_sec;
}
static void pushEvent(const Telemetry& t){
  EvRec r; r.sec=g_sec; r.lat=t.fix.lat_e6; r.lon=t.fix.lon_e6; r.mv=t.mv; r.flags=t.flags; r.hr=t.hr; r.act=t.act;
  r.sats=t.fix.sats; r.fixValid=t.fix.valid?1:0; r.pct=t.pct; storePush(r);
}
static void fromRec(Telemetry& t,const EvRec& r){
  t.flags=r.flags|FLAG_LATE; t.fix.valid=r.fixValid; t.fix.lat_e6=r.lat; t.fix.lon_e6=r.lon; t.fix.sats=r.sats; t.fix.hdop_x10=0; t.fix.sec=r.sec;
  t.mv=r.mv; t.pct=r.pct; t.hr=r.hr; t.hq=0; t.act=r.act; t.strapClosed=strapClosed; t.usb=false; t.rssi=-1; t.upSec=r.sec;
}

// ---------- delivery ----------
static bool deliver(const Telemetry& t,uint8_t attempts){
  if(!modemUp) modemUp=modemInit();
  uint32_t seq=storeNextSeq();                        // same sequence number on every retry: the server de-duplicates
  uint16_t len=buildPayload(payload,sizeof(payload),t,seq);
  if(!len) return false;
  for(uint8_t i=0;i<attempts;i++){
    wdtKick(); uint16_t st=0;
    if(modemHttpPost(payload,len,&st) && st==200){ failStreak=0; return true; }   // only HTTP 200 counts as an acknowledgment
    DBGN("post fail status ",st);
    if(++failStreak>=3){ modemReset(); modemUp=modemInit(); failStreak=0; }
    if(i+1<attempts) for(uint8_t s=0;s<SEND_RETRY_GAP_S*10;s++){ wdtKick(); delay(100); }
  }
  return false;
}
// sends stored events oldest first; stops at the first failure
static bool drainQueue(uint8_t attemptsEach,uint8_t maxItems){
  EvRec r; uint8_t n=0;
  while(n<maxItems && storePeek(r)){
    Telemetry t; fromRec(t,r);
    if(!deliver(t,attemptsEach)) return false;
    storePop(); n++;
  }
  return true;
}
static bool doReport(uint8_t flags,bool refreshGps){
  wdtKick();
  if(refreshGps){ GpsFix f; if(gpsGetFix(GPS_TIMEOUT_S,f)) g_lastFix=f; }
  Telemetry t; fillTelemetry(t,flags);
  bool ok=deliver(t,3);
  lastReport=g_sec; led(30);
  return ok;
}

// ---------- SOS ----------
static void handleSos(){
  if(!g_sosFlag && digitalRead(PIN_SOS)!=LOW) return;
  g_sosFlag=false;
  uint32_t t0=millis();
  while(digitalRead(PIN_SOS)==LOW && millis()-t0<SOS_HOLD_MS) wdtKick();
  if(digitalRead(PIN_SOS)!=LOW) return;               // released before the hold time: ignored (accidental press)
  vib(300); beep(100);                                // press registered
  Telemetry t; fillTelemetry(t,FLAG_SOS);
  pushEvent(t);                                       // survives a reset or a dead network
  sosUntil=g_sec+SOS_ACTIVE_MIN*60UL; lastSosReport=g_sec;
  uint32_t start=g_sec; bool ok=false;
  while(!ok && g_sec-start<SOS_RETRY_MAX_S){          // retry until acknowledged (HTTP 200)
    ok=drainQueue(1,storeCount());
    if(!ok){ led(60); if(((g_sec-start)%30)<SEND_RETRY_GAP_S) beep(40); for(uint8_t s=0;s<20;s++){ wdtKick(); delay(100); } }
  }
  if(ok){ vib(150); delay(120); vib(150); beep(150); } // acknowledged
}

// ---------- fall / strap / battery ----------
static void onImpact(){
  g_motionFlag=false;
  if(imuFallCheck()){
    Telemetry t; fillTelemetry(t,FLAG_FALL); pushEvent(t);
    drainQueue(2,storeCount());
  }
  armMotionWake(false); motionArmed=false; motionRearmAt=g_sec+10;   // ignore further impacts for 10 s
}
static void pollStrap(){
  bool closed=(digitalRead(PIN_INTEG)==LOW);
  if(closed){
    if(!strapClosed){ strapClosed=true; strapOpenReported=false; doReport(0,false); }   // closed again: clears the alert
  }else{
    if(strapClosed){ strapClosed=false; strapOpenSince=g_sec; }
    else if(!strapOpenReported && g_sec-strapOpenSince>=STRAP_DEBOUNCE_S && !usbPresent()){   // charging is not an alert
      strapOpenReported=true;
      Telemetry t; fillTelemetry(t,FLAG_STRAP_OPEN); pushEvent(t); drainQueue(2,storeCount());
    }
  }
}
static void criticalShutdown(){
  doReport(FLAG_CRIT_BATT|FLAG_LOW_BATT,false);
  modemPowerDown(); digitalWrite(PIN_GPS_EN,LOW); armMotionWake(false);
  while(!usbPresent()){ g_sosFlag=false; g_motionFlag=false; wdtKick(); sleepTick(); }   // protect the cell until it is charging
  softReset();
}

// ---------- once per second ----------
static void secondTasks(){
  if(lastBatt==0 || g_sec-lastBatt>=30){
    lastBatt=g_sec; uint16_t mv=batteryMv(); bool usb=usbPresent();
    if(mv<CRIT_BATT_MV && !usb) criticalShutdown();
    else if(mv<LOW_BATT_MV && !usb && !lowBattReported){ lowBattReported=true; doReport(FLAG_LOW_BATT,false); }
    else if(mv>LOW_BATT_MV+100) lowBattReported=false;
  }
  if(g_sec-lastAct>=ACTIVITY_POLL_S){
    lastAct=g_sec;
    if(imuOk()){
      g_act=imuClassify();
      if(g_act==ACT_MOVING){ lastMotion=g_sec; inactivityReported=false; }
      else if(g_act==ACT_STILL && !inactivityReported && g_sec-lastMotion>=INACTIVITY_S){ inactivityReported=true; doReport(FLAG_INACTIVITY,false); }
    }
  }
  if(hrOk() && strapClosed && g_sec-lastHr>=HR_PERIOD_S){
    lastHr=g_sec; HrResult r=hrMeasure(HR_WINDOW_S); g_hr=r.bpm; g_hq=r.quality;
  }
  if(g_sec<sosUntil && g_sec-lastSosReport>=SOS_REPORT_S){ lastSosReport=g_sec; doReport(FLAG_SOS,true); }
  uint32_t iv=(g_act==ACT_MOVING)?REPORT_MOVING_S:REPORT_STILL_S;
  if(g_sec-lastReport>=iv) doReport(0,true);
  if(storeCount()>0) drainQueue(1,2);                  // backlog from earlier failures
}

void setup(){
  pinMode(PIN_GPS_EN,OUTPUT); digitalWrite(PIN_GPS_EN,LOW);         // GPS off first
  pinMode(PIN_VIB,OUTPUT); pinMode(PIN_BUZ,OUTPUT); pinMode(PIN_LED,OUTPUT);
  digitalWrite(PIN_VIB,LOW); digitalWrite(PIN_BUZ,LOW); digitalWrite(PIN_LED,LOW);
  pinMode(PIN_SOS,INPUT_PULLUP); pinMode(PIN_INTEG,INPUT);           // strap loop has a 1 Mohm pull-up on the board
  pinMode(PIN_IMU_INT,INPUT); pinMode(PIN_HR_INT,INPUT); pinMode(PIN_SIM_RST,INPUT);
  powerInit(); dbgBegin(); storeInit(); modemBegin();
  led(200);
  imuInit(); hrInit();
  modemUp=modemInit();
  strapClosed=(digitalRead(PIN_INTEG)==LOW); lastMotion=g_sec; lastHr=g_sec+20-HR_PERIOD_S;   // first heart-rate window after 20 s
  doReport(FLAG_BOOT,true);
}
void loop(){
  wdtKick();
  handleSos();
  pollStrap();
  if(g_motionFlag) onImpact();
  if(!motionArmed && g_sec>=motionRearmAt){ imuClearInt(); armMotionWake(true); motionArmed=true; }
  if(g_sec!=lastTick){ lastTick=g_sec; secondTasks(); }
  if(!g_sosFlag && !g_motionFlag) sleepTick();                     // power-down until the next 1 s tick or an interrupt
}
