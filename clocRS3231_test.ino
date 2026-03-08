#include "esp_adc_cal.h"
#include "esp_sleep.h"
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

// ============================================================
// Battery configuration
// ============================================================
#define BAT_ADC 2

float Voltage = 0.0;

uint32_t readADC_Cal(int ADC_Raw) {
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100,
                           &adc_chars);
  return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

// ============================================================
// RTC object
// ============================================================
RTC_DS3231 rtc;

// ============================================================
// Hardware configuration
// IMPORTANT:
// On ESP32-C3, deep-sleep wake by external GPIO only works on
// RTC-capable GPIOs (GPIO0 to GPIO5).
// Move this pin to one valid RTC GPIO on your board.
// ============================================================
static const int RTC_INT_PIN = 5; // DS3232 SQW/INT connected here
static const int STATUS_PIN = 3;  // Optional debug LED or output

// ============================================================
// Global alarm configuration
// ============================================================
bool enableAlarm1 = true; // enable alarm true or false
uint8_t alarm1Hour = 18;  // <-- Change alarm hour here
uint8_t alarm1Minute = 7; // <-- Change alarm minute here
uint8_t alarm1Second = 0; // <-- Change alarm second here

bool enableAlarm2 = true; // enable alarm true or false
uint8_t alarm2Hour = 18;  // Alarm 2 hour
uint8_t alarm2Minute = 8; // Alarm 2 minute

// ============================================================
// Global status flags
// ============================================================
bool rtcOk = false;
bool wokeByExternalGpio = false;
bool alarmFlagWasSet = false;

// ============================================================
// Helper: print the current wakeup cause
// ============================================================
void printWakeupReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  Serial.print("Wakeup cause: ");

  switch (cause) {
  case ESP_SLEEP_WAKEUP_GPIO:
    Serial.println("GPIO from deep sleep");
    wokeByExternalGpio = true;
    // Optional debug output
    digitalWrite(STATUS_PIN, HIGH);
    delay(5000);
    digitalWrite(STATUS_PIN, LOW);

    break;

  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Internal timer");
    break;

  case ESP_SLEEP_WAKEUP_UNDEFINED:
    Serial.println("Power-on or reset");
    break;

  default:
    Serial.print("Other cause: ");
    Serial.println((int)cause);
    break;
  }
}

// ============================================================
// Helper: initialize RTC without aborting setup
// ============================================================
void initRtc() {
  rtcOk = rtc.begin();

  if (!rtcOk) {
    Serial.println("RTC not detected");
    return;
  }

  Serial.println("RTC detected");

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, adjusting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Disable 32K output if not used
  rtc.disable32K();

  // Make sure SQW pin is used as interrupt/alarm output,
  // not as square wave output
  rtc.writeSqwPinMode(DS3231_OFF);

  // Clear old alarm flags just in case
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // We disable both initially, will be configured properly later
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
}

// ============================================================
// Helper: calculate the NEXT occurrence of the configured time
//
// Example:
// if now is 06:30 and alarm is 07:00:00 -> today 07:00:00
// if now is 08:10 and alarm is 07:00:00 -> tomorrow 07:00:00
// ============================================================
DateTime calculateNextAlarmTime(uint8_t hour, uint8_t minute, uint8_t second) {
  DateTime now = rtc.now();

  // Build "today at hour:minute:second"
  DateTime targetToday(now.year(), now.month(), now.day(), hour, minute,
                       second);

  // If current time is still BEFORE today's target,
  // use today's target.
  if (now.unixtime() < targetToday.unixtime()) {
    return targetToday;
  }

  // Otherwise, today's alarm time already passed,
  // so schedule it for the next day.
  return targetToday + TimeSpan(1, 0, 0, 0);
}

// ============================================================
// Helper: program Alarms using the global alarm variables
// ============================================================
void configureAlarms() {
  if (!rtcOk) {
    Serial.println("Cannot configure alarm: RTC not available");
    return;
  }

  DateTime now = rtc.now();
  Serial.print("Current RTC time: ");
  Serial.println(now.timestamp(DateTime::TIMESTAMP_FULL));

  // Clear flags before setting new alarms
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  if (enableAlarm1) {
    DateTime nextAlarm1 =
        calculateNextAlarmTime(alarm1Hour, alarm1Minute, alarm1Second);
    bool ok1 = rtc.setAlarm1(nextAlarm1, DS3231_A1_Date);

    Serial.print("Configured Alarm 1 for: ");
    Serial.println(nextAlarm1.timestamp(DateTime::TIMESTAMP_FULL));
    if (!ok1) {
      Serial.println("Failed to configure Alarm 1");
    } else {
      Serial.println("Alarm 1 configured successfully");
    }
  } else {
    rtc.disableAlarm(1);
    Serial.println("Alarm 1 is disabled and will not trigger.");
  }

  if (enableAlarm2) {
    DateTime nextAlarm2 = calculateNextAlarmTime(alarm2Hour, alarm2Minute, 0);
    bool ok2 = rtc.setAlarm2(nextAlarm2, DS3231_A2_Date);

    Serial.print("Configured Alarm 2 for: ");
    Serial.println(nextAlarm2.timestamp(DateTime::TIMESTAMP_FULL));
    if (!ok2) {
      Serial.println("Failed to configure Alarm 2");
    } else {
      Serial.println("Alarm 2 configured successfully");
    }
  } else {
    rtc.disableAlarm(2);
    Serial.println("Alarm 2 is disabled and will not trigger.");
  }
}

// ============================================================
// Helper: clear alarm if it fired
//
// DS3232/DS3231 keeps the interrupt line LOW until the alarm
// flag is cleared via I2C. This step is mandatory.
// ============================================================
void clearAlarmIfNeeded() {
  if (!rtcOk) {
    return;
  }

  bool fired1 = rtc.alarmFired(1);
  bool fired2 = rtc.alarmFired(2);
  alarmFlagWasSet = fired1 || fired2;

  Serial.print("Alarm1 fired flag: ");
  Serial.println(fired1 ? "YES" : "NO");
  Serial.print("Alarm2 fired flag: ");
  Serial.println(fired2 ? "YES" : "NO");

  if (alarmFlagWasSet) {
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    Serial.println("Alarm flags cleared");
  }
}

// ============================================================
// Helper: enter deep sleep using external GPIO wakeup
//
// IMPORTANT FOR ESP32-C3:
// Use a valid RTC-domain GPIO for deep sleep wakeup.
// The wake condition is LOW because DS3232 pulls SQW/INT low
// when the alarm fires.
// ============================================================
void enterDeepSleep() {
  if (!rtcOk) {
    Serial.println("RTC unavailable, deep sleep not entered");
    return;
  }

  Serial.println("Preparing GPIO wakeup from deep sleep");

  // Enable wakeup when RTC_INT_PIN goes LOW
  gpio_wakeup_enable((gpio_num_t)RTC_INT_PIN, GPIO_INTR_LOW_LEVEL);
  esp_deep_sleep_enable_gpio_wakeup(BIT64(RTC_INT_PIN),
                                    ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.println("Entering deep sleep");
  delay(100);

  esp_deep_sleep_start();
}

// ============================================================
// Arduino setup
// ============================================================
void setup() {
  pinMode(STATUS_PIN, OUTPUT);
  digitalWrite(STATUS_PIN, LOW);

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("Boot start");

  // Read and print battery voltage
  Voltage = (readADC_Cal(analogRead(BAT_ADC))) * 2.0;
  Serial.printf("Battery Voltage measure: %.2fV\n", Voltage / 1000.0);

  Wire.begin();
  pinMode(RTC_INT_PIN, INPUT_PULLUP);

  printWakeupReason();
  initRtc();

  if (rtcOk) {
    // If wake came from the external DS3232 line,
    // or the RTC flag is still set, clear it first.
    clearAlarmIfNeeded();

    // Reconfigure the next daily alarms
    configureAlarms();
  }

  Serial.println("Setup complete");
  delay(500);

  // Optional debug output
  digitalWrite(STATUS_PIN, HIGH);
  delay(500);
  digitalWrite(STATUS_PIN, LOW);

  enterDeepSleep();
}

// ============================================================
// Arduino loop
// Normally not reached because we go to deep sleep in setup()
// ============================================================
void loop() {
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint > 2000) {
    lastPrint = millis();

    Serial.print("RTC status: ");
    Serial.println(rtcOk ? "OK" : "NOT OK");

    Serial.print("Woke by external GPIO: ");
    Serial.println(wokeByExternalGpio ? "YES" : "NO");

    Serial.print("Alarm flag was set: ");
    Serial.println(alarmFlagWasSet ? "YES" : "NO");
  }
}