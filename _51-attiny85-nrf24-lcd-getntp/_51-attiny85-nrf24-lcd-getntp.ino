// https://github.com/PaulStoffregen/Time
#include <TimeLib.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
// https://tmrh20.github.io/RF24/
#include <nRF24L01.h>
#include <RF24.h>
// https://github.com/adafruit/TinyWireM
#include "USI_TWI_Master.h"
#include <TinyWireM.h>
// https://github.com/chaeplin/LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>

// If CEN_PIN is moved to D5, D3 is available.
#define CE_PIN  5
//#define CE_PIN  3
#define CSN_PIN 4

#define DEVICE_ID 25
#define CHANNEL 100

const uint64_t pipes[1] = { 0xFFFFFFFFCDLL };

struct {
  uint32_t timestamp;
} time_ackpayload;

struct {
  uint32_t timestamp;
} time_reqpayload;

struct {
  uint16_t powerAvg;
  uint16_t WeightAvg;
  uint16_t Humidity;
  uint16_t data1;
  uint8_t  data2;
  uint8_t  data3;
  int8_t   Temperature1;
  int8_t   Temperature2;
} solar_ackpayload;


struct {
  uint8_t data1;
} solar_reqpayload;

RF24 radio(CE_PIN, CSN_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  time_reqpayload.timestamp = 0;
  //
  time_ackpayload.timestamp = 0;
  //
  solar_reqpayload.data1    = 0;
  //
  solar_ackpayload.data1        = 0;
  solar_ackpayload.data2        = 0;
  solar_ackpayload.data3        = 0;
  solar_ackpayload.powerAvg     = 0;
  solar_ackpayload.WeightAvg    = 0;
  solar_ackpayload.Humidity     = 0;
  solar_ackpayload.Temperature1 = 0;
  solar_ackpayload.Temperature2 = 0;

  //
  radio.begin();
  radio.setChannel(CHANNEL);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(15, 15);
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.openWritingPipe(pipes[0]);
  radio.stopListening();

  TinyWireM.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  setSyncProvider(getNrfTime);
}

time_t prevDisplay = 0;

void loop() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();

      //if ( second() % 5 == 0 ) {
      //  getnrfsolar();
      //}

      USICR =  (1 << USIWM1) | (0 << USIWM0);
      digitalClockDisplay();
    }
  }
}


void digitalClockDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("[");
  lcd.setCursor(1, 0);
  printDigitsnocolon(month());
  lcd.print("/");
  printDigitsnocolon(day());

  lcd.setCursor(7, 0);
  lcd.print(dayShortStr(weekday()));
  lcd.setCursor(11, 0);
  printDigitsnocolon(hour());
  printDigits(minute());
  printDigits(second());
  lcd.setCursor(19, 0);
  lcd.print("]");
}

void printDigitsnocolon(int digits) {
  if (digits < 10) {
    lcd.print('0');
  }
  lcd.print(digits);
}

void printDigits(int digits) {
  lcd.print(":");
  if (digits < 10) {
    lcd.print('0');
  }
  lcd.print(digits);
}

void getnrfsolar() {
  USICR =  (1 << USIWM0) | (1 << USICS1) | (1 << USICLK) | (1 << USITC);
  SPI.begin();
  radio.write(&solar_reqpayload , sizeof(uint8_t));
  if (radio.isAckPayloadAvailable()) {
    uint8_t len = radio.getDynamicPayloadSize();
    if ( len == sizeof(solar_ackpayload)) {
      radio.read(&solar_ackpayload, sizeof(solar_ackpayload));
    }
  }
}

time_t getNrfTime() {
  USICR =  (1 << USIWM0) | (1 << USICS1) | (1 << USICLK) | (1 << USITC);
  SPI.begin();

  uint32_t beginWait = millis();
  while (millis() - beginWait < 2000) {
    radio.write(&time_reqpayload , sizeof(time_reqpayload));
    if (radio.isAckPayloadAvailable()) {
      uint8_t len = radio.getDynamicPayloadSize();
      if ( len == sizeof(time_ackpayload)) {
        radio.read(&time_ackpayload, sizeof(time_ackpayload));
      }
    }

    radio.write(&time_reqpayload , sizeof(time_reqpayload));
    if (radio.isAckPayloadAvailable()) {
      uint8_t len = radio.getDynamicPayloadSize();
      if ( len == sizeof(time_ackpayload)) {
        radio.read(&time_ackpayload, sizeof(time_ackpayload));
      }
    }
    
    radio.write(&time_reqpayload , sizeof(time_reqpayload));
    if (radio.isAckPayloadAvailable()) {
      uint8_t len = radio.getDynamicPayloadSize();
      if ( len == sizeof(time_ackpayload)) {
        radio.read(&time_ackpayload, sizeof(time_ackpayload));
        return (unsigned long)time_ackpayload.timestamp;
      }
    }
  }
  return 0;
}

// end
