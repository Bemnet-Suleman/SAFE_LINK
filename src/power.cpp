#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "pins.h"
#include "config.h"
#include "power.h"

volatile bool g_sosFlag=false, g_motionFlag=false;
volatile uint32_t g_sec=0;
static volatile uint16_t s_stall=0;
static bool s_motionArmed=true;

// Clear any watchdog reset flag as early as possible, otherwise the shortest watchdog timeout keeps resetting us.
#ifdef __AVR__
void wdtEarly(void) __attribute__((naked,used,section(".init3")));
#endif
void wdtEarly(void){ MCUSR=0; wdt_disable(); }

ISR(WDT_vect){
  g_sec++;
  if(++s_stall>WDT_STALL_LIMIT_S){      // main loop hung: force a hardware reset
    cli(); WDTCSR|=(1<<WDCE)|(1<<WDE); WDTCSR=(1<<WDE)|(1<<WDP0); while(1){}
  }
}
void wdtKick(){ s_stall=0; }
static void isrSos(){ detachInterrupt(digitalPinToInterrupt(PIN_SOS)); g_sosFlag=true; }
static void isrMotion(){ detachInterrupt(digitalPinToInterrupt(PIN_IMU_INT)); g_motionFlag=true; }

void powerInit(){
  cli(); wdt_reset(); MCUSR&=~(1<<WDRF);
  WDTCSR|=(1<<WDCE)|(1<<WDE); WDTCSR=(1<<WDIE)|(1<<WDP2)|(1<<WDP1);   // interrupt mode, 1 s period
  sei();
  power_spi_disable(); power_timer1_disable(); power_timer2_disable();
}
uint16_t batteryMv(){
  analogReference(INTERNAL); delay(10); analogRead(PIN_VBAT_SENS);
  uint32_t a=0; for(uint8_t i=0;i<8;i++){ a+=analogRead(PIN_VBAT_SENS); delay(2); }
  a/=8;
  analogReference(DEFAULT);
  return (uint16_t)((a*43338UL)/10000UL);        // 1.1 V ref, divider 330k/(1M+330k)
}
uint8_t batteryPercent(uint16_t mv){
  static const uint16_t V[7]={3300,3500,3650,3750,3850,4000,4200};
  static const uint8_t  P[7]={0,5,20,40,60,80,100};
  if(mv<=V[0]) return 0;
  if(mv>=V[6]) return 100;
  for(uint8_t i=1;i<7;i++) if(mv<=V[i]) return P[i-1]+(uint32_t)(mv-V[i-1])*(P[i]-P[i-1])/(V[i]-V[i-1]);
  return 100;
}
bool usbPresent(){
  analogReference(DEFAULT); delay(5); analogRead(PIN_VBUS_SENS);
  uint16_t a=analogRead(PIN_VBUS_SENS);
  return a>300;                                   // divider 120k/(100k+120k): 5 V reads about 846
}
void armMotionWake(bool enable){ s_motionArmed=enable; }
void sleepTick(){
  Serial.flush(); Wire.end();
  if(digitalRead(PIN_SOS)!=LOW) attachInterrupt(digitalPinToInterrupt(PIN_SOS),isrSos,LOW);
  else g_sosFlag=true;
  if(s_motionArmed) attachInterrupt(digitalPinToInterrupt(PIN_IMU_INT),isrMotion,LOW);
  if(g_sosFlag||g_motionFlag) return;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA&=~(1<<ADEN); power_adc_disable();
  noInterrupts(); sleep_enable(); sleep_bod_disable(); interrupts(); sleep_cpu(); sleep_disable();
  power_adc_enable(); ADCSRA|=(1<<ADEN);
}
void softReset(){ cli(); WDTCSR|=(1<<WDCE)|(1<<WDE); WDTCSR=(1<<WDE)|(1<<WDP0); while(1){} }
