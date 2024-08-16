/****************************************************************
 * Countdown Timer v4.5.1
 * 
 * Developed by KGR
 * 
 * This version incorporates:
 * - Improved debounce for touch inputs
 * - Enhanced backlight adjustment for better responsiveness
 * - Optimized battery level calculation and display
 * - Refined WiFi connection and time update processes
 * - Improved error handling and user feedback
 *****************************************************************/

#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include <Arduino.h>

// Touch screen setup
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
#define TFT_LIGHTBLUE 0x867F
#define TFT_BL 21                // Backlight control pin
#define TFT_BACKLIGHT_CHANNEL 0  // PWM channel to use for backlight
#define LDR_PIN 34               // Analog pin for the Light Dependent Resistor (LDR)
#define BUZZER_PIN 26
#define MAX_ALARMS 5
#define TOUCH_X_MIN 400
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 200
#define TOUCH_Y_MAX 3800
#define CALIBRATION_POINTS 6
int16_t TOUCH_X_OFFSET = 0;  // Adjust this value to shift touch recognition left or right
int16_t TOUCH_Y_OFFSET = 0;  // Adjust this value to shift touch recognition up or down
bool calibrationMode = false;
unsigned long lastCalibrationTouch = 0;
const unsigned long CALIBRATION_DEBOUNCE_TIME = 1000;  // 1 second debounce
unsigned long lastDebugPrint = 0;
float TOUCH_X_SCALE = 1.0;
float TOUCH_Y_SCALE = 1.0;

// Global color variables
uint16_t bgColor = TFT_BLACK;  // dark teal
uint16_t timeColor = 0x0d18;   // light cyan
uint16_t boxColor = 0x01e8;
uint16_t countdownColor = 0x01e8;
uint16_t dateColor = 0x0390;
uint16_t targetColor = 0xbde0;
uint16_t welcomeColor = 0x0390;

TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

int invert = 1; //0=std, 1=invverted screen colours

// Global variables for WiFi connection attempts
int wifiAttempts = 0;
const int MAX_WIFI_ATTEMPTS = 3;
const int WIFI_RETRY_DELAY = 3000;  // 5 seconds delay between attempts

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;  // Set to your time zone offset

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

const int BATTERY_PIN = 35;  // Using GPIO35 for battery reading

int batteryLevel;
int rawValue = analogRead(BATTERY_PIN);

float calibrationFactor = 1.0;  // Adjust this based on your measurements
float voltage = ((rawValue / 4095.0) * 2 * 3.3) * calibrationFactor;
const float MIN_VOLTAGE = 3.3;  // Minimum voltage when battery is considered empty
const float MAX_VOLTAGE = 4.2;  // Maximum battery voltage (fully charged)
int percentage = map(voltage * 100, MIN_VOLTAGE * 100, MAX_VOLTAGE * 100, 0, 100);

int ct = 35;   //position of current time box
int rt = 156;  //position of remaining time box
int td = 222;  //position of target date box

// Alarm/display variables
unsigned long alarmTriggerTime = 0;
bool isAlarmFlashing = false;
bool isAlarmRedPhase = false;
bool isRemainingTimeHighlighted = false;
unsigned long alarmActivationTime = 0;
int activeAlarmIndex = -1;
unsigned long remainingTimeHighlightStart = 0;
const unsigned long REMAINING_TIME_HIGHLIGHT_DURATION = 5000;  // 5 seconds

// Define global variables to hold countdown values
int countdownValues[6] = { 0 };  // {years, months, days, hours, minutes, seconds}

// Backlight Levels
int backlightIntensity = 255;  // 0-255, where 255 is full brightness
const int minBacklight = 10;   // Minimum backlight level
const int maxBacklight = 255;  // Maximum backlight level
unsigned long lastBacklightUpdate = 0;

// Alarm structure
struct Alarm {
  int hour;
  int minute;
  bool enabled;
  char sound[20];  // Name of the sound file
};

struct CalibrationPoint {
  int16_t screenX;
  int16_t screenY;
  int16_t touchX;
  int16_t touchY;
};

Alarm alarms[MAX_ALARMS];

CalibrationPoint calibrationPoints[CALIBRATION_POINTS];
int currentCalibrationPoint = 0;

String targetDate = "03/07/2024";  // Target date in DD/MM/YYYY format
String targetTime = "08:30";       // Target time in HH:MM format

#define MAX_WIFI_OPTIONS 5
#define MAX_TARGET_OPTIONS 5

// Define the WiFiOption struct
struct WiFiOption {
  char name[33];
  char password[65];
  char nickname[21];
};

// Declare the options array with initial values
WiFiOption options[MAX_WIFI_OPTIONS] = {
  { "VM1459801", "qw4Hdxbrs5wx", "home" },
  { "CASWIFI", "securew1f1", "work" },
  { "Kevin s Galaxy A14 5G", "1a1a1a1a", "phone" },
  { "", "", "" },
  { "", "", "" }
};

struct TargetOption {
  char trgtTime[6];
  char target[11];
  char reason[30];
};

TargetOption targetOptions[MAX_TARGET_OPTIONS] = {
  { "00:00", "13/06/2028", "Retirement" },
  { "17:00", "24/03/2025", "Cruise!" },
  { "00:00", "13/06/2025", "When I'm 64!" },
  { "10:00", "24/03/2025", "Cruise to Cyprus" },
  { "06:00", "16/09/2024", "Cruise to Turkey!" }
};

// Global variables for tracking last update times
unsigned long lastMinuteChange = 0;
unsigned long lastDateChange = 0;
unsigned long lastSymbolUpdate = 0;
unsigned long lastShiftPress = 0;
unsigned long lastCapsLockPress = 0;
const unsigned long KEY_DEBOUNCE_TIME = 500;  // 500ms debounce time
bool colonVisible = true;
wl_status_t lastWiFiStatus = WL_IDLE_STATUS;
int lastBatteryLevel = -1;
unsigned long lastDisplayUpdate = 0;
unsigned long lastTimeUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000;   // 1 second
const unsigned long TIME_UPDATE_INTERVAL = 86400000;  // 24 hours
const int MAX_NTP_ATTEMPTS = 5;
const int NTP_RETRY_DELAY = 2000;  // 2 seconds

time_t lastNTPSync = 0;
String currentTimeZone = "GMT0BST,M3.5.0/1,M10.5.0";  // Default to UK time
const char* TIME_ZONE = "GMT0BST,M3.5.0/1,M10.5.0";   // UK time zone
int rot = 1;
int selectedTargetIndex = 0;
int editingTargetIndex = -1;
int editingWiFiIndex = -1;

// Update your ScreenState enum to include the new screen:
enum ScreenState {
  WELCOME_SCREEN,
  MAIN_SCREEN,
  TARGET_SCREEN,
  EDIT_TARGET_SCREEN,
  WIFI_SCREEN,
  EDIT_WIFI_SCREEN,
  ALARM_SCREEN,
  ALARM_ACTIVE_SCREEN,
  BACKLIGHT_CONTROL_SCREEN,
  CALIBRATION_SCREEN
};

ScreenState currentScreenState = WELCOME_SCREEN;

#define TOUCH_DEBOUNCE_TIME 300  // Increased debounce time
//const unsigned long KEY_DEBOUNCE_TIME = 200;  // Global key debounce time in milliseconds
#define SHIFT_DEBOUNCE_TIME 100
unsigned long lastTouchTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastCountdownUpdate = 0;

// Keyboard layouts
const char* numericKeys[] = { "1", "2", "3", ":", "4", "5", "6", "/", "7", "8", "9", "<", "<-", "0", "C", "OK" };
const char* alphaKeys1 = "QWERTYUIOP";
const char* alphaKeys2 = "ASDFGHJKL<";
const char* alphaKeys3 = "ZXCVBNM,.>";
const char* lowerAlphaKeys1 = "qwertyuiop";
const char* lowerAlphaKeys2 = "asdfghjkl<";
const char* lowerAlphaKeys3 = "zxcvbnm,.>";
const char* numberRow = "1234567890";
const char* symbolRow = "!@#$%^&*()";
const char* const upperKeys[] = { alphaKeys1, alphaKeys2, alphaKeys3 };
const char* const lowerKeys[] = { lowerAlphaKeys1, lowerAlphaKeys2, lowerAlphaKeys3 };

#define SLIDER_X 50
#define SLIDER_Y 120
#define SLIDER_WIDTH 220
#define SLIDER_HEIGHT 20
#define KNOB_RADIUS 15

int userSetBacklight = maxBacklight;
bool userControlledBacklight = false;

int copyright = 0;

bool shiftPressed = false;
bool capsLockOn = false;
bool timeSet = false;
bool wifiIntentionallyDisconnected = false;

String lastConnectedSSID = "";
bool wifiSymbolVisible = false;

// Function declarations
void initializeWiFiOptions();
void loadWiFiOptions();
void saveWiFiOptions();
bool updateTimeAndDisconnect();
void disconnectWiFi();
void updateLocationAndTimeZone();
void displayMessage(const char* message, const char* submessage, uint16_t color, int duration);
void displayTimeOnTFT(uint16_t textColor, uint16_t bgColor, bool forceUpdate);
void displayCountdownBoxes();
void displayCountdownLabels();
void displayCountdownValues();
void calculateTimeDifference();
bool isLeapYear(int year);
void displayCountdownOnTFT();
void updateDisplay();
void handleTouch(uint16_t x, uint16_t y);
void handleMainScreenTouch(int16_t x, int16_t y);
void displayTargetScreen();
void handleTargetScreenTouch(int16_t x, int16_t y);
void displayEditTargetScreen();
void handleEditTargetScreenTouch(int16_t x, int16_t y);
void displayWiFiOptions();
void handleWiFiScreenTouch(int16_t x, int16_t y);
void displayEditWiFiScreen();
void handleEditWiFiScreenTouch(int16_t x, int16_t y);
void updateMainScreenElements();
void drawKeyboard(bool numeric);
char handleKeyboardTouch(int16_t x, int16_t y, bool numeric);
String editText(String initialText, bool numeric, int maxLength, const char* editType);
void updateEditDisplayText(const String& text, int cursorPosition);
void editTargetDate();
void editTargetTime();
void editTargetReason();
void deleteTarget();
void saveTargetDates();
void loadTargetDates();
int getDaysInMonth(int month, int year);
void drawTargetOptions(int highlightedIndex);
void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text);
bool isValidDate(const String& date);
bool isValidTime(const String& time);
bool connectToWiFi();
void displayWiFiNickname();
void drawWiFiSymbol(int16_t x, int16_t y, uint16_t color);
void drawBatterySymbol(int16_t x, int16_t y, int level);
uint16_t getBatteryColor(int level);
int getBatteryLevel();
void displayLowBatteryWarning();
void updateWiFiSymbol();
bool attemptWiFiConnection(const char* ssid, const char* password, const char* nickname);
bool connectToWiFiAndSetTime();
void setBacklight(int intensity);
void adjustBacklight();
void drawTimeUpdateArea();
void handleTimeUpdateTouch();
void loadAlarms();
void saveAlarms();
void checkAlarms();
void playStarTrekAlarm();
void displayAlarmScreen();
void handleAlarmScreenTouch(int16_t x, int16_t y);
void drawAlarmIcon(int x, int y);
void editAlarm(int index);
void updateMainScreen();
void stopAlarm();
void updateBatteryDisplay();
bool attemptWiFiConnectionWithRetry();
void handleBacklightControl(int16_t x, int16_t y);
void displayBacklightControl();
void drawSlider(int value);
void setUserBacklight(int level);
void toggleCalibrationMode();
void startCalibration();
void drawCross(int16_t x, int16_t y, uint16_t color);
void handleCalibrationTouch(int16_t x, int16_t y);
void displayCalibrationResults();
void drawToggleSwitch(int x, int y, bool state);
void toggleAlarm(int index);
void logSystemStatus();
// end of Function declarations

void initializeWiFiOptions() {
  preferences.begin("wifi_prefs", false);
  bool prefsExist = preferences.getBool("initialized", false);

  if (!prefsExist) {
    for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
      String key = "wifi_" + String(i);
      String value = String(options[i].name) + "," + String(options[i].password) + "," + String(options[i].nickname);
      preferences.putString(key.c_str(), value);
    }
    preferences.putBool("initialized", true);
  } else {
    loadWiFiOptions();
  }

  preferences.end();
}
//end of initializeWiFiOptions function

void loadWiFiOptions() {
  for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
    String key = "wifi_" + String(i);
    String value = preferences.getString(key.c_str(), "");
    if (value.length() > 0) {
      int firstComma = value.indexOf(',');
      int secondComma = value.indexOf(',', firstComma + 1);
      strncpy(options[i].name, value.substring(0, firstComma).c_str(), sizeof(options[i].name) - 1);
      strncpy(options[i].password, value.substring(firstComma + 1, secondComma).c_str(), sizeof(options[i].password) - 1);
      strncpy(options[i].nickname, value.substring(secondComma + 1).c_str(), sizeof(options[i].nickname) - 1);
      options[i].name[sizeof(options[i].name) - 1] = '\0';
      options[i].password[sizeof(options[i].password) - 1] = '\0';
      options[i].nickname[sizeof(options[i].nickname) - 1] = '\0';
    }
  }
}
//end of loadWiFiOptions function

void saveWiFiOptions() {
  preferences.begin("wifi_prefs", false);
  for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
    String key = "wifi_" + String(i);
    String value = String(options[i].name) + "," + String(options[i].password) + "," + String(options[i].nickname);
    preferences.putString(key.c_str(), value);
  }
  preferences.end();
}
//end of saveWiFiOptions function

bool updateTimeAndDisconnect() {
  Serial.println("Updating time from WiFi...");

  for (int attempt = 1; attempt <= MAX_NTP_ATTEMPTS; attempt++) {
    displayMessage("Updating Time...",
                   String("Attempt " + String(attempt) + "/" + String(MAX_NTP_ATTEMPTS)).c_str(),
                   TFT_WHITE,
                   1000);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    int retry = 0;
    const int retry_count = 10;
    while (time(nullptr) < 1000000000 && ++retry < retry_count) {
      Serial.print(".");
      delay(500);
    }

    if (retry < retry_count) {
      lastNTPSync = time(nullptr);
      Serial.println("NTP time sync successful");
      displayMessage("Time updated successfully", "", TFT_GREEN, 1000);
      updateLocationAndTimeZone();
      timeSet = true;

      disconnectWiFi();
      delay(200);
      return true;
    } else {
      Serial.printf("NTP time sync failed (Attempt %d of %d)\n", attempt, MAX_NTP_ATTEMPTS);
      displayMessage("Time sync failed", "Retrying...", TFT_YELLOW, 1000);
      delay(NTP_RETRY_DELAY);
    }
  }

  Serial.println("All NTP time sync attempts failed");
  displayMessage("Time sync failed", "Check internet connection", TFT_RED, 1000);
  return false;
}
//end of updateTimeAndDisconnect function

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.println("WiFi disconnected");
  displayMessage("WiFi disconnected", "", TFT_WHITE, 1000);

  wifiIntentionallyDisconnected = true;
  updateWiFiSymbol();
}
//end of disconnectWiFi function

void updateLocationAndTimeZone() {
  setenv("TZ", TIME_ZONE, 1);
  tzset();
  Serial.println("Time zone updated");
}
//end of updateLocationAndTimeZone function

void displayMessage(const char* message, const char* submessage, uint16_t color, int duration) {
  tft.fillScreen(bgColor);
  tft.setTextColor(color, bgColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  copyright = 0;
  tft.setTextSize(1);
  tft.drawString(message, tft.width() / 2, tft.height() / 2 - 20);
  if (strlen(submessage) > 0) {
    tft.drawString(submessage, tft.width() / 2, tft.height() / 2 + 20);
  }
  delay(duration);
}
//end of displayMessage function

void displayTimeOnTFT(uint16_t textColor, uint16_t bgColor, bool forceUpdate) {
  static char lastDisplayedTime[6] = "     ";
  static char lastDisplayedDate[30] = "                             ";
  static char lastDisplayedSeconds[3] = "  ";

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char currentTime[6];
  char currentDate[30];
  char currentSeconds[3];
  strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);
  strftime(currentDate, sizeof(currentDate), "%a %d/%m/%Y", &timeinfo);
  strftime(currentSeconds, sizeof(currentSeconds), "%S", &timeinfo);

  unsigned long currentMillis = millis();
  bool minuteChanged = strcmp(currentTime, lastDisplayedTime) != 0;
  bool dateChanged = strcmp(currentDate, lastDisplayedDate) != 0;
  bool secondsChanged = strcmp(currentSeconds, lastDisplayedSeconds) != 0;

  if (forceUpdate || minuteChanged || dateChanged) {
    tft.setTextSize(2);
    tft.setTextFont(7);
    tft.setTextColor(timeColor, bgColor);
    tft.setTextDatum(ML_DATUM);
    int32_t x = 20;
    int32_t y = ct + 42;

    tft.fillRect(x, y - 42, 270, 84, bgColor);
    tft.drawString(currentTime, x, y);

    strcpy(lastDisplayedTime, currentTime);

    tft.drawString(":", x + tft.textWidth("00"), y);

    if (dateChanged || forceUpdate) {
      tft.fillRect(104, ct + 100, 113, 15, bgColor);
      tft.setTextFont(2);
      tft.setTextSize(1);
      tft.setTextColor(dateColor, bgColor);
      tft.setTextDatum(TC_DATUM);
      tft.drawString(currentDate, tft.width() / 2, ct + 98);
      strcpy(lastDisplayedDate, currentDate);
      lastDateChange = currentMillis;
    }
  }

  if (secondsChanged || forceUpdate) {
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(dateColor, bgColor);

    tft.fillRect(tft.width() - 70, ct + 100, 25, 20, bgColor);
    tft.drawRect(tft.width() - 70, ct + 100, 25, 20, boxColor);

    tft.drawString(currentSeconds, tft.width() - 50, ct + 102);
    strcpy(lastDisplayedSeconds, currentSeconds);
  }

  if (currentMillis % 2000 < 1000) {
    if (!colonVisible) {
      tft.setTextSize(2);
      tft.setTextFont(7);
      tft.setTextColor(timeColor, bgColor);
      tft.setTextDatum(ML_DATUM);
      int32_t x = 20;
      int32_t y = ct + 42;
      tft.drawString(":", x + tft.textWidth("00"), y);
      colonVisible = true;
    }
  } else {
    if (colonVisible) {
      tft.setTextSize(2);
      tft.setTextFont(7);
      tft.setTextColor(bgColor, bgColor);
      tft.setTextDatum(ML_DATUM);
      int32_t x = 20;
      int32_t y = ct + 42;
      tft.drawString(":", x + tft.textWidth("00"), y);
      colonVisible = false;
    }
  }

  tft.setTextFont(4);
  tft.setTextSize(1);
}
//end of displayTimeOnTFT function

void displayCountdownBoxes() {
  if (currentScreenState != MAIN_SCREEN) return;
  int boxWidth = 48;
  int boxHeight = 36;
  int spacing = 5;
  int startX = (tft.width() - (boxWidth * 6 + spacing * 5)) / 2;
  int startY = rt + 2;

  uint16_t color = isRemainingTimeHighlighted ? timeColor : boxColor;

  for (int i = 0; i < 6; i++) {
    tft.drawRoundRect(startX + (boxWidth + spacing) * i, startY, boxWidth, boxHeight, 5, color);
  }
}
//end of displayCountdownBoxes function

void displayCountdownLabels() {
  if (currentScreenState != MAIN_SCREEN) return;
  int boxWidth = 48;
  int spacing = 5;
  int startX = ((tft.width() - (boxWidth * 6 + spacing * 5)) / 2);
  int startY = rt + 37;

  String labels[] = { "  y", "mo", "  d", "  h", "  m", "  s" };

  uint16_t color = isRemainingTimeHighlighted ? timeColor : boxColor;

  for (int i = 0; i < 6; i++) {
    tft.setTextColor(color);
    tft.setCursor(startX - 5 + (boxWidth + spacing) * i + boxWidth / 4, startY);
    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.println(labels[i]);
  }

  int borderWidth2 = 4;
  int borderRadius2 = 10;
  int boxWidth2 = 305;
  int boxHeight2 = 110;
  int boxX2 = (tft.width() - boxWidth2) / 2;
  int boxWidth3 = 140;
  int boxHeight3 = 20;
  int boxX3 = (tft.width() - boxWidth3) / 2;  // border around date

  tft.drawRoundRect(boxX2, ct - 13, boxWidth2, boxHeight2, borderRadius2, boxColor);
  tft.drawRoundRect(boxX2 + borderWidth2, ct - 13 + borderWidth2, boxWidth2 - 2 * borderWidth2, boxHeight2 - 2 * borderWidth2, borderRadius2 - borderWidth2, boxColor);
  tft.drawRoundRect(boxX3, ct + 98, boxWidth3, boxHeight3, borderRadius2 - 4, boxColor);
}
//end of displayCountdownLabels function

void displayCountdownValues() {
  if (currentScreenState != MAIN_SCREEN) return;
  int boxWidth = 48;
  int boxHeight = 36;
  int spacing = 5;
  int startX = ((tft.width() - (boxWidth * 6 + spacing * 5)) / 2) - 2;
  int startY = rt - 2;

  tft.setTextFont(4);
  tft.setTextSize(1);
  bool allZero = true;
  bool previousZero = true;

  uint16_t activeColor = isRemainingTimeHighlighted ? timeColor : countdownColor;

  if (countdownValues[0] < 0) {
    for (int i = 0; i < 6; i++) {
      tft.setTextColor(bgColor, bgColor);
      tft.setCursor(startX + (boxWidth + spacing) * i + 10, startY + 10);
      tft.println("00");
    }
    return;
  }

  for (int i = 0; i < 6; i++) {
    char valueStr[4];
    sprintf(valueStr, "%02d", countdownValues[i]);

    if (countdownValues[i] == 0 && previousZero) {
      tft.setTextColor(0x4000, bgColor);
    } else {
      tft.setTextColor(activeColor, bgColor);
      allZero = false;
    }

    tft.setCursor(startX + (boxWidth + spacing) * i + 10, startY + 10);
    tft.println(valueStr);

    previousZero = (countdownValues[i] == 0);
  }

  if (allZero) {
    for (int i = 0; i < 6; i++) {
      countdownValues[i] = -1;
      tft.setTextColor(bgColor, bgColor);
      tft.setCursor(startX + (boxWidth + spacing) * i + 10, startY + 10);
      tft.println("00");
    }
  }
}
//end of displayCountdownValues function

void calculateTimeDifference() {
  struct tm currentTime;
  if (!getLocalTime(&currentTime)) {
    Serial.println("Failed to obtain current time");
    return;
  }

  struct tm targetTm = {};
  sscanf(targetDate.c_str(), "%d/%d/%d", &targetTm.tm_mday, &targetTm.tm_mon, &targetTm.tm_year);
  sscanf(targetTime.c_str(), "%d:%d", &targetTm.tm_hour, &targetTm.tm_min);
  targetTm.tm_mon -= 1;
  targetTm.tm_year -= 1900;
  targetTm.tm_sec = 0;

  countdownValues[0] = targetTm.tm_year - currentTime.tm_year;

  countdownValues[1] = targetTm.tm_mon - currentTime.tm_mon;
  if (countdownValues[1] < 0) {
    countdownValues[1] += 12;
    countdownValues[0]--;
  }

  if (targetTm.tm_mday < currentTime.tm_mday) {
    countdownValues[1]--;
    if (countdownValues[1] < 0) {
      countdownValues[1] += 12;
      countdownValues[0]--;
    }
  }

  int daysInCurrentMonth = getDaysInMonth(currentTime.tm_mon + 1, currentTime.tm_year + 1900);
  countdownValues[2] = (targetTm.tm_mday - currentTime.tm_mday + daysInCurrentMonth) % daysInCurrentMonth;

  countdownValues[3] = targetTm.tm_hour - currentTime.tm_hour;
  if (countdownValues[3] < 0) {
    countdownValues[3] += 24;
    countdownValues[2]--;
    if (countdownValues[2] < 0) {
      countdownValues[2] += daysInCurrentMonth;
      countdownValues[1]--;
      if (countdownValues[1] < 0) {
        countdownValues[1] += 12;
        countdownValues[0]--;
      }
    }
  }

  countdownValues[4] = targetTm.tm_min - currentTime.tm_min;
  if (countdownValues[4] < 0) {
    countdownValues[4] += 60;
    countdownValues[3]--;
    if (countdownValues[3] < 0) {
      countdownValues[3] += 24;
      countdownValues[2]--;
      if (countdownValues[2] < 0) {
        countdownValues[2] += daysInCurrentMonth;
        countdownValues[1]--;
        if (countdownValues[1] < 0) {
          countdownValues[1] += 12;
          countdownValues[0]--;
        }
      }
    }
  }

  countdownValues[5] = 60 - currentTime.tm_sec;
  if (countdownValues[5] == 60) {
    countdownValues[5] = 0;
  } else {
    countdownValues[4]--;
    if (countdownValues[4] < 0) {
      countdownValues[4] += 60;
      countdownValues[3]--;
      if (countdownValues[3] < 0) {
        countdownValues[3] += 24;
        countdownValues[2]--;
        if (countdownValues[2] < 0) {
          countdownValues[2] += daysInCurrentMonth;
          countdownValues[1]--;
          if (countdownValues[1] < 0) {
            countdownValues[1] += 12;
            countdownValues[0]--;
          }
        }
      }
    }
  }
  if (countdownValues[0] < 0) {
    for (int i = 0; i < 6; i++) {
      countdownValues[i] = 0;
    }
  }
}
//end of calculateTimeDifference function

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}
//end of isLeapYear function

void displayCountdownOnTFT() {
  if (currentScreenState != MAIN_SCREEN) return;
  tft.setTextColor(targetColor, bgColor);
  tft.setTextFont(2);
  tft.setTextSize(1);
  String firstLine = "Target: " + String(targetDate) + " " + String(targetTime) + " - " + targetOptions[selectedTargetIndex].reason;
  int16_t firstLineWidth = tft.textWidth(firstLine);
  tft.setCursor((tft.width() - firstLineWidth) / 2, td);
  tft.println(firstLine);
  tft.setTextFont(4);
}
//end of displayCountdownOnTFT function

void updateDisplay() {
  if (currentScreenState == MAIN_SCREEN && !calibrationMode) {
    displayTimeOnTFT(timeColor, bgColor, false);
    displayCountdownBoxes();
    displayCountdownLabels();
    calculateTimeDifference();
    displayCountdownValues();
    displayCountdownOnTFT();
    updateBatteryDisplay();
    updateWiFiSymbol();
  }
  switch (currentScreenState) {
    case MAIN_SCREEN:
      updateMainScreenElements();
      break;
    case ALARM_SCREEN:
    case ALARM_ACTIVE_SCREEN:
      displayAlarmScreen();
      break;
    default:
      break;
  }
}
//end of updateDisplay function

void handleTouch(uint16_t x, uint16_t y) {
  static uint16_t lastX = 0, lastY = 0;
  static unsigned long lastTouchTime = 0;
  unsigned long currentTime = millis();

  // Ignore touches that are too close in time or position
  if (currentTime - lastTouchTime < TOUCH_DEBOUNCE_TIME ||
      (abs(x - lastX) < 10 && abs(y - lastY) < 10)) {
    return;
  }

  lastTouchTime = currentTime;
  lastX = x;
  lastY = y;

  Serial.printf("Handling touch: x=%d, y=%d\n", x, y);

  switch (currentScreenState) {
    case WELCOME_SCREEN:
      if (!timeSet) {
        currentScreenState = WIFI_SCREEN;
        displayWiFiOptions();
      } else {
        updateMainScreen();
      }
      break;
    case MAIN_SCREEN:
      handleMainScreenTouch(x, y);
      break;
    case TARGET_SCREEN:
      handleTargetScreenTouch(x, y);
      break;
    case EDIT_TARGET_SCREEN:
      handleEditTargetScreenTouch(x, y);
      break;
    case WIFI_SCREEN:
      handleWiFiScreenTouch(x, y);
      break;
    case EDIT_WIFI_SCREEN:
      handleEditWiFiScreenTouch(x, y);
      break;
    case BACKLIGHT_CONTROL_SCREEN:
      handleBacklightControl(x, y);
      break;
    case ALARM_SCREEN:
    case ALARM_ACTIVE_SCREEN:
      handleAlarmScreenTouch(x, y);
      break;
  }
}

void handleMainScreenTouch(int16_t x, int16_t y) {
  Serial.printf("Main screen touch: x=%d, y=%d\n", x, y);

  if (y < 60 && x > tft.width() / 2 - 60 && x < tft.width() / 2 + 60) {
    Serial.println("Opening backlight control");
    currentScreenState = BACKLIGHT_CONTROL_SCREEN;
    displayBacklightControl();
    return;
  }

  if (y > tft.height() - 25) {// was 30
    Serial.println("Opening target screen");
    currentScreenState = TARGET_SCREEN;
    displayTargetScreen();
  } else if (x < 40 && y < 40) {
    Serial.println("Opening WiFi screen");
    currentScreenState = WIFI_SCREEN;
    displayWiFiOptions();
  } else if (x < 30 && y > ct + 90 && y < ct + 120) {
    Serial.println("Opening alarm screen");
    currentScreenState = ALARM_SCREEN;
    displayAlarmScreen();
  } else if (x > tft.width() - 70 && x < tft.width() - 40 && y > ct + 90 && y < ct + 120) {
    Serial.println("Handling time update touch");
    handleTimeUpdateTouch();
  } else if (y >= rt && y < rt + 84) {
    Serial.println("Highlighting remaining time");
    isRemainingTimeHighlighted = true;
    remainingTimeHighlightStart = millis();
    displayCountdownBoxes();
    displayCountdownLabels();
    displayCountdownValues();
  } else if (x > tft.width() - 30 && y > ct + 90 && y < ct + 120) {
    Serial.println("Toggling calibration mode");
    toggleCalibrationMode();
  }
}


void displayTargetScreen() {
  currentScreenState = TARGET_SCREEN;
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("Select Target Date", tft.width() / 2, 20);
  drawTargetOptions(selectedTargetIndex);

  drawButton(5, 5, 30, 30, TFT_BLUE, TFT_WHITE, 5, "Edit");
  drawButton(tft.width() - 35, 5, 30, 30, TFT_GREEN, TFT_BLACK, 5, "Conf");
}
//end of displayTargetScreen function

void handleTargetScreenTouch(int16_t x, int16_t y) {
  int optionHeight = 35;

  for (int i = 0; i < 5; i++) {
    if (y > 50 + i * optionHeight && y < 50 + (i + 1) * optionHeight && x > 10 && x < tft.width() - 10) {
      selectedTargetIndex = i;
      drawTargetOptions(selectedTargetIndex);
      return;
    }
  }

  // Adjust these values to make the touch areas more precise
  if (x < 50 && y < 40) {
    currentScreenState = EDIT_TARGET_SCREEN;
    editingTargetIndex = selectedTargetIndex;
    displayEditTargetScreen();
  } else if (x > tft.width() - 50 && y < 40) {
    targetDate = String(targetOptions[selectedTargetIndex].target);
    targetTime = String(targetOptions[selectedTargetIndex].trgtTime);
    saveTargetDates();
    updateMainScreen();
  }
}
//end of handleTargetScreenTouch function

void displayEditTargetScreen() {
  currentScreenState = EDIT_TARGET_SCREEN;
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);

  tft.drawString("Edit Target", 10, 10);
  tft.drawString("Date: " + String(targetOptions[editingTargetIndex].target), 10, 50);
  tft.drawString("Time: " + String(targetOptions[editingTargetIndex].trgtTime), 10, 90);
  tft.drawString("Reason: " + String(targetOptions[editingTargetIndex].reason), 10, 130);

  drawButton(10, 170, 100, 40, TFT_RED, TFT_WHITE, 5, "Delete");
  drawButton(210, 170, 100, 40, TFT_BLUE, TFT_WHITE, 5, "Back");
}
//end of displayEditTargetScreen function

void handleEditTargetScreenTouch(int16_t x, int16_t y) {
  if (y > 40 && y < 80) {
    editTargetDate();
  } else if (y > 80 && y < 120) {
    editTargetTime();
  } else if (y > 120 && y < 160) {
    editTargetReason();
  } else if (y > 160 && y < 210 && x > 10 && x < 110) {
    deleteTarget();
  } else if (y > 160 && y < 210 && x > 210 && x < 320) {
    currentScreenState = TARGET_SCREEN;
    displayTargetScreen();
  }
}
//end of handleEditTargetScreenTouch function

void displayWiFiOptions() {
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("WiFi Setup", tft.width() / 2, 20);

  for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
    String displayText = options[i].nickname;
    if (displayText.length() == 0) {
      displayText = "<Empty>";
    }
    drawButton(10, 50 + i * 40, 300, 35, i == editingWiFiIndex ? TFT_YELLOW : TFT_LIGHTGREY, TFT_BLACK, 5, displayText.c_str());
  }

  drawButton(5, 5, 30, 30, TFT_BLUE, TFT_WHITE, 5, "E");
  drawButton(tft.width() - 35, 5, 30, 30, TFT_GREEN, TFT_BLACK, 5, "C");
}
//end of displayWiFiOptions function

void handleWiFiScreenTouch(int16_t x, int16_t y) {
  if (x < 35 && y < 35) {
    currentScreenState = EDIT_WIFI_SCREEN;
    displayEditWiFiScreen();
    return;
  } else if (x > tft.width() - 35 && y < 35) {
    updateMainScreen();
    return;
  }

  for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
    if (y > 50 + i * 40 && y < 85 + i * 40 && x > 10 && x < tft.width() - 10) {
      editingWiFiIndex = i;
      if (strlen(options[i].name) == 0) {
        currentScreenState = EDIT_WIFI_SCREEN;
        displayEditWiFiScreen();
      } else {
        if (attemptWiFiConnection(options[i].name, options[i].password, options[i].nickname)) {
          if (updateTimeAndDisconnect()) {
            timeSet = true;
            updateMainScreen();
          } else {
            displayMessage("Failed to set time", "", TFT_RED, 2000);
            displayWiFiOptions();
          }
        } else {
          displayMessage("Failed to connect", "Retry?", TFT_RED, 2000);
          displayWiFiOptions();
        }
      }
      return;
    }
  }
}
//end of handleWiFiScreenTouch function

void displayEditWiFiScreen() {
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);

  tft.drawString("Edit WiFi Option " + String(editingWiFiIndex + 1), 10, 10);

  drawButton(10, 50, 300, 35, TFT_LIGHTGREY, TFT_BLACK, 5, ("SSID: " + String(options[editingWiFiIndex].name)).c_str());
  drawButton(10, 90, 300, 35, TFT_LIGHTGREY, TFT_BLACK, 5, ("Password: " + String(options[editingWiFiIndex].password)).c_str());
  drawButton(10, 130, 300, 35, TFT_LIGHTGREY, TFT_BLACK, 5, ("Nickname: " + String(options[editingWiFiIndex].nickname)).c_str());

  drawButton(10, 170, 95, 40, TFT_RED, TFT_WHITE, 5, "Delete");
  drawButton(112, 170, 95, 40, TFT_BLUE, TFT_WHITE, 5, "Back");
  drawButton(214, 170, 95, 40, TFT_GREEN, TFT_WHITE, 5, "Confirm");
}
//end of displayEditWiFiScreen function

void handleEditWiFiScreenTouch(int16_t x, int16_t y) {
  if (y > 170 && y < 210) {
    if (x > 10 && x < 105) {
      // Delete button pressed
      memset(&options[editingWiFiIndex], 0, sizeof(WiFiOption));
      saveWiFiOptions();
      currentScreenState = WIFI_SCREEN;
      displayWiFiOptions();
    } else if (x > 112 && x < 207) {
      // Back button pressed
      currentScreenState = WIFI_SCREEN;
      displayWiFiOptions();
    } else if (x > 214 && x < 309) {
      // Confirm button pressed
      if (strlen(options[editingWiFiIndex].name) > 0) {
        bool connected = attemptWiFiConnection(options[editingWiFiIndex].name,
                                               options[editingWiFiIndex].password,
                                               options[editingWiFiIndex].nickname);
        if (connected) {
          displayMessage("Connected to WiFi", "", TFT_GREEN, 2000);
          Serial.println("Connected to WiFi");
        } else {
          displayMessage("Failed to connect to WiFi", "", TFT_RED, 2000);
          Serial.println("Failed to connect to WiFi");
        }
      }
      currentScreenState = WIFI_SCREEN;
      displayWiFiOptions();
    }
    return;
  }

  if (y > 50 && y < 85) {
    String newSSID = editText(options[editingWiFiIndex].name, false, 32, "SSID");
    strncpy(options[editingWiFiIndex].name, newSSID.c_str(), sizeof(options[editingWiFiIndex].name) - 1);
    options[editingWiFiIndex].name[sizeof(options[editingWiFiIndex].name) - 1] = '\0';
  } else if (y > 90 && y < 125) {
    String newPassword = editText(options[editingWiFiIndex].password, false, 64, "Password");
    strncpy(options[editingWiFiIndex].password, newPassword.c_str(), sizeof(options[editingWiFiIndex].password) - 1);
    options[editingWiFiIndex].password[sizeof(options[editingWiFiIndex].password) - 1] = '\0';
  } else if (y > 130 && y < 165) {
    String newNickname = editText(options[editingWiFiIndex].nickname, false, 20, "Nickname");
    strncpy(options[editingWiFiIndex].nickname, newNickname.c_str(), sizeof(options[editingWiFiIndex].nickname) - 1);
    options[editingWiFiIndex].nickname[sizeof(options[editingWiFiIndex].nickname) - 1] = '\0';
  }

  displayEditWiFiScreen();
}
//end of handleEditWiFiScreenTouch function

void updateMainScreenElements() {
  displayTimeOnTFT(timeColor, bgColor, false);
  calculateTimeDifference();
  displayCountdownValues();
  updateBatteryDisplay();
  updateWiFiSymbol();
}
//end of updateMainScreenElements function

void drawKeyboard(bool numeric = false) {
  tft.fillRect(0, 100, tft.width(), tft.height() - 100, TFT_DARKGREY);

  int keyWidth = tft.width() / 10;
  int keyHeight = (tft.height() - 100) / 5;

  if (numeric) {
    for (int i = 0; i < 16; i++) {
      int x = (i % 4) * (tft.width() / 4);
      int y = (i / 4) * keyHeight + 100;
      tft.fillRoundRect(x + 1, y + 1, tft.width() / 4 - 2, keyHeight - 2, 5, TFT_LIGHTGREY);
      tft.setTextColor(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextFont(4);
      tft.setTextSize(1);  // Set text size to 1
      tft.drawString(String(numericKeys[i]), x + tft.width() / 8, y + keyHeight / 2);
    }
  } else {
    const char* const* keys = (shiftPressed || capsLockOn) ? upperKeys : lowerKeys;
    const char* topRow = shiftPressed ? symbolRow : numberRow;

    // Draw number/symbol row
    for (int col = 0; col < 10; col++) {
      int x = col * keyWidth;
      int y = 100;
      tft.fillRoundRect(x + 1, y + 1, keyWidth - 2, keyHeight - 2, 5, TFT_LIGHTGREY);
      tft.setTextColor(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextFont(4);
      tft.setTextSize(1);  // Set text size to 1
      tft.drawString(String(topRow[col]), x + keyWidth / 2, y + keyHeight / 2);
    }

    // Draw letter keys
    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < 10; col++) {
        int x = col * keyWidth;
        int y = (row + 1) * keyHeight + 100;
        tft.fillRoundRect(x + 1, y + 1, keyWidth - 2, keyHeight - 2, 5, TFT_LIGHTGREY);
        tft.setTextColor(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);  // Set text size to 1
        tft.drawString(String(keys[row][col]), x + keyWidth / 2, y + keyHeight / 2);
      }
    }

    // Draw special keys
    tft.fillRoundRect(1, tft.height() - keyHeight + 1, keyWidth * 2 - 2, keyHeight - 2, 5, shiftPressed ? TFT_LIGHTBLUE : TFT_LIGHTGREY);
    tft.drawString("Shift", keyWidth, tft.height() - keyHeight / 2);

    tft.fillRoundRect(keyWidth * 2 + 1, tft.height() - keyHeight + 1, keyWidth * 6 - 2, keyHeight - 2, 5, TFT_LIGHTGREY);
    tft.drawString("Space", tft.width() / 2, tft.height() - keyHeight / 2);

    tft.fillRoundRect(keyWidth * 8 + 1, tft.height() - keyHeight + 1, keyWidth - 2, keyHeight - 2, 5, capsLockOn ? TFT_LIGHTBLUE : TFT_LIGHTGREY);
    tft.drawString("^", keyWidth * 8.5, tft.height() - keyHeight / 2);

    tft.fillRoundRect(keyWidth * 9 + 1, tft.height() - keyHeight + 1, keyWidth - 2, keyHeight - 2, 5, TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("C", keyWidth * 9.5, tft.height() - keyHeight / 2);
  }

  // Draw "OK" button
  tft.fillRoundRect(tft.width() - 60, 5, 55, 30, 5, TFT_GREEN);
  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);  // Set text size to 1
  tft.drawString("OK", tft.width() - 32, 20);
}
// end of drawKeyboard()

char handleKeyboardTouch(int16_t x, int16_t y, bool numeric = false) {
  int keyWidth = tft.width() / 10;
  int keyHeight = (tft.height() - 100) / 5;

  // Check if "OK" button is pressed
  if (x > tft.width() - 60 && x < tft.width() - 5 && y > 5 && y < 35) {
    return '\n';  // Return newline to indicate "OK" was pressed
  }

  if (numeric) {
    int numKeyWidth = tft.width() / 4;
    for (int i = 0; i < 16; i++) {
      int kx = (i % 4) * numKeyWidth;
      int ky = (i / 4) * keyHeight + 100;
      if (x >= kx && x < kx + numKeyWidth && y >= ky && y < ky + keyHeight) {
        return numericKeys[i][0];  // Return the first character of the string
      }
    }
  } else {
    // Handle number/symbol row
    if (y >= 100 && y < 100 + keyHeight) {
      int col = x / keyWidth;
      if (col >= 0 && col < 10) {
        char key = shiftPressed ? symbolRow[col] : numberRow[col];
        if (shiftPressed && !capsLockOn) {
          shiftPressed = false;
          drawKeyboard(numeric);
        }
        return key;
      }
    }

    // Handle letter keys
    for (int row = 0; row < 3; row++) {
      int ky = (row + 1) * keyHeight + 100;
      if (y >= ky && y < ky + keyHeight) {
        int col = x / keyWidth;
        if (col >= 0 && col < 10) {
          char key = (shiftPressed || capsLockOn) ? upperKeys[row][col] : lowerKeys[row][col];
          if (shiftPressed && !capsLockOn) {
            shiftPressed = false;
            drawKeyboard(numeric);
          }
          return key;
        }
      }
    }

    // Handle special keys
    if (y >= tft.height() - keyHeight) {
      if (x < keyWidth * 2) {  // Shift
        shiftPressed = !shiftPressed;
        drawKeyboard(numeric);
        return '\0';
      } else if (x >= keyWidth * 2 && x < keyWidth * 8) {  // Space
        return ' ';
      } else if (x >= keyWidth * 8 && x < keyWidth * 9) {  // Caps Lock
        capsLockOn = !capsLockOn;
        drawKeyboard(numeric);
        return '\0';
      } else if (x >= keyWidth * 9) {  // Clear
        return '#';
      }
    }
  }

  return '\0';
}
//end of handleKeyboardTouch function

String editText(String initialText, bool numeric, int maxLength, const char* editType) {
  String currentText = initialText;
  int cursorPosition = currentText.length();
  unsigned long lastKeyPressTime = 0;
  const unsigned long editKeyDebounceTime = 200;  // 200ms debounce time for edit function

  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("Edit " + String(editType) + ": ", 10, 10);
  drawKeyboard(numeric);

  updateEditDisplayText(currentText, cursorPosition);

  while (true) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      int16_t tx = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_WIDTH);
      int16_t ty = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_HEIGHT);
      
      // Apply calibration
      tx = TOUCH_X_SCALE * tx + TOUCH_X_OFFSET;
      ty = TOUCH_Y_SCALE * ty + TOUCH_Y_OFFSET;

      unsigned long currentTime = millis();
      if (currentTime - lastKeyPressTime > KEY_DEBOUNCE_TIME) {
        char key = handleKeyboardTouch(tx, ty, numeric);
        if (key != '\0') {
          lastKeyPressTime = currentTime;
          if (key == '<') {
            if (cursorPosition > 0) {
              currentText.remove(cursorPosition - 1, 1);
              cursorPosition--;
            }
          } else if (key == '>') {
            if (cursorPosition < currentText.length()) {
              cursorPosition++;
            }
          } else if (key == '#') {
            currentText = "";
            cursorPosition = 0;
          } else if (key == '\n') {  // OK button pressed
            return currentText;
          } else {
            if (currentText.length() < maxLength) {
              currentText = currentText.substring(0, cursorPosition) + key + currentText.substring(cursorPosition);
              cursorPosition++;
            }
          }
          updateEditDisplayText(currentText, cursorPosition);
        }
      }
    }
    delay(10);  // Small delay to prevent CPU hogging
  }
}
//end of editText function

void updateEditDisplayText(const String& text, int cursorPosition) {
  tft.fillRect(10, 40, tft.width() - 20, 30, bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);

  String displayText = text.substring(0, cursorPosition) + "|" + text.substring(cursorPosition);
  tft.drawString(displayText, 10, 40);
}
//end of updateEditDisplayText function

void editTargetDate() {
  String newDate;
  do {
    newDate = editText(targetOptions[editingTargetIndex].target, true, 10, "Date (DD/MM/YYYY)");
    if (!isValidDate(newDate)) {
      displayMessage("Invalid date format", "Please use DD/MM/YYYY", TFT_RED, 1000);
    }
  } while (!isValidDate(newDate));

  strncpy(targetOptions[editingTargetIndex].target, newDate.c_str(), sizeof(targetOptions[editingTargetIndex].target) - 1);
  targetOptions[editingTargetIndex].target[sizeof(targetOptions[editingTargetIndex].target) - 1] = '\0';
  saveTargetDates();
  displayEditTargetScreen();
}
//end of editTargetDate function

void editTargetTime() {
  String newTime;
  do {
    newTime = editText(targetOptions[editingTargetIndex].trgtTime, true, 5, "Time (HH:MM)");
    if (!isValidTime(newTime)) {
      displayMessage("Invalid time format", "Please use HH:MM", TFT_RED, 1000);
    }
  } while (!isValidTime(newTime));

  strncpy(targetOptions[editingTargetIndex].trgtTime, newTime.c_str(), sizeof(targetOptions[editingTargetIndex].trgtTime) - 1);
  targetOptions[editingTargetIndex].trgtTime[sizeof(targetOptions[editingTargetIndex].trgtTime) - 1] = '\0';
  saveTargetDates();
  displayEditTargetScreen();
}
//end of editTargetTime function

void editTargetReason() {
  String newReason = editText(targetOptions[editingTargetIndex].reason, false, 29, "Reason");
  strncpy(targetOptions[editingTargetIndex].reason, newReason.c_str(), sizeof(targetOptions[editingTargetIndex].reason) - 1);
  targetOptions[editingTargetIndex].reason[sizeof(targetOptions[editingTargetIndex].reason) - 1] = '\0';
  saveTargetDates();
  displayEditTargetScreen();
}
//end of editTargetReason function

void deleteTarget() {
  if (editingTargetIndex >= 0 && editingTargetIndex < MAX_TARGET_OPTIONS) {
    memset(&targetOptions[editingTargetIndex], 0, sizeof(TargetOption));
    for (int i = editingTargetIndex; i < MAX_TARGET_OPTIONS - 1; i++) {
      if (strlen(targetOptions[i + 1].target) > 0) {
        memcpy(&targetOptions[i], &targetOptions[i + 1], sizeof(TargetOption));
        memset(&targetOptions[i + 1], 0, sizeof(TargetOption));
      } else {
        break;
      }
    }
    saveTargetDates();
    currentScreenState = TARGET_SCREEN;
    displayTargetScreen();
  }
}
//end of deleteTarget function

void saveTargetDates() {
  preferences.begin("countdown", false);
  for (int i = 0; i < MAX_TARGET_OPTIONS; i++) {
    String key = "target_" + String(i);
    String value = String(targetOptions[i].trgtTime) + "," + String(targetOptions[i].target) + "," + String(targetOptions[i].reason);
    preferences.putString(key.c_str(), value);
  }
  preferences.putInt("selectedTarget", selectedTargetIndex);
  preferences.end();

  isRemainingTimeHighlighted = true;
  remainingTimeHighlightStart = millis();
}
//end of saveTargetDates function

void loadTargetDates() {
  preferences.begin("countdown", true);
  bool hasData = false;
  for (int i = 0; i < MAX_TARGET_OPTIONS; i++) {
    String key = "target_" + String(i);
    String value = preferences.getString(key.c_str(), "");
    if (value.length() > 0) {
      hasData = true;
      int firstComma = value.indexOf(',');
      int secondComma = value.indexOf(',', firstComma + 1);
      strncpy(targetOptions[i].trgtTime, value.substring(0, firstComma).c_str(), sizeof(targetOptions[i].trgtTime) - 1);
      strncpy(targetOptions[i].target, value.substring(firstComma + 1, secondComma).c_str(), sizeof(targetOptions[i].target) - 1);
      strncpy(targetOptions[i].reason, value.substring(secondComma + 1).c_str(), sizeof(targetOptions[i].reason) - 1);
      targetOptions[i].trgtTime[sizeof(targetOptions[i].trgtTime) - 1] = '\0';
      targetOptions[i].target[sizeof(targetOptions[i].target) - 1] = '\0';
      targetOptions[i].reason[sizeof(targetOptions[i].reason) - 1] = '\0';
    } else if (!hasData) {
      strncpy(targetOptions[i].trgtTime, targetOptions[i].trgtTime, sizeof(targetOptions[i].trgtTime));
      strncpy(targetOptions[i].target, targetOptions[i].target, sizeof(targetOptions[i].target));
      strncpy(targetOptions[i].reason, targetOptions[i].reason, sizeof(targetOptions[i].reason));
    }
  }
  selectedTargetIndex = preferences.getInt("selectedTarget", 0);
  preferences.end();

  targetDate = String(targetOptions[selectedTargetIndex].target);
  targetTime = String(targetOptions[selectedTargetIndex].trgtTime);
}
//end of loadTargetDates function

int getDaysInMonth(int month, int year) {
  static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return daysInMonth[month - 1];
}
//end of getDaysInMonth function

void drawTargetOptions(int highlightedIndex) {
  tft.fillRect(8, 45, 304, 180, bgColor);
  for (int i = 0; i < 5; i++) {
    String displayText = String(targetOptions[i].target) + " - " + String(targetOptions[i].reason);
    drawButton(10, 50 + i * 35, 300, 30, i == highlightedIndex ? TFT_YELLOW : TFT_LIGHTGREY, TFT_BLACK, 5, displayText.c_str());
  }
  tft.setTextFont(4);
  tft.setTextSize(1);
}
//end of drawTargetOptions function

void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text) {
  tft.fillRoundRect(x, y, w, h, radius, boxColor);
  tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, radius + 2, TFT_DARKGREY);
  tft.setTextColor(textColor, boxColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.drawString(text, x + w / 2, y + h / 2);
}
//end of drawButton function

bool isValidDate(const String& date) {
  int day, month, year;
  if (sscanf(date.c_str(), "%d/%d/%d", &day, &month, &year) != 3) {
    return false;
  }
  if (month < 1 || month > 12) {
    return false;
  }
  int maxDays = getDaysInMonth(month, year);
  if (day < 1 || day > maxDays) {
    return false;
  }
  return true;
}
//end of isValidDate function

bool isValidTime(const String& time) {
  int hour, minute;
  if (sscanf(time.c_str(), "%d:%d", &hour, &minute) != 2) {
    return false;
  }
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return false;
  }
  return true;
}
//end of isValidTime function

bool connectToWiFi() {
  wifiIntentionallyDisconnected = false;

  Serial.println("Attempting to connect to WiFi...");
  displayMessage("Connecting to WiFi...", "", TFT_WHITE, 1000);

  int numNetworks = WiFi.scanNetworks();

  for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
    if (strlen(options[i].name) > 0) {
      bool ssidFound = false;

      for (int j = 0; j < numNetworks; j++) {
        if (strcmp(WiFi.SSID(j).c_str(), options[i].name) == 0) {
          ssidFound = true;
          break;
        }
      }

      if (ssidFound) {
        Serial.printf("Attempting to connect to %s\n", options[i].name);
        WiFi.begin(options[i].name, options[i].password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.printf("\nConnected to %s\n", options[i].name);
          displayMessage("Connected to", String(options[i].nickname).c_str(), TFT_GREEN, 2000);
          updateWiFiSymbol();
          return true;
        } else {
          Serial.printf("\nFailed to connect to %s\n", options[i].name);
        }
      }
    }
  }

  Serial.println("Failed to connect to any known network");
  displayMessage("No known networks available", "", TFT_RED, 2000);
  updateWiFiSymbol();
  return false;
}
//end of connectToWiFi function

void displayWiFiNickname() {
  tft.setTextFont(2);
  tft.setTextSize(1);
  String displayText;

  if (WiFi.status() == WL_CONNECTED) {
    String connectedSSID = WiFi.SSID();
    String nickname = "";

    for (int i = 0; i < MAX_WIFI_OPTIONS; i++) {
      if (strcmp(options[i].name, connectedSSID.c_str()) == 0) {
        nickname = options[i].nickname;
        break;
      }
    }

    if (nickname.length() == 0) {
      nickname = connectedSSID;
    }

    tft.setTextColor(dateColor, bgColor);
    displayText = "Connected to: " + nickname;

    int16_t textWidth = tft.textWidth(displayText);
    tft.setCursor((tft.width() - textWidth) / 2, 5);
    tft.println(displayText);
  }
}
//end of displayWiFiNickname function

void drawWiFiSymbol(int16_t x, int16_t y, uint16_t color) {
  tft.fillRect(x, y + 8, 3, 6, color);
  tft.fillRect(x + 5, y + 4, 3, 10, color);
  tft.fillRect(x + 10, y, 3, 14, color);
}
//end of drawWiFiSymbol function

void drawBatterySymbol(int16_t x, int16_t y, int level) {
  uint16_t color = getBatteryColor(level);
  int batteryWidth = 50;
  int batteryHeight = 20;
  int innerWidth = 44;
  int innerHeight = 16;
  int filledWidth = map(level, 0, 100, 0, innerWidth);

  tft.drawRect(x, y, batteryWidth, batteryHeight, 0x5B2D);
  tft.fillRect(x + batteryWidth, y + 4, 3, batteryHeight - 8, 0x5B2D);

  tft.fillRect(x + 2, y + 2, innerWidth, innerHeight, bgColor);

  if (filledWidth > 0) {
    tft.fillRect(x + 2, y + 2, filledWidth, innerHeight, color);
  }

  tft.setTextColor(TFT_GOLD, color);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString(String(level) + "%", x - 1 + batteryWidth / 2, y + batteryHeight / 2);
}
//end of drawBatterySymbol function

uint16_t getBatteryColor(int level) {
  if (level > 75) return TFT_DARKGREEN;
  if (level > 50) return TFT_YELLOW;
  if (level > 25) return TFT_ORANGE;
  return TFT_RED;
}
//end of getBatteryColor function

int getBatteryLevel() {
  const int NUM_SAMPLES = 10;
  int rawValue = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    rawValue += analogRead(BATTERY_PIN);
    delay(10);
  }
  rawValue /= NUM_SAMPLES;

  float voltage = (rawValue / 4095.0) * 2 * 3.3 * calibrationFactor;

  if (voltage < 1.0) {
    return -1;  // No battery present
  }

  const float MIN_VOLTAGE = 3.5;
  const float MAX_VOLTAGE = 4.2;
  voltage = constrain(voltage, MIN_VOLTAGE, MAX_VOLTAGE);
  int percentage = round((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100);
  return constrain(percentage, 0, 100);
}
//end of getBatteryLevel function

void displayLowBatteryWarning() {
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("LOW BATTERY!", tft.width() / 2, tft.height() / 2 - 20);
  tft.drawString("Please charge soon", tft.width() / 2, tft.height() / 2 + 20);
  delay(2000);
  updateDisplay();
}
//end of displayLowBatteryWarning function

void updateWiFiSymbol() {
  if (WiFi.status() == WL_CONNECTED) {
    drawWiFiSymbol(8, 0, TFT_DARKGREEN);
    wifiIntentionallyDisconnected = false;
    wifiSymbolVisible = true;
  } else if (wifiIntentionallyDisconnected) {
    drawWiFiSymbol(8, 0, bgColor);
    wifiSymbolVisible = false;
  } else {
    drawWiFiSymbol(8, 0, TFT_RED);
    wifiSymbolVisible = true;
  }
}
//end of updateWiFiSymbol function

bool attemptWiFiConnection(const char* ssid, const char* password, const char* nickname) {
  displayMessage("Connecting to WiFi...", "", TFT_WHITE, 1000);
  Serial.printf("Attempting to connect to %s\n", ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected to %s\n", ssid);
    lastConnectedSSID = ssid;
    wifiSymbolVisible = true;
    displayMessage("Connected to", nickname, TFT_GREEN, 2000);
    return true;
  } else {
    Serial.printf("\nFailed to connect to %s\n", ssid);
    displayMessage("Failed to connect to", nickname, TFT_RED, 1000);
    return false;
  }
}
//end of attemptWiFiConnection function
void setUserBacklight(int level) {
  userSetBacklight = constrain(level, minBacklight, 255);
  adjustBacklight();
}
//end of setUserBacklight function

void setBacklight(int intensity) {
  intensity = constrain(intensity, minBacklight, 255);
  analogWrite(TFT_BL, intensity);
  backlightIntensity = intensity;
}
//end of setBacklight function

void adjustBacklight() {
  int ldrValue = analogRead(LDR_PIN);
  int newIntensity;

  newIntensity = map(ldrValue, 1000, 0, minBacklight, userSetBacklight);  // dimmer in dark
                                                                          // newIntensity = map(ldrValue, 1000, 0, userSetBacklight, minBacklight);// dimmer in light


  newIntensity = constrain(newIntensity, minBacklight, userSetBacklight);
  backlightIntensity = (backlightIntensity + 7 * newIntensity) / 8;
  setBacklight(backlightIntensity);
}
//end of adjustBacklight function

/*
void drawTimeUpdateArea() {
  tft.drawRect(tft.width() - 55, ct + 95, 55, 25, boxColor);
}
//end of drawTimeUpdateArea function*/

void handleTimeUpdateTouch() {
  displayMessage("Updating Time...", "", TFT_WHITE, 1000);

  if (attemptWiFiConnectionWithRetry()) {
    if (updateTimeAndDisconnect()) {
      displayMessage("Time updated successfully", "", TFT_GREEN, 2000);
      timeSet = true;
    } else {
      displayMessage("Time update failed", "", TFT_RED, 2000);
      if (!timeSet) {
        displayMessage("Time not set", "Using last known time", TFT_YELLOW, 2000);
      }
    }
  } else {
    displayMessage("WiFi connection failed", "Unable to update time", TFT_RED, 2000);
  }

  delay(2000);
  currentScreenState = MAIN_SCREEN;
  updateMainScreen();
}
//end of handleTimeUpdateTouch function

void loadAlarms() {
  preferences.begin("alarms", true);
  for (int i = 0; i < MAX_ALARMS; i++) {
    String key = "alarm_" + String(i);
    String value = preferences.getString(key.c_str(), "");
    if (value.length() > 0) {
      sscanf(value.c_str(), "%d:%d,%d,%s", &alarms[i].hour, &alarms[i].minute, &alarms[i].enabled, alarms[i].sound);
    } else {
      alarms[i] = { 0, 0, false, "beep" };  // Default values
    }
  }
  preferences.end();
}
//end of loadAlarms function

void saveAlarms() {
  preferences.begin("alarms", false);
  for (int i = 0; i < MAX_ALARMS; i++) {
    String key = "alarm_" + String(i);
    String value = String(alarms[i].hour) + ":" + String(alarms[i].minute) + "," + String(alarms[i].enabled) + "," + String(alarms[i].sound);
    preferences.putString(key.c_str(), value);
  }
  preferences.end();
}
//end of saveAlarms function

void checkAlarms() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    for (int i = 0; i < MAX_ALARMS; i++) {
      if (alarms[i].enabled) {
        if (timeinfo.tm_hour == alarms[i].hour && timeinfo.tm_min == alarms[i].minute && timeinfo.tm_sec == 0) {
          activeAlarmIndex = i;
          alarmActivationTime = millis();
          currentScreenState = ALARM_ACTIVE_SCREEN;
          return;
        }
      }
    }
  }
}
//end of checkAlarms function

void playStarTrekAlarm() {
  for (int i = 0; i < 3; i++) {
    for (int freq = 500; freq < 2000; freq += 20) {
      tone(BUZZER_PIN, freq, 10);
      delay(10);
    }
    noTone(BUZZER_PIN);
    delay(100);
  }
}
//end of playStarTrekAlarm function

void displayAlarmScreen() {
  if (currentScreenState == ALARM_ACTIVE_SCREEN) {
    unsigned long currentTime = millis();
    bool isRedPhase = ((currentTime - alarmActivationTime) / 1000) % 2 == 0;

    tft.fillScreen(isRedPhase ? TFT_RED : bgColor);
    tft.setTextColor(isRedPhase ? bgColor : TFT_RED);
    tft.setTextFont(4);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);

    tft.drawString("ALARM FOR", tft.width() / 2, tft.height() / 3);
    tft.drawString(String(alarms[activeAlarmIndex].hour) + ":" + (alarms[activeAlarmIndex].minute < 10 ? "0" : "") + String(alarms[activeAlarmIndex].minute),
                   tft.width() / 2, tft.height() / 2);

    tft.setTextSize(1);
    tft.drawString("Touch screen to dismiss", tft.width() / 2, tft.height() * 2 / 3);
  } else {
    tft.fillScreen(bgColor);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.drawString("Alarms", tft.width() / 2, 20);

    drawButton(5, 5, 50, 30, TFT_BLUE, TFT_WHITE, 5, "Back");

    for (int i = 0; i < MAX_ALARMS; i++) {
      String alarmText = String(alarms[i].hour) + ":" + (alarms[i].minute < 10 ? "0" : "") + String(alarms[i].minute);
      drawButton(10, 50 + i * 40, 200, 35, TFT_LIGHTGREY, TFT_BLACK, 5, alarmText.c_str());

      drawToggleSwitch(220, 50 + i * 40 + 5, alarms[i].enabled);
    }
  }
}
//end of displayAlarmScreen function

void handleAlarmScreenTouch(int16_t x, int16_t y) {
  if (currentScreenState == ALARM_ACTIVE_SCREEN) {
    stopAlarm();
    return;
  }

  if (x < 55 && y < 35) {
    updateMainScreen();
    return;
  }

  for (int i = 0; i < MAX_ALARMS; i++) {
    if (y > 50 + i * 40 && y < 85 + i * 40) {
      if (x < 210) {
        editAlarm(i);
      } else if (x > 220 && x < 270) {
        toggleAlarm(i);
      }
      return;
    }
  }
}
//end of handleAlarmScreenTouch function

void drawAlarmIcon(int x, int y) {
  uint16_t darkGrey = tft.color565(64, 64, 64);
  tft.drawCircle(x, y + 5, 7, darkGrey);
  tft.drawLine(x, y + 5, x + 3, y + 3, darkGrey);
  tft.drawLine(x, y + 5, x, y - 5, darkGrey);
}
//end of drawAlarmIcon function

void editAlarm(int index) {
  String newTime = editText(String(alarms[index].hour) + ":" + String(alarms[index].minute), true, 5, "Alarm Time (HH:MM)");
  if (isValidTime(newTime)) {
    sscanf(newTime.c_str(), "%d:%d", &alarms[index].hour, &alarms[index].minute);
    alarms[index].enabled = true;
    strcpy(alarms[index].sound, "beep");
    saveAlarms();
  }
  currentScreenState = ALARM_SCREEN;
  displayAlarmScreen();
}
//end of editAlarm function

void updateMainScreen() {
  currentScreenState = MAIN_SCREEN;
  tft.fillScreen(bgColor);
  displayTimeOnTFT(timeColor, bgColor, true);
  displayCountdownBoxes();
  displayCountdownLabels();
  calculateTimeDifference();
  displayCountdownValues();
  displayCountdownOnTFT();
  displayWiFiNickname();
  updateWiFiSymbol();
  updateBatteryDisplay();
  drawAlarmIcon(10, ct + 105);
  // Add this at the end of the function
  drawButton(290, ct + 97, 25, 25, TFT_BLACK, TFT_BLACK, 5, "Cal");
}

//end of updateMainScreen function

void stopAlarm() {
  noTone(BUZZER_PIN);
  currentScreenState = MAIN_SCREEN;
  updateMainScreen();
}
//end of stopAlarm function

void updateBatteryDisplay() {
  batteryLevel = getBatteryLevel();

  if (batteryLevel < 0) {
    tft.fillRect(265, 0, 50, 20, bgColor);
  } else {
    drawBatterySymbol(265, 0, batteryLevel);
  }
}
//end of updateBatteryDisplay function

bool attemptWiFiConnectionWithRetry() {
  wifiAttempts = 0;
  while (wifiAttempts < MAX_WIFI_ATTEMPTS) {
    if (connectToWiFi()) {
      return true;
    }
    wifiAttempts++;
    displayMessage("WiFi connection failed",
                   String("Attempt " + String(wifiAttempts) + "/" + String(MAX_WIFI_ATTEMPTS)).c_str(),
                   TFT_RED,
                   1000);
    delay(WIFI_RETRY_DELAY);
  }
  return false;
}
//end of attemptWiFiConnectionWithRetry function

void handleBacklightControl(int16_t x, int16_t y) {
  if (y >= SLIDER_Y - KNOB_RADIUS && y <= SLIDER_Y + SLIDER_HEIGHT + KNOB_RADIUS) {
    if (x >= SLIDER_X - KNOB_RADIUS && x <= SLIDER_X + SLIDER_WIDTH + KNOB_RADIUS) {
      int newValue = map(x - SLIDER_X, 0, SLIDER_WIDTH, minBacklight, 255);
      userSetBacklight = constrain(newValue, minBacklight, 255);

      preferences.begin("settings", false);
      preferences.putInt("userSetBacklight", userSetBacklight);
      preferences.end();

      drawSlider(userSetBacklight);

      adjustBacklight();
    }
  } else if (y > tft.height() - 50 && x < 70) {
    updateMainScreen();
  }
}
//end of handleBacklightControl function

void displayBacklightControl() {
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Backlight Control", tft.width() / 2, 20);

  drawSlider(userSetBacklight);

  drawButton(10, tft.height() - 50, 60, 40, TFT_BLUE, TFT_WHITE, 5, "Back");
}
//end of displayBacklightControl function

void drawSlider(int value) {
  int sliderPosition = map(value, minBacklight, maxBacklight, 0, SLIDER_WIDTH);

  tft.fillRect(SLIDER_X, SLIDER_Y, SLIDER_WIDTH, SLIDER_HEIGHT, TFT_DARKGREY);
  tft.fillRect(SLIDER_X, SLIDER_Y, sliderPosition, SLIDER_HEIGHT, TFT_BLUE);
  tft.fillRect(SLIDER_X - KNOB_RADIUS, SLIDER_Y - KNOB_RADIUS - 40,
               SLIDER_WIDTH + KNOB_RADIUS * 2, SLIDER_HEIGHT + KNOB_RADIUS * 2 + 40, bgColor);
  tft.fillCircle(SLIDER_X + sliderPosition, SLIDER_Y + SLIDER_HEIGHT / 2, KNOB_RADIUS, TFT_WHITE);
  tft.drawCircle(SLIDER_X + sliderPosition, SLIDER_Y + SLIDER_HEIGHT / 2, KNOB_RADIUS, TFT_DARKGREY);

  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(4);
  tft.setTextSize(1);
  tft.drawString("Max Backlight: " + String(value), tft.width() / 2, SLIDER_Y - 40);
}
//end of drawSlider function

void toggleAlarm(int index) {
  alarms[index].enabled = !alarms[index].enabled;
  saveAlarms();
  drawToggleSwitch(220, 50 + index * 40 + 5, alarms[index].enabled);
}
//end of toggleAlarm function

void drawToggleSwitch(int x, int y, bool state) {
  int width = 50;
  int height = 25;

  tft.drawRoundRect(x, y, width, height, height / 2, TFT_WHITE);
  if (state) {
    tft.fillRoundRect(x + width / 2, y, width / 2, height, height / 2, TFT_GREEN);
    tft.fillCircle(x + width - height / 2, y + height / 2, height / 2 - 2, TFT_WHITE);
  } else {
    tft.fillRoundRect(x, y, width / 2, height, height / 2, TFT_DARKGREY);
    tft.fillCircle(x + height / 2, y + height / 2, height / 2 - 2, TFT_WHITE);
  }
}
//end of drawToggleSwitch function

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.invertDisplay(invert);// comment out if std CYD
  tft.setRotation(rot);
  tft.fillScreen(bgColor);

  pinMode(BATTERY_PIN, INPUT);
  pinMode(TFT_BL, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setUserBacklight(125);

  preferences.begin("settings", false);
  userSetBacklight = preferences.getInt("userSetBacklight", 125);
  preferences.end();

  preferences.begin("calibration", true);
  TOUCH_X_OFFSET = preferences.getInt("x_offset", 0);
  TOUCH_Y_OFFSET = preferences.getInt("y_offset", 0);
  preferences.end();

  Serial.printf("Loaded calibration: X_OFFSET=%d, Y_OFFSET=%d\n", TOUCH_X_OFFSET, TOUCH_Y_OFFSET);

  adjustBacklight();

  loadAlarms();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  preferences.begin("countdown", false);
  loadTargetDates();
  preferences.end();


  // Reset calibration to defaults
  TOUCH_X_SCALE = 1.0;
  TOUCH_Y_SCALE = 1.0;
  TOUCH_X_OFFSET = 0;
  TOUCH_Y_OFFSET = 0;

  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(rot);

  copyright = 1;
  displayMessage("Welcome to The Timer", "(C) ac30kev 2024. Rev 4.5.1", welcomeColor, 2000);
  delay(2000);

  initializeWiFiOptions();

  if (!timeSet) {
    if (attemptWiFiConnectionWithRetry()) {
      if (updateTimeAndDisconnect()) {
        updateMainScreen();
      } else {
        displayMessage("Time not set", "Proceeding anyway", TFT_YELLOW, 1000);
        delay(2000);
        updateMainScreen();
      }
    } else {
      currentScreenState = WIFI_SCREEN;
      displayWiFiOptions();
    }
  } else {
    updateMainScreen();
  }

  Serial.println("Countdown Timer v4.5.1");
}
//end of setup function

void loop() {
  unsigned long currentMillis = millis();

  static unsigned long lastUpdateTime = 0;
  static unsigned long lastAlarmCheck = 0;
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastBatteryUpdate = 0;
  static unsigned long lastBacklightUpdate = 0;

  // Always run the countdown in the background
  if (currentMillis - lastCountdownUpdate >= 1000) {
    lastCountdownUpdate = currentMillis;
    calculateTimeDifference();
  }

  if (currentMillis - lastAlarmCheck >= 1000) {
    lastAlarmCheck = currentMillis;
    checkAlarms();
  }

  if (isRemainingTimeHighlighted && currentMillis - remainingTimeHighlightStart > REMAINING_TIME_HIGHLIGHT_DURATION) {
    isRemainingTimeHighlighted = false;
    displayCountdownBoxes();
    displayCountdownLabels();
    displayCountdownValues();
  }

  // Only update the display if not in calibration mode
  if (currentScreenState == MAIN_SCREEN && !calibrationMode) {
    if (currentMillis - lastDisplayUpdate >= 1000) {
      lastDisplayUpdate = currentMillis;
      updateDisplay();
    }


    if (currentMillis - lastBacklightUpdate >= 1000) {
      lastBacklightUpdate = currentMillis;
      adjustBacklight();
    }

    if (currentMillis - lastUpdateTime >= 10000) {
      lastUpdateTime = currentMillis;
      logSystemStatus();
    }

    if (currentMillis - lastBatteryUpdate >= 60000) {
      lastBatteryUpdate = currentMillis;
      updateBatteryDisplay();

      int batteryLevel = getBatteryLevel();
      if (batteryLevel > 0) {
        float voltage = (analogRead(BATTERY_PIN) / 4095.0) * 2 * 3.3 * calibrationFactor;
        Serial.printf("Battery Raw: %d | Voltage: %.2fV | Percentage: %d%%\n",
                      analogRead(BATTERY_PIN), voltage, batteryLevel);
      } else {
        Serial.println("No battery detected");
      }
    }

    if (currentScreenState == ALARM_ACTIVE_SCREEN) {
      if (ts.touched()) {
        stopAlarm();
      } else if (currentMillis - alarmActivationTime >= 60000) {  // 60 seconds
        stopAlarm();
      } else {
        playStarTrekAlarm();
      }
    }
  }

  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    Serial.printf("Raw touch: x=%d, y=%d\n", p.x, p.y);
    
    // Map raw touch coordinates to screen coordinates
    int16_t tx = map(p.x, 200, 3800, 0, tft.width());
    int16_t ty = map(p.y, 200, 3800, 0, tft.height());
    
    // Apply calibration
    tx = TOUCH_X_SCALE * tx + TOUCH_X_OFFSET;
    ty = TOUCH_Y_SCALE * ty + TOUCH_Y_OFFSET;
    
    Serial.printf("Mapped and calibrated touch: x=%d, y=%d\n", tx, ty);
    Serial.printf("Current screen state: %d\n", currentScreenState);
    
    if (calibrationMode) {
      handleCalibrationTouch(tx, ty);
    } else {
      handleTouch(tx, ty);
    }
  }


  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (timeinfo.tm_hour == 3 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) {
      if (attemptWiFiConnectionWithRetry()) {
        if (updateTimeAndDisconnect()) {
          displayMessage("Time updated at 3 AM", "Returning to main screen", TFT_GREEN, 1000);
        } else {
          displayMessage("3 AM time update failed", "Using last known time", TFT_YELLOW, 1000);
        }
      } else {
        displayMessage("WiFi connection failed", "Skipping 3 AM time update", TFT_RED, 1000);
      }
      delay(2000);
      updateMainScreen();
    }
  }

  delay(10);  // Small delay to prevent CPU hogging
}

//end of loop function

void logSystemStatus() {
  int ldrValue = analogRead(LDR_PIN);
  int batteryRaw = analogRead(BATTERY_PIN);
  float voltage = (batteryRaw / 4095.0) * 2 * 3.3 * calibrationFactor;
  int percentage = getBatteryLevel();

  //Serial.printf("LDR: %d | Backlight: %d | User Backlight: %d | Battery Raw: %d | Voltage: %.2fV | Percentage: %d%%\n",
  //               ldrValue, backlightIntensity, userSetBacklight, batteryRaw, voltage, percentage);
}
//end of logSystemStatus function

void startCalibration() {
  currentScreenState = CALIBRATION_SCREEN;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.drawString("Calibration starting in 1 second...", tft.width() / 2, 20);
  delay(1000);  // 1-second delay
  
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Touch the highlighted cross", tft.width() / 2, 20);

  // Define 6 points for calibration
  const int16_t calibrationCoords[CALIBRATION_POINTS][2] = {
    { 20, 20 }, { tft.width() - 20, 20 }, { 20, tft.height() / 2 }, { tft.width() - 20, tft.height() / 2 }, { 20, tft.height() - 40 }, { tft.width() - 20, tft.height() - 40 }  // Moved up slightly
  };


  for (int i = 0; i < CALIBRATION_POINTS; i++) {
    calibrationPoints[i].screenX = calibrationCoords[i][0];
    calibrationPoints[i].screenY = calibrationCoords[i][1];
    drawCross(calibrationPoints[i].screenX, calibrationPoints[i].screenY, i == 0 ? TFT_RED : TFT_DARKGREY);
  }

  currentCalibrationPoint = 0;
  updateCalibrationInstruction();
  Serial.println("Calibration started");
}

void drawCross(int16_t x, int16_t y, uint16_t color) {
  tft.drawLine(x - 10, y, x + 10, y, color);
  tft.drawLine(x, y - 10, x, y + 10, color);
}

void updateCalibrationInstruction() {
  tft.fillRect(0, tft.height() - 30, tft.width(), 30, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(BC_DATUM);
  tft.setTextFont(2);
  tft.drawString("Touch cross " + String(currentCalibrationPoint + 1) + " of " + String(CALIBRATION_POINTS), tft.width() / 2, tft.height() - 5);
}


void handleCalibrationTouch(int16_t x, int16_t y) {
  unsigned long currentTime = millis();
  if (currentTime - lastCalibrationTouch < CALIBRATION_DEBOUNCE_TIME) {
    return;
  }
  lastCalibrationTouch = currentTime;

  if (currentCalibrationPoint >= CALIBRATION_POINTS) {
    // Check for return button press with a larger touch area
    if (y > tft.height() - 70 && x > tft.width() - 120) {
      Serial.println("Exiting calibration mode");
      saveCalibration();
      currentCalibrationPoint = 0;
      toggleCalibrationMode();
      return;
    }
  }

  Serial.printf("Calibration touch received: x=%d, y=%d\n", x, y);

  if (currentCalibrationPoint < CALIBRATION_POINTS) {
    calibrationPoints[currentCalibrationPoint].touchX = x;
    calibrationPoints[currentCalibrationPoint].touchY = y;

    Serial.printf("Point %d: Screen(%d,%d) Touch(%d,%d)\n",
                  currentCalibrationPoint + 1,
                  calibrationPoints[currentCalibrationPoint].screenX,
                  calibrationPoints[currentCalibrationPoint].screenY,
                  x, y);

    drawCross(calibrationPoints[currentCalibrationPoint].screenX, calibrationPoints[currentCalibrationPoint].screenY, TFT_GREEN);

    currentCalibrationPoint++;

    if (currentCalibrationPoint < CALIBRATION_POINTS) {
      drawCross(calibrationPoints[currentCalibrationPoint].screenX, calibrationPoints[currentCalibrationPoint].screenY, TFT_RED);
      updateCalibrationInstruction();
    } else {
      calculateCalibration();
      displayCalibrationResults();
    }
  } else {
    Serial.printf("Calibration complete touch: x=%d, y=%d\n", x, y);
    if (y > tft.height() - 60 && x > tft.width() - 110) {
      Serial.println("Exiting calibration mode");
      saveCalibration();
      currentCalibrationPoint = 0;
      toggleCalibrationMode();
    }
  }
}

void displayCalibrationResults() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  tft.drawString("Calibration Results:", 10, 10);
  for (int i = 0; i < CALIBRATION_POINTS; i++) {
    String result = "Point " + String(i + 1) + ": ";
    result += "Screen(" + String(calibrationPoints[i].screenX) + "," + String(calibrationPoints[i].screenY) + ") ";
    result += "Touch(" + String(calibrationPoints[i].touchX) + "," + String(calibrationPoints[i].touchY) + ")";
    tft.drawString(result, 10, 30 + i * 20);
  }

  // Draw "Return to Main Screen" button
  drawButton(tft.width() - 100, tft.height() - 50, 90, 40, TFT_BLUE, TFT_WHITE, 5, "Return");
  Serial.println("Calibration completed. Touch 'Return' button to exit.");
}

void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint16_t textColor, int cornerRadius, const char* label) {
  tft.fillRoundRect(x, y, w, h, cornerRadius, color);
  tft.setTextColor(textColor);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.drawString(label, x + w / 2, y + h / 2);
}

void calculateCalibration() {
  long sumX = 0, sumY = 0;
  long sumScreenX = 0, sumScreenY = 0;
  long sumXX = 0, sumYY = 0;
  long sumXScreenX = 0, sumYScreenY = 0;

  for (int i = 0; i < CALIBRATION_POINTS; i++) {
    sumX += calibrationPoints[i].touchX;
    sumY += calibrationPoints[i].touchY;
    sumScreenX += calibrationPoints[i].screenX;
    sumScreenY += calibrationPoints[i].screenY;
    sumXX += (long)calibrationPoints[i].touchX * calibrationPoints[i].touchX;
    sumYY += (long)calibrationPoints[i].touchY * calibrationPoints[i].touchY;
    sumXScreenX += (long)calibrationPoints[i].touchX * calibrationPoints[i].screenX;
    sumYScreenY += (long)calibrationPoints[i].touchY * calibrationPoints[i].screenY;
  }

  long n = CALIBRATION_POINTS;
  float denomX = n * sumXX - sumX * sumX;
  float denomY = n * sumYY - sumY * sumY;

  if (abs(denomX) > 1e-6 && abs(denomY) > 1e-6) {
    TOUCH_X_SCALE = (float)(n * sumXScreenX - sumX * sumScreenX) / denomX;
    TOUCH_Y_SCALE = (float)(n * sumYScreenY - sumY * sumScreenY) / denomY;
    TOUCH_X_OFFSET = (sumScreenX - TOUCH_X_SCALE * sumX) / n;
    TOUCH_Y_OFFSET = (sumScreenY - TOUCH_Y_SCALE * sumY) / n;
  } else {
    // If denominator is too close to zero, use default values
    TOUCH_X_SCALE = 1.0;
    TOUCH_Y_SCALE = 1.0;
    TOUCH_X_OFFSET = 0;
    TOUCH_Y_OFFSET = 0;
    Serial.println("Warning: Calibration calculation failed. Using default values.");
  }

  Serial.printf("Calibration results: X_SCALE=%.4f, Y_SCALE=%.4f, X_OFFSET=%d, Y_OFFSET=%d\n", 
                TOUCH_X_SCALE, TOUCH_Y_SCALE, TOUCH_X_OFFSET, TOUCH_Y_OFFSET);
}
//end of calculateCalibration function

void saveCalibration() {
  preferences.begin("calibration", false);
  preferences.putFloat("x_scale", TOUCH_X_SCALE);
  preferences.putFloat("y_scale", TOUCH_Y_SCALE);
  preferences.putInt("x_offset", TOUCH_X_OFFSET);
  preferences.putInt("y_offset", TOUCH_Y_OFFSET);
  preferences.end();
  Serial.printf("Calibration saved: X_SCALE=%.4f, Y_SCALE=%.4f, X_OFFSET=%d, Y_OFFSET=%d\n",
                TOUCH_X_SCALE, TOUCH_Y_SCALE, TOUCH_X_OFFSET, TOUCH_Y_OFFSET);
}
//end of saveCalibration function

void toggleCalibrationMode() {
  calibrationMode = !calibrationMode;
  Serial.printf("Calibration mode toggled: %s\n", calibrationMode ? "ON" : "OFF");
  if (calibrationMode) {
    startCalibration();
  } else {
    updateMainScreen();
  }
}
//end of toggleCalibrationMode function
