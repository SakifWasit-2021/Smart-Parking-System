#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ------------------------- Pin Configurations ----------------------------
const int ledPins[6] = {2, 3, 4, 5, 6, 7};       // LED strips
const int entranceIR = 0;                        // Entrance IR sensor
const int slotIR[4] = {A1, A2, A3, 1};           // Slot IRs: A1, A2, A3, D1
const int servoPin = A0;                         // Servo motor pin

// RFID setup
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo object
Servo gateServo;

// ---------------------- Allowed RFID UIDs -------------------------
byte allowedUIDs[4][4] = {
  {0x05, 0xAF, 0x41, 0x02},
  {0x23, 0x39, 0xEB, 0x2C},
  {0x02, 0x3A, 0x46, 0x02},
  {0x2D, 0x59, 0xC8, 0x01}
};

// ------------------------- Setup Function ----------------------------
void setup() {
  // Init pins
  Serial.begin(9600);
  Serial.println("Smart Parking System Initialized");
  for (int i = 0; i < 4; i++) pinMode(slotIR[i], INPUT);
  pinMode(entranceIR, INPUT);
  for (int i = 0; i < 6; i++) pinMode(ledPins[i], OUTPUT);

  // Servo
  gateServo.attach(servoPin);
  gateServo.write(0);  // Gate closed

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");

  // RFID
  SPI.begin();
  rfid.PCD_Init();
}

// --------------------------- Loop ------------------------------------
void loop() {
  bool slotStatus[4];
  int freeCount = 0;

  // Check each slot
  for (int i = 0; i < 4; i++) {
    slotStatus[i] = digitalRead(slotIR[i]) == LOW;  // LOW = occupied
    if (!slotStatus[i]) freeCount++;
  }

  // LCD update
  lcd.setCursor(0, 1);
  lcd.print("Free Slots: ");
  lcd.print(freeCount);
  lcd.print("   ");

  // Check for car at entrance
  if (digitalRead(entranceIR) == LOW) {
    delay(500);  // debounce
    Serial.println("Car detected at entrance!");

    if (freeCount > 0) {
      if (checkRFID()) {
        Serial.println("Valid RFID detected - Access Granted!");
        int slotIndex = findNearestFreeSlot(slotStatus);
        guideCar(slotIndex);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Granted");
        lcd.setCursor(0, 1);
        lcd.print("Go to ");
        if (slotIndex < 2) lcd.print("Left ");
        else lcd.print("Right ");
        lcd.print(slotIndex % 2 + 1);
        
        openGate();
        delay(3000);  // Allow time to enter
        stopGuiding();
        lcd.clear();
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Access Denied");
        Serial.println("Access Denied");
        delay(2000);
      }
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No Slot Avail.");
      delay(2000);
    }
  }
}

// --------------------- RFID UID Check Function ----------------------
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  for (int i = 0; i < 4; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (rfid.uid.uidByte[j] != allowedUIDs[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      rfid.PICC_HaltA();
      return true;
    }
  }

  rfid.PICC_HaltA();
  return false;
}

// ---------------------- Slot & LED Handling --------------------------
int findNearestFreeSlot(bool status[]) {
  for (int i = 0; i < 4; i++) {
    if (!status[i]) return i;
  }
  return -1;
}

void guideCar(int slotIndex) {
  // Left slots: 0,1 → LEDs 0,1,2
  // Right slots: 2,3 → LEDs 3,4,5

  if (slotIndex < 2) {
    for (int i = 0; i <= slotIndex; i++) digitalWrite(ledPins[i], HIGH);
  } else {
    for (int i = 3; i <= 3 + (slotIndex - 2); i++) digitalWrite(ledPins[i], HIGH);
  }
}

void stopGuiding() {
  for (int i = 0; i < 6; i++) digitalWrite(ledPins[i], LOW);
}

void openGate() {
  gateServo.write(90);
  delay(3000);
  gateServo.write(0);
}