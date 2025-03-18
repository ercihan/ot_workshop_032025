/**********************************************/
/***** 1) INCLUDE THE NECESSARY LIBRARIES *****/
/**********************************************/
#include <WiFiNINA.h>           // Needed for controlling the onboard RGB LED
#include "utility/wifi_drv.h"   // Gives access to low-level pin control on the Nina module

#include <SPI.h>                // For MFRC522
#include <MFRC522.h>

/**************************************************/
/***** 2) PIN DEFINITIONS FOR LED AND MFRC522 *****/
/**************************************************/
// The MKR WiFi 1010's on-board RGB LED pins
#define LED_RED   25
#define LED_GREEN 26
#define LED_BLUE  27

// MFRC522 pins
#define RST_PIN 6
#define SS_PIN  11

// Create MFRC522 reader instance
MFRC522 mfrc522(SS_PIN, RST_PIN);

/***************************************/
/***** 3) KNOWN CARD UID TO MATCH *****/
/***************************************/
// Store the 7 bytes of the known UID in an array.
byte uidToCheck[7] = {0x04, 0x15, 0x91, 0x2A, 0x76, 0xF9, 0x18};

/***** 4) HELPER FUNCTION TO SET THE ON-BOARD RGB LED COLOR *****/
// Values are 0 to 255 for each channel
void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  WiFiDrv::analogWrite(LED_RED,   r);
  WiFiDrv::analogWrite(LED_GREEN, g);
  WiFiDrv::analogWrite(LED_BLUE,  b);
}

/**********************************************************/
/***** 5) COMPARE THE DETECTED UID WITH OUR KNOWN UID *****/
/**********************************************************/
bool checkUID(MFRC522::Uid uid) {
  // If the size does not match 7 bytes, return false immediately.
  if (uid.size != 7) return false;

  // Compare byte-by-byte
  for (byte i = 0; i < 7; i++) {
    if (uid.uidByte[i] != uidToCheck[i]) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Wait for the Serial connection
  }

  // Initialize the RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);

  // Print MFRC522 Firmware version
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println("Place an RFID/NFC card near the reader...");

  // Initialize LED pins on Nina module
  WiFiDrv::pinMode(LED_RED,   OUTPUT);
  WiFiDrv::pinMode(LED_GREEN, OUTPUT);
  WiFiDrv::pinMode(LED_BLUE,  OUTPUT);

  // Start with the LED showing Red
  setLEDColor(255, 0, 0);
}

void loop() {
  // Look for a new card
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select the card (load UID into mfrc522.uid)
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Print out detected UID for debugging
  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Check if this UID matches our known one
  if (checkUID(mfrc522.uid)) {
    Serial.println("Known card detected! Turning LED GREEN for 10 seconds...");
    setLEDColor(0, 255, 0); // GREEN
    delay(10000);
    Serial.println("Turning LED back to RED");
    setLEDColor(255, 0, 0); // RED
  } else {
    Serial.println("Unknown or unauthorized card. LED remains RED.");
    // If you want to do something else (like blink), do it here
  }

  // Halt PICC and stop encryption on the reader
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
