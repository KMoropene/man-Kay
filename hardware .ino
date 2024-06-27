#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h> // Include the Time library

#define FIREBASE_HOST "zidane-a422b-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyAwJ9APZl_GSRrAbtiOF9s3HNbcAeeEylY"

#define ROWS 4
#define COLS 3

char keyMap[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

uint8_t rowPins[ROWS] = {14, 27, 26, 25};
uint8_t colPins[COLS] = {33, 32, 18};
const int buzzerPin = 5; // Pin for the buzzer

Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);

FirebaseData firebaseData;

String passcode = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT); // Set the buzzer pin as an output
  digitalWrite(buzzerPin, LOW); // Turn off the buzzer
  lcd.init();
  lcd.backlight();
  lcd.print("Waiting for Wi-Fi");

  WiFiManager wifiManager;
  wifiManager.autoConnect("AttendanceSystemAP");
  lcd.init();
  lcd.print("Connected");
  delay(3000);
  lcd.clear();
  lcd.print("Enter Passcode");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  timeClient.begin(); // Initialize NTPClient
  setTime(timeClient.getEpochTime()); // Set the system time using NTP
}

void loop() {
  timeClient.update(); // Update NTPClient

  char key = keypad.getKey();
  if (key) {
    if (key == '*') {
      if (passcode.length() > 0) {
        passcode.remove(passcode.length() - 1);
        lcd.setCursor(passcode.length(), 1);
        lcd.print(" ");
      }
    } else if (key == '#') {
      lcd.setCursor(0, 1);
      lcd.print("Processing...");

      int maxUserId = 10; // Adjust this to the maximum user ID in your Firebase
      bool userFound = false;

time_t currentTime = timeClient.getEpochTime();
int timeZoneOffsetInSeconds = 7200; // 2 hours in seconds

// Add daylight saving time (DST) offset if applicable
bool isDst = false; // Set this to true if DST is in effect
if (isDst) {
  timeZoneOffsetInSeconds += 3600; // Add 1 hour for DST
}

currentTime += timeZoneOffsetInSeconds; // Apply the time zone offset

String currentDateTime = formatTimeDigits(year(currentTime)) + "-" +
                         formatTimeDigits(month(currentTime)) + "-" +
                         formatTimeDigits(day(currentTime)) + " " +
                         formatTimeDigits(hour(currentTime)) + ":" +
                         formatTimeDigits(minute(currentTime)) + ":" +
                         formatTimeDigits(second(currentTime));


                               
      for (int i = 1; i <= maxUserId; i++) {
        Firebase.getString(firebaseData, "/users/user" + String(i) + "/passcode");

        if (firebaseData.dataType() == "string") {
          String storedPasscode = firebaseData.stringData();
           Serial.println("Stored Passcode: " + storedPasscode); // Add this line
    
          if (storedPasscode == passcode) {
            Firebase.getString(firebaseData, "/users/user" + String(i) + "/name");
            if (firebaseData.dataType() == "string") {
              String userName = firebaseData.stringData();
              Serial.println("User Name: " + userName); // Add this line
              if (!userName.isEmpty()) {
                lcd.clear();
                lcd.print("Welcome " + userName); // Display welcome message
                delay(2000);

                Firebase.setString(firebaseData, "/attendance/" + userName + "/" + currentDateTime, "present");

                if (firebaseData.httpCode() == 200) {
                  lcd.clear();
                  lcd.print("Attendance marked");
                  delay(2000);
                  userFound = true; // Set the flag to indicate a valid user was found
                  break; // Exit the loop once a valid user is found and attendance is marked
                } else {
                  lcd.clear();
                  lcd.print("Failed to mark");
                  delay(2000);
                }
              } else {
                lcd.clear();
                lcd.print("User not found");
                delay(2000);
              }
            }
          }
        }
      }

      if (!userFound) {
        lcd.clear();
        lcd.print("Invalid passcode");
        digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
        delay(500); // Beep for 0.5 seconds
        digitalWrite(buzzerPin, LOW); // Turn off the buzzer
        delay(2000);
      }

      passcode = "";
      lcd.clear();
      lcd.print("Enter passcode:");
    } else {
      passcode += key;
      lcd.setCursor(passcode.length() - 1, 1);
      lcd.print(key);
    }
  }
}


String formatTimeDigits(int value) {
  if (value < 10) {
    return "0" + String(value);
  } else {
    return String(value);
  }
}
