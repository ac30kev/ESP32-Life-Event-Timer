/*
   REV 0.1 - first attempt
   REV 0.2 - slight changes to add better time display
   REV 0.3 - added config button to main display and softAP details with unique name.
   also added code for back and cfg buttons - no code yet though...
   REV 0.4 - changed main page slightly...
   REV 0.5 - working on creating the cfg page...
   REV 0.6 - making the 2nd screen for wifi call...
   REV 0.7 - added back button to all except main screen, which had cfg button (Claude)
   REV 0.7.1 - made options display properly
   REV 0.7.2 - added better functions for above
   REV 0.7.3 - checks if wifi matches, added confirm button and passed highlighted options to main code
   REV 0.7.4 - works but does not highlight chosen option - yet. all else is good
   */

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <TimeLib.h>  // Include Time library if not already included
#include <Timezone.h>
#include "converted_image.h"  // Include the converted image array

//rotation of screen and touch positions
int rot = 3;
// Replace with your network credentials
//const char* ssid = "CASWIFI";
//const char* password = "securew1f1";
const char* apSsid = "ESP32_ACCESS_POINT";
const char* apPassword = "timer@123";

// NTP and Time Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;  // Set to your time zone offset

// Define global variables
//String targetDate = "13/06/2028";  // Target date in DD/MM/YYYY format
//int countdownValues[6] = { 0 };  // {years, months, days, hours, minutes, seconds}
char formattedTargetDate[20];

// Define a function to convert formatted date to Unix timestamp
time_t formattedDateToTimestamp(const char* formattedDate) {
  struct tm tm = {};
  strptime(formattedDate, "%H:%M - %d/%m/%Y", &tm);
  time_t timestamp = mktime(&tm);
  return timestamp;
}

long countdownValues[6] = { 0 };  // Array to store countdown values {years, months, days, hours, minutes, seconds}

// Touch Screen pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Button dimensions
#define BUTTON_WIDTH 32
#define BUTTON_HEIGHT 32
#define CFG_BUTTON_X 288
#define BACK_BUTTON_X 0
#define BUTTON_Y 0

constexpr int16_t buttonDrawX1 = 15;
constexpr int16_t buttonDrawX2 = 175;
constexpr int16_t buttonDrawY = 200;
constexpr int16_t buttonDrawW = 130;
constexpr int16_t buttonDrawH = 26;
constexpr int16_t buttonTouchX1 = 320;
constexpr int16_t buttonTouchX2 = 2130;
constexpr int16_t buttonTouchY = 3250;
constexpr int16_t buttonTouchW = 1430;
constexpr int16_t buttonTouchH = 350;


// Box positions
int ct = 33;   //position of current time box
int rt = 100;  //position of remaining time box
int td = 202;  //position of target date box

//Function declarations
void setupTouchscreen();
void setupDisplay();
void setup();
void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text);
void drawConfirmButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text);
void renderMainScreen(unsigned long epochTime);
void displayConfigScreen();
void displayWiFiScreen();
void displayTargetScreen();
void displayCfgButtonData();
void displayBackButtonData();
void drawWiFiButton(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, const char* text, uint8_t fontSize, uint8_t font, bool highlighted);
void drawWiFiOptions(int highlightedIndex);
void drawTargetOptions(int highlightedIndex);
void displayTimeOnTFT(unsigned long currentTime, uint16_t textColor, uint16_t bgColor);
void displayCountdownBoxes();
void displayCountdownLabels();
void displayCountdownValues();
void displayCountdownOnTFT();
// Correct function declaration
void calculateTimeDifference(unsigned long currentTime, const char* formattedTargetDate);
int getDaysInMonth(int year, int month);
bool isLeapYear(int year);
void handleTouchEvent();
void loop();
void handleWiFiTouchEvent();
void handleTargetTouchEvent();

// Objects and instances
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, daylightOffset_sec);

// Define the DST and STD rules for your time zone
TimeChangeRule myDST = { "BST", Last, Sun, Mar, 1, 60 };  // British Summer Time (BST) rule
TimeChangeRule mySTD = { "GMT", Last, Sun, Oct, 2, 0 };   // Greenwich Mean Time (GMT) rule

// Create a Timezone object with the DST and STD rules
Timezone myTZ(myDST, mySTD);

// Enum for screen states
enum ScreenState {
  MAIN_SCREEN,
  CONFIG_SCREEN,
  WIFI_SCREEN,
  TARGET_SCREEN
};

// Struct to hold WiFi information
struct WiFiOption {
  const char* name;
  const char* password;
  const char* nickname;
};

WiFiOption options[] = {
  { "VM1459801", "qw4Hdxbrs5wx", "home" },
  { "CASWIFI", "securew1f1", "work" },
  { "GuestNet", "guestpass", "guest" },
  { "OfficeNet", "office123", "office" },
  { "CafeNet", "coffee1", "cafe" }
};

struct TargetOption {
  const char* trgtTime;
  const char* target;
  const char* reason;
};

TargetOption targetOptions[] = {
  { "00:00", "13/06/2028", "Retirement" },
  { "00:00", "25/12/2024", "Xmas!" },
  { "00:00", "13/06/2025", "When I'm 64!" },
  { "15:00", "19/05/2025", "Holiday to Spain!" },
  { "12:00", "18/09/2024", "Cruise to Turkey!" }
};

int highlightedOption = 0;  // Index of the currently highlighted option

// Current screen state
ScreenState currentScreenState = MAIN_SCREEN;

// Generic touch event handling function
void handleOptionTouchEvent() {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();

    if (currentScreenState == WIFI_SCREEN || currentScreenState == TARGET_SCREEN) {
      for (int i = 0; i < (currentScreenState == WIFI_SCREEN ? sizeof(options) / sizeof(options[0]) : sizeof(targetOptions) / sizeof(targetOptions[0])); i++) {
        int buttonY = 400 + (400 + i * 450);
        if (p.x >= 700 && p.x <= 3200 && p.y >= buttonY && p.y <= buttonY + 350) {
          highlightedOption = i;  // Store the touched option index
          if (currentScreenState == WIFI_SCREEN) {
            drawWiFiOptions(highlightedOption);
          } else {
            drawTargetOptions(highlightedOption);
          }
          delay(200);  // Debounce
          break;       // Exit loop once a button is pressed
        }
      }
    }
  }
}

// Add this global variable to store the selected WiFi option
WiFiOption selectedWiFiOption = options[0];

// Add this global variable to store the selected target date
String selectedTargetDate = targetOptions[0].target;

// New function to check if stored WiFi credentials match the local WiFi network
bool checkWiFiCredentials() {
  WiFi.begin(selectedWiFiOption.name, selectedWiFiOption.password);
  Serial.print("wifi nick = ");
  Serial.println(selectedWiFiOption.nickname);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
    delay(500);
    retryCount++;
  }
  return (WiFi.status() == WL_CONNECTED);
}

void setupTouchscreen() {
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(rot);
}

void setupDisplay() {
  tft.init();
  tft.setRotation(rot);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
}

void setup() {
  Serial.begin(115200);
  setupTouchscreen();
  setupDisplay();

  // Check if stored WiFi credentials match the local WiFi network
  if (checkWiFiCredentials()) {
    // WiFi connection successful, proceed to main screen
    Serial.print("Connected to WiFi: ");
    Serial.println(selectedWiFiOption.name);
    currentScreenState = MAIN_SCREEN;
  } else {
    // WiFi connection failed, display WiFi screen
    Serial.println("WiFi connection failed");
    Serial.print("wifi nick = ");
    Serial.println(selectedWiFiOption.nickname);
    displayWiFiScreen();
  }
  /*
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
  }
  Serial.println("Connected to WiFi");
  */

  // Initialize NTP client
  timeClient.begin();
  /* //set up later
  // Set up soft AP
  uint64_t chipId = ESP.getEfuseMac();
  String apSsidUnique = "ESP32_" + String(chipId, HEX);
  WiFi.softAP(apSsidUnique.c_str(), apPassword);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP SSID: ");
  Serial.println(apSsidUnique);
  Serial.print("AP IP address: ");
  Serial.println(apIP);
}
*/
}

void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text) {
  tft.fillRoundRect(x, y, w, h, radius, boxColor);
  tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, radius + 2, TFT_DARKGREY);
  tft.setTextColor(textColor, boxColor);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void drawConfirmButton(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t boxColor, uint16_t textColor, uint8_t radius, const char* text) {
  tft.fillRoundRect(x, y, w, h, radius, boxColor);
  tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, radius + 2, TFT_DARKGREY);
  tft.setTextColor(textColor, boxColor);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void renderMainScreen(unsigned long epochTime) {
  displayCfgButtonData();
  displayTimeOnTFT(epochTime, TFT_VIOLET, TFT_BLACK);
  displayCountdownBoxes();
  calculateTimeDifference(epochTime, formattedTargetDate);
  displayCountdownLabels();
  displayCountdownValues();
  displayCountdownOnTFT();
}

void displayConfigScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Config Screen", tft.width() / 2, 50);
  displayBackButtonData();  // Display back button symbol
  drawButton(buttonDrawX1, buttonDrawY, buttonDrawW, buttonDrawH, TFT_SKYBLUE, TFT_BLACK, 5, "WIFI");
  drawButton(buttonDrawX2, buttonDrawY, buttonDrawW, buttonDrawH, TFT_SKYBLUE, TFT_BLACK, 5, "TARGET");
  currentScreenState = CONFIG_SCREEN;
}

void displayWiFiScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("WIFI Screen", tft.width() / 2, 14);
  displayBackButtonData();  // Display back button symbol
  drawWiFiOptions(-1);
  drawConfirmButton(50, 200, 220, 27, TFT_GREEN, TFT_BLACK, 5, "Confirm");
  currentScreenState = WIFI_SCREEN;

}

void displayTargetScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Target Screen", tft.width() / 2, 14);
  displayBackButtonData();  // Display back button symbol
  drawTargetOptions(-1);
  drawConfirmButton(50, 200, 220, 27, TFT_GREEN, TFT_BLACK, 5, "Confirm");
  currentScreenState = TARGET_SCREEN;
}

void displayCfgButtonData() {
  // Get the size of the array
  int dataSize = sizeof(NN_Config) / sizeof(NN_Config[0]);
  // Calculate the width and height of the image
  int imgWidth = 32;  // Change this according to your image size
  int imgHeight = dataSize / imgWidth;

  // Draw the image on TFT screen
  for (int y = 0; y < imgHeight; y++) {
    for (int x = 0; x < imgWidth; x++) {
      // Get the pixel value from PROGMEM array
      unsigned short pixelColor = pgm_read_word(&NN_Config[y * imgWidth + x]);
      tft.drawPixel(x + 288, y, pixelColor);  // Draw pixel at (x+288,y) on TFT
    }
  }
}

void displayBackButtonData() {
  int dataSize = sizeof(back) / sizeof(back[0]);
  int imgWidth = 32;
  int imgHeight = dataSize / imgWidth;

  for (int y = BUTTON_Y; y < imgHeight; y++) {
    for (int x = BACK_BUTTON_X; x < imgWidth; x++) {
      unsigned short pixelColor = pgm_read_word(&back[y * imgWidth + x]);
      tft.drawPixel(BACK_BUTTON_X + x - BACK_BUTTON_X, BUTTON_Y + y - BUTTON_Y, pixelColor);
    }
  }
}


void drawWiFiButton(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius, const char* text, uint8_t fontSize = 1, uint8_t font = 1, bool highlighted = false) {
  if (highlighted) {
    tft.fillRoundRect(x, y, w, h, radius, TFT_PINK);
  } else {
    tft.fillRoundRect(x, y, w, h, radius, TFT_YELLOW);
  }
  tft.drawRoundRect(x - 2, y - 2, w + 4, h + 4, radius + 2, TFT_DARKGREY);
  tft.setTextColor(TFT_BLACK, highlighted ? TFT_PINK : TFT_YELLOW);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(fontSize);
  tft.setTextFont(font);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void drawWiFiOptions(int highlightedIndex = -1) {
  tft.fillRect(8, 30, 240, 150, TFT_BLACK);  // Clear previous options
  for (int i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
    String displayText = String(options[i].name) + " (" + String(options[i].nickname) + ")";
    drawWiFiButton(50, 40 + i * 30, 220, 20, 5, displayText.c_str(), 1, 2, i == highlightedIndex);
  }
  tft.setTextFont(4);
}

void drawTargetOptions(int highlightedIndex = -1) {
  tft.fillRect(8, 30, 240, 150, TFT_BLACK);  // Clear previous options
  for (int i = 0; i < sizeof(targetOptions) / sizeof(targetOptions[0]); i++) {
    String displayText = String(targetOptions[i].target);
    drawWiFiButton(50, 40 + i * 30, 220, 20, 5, displayText.c_str(), 1, 2, i == highlightedIndex);
  }
  tft.setTextFont(4);
}

void displayTimeOnTFT(unsigned long currentTime, uint16_t textColor, uint16_t bgColor) {
  tft.setTextColor(TFT_VIOLET, TFT_BLACK);
  tft.setCursor((tft.width() - 140) / 2, 2);  // Center horizontally
  tft.println("Current Time:");
  time_t now = myTZ.toLocal(currentTime);
  struct tm* currentTimeTm = localtime(&now);
  char formattedTime[30];
  sprintf(formattedTime, "%02d:%02d:%02d - %02d/%02d/%04d", currentTimeTm->tm_hour, currentTimeTm->tm_min, currentTimeTm->tm_sec, currentTimeTm->tm_mday, currentTimeTm->tm_mon + 1, currentTimeTm->tm_year + 1900);
  tft.drawRoundRect((tft.width() - 250) / 2, ct - 5, 255, 32, 5, TFT_MAGENTA);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setCursor((tft.width() - 240) / 2, ct);
  tft.println(formattedTime);
}

void displayCountdownBoxes() {
  tft.setCursor(0, rt);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.setCursor((tft.width() - 180) / 2, rt - 28);
  tft.println("Remaining Time:");
  int boxWidth = 45;
  int boxHeight = 35;
  int spacing = 5;
  int startX = (tft.width() - (boxWidth * 6 + spacing * 5)) / 2;
  int startY = rt;

  for (int i = 0; i < 6; i++) {
    tft.drawRoundRect(startX + (boxWidth + spacing) * i, startY, boxWidth, boxHeight, 5, TFT_MAGENTA);
  }
}

void displayCountdownLabels() {
  int boxWidth = 45;
  int spacing = 5;
  int startX = ((tft.width() - (boxWidth * 6 + spacing * 5)) / 2);
  int startY = 100 + 37;

  String labels[] = { "  y", "mo", "  d", "  h", "  m", "  s" };

  for (int i = 0; i < 6; i++) {
    tft.setTextColor(TFT_SKYBLUE);
    tft.setCursor(startX - 5 + (boxWidth + spacing) * i + boxWidth / 4, startY);
    tft.println(labels[i]);
  }
}

void displayCountdownValues() {
  int boxWidth = 45;
  int boxHeight = 35;
  int spacing = 5;
  int startX = (tft.width() - (boxWidth * 6 + spacing * 5)) / 2;
  int startY = 100 - 2;

  for (int i = 0; i < 6; i++) {
    char valueStr[4];
    sprintf(valueStr, "%02d", countdownValues[i]);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(startX + (boxWidth + spacing) * i + 10, startY + 10);
    tft.println(valueStr);
  }
}

void displayCountdownOnTFT() {
  // Get the target date details from the targetOptions array
  int hour, minute, day, month, year;
  sscanf(targetOptions[highlightedOption].trgtTime, "%d:%d", &hour, &minute);
  sscanf(targetOptions[highlightedOption].target, "%d/%d/%d", &day, &month, &year);

  char formattedTargetDate[20];
  sprintf(formattedTargetDate, "%02d:%02d - %02d/%02d/%04d", hour, minute, day, month, year);

  tft.setTextColor(TFT_GOLD, TFT_BLACK);

  // Print the reason
  tft.setCursor((tft.width() - tft.textWidth(targetOptions[highlightedOption].reason)) / 2, 200 - 31);
  tft.println(targetOptions[highlightedOption].reason);

  // Print the formatted target date
  int textWidth = tft.textWidth(formattedTargetDate);  // Get the width of the text
  int textX = (tft.width() - textWidth) / 2;           // Calculate the X-coordinate for centering
  int textY = 200;                                     // Adjust the Y-coordinate as needed

  // Center the round rectangle based on the text
  int rectX = (tft.width() - textWidth) / 2 - 10;  // Offset by 10 pixels
  int rectWidth = textWidth + 20;                  // Add 20 pixels for the offset
  tft.drawRoundRect(rectX, textY - 4, rectWidth, 32, 5, TFT_MAGENTA);
  tft.setCursor(textX, textY);       // Set the cursor position
  tft.println(formattedTargetDate);  // Display the formatted target date
 
}

// Correct function definition
void calculateTimeDifference(unsigned long currentTime, const char* formattedTargetDate) {
  
  // Assuming currentTime is of type time_t

// Define and initialize a struct tm variable for the target time
struct tm targetTm = {};
//strptime(formattedTargetDate, "%H:%M - %d/%m/%Y", &targetTm);  // Parse formatted string to fill targetTm
 if (strptime(formattedTargetDate, "%H:%M - %d/%m/%Y", &targetTm) == NULL) {
    Serial.print("Error parsing target date: ");
    Serial.println(formattedTargetDate);
    return;  // Exit the function if parsing fails
    // Print individual characters in the string for debugging
    Serial.println("Characters in the string:");
    for (int i = 0; formattedTargetDate[i] != '\0'; i++) {
        Serial.print(formattedTargetDate[i]);
        Serial.print(" (ASCII ");
        Serial.print(formattedTargetDate[i]);
        Serial.println(")");
    }

  }
// Convert targetTm struct to a time_t value (seconds since epoch)
time_t targetDateTime = mktime(&targetTm);
if (targetDateTime == -1) {
    Serial.println("Error converting target date");
    return;  // Exit the function if conversion fails
  }
// For the current time, since no time zone conversion is needed, simply assign the value
time_t localCurrentTime = currentTime;

// Convert localCurrentTime to a struct tm variable if needed
 struct tm currentTm = *localtime(&localCurrentTime);

  // Calculate time difference in seconds
  long timeDifferenceSeconds = difftime(targetDateTime, localCurrentTime);

  // Calculate years, months, days, hours, minutes, and seconds
  long years = timeDifferenceSeconds / 31536000;  // 365 days
  long remainder = timeDifferenceSeconds % 31536000;
  long daysPerYear = isLeapYear(currentTm.tm_year + 1900) ? 366 : 365;
  long months = 0;
  long days = 0;
  long hours = 0;
  long minutes = 0;
  long seconds = 0;

  while (remainder >= daysPerYear * 86400) {
    years++;
    remainder -= daysPerYear * 86400;
    daysPerYear = isLeapYear(currentTm.tm_year + years + 1900) ? 366 : 365;
  }

  daysPerYear = isLeapYear(currentTm.tm_year + years + 1900) ? 366 : 365;
  int monthDays = 0;
  for (months = 0; months < 12; months++) {
    monthDays = getDaysInMonth(currentTm.tm_year + years + 1900, months + 1);
    if (remainder >= monthDays * 86400) {
      remainder -= monthDays * 86400;
      days += monthDays;
    } else {
      break;
    }
  }

  hours = remainder / 3600;
  remainder %= 3600;
  minutes = remainder / 60;
  seconds = remainder % 60;

  countdownValues[0] = years;
  countdownValues[1] = months;
  countdownValues[2] = days;
  countdownValues[3] = hours;
  countdownValues[4] = minutes;
  countdownValues[5] = seconds;
}


int getDaysInMonth(int year, int month) {
  static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }

  return daysInMonth[month - 1];
}

bool isLeapYear(int year) {
  if (year % 4 != 0) {
    return false;
  } else if (year % 100 != 0) {
    return true;
  } else if (year % 400 != 0) {
    return false;
  } else {
    return true;
  }
}

void handleTouchEvent() {
  TS_Point p = ts.getPoint();

  if (p.z > 0) {
    int mappedX = map(p.x, 200, 3700, 0, SCREEN_WIDTH);
    int mappedY = map(p.y, 240, 3800, 0, SCREEN_HEIGHT);

    // Check for back button press
    if (mappedX >= BACK_BUTTON_X && mappedX <= BACK_BUTTON_X + BUTTON_WIDTH && mappedY >= BUTTON_Y && mappedY <= BUTTON_Y + BUTTON_HEIGHT) {
      Serial.println("Back button pressed");
      if (currentScreenState != MAIN_SCREEN) {
        currentScreenState = MAIN_SCREEN;
        tft.fillScreen(TFT_BLACK);
      }
    }
    // Check for config button press
    else if (mappedX >= CFG_BUTTON_X && mappedX <= CFG_BUTTON_X + BUTTON_WIDTH && mappedY >= BUTTON_Y && mappedY <= BUTTON_Y + BUTTON_HEIGHT) {
      Serial.println("Config button pressed");
      if (currentScreenState == MAIN_SCREEN) {
        tft.fillScreen(TFT_BLACK);
        displayConfigScreen();
      }
    }
    // Check for WIFI button press
    else if (currentScreenState == CONFIG_SCREEN) {

      if (mappedX >= buttonDrawX1 && mappedX <= buttonDrawX1 + buttonDrawW && mappedY >= buttonDrawY && mappedY <= buttonDrawY + buttonDrawH) {
        Serial.println("WIFI button pressed");
        displayWiFiScreen();
      } else if (mappedX >= buttonDrawX2 && mappedX <= buttonDrawX2 + buttonDrawW && mappedY >= buttonDrawY && mappedY <= buttonDrawY + buttonDrawH) {
        Serial.println("Target button pressed");
        displayTargetScreen();
      }
    }

    // Check for Confirm button press
    else if ((currentScreenState == WIFI_SCREEN || currentScreenState == TARGET_SCREEN) && mappedX >= 50 && mappedX <= 50 + 220 && mappedY >= 200 && mappedY <= 200 + 30) {
      Serial.println("Confirm button pressed");
      if (currentScreenState == WIFI_SCREEN) {
        selectedWiFiOption = options[highlightedOption];
        Serial.print("Selected WiFi Option: ");
        Serial.println(selectedWiFiOption.name);
        if (checkWiFiCredentials()) {
          tft.fillScreen(TFT_BLACK);
          currentScreenState = MAIN_SCREEN;
        } else {
          displayConfigScreen();
        }
      } else if (currentScreenState == TARGET_SCREEN) {
        selectedTargetDate = targetOptions[highlightedOption].target;
        Serial.print("Selected Target Date: ");
        Serial.println(selectedTargetDate);
        //displayConfigScreen();
        tft.fillScreen(TFT_BLACK);
        currentScreenState = MAIN_SCREEN;
      }
    }
  }
}


void loop() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  switch (currentScreenState) {
    case MAIN_SCREEN:
      renderMainScreen(epochTime);
      break;
    case CONFIG_SCREEN:
      // Render the config screen
      break;
    case WIFI_SCREEN:
      // Render the WiFi screen
      break;
    case TARGET_SCREEN:
      // Render the target screen
      break;
  }

  handleTouchEvent();
  handleWiFiTouchEvent();
  handleTargetTouchEvent();
  delay(100);
}

void handleWiFiTouchEvent() {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();

    if (currentScreenState == WIFI_SCREEN) {
      for (int i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
        // Check if touch point is within the region of an option button
        int buttonY = 300 + (400 + i * 450);
        if (p.x >= 700 && p.x <= 3200 && p.y >= buttonY && p.y <= buttonY + 350) {
          highlightedOption = i;  // Highlight the touched option
          //drawOptions(); // Redraw options to update highlight // change this to change background colour?
          Serial.print("Option Highlighted: ");
          Serial.println(options[highlightedOption].name);
          delay(200);  // Debounce
          break;       // Exit loop once a button is pressed
        }
      }
    }
  }
}

void handleTargetTouchEvent() {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();

    if (currentScreenState == TARGET_SCREEN) {
      for (int i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
        // Check if touch point is within the region of an option button
        int buttonY = 300 + (400 + i * 450);
        if (p.x >= 700 && p.x <= 3200 && p.y >= buttonY && p.y <= buttonY + 350) {
          highlightedOption = i;  // Highlight the touched option
          //drawOptions(); // Redraw options to update highlight // change this to change background colour?
          Serial.print("Option Highlighted: ");
          Serial.println(targetOptions[highlightedOption].target);
          delay(200);  // Debounce
          break;       // Exit loop once a button is pressed
        }
      }
    }
  }
}

/*
using ESP32-2432s028r and XPT2046_Touchscreen.h and TFT_eSPI.h 
to create a countdown timer. here is the code, which works so far, 
but I want to make it better:-
*/
