/*
  ChronoLatch

  Relay: D2
  DS3231: SDA=A4, SCL=A5 on Arduino Nano/Uno

  Install RTClib from the Arduino Library Manager.

  Serial commands at 9600 baud, newline enabled:
    TIME
    SETTIME 2026-07-03 18:30:00
    OPEN
    SETOPEN 07:00
    PULSE
*/

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

const byte RELAY_PIN = 2;
const bool RELAY_ACTIVE_LOW = true;
const unsigned long RELAY_MS = 3000UL;
const unsigned long I2C_TIMEOUT_US = 25000UL;
const unsigned long BOOT_SERIAL_WINDOW_MS = 30000UL;
const unsigned long WAKE_SERIAL_WINDOW_MS = 2000UL;
const unsigned long SERIAL_COMMAND_WINDOW_MS = 30000UL;

const int EEPROM_MAGIC = 0;
const int EEPROM_OPEN_HOUR = 1;
const int EEPROM_OPEN_MINUTE = 2;
const int EEPROM_LAST_YEAR = 3;
const int EEPROM_LAST_MONTH = 4;
const int EEPROM_LAST_DAY = 5;
const byte MAGIC = 0x42;

RTC_DS3231 rtc;
byte openHour = 7;
byte openMinute = 0;
unsigned long stayAwakeUntil = 0;
volatile bool watchdogWake = false;
bool rtcReady = false;

ISR(WDT_vect) {
  watchdogWake = true;
}

byte relayOnLevel() {
  return RELAY_ACTIVE_LOW ? LOW : HIGH;
}

byte relayOffLevel() {
  return RELAY_ACTIVE_LOW ? HIGH : LOW;
}

void twoDigits(int value) {
  if (value < 10) Serial.print('0');
  Serial.print(value);
}

void printTime(const DateTime &time) {
  Serial.print(time.year());
  Serial.print('-');
  twoDigits(time.month());
  Serial.print('-');
  twoDigits(time.day());
  Serial.print(' ');
  twoDigits(time.hour());
  Serial.print(':');
  twoDigits(time.minute());
  Serial.print(':');
  twoDigits(time.second());
  Serial.println();
}

void loadSettings() {
  if (EEPROM.read(EEPROM_MAGIC) == MAGIC) {
    openHour = EEPROM.read(EEPROM_OPEN_HOUR);
    openMinute = EEPROM.read(EEPROM_OPEN_MINUTE);
  }

  if (openHour > 23 || openMinute > 59) {
    openHour = 7;
    openMinute = 0;
  }
}

void saveOpenTime() {
  EEPROM.update(EEPROM_MAGIC, MAGIC);
  EEPROM.update(EEPROM_OPEN_HOUR, openHour);
  EEPROM.update(EEPROM_OPEN_MINUTE, openMinute);
}

bool openedToday(const DateTime &now) {
  return EEPROM.read(EEPROM_LAST_YEAR) == now.year() - 2000 &&
         EEPROM.read(EEPROM_LAST_MONTH) == now.month() &&
         EEPROM.read(EEPROM_LAST_DAY) == now.day();
}

void markOpenedToday(const DateTime &now) {
  EEPROM.update(EEPROM_LAST_YEAR, now.year() - 2000);
  EEPROM.update(EEPROM_LAST_MONTH, now.month());
  EEPROM.update(EEPROM_LAST_DAY, now.day());
}

void clearLastOpenDate() {
  EEPROM.update(EEPROM_LAST_YEAR, 0);
  EEPROM.update(EEPROM_LAST_MONTH, 0);
  EEPROM.update(EEPROM_LAST_DAY, 0);
}

bool requireRtc() {
  if (rtcReady) {
    return true;
  }

  Serial.println(F("RTC not available; check DS3231 power/SDA/SCL"));
  return false;
}

void pulseRelay() {
  Serial.println(F("Relay closed"));
  digitalWrite(RELAY_PIN, relayOnLevel());
  delay(RELAY_MS);
  digitalWrite(RELAY_PIN, relayOffLevel());
  Serial.println(F("Relay opened"));
}

void checkGateTime() {
  if (!rtcReady) {
    return;
  }

  DateTime now = rtc.now();
  if (now.hour() == openHour && now.minute() == openMinute && !openedToday(now)) {
    pulseRelay();
    markOpenedToday(now);
  }
}

void sleepEightSeconds() {
  watchdogWake = false;
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);

  ADCSRA &= ~(1 << ADEN);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  noInterrupts();
  sleep_enable();
#if defined(BODS) && defined(BODSE)
  sleep_bod_disable();
#endif
  interrupts();

  sleep_cpu();
  sleep_disable();
  ADCSRA |= (1 << ADEN);
  wdt_disable();
}

void handleCommand(String line) {
  line.trim();
  line.toUpperCase();
  stayAwakeUntil = millis() + SERIAL_COMMAND_WINDOW_MS;

  if (line == "TIME") {
    if (requireRtc()) {
      printTime(rtc.now());
    }
  } else if (line.startsWith("SETTIME ")) {
    int y, mo, d, h, mi, s;
    if (sscanf(line.c_str(), "SETTIME %d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6) {
      if (requireRtc()) {
        rtc.adjust(DateTime(y, mo, d, h, mi, s));
        clearLastOpenDate();
        Serial.println(F("Clock set"));
      }
    } else {
      Serial.println(F("Use: SETTIME YYYY-MM-DD HH:MM:SS"));
    }
  } else if (line == "OPEN") {
    twoDigits(openHour);
    Serial.print(':');
    twoDigits(openMinute);
    Serial.println();
  } else if (line.startsWith("SETOPEN ")) {
    int h, m;
    if (sscanf(line.c_str(), "SETOPEN %d:%d", &h, &m) == 2 && h >= 0 && h < 24 && m >= 0 && m < 60) {
      openHour = h;
      openMinute = m;
      saveOpenTime();
      clearLastOpenDate();
      Serial.println(F("Open time set"));
    } else {
      Serial.println(F("Use: SETOPEN HH:MM"));
    }
  } else if (line == "PULSE") {
    pulseRelay();
  } else {
    Serial.println(F("Commands: TIME, SETTIME, OPEN, SETOPEN, PULSE"));
  }
}

void handleSerial() {
  if (Serial.available()) {
    stayAwakeUntil = millis() + SERIAL_COMMAND_WINDOW_MS;
    handleCommand(Serial.readStringUntil('\n'));
  }
}

void setup() {
  digitalWrite(RELAY_PIN, relayOffLevel());
  pinMode(RELAY_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println(F("ChronoLatch booting..."));
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(I2C_TIMEOUT_US, true);
#endif
  loadSettings();

  Serial.println(F("Checking RTC..."));
  rtcReady = rtc.begin();
#if defined(WIRE_HAS_TIMEOUT)
  if (Wire.getWireTimeoutFlag()) {
    Wire.clearWireTimeoutFlag();
    Serial.println(F("I2C timeout while checking RTC"));
  }
#endif
  if (!rtcReady) {
    Serial.println(F("RTC not found"));
  } else if (rtc.lostPower()) {
    Serial.println(F("RTC lost power; set clock with SETTIME"));
  }

  stayAwakeUntil = millis() + BOOT_SERIAL_WINDOW_MS;
  Serial.println(F("ChronoLatch ready. Relay on D2."));
  Serial.println(F("Commands: TIME, SETTIME, OPEN, SETOPEN, PULSE"));
}

void loop() {
  handleSerial();
  checkGateTime();

  if ((long)(millis() - stayAwakeUntil) >= 0) {
    Serial.flush();
    sleepEightSeconds();
    stayAwakeUntil = millis() + WAKE_SERIAL_WINDOW_MS;
  } else {
    delay(100);
  }
}
