#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN         D1          // Configurable, see typical pin layout above
#define SS_PIN          D2          // Configurable, see typical pin layout above
#define REDL       D8
#define GREENL     D4
#define CARDBUTT   D0
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

void setup() {
  pinMode(REDL, OUTPUT);
  pinMode(GREENL, OUTPUT);
  pinMode(CARDBUTT, INPUT);
  Serial.begin(115200);        // Initialize serial communications with the PC
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card
  Serial.println(F("Escrever o ID de 8 caracteres MIFARE PICC "));
}

void loop() {

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  MFRC522::MIFARE_Key keyN;
  keyN.keyByte[0] = 0x54;
  keyN.keyByte[1] = 0x6b;
  keyN.keyByte[2] = 0xad;
  keyN.keyByte[3] = 0xa2;
  keyN.keyByte[4] = 0x92;
  keyN.keyByte[5] = 0xcb;
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("Card UID:"));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print(F(" PICC type: "));   // Dump PICC type
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;
  byte len;
  block = 5;
  for(int i =0;i<6;i++){
    buffer[i] = keyN.keyByte[i];
  }
  buffer[6] = 0xFF;
  buffer[7] = 0x07;
  buffer[8] = 0x80;
  buffer[9] = 0x69;
  for(int i = 10;i<16;i++) buffer[i] = 0x00;
  MFRC522::MIFARE_Key keyT;
  for(int i = 0;i<6;i++) keyT.keyByte[i] = 0x00;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block+2, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("Chave já não é padrão"));
    mfrc522.PICC_HaltA(); // Halt PICC
    mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
    keyN.keyByte[0] = 0x54;
    keyN.keyByte[1] = 0x6b;
    keyN.keyByte[2] = 0xad;
    keyN.keyByte[3] = 0xa2;
    keyN.keyByte[4] = 0x92;
    keyN.keyByte[5] = 0xcb;
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;
  }
  else {
    Serial.println(F("Reescrevendo chave..."));
    // Write block
    status = mfrc522.MIFARE_Write(block+2, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    else Serial.println(F("MIFARE_Write() success: "));
  }

  
  
  Serial.setTimeout(20000L) ;     // wait until 20 seconds for input from serial
  // Ask personal data: Family name
  Serial.println(F("Digite 8 caracteres para ser o ID #"));
  len = Serial.readBytesUntil('#', (char *) buffer, 8) ; // read family name from serial
  for (byte i = len; i < 16; i++) buffer[i] = ' ';     // pad with spaces

  
  //Serial.println(F("Authenticating using keyN A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &keyN, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else Serial.println(F("PCD_Authenticate() success: "));

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else Serial.println(F("MIFARE_Write() success: "));

//  //Serial.println(F("Authenticating using keyN A..."));
//  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &keyN, &(mfrc522.uid));
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("PCD_Authenticate() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//
//  // Write block
//  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("MIFARE_Write() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//  else Serial.println(F("MIFARE_Write() success: "));

  // Ask personal data: First name
//  Serial.println(F("Type First name, ending with #"));
//  len = Serial.readBytesUntil('#', (char *) buffer, 20) ; // read first name from serial
//  for (byte i = len; i < 20; i++) buffer[i] = ' ';     // pad with spaces
//
//  block = 4;
//  //Serial.println(F("Authenticating using key A..."));
//  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("PCD_Authenticate() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//
//  // Write block
//  status = mfrc522.MIFARE_Write(block, buffer, 16);
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("MIFARE_Write() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//  else Serial.println(F("MIFARE_Write() success: "));
//
//  block = 5;
//  //Serial.println(F("Authenticating using key A..."));
//  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("PCD_Authenticate() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//
//  // Write block
//  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
//  if (status != MFRC522::STATUS_OK) {
//    Serial.print(F("MIFARE_Write() failed: "));
//    Serial.println(mfrc522.GetStatusCodeName(status));
//    return;
//  }
//  else Serial.println(F("MIFARE_Write() success: "));


  Serial.println(" ");
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD

}
