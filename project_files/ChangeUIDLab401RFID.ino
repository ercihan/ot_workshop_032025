#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  11    // SS (SDA) pin
#define RST_PIN 6     // RST pin

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Place the UID-writable Ultralight tag near the reader..."));
}

void loop() {
  // Wait for a new card and select it
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;
  
  // Verify the card is Ultralight (UID-writable type)
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_UL) {
    Serial.println(F("The detected card is not a MIFARE Ultralight tag."));
    mfrc522.PICC_HaltA();
    return;
  }

  // Define the new 7-byte UID to program
  byte newUid[7] = { 0x04, 0xAB, 0xCD, 0x12, 0x34, 0x56, 0x78 };  
  //byte newUid[7] = { 0x04, 0x15, 0x91, 0x2A, 0x76, 0xF9, 0x18 }; //Overwrite with that for unlocking!
  
  // Compute BCC bytes per ISO14443-3 (using CT = 0x88 for 7-byte UID).
  byte bcc0 = 0x88 ^ newUid[0] ^ newUid[1] ^ newUid[2];  // BCC0 = 0x88 \XOR UID0 \XOR UID1 \XOR UID2
  byte bcc1 = newUid[3] ^ newUid[4] ^ newUid[5] ^ newUid[6];        // BCC1 = UID3 \XOR UID4 \XOR UID5 \XOR UID6
  //BCC stands for Block Check Character. It is essentially a one-byte checksum used as part of the anti-collision
  //and selection process in ISO14443 (the standard for contactless smartcards).

  // Prepare data for each page. Page 1 contains UID bytes 3-6, page 0 contains UID0-UID2 and BCC0.
  byte page1Data[4] = { newUid[3], newUid[4], newUid[5], newUid[6] };
  byte page0Data[4] = { newUid[0], newUid[1], newUid[2], bcc0 };

  // Read current Page 2 to preserve the internal and lock bytes, but update BCC1.
  byte readBuf[18];
  byte bufSize = sizeof(readBuf);
  MFRC522::StatusCode status;
  status = mfrc522.MIFARE_Read(2, readBuf, &bufSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Failed to read page 2: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return;
  }
  // Prepare page 2 data: update byte0 with new BCC1, keep internal and lock bytes unchanged.
  byte page2Data[4];
  page2Data[0] = bcc1;
  page2Data[1] = readBuf[1];  // internal byte (reserved)
  page2Data[2] = readBuf[2];  // lock byte 0
  page2Data[3] = readBuf[3];  // lock byte 1

  // Write new UID page (reverse order - like Support from Lab401 told me)
  // Write page 1 (UID[3..6]) first.
  status = mfrc522.MIFARE_Ultralight_Write(1, page1Data, 4);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Writing page 1 failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return;
  }
  // Write page 0 (UID[0..2] + BCC0) second.
  status = mfrc522.MIFARE_Ultralight_Write(0, page0Data, 4);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Writing page 0 failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return;
  }
  // Finally, write page 2 to update BCC1 (preserving internal/lock bytes).
  status = mfrc522.MIFARE_Ultralight_Write(2, page2Data, 4);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Writing page 2 (BCC1/lock) failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return;
  }

  Serial.println(F("UID successfully written to tag!"));
  mfrc522.PICC_HaltA();            // Halt PICC
  mfrc522.PCD_StopCrypto1();       // Stop encryption (if any)
}