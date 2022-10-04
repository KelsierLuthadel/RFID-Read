/*

Instruction 	                      Code 	                        Execution Description
                        RS 	R/W 	B7 	B6 	B5 	B4 	B3 	B2 	B1 	B0  time
                        ----------------------------------------
Clear display 	        0 	0 	  0 	0 	0 	0 	0 	0 	0 	1   1.52 ms   Clears display and returns cursor to the home position (address 0).
Cursor home 	          0 	0 	  0 	0 	0 	0 	0 	0 	1 	* 	1.52 ms   Returns cursor to home position. Also returns display being shifted to the original position. DDRAM content remains unchanged. 	
Entry mode set 	        0 	0 	  0 	0 	0 	0 	0 	1 	I/D S 	37 μs     Sets cursor move direction (I/D); specifies to shift the display (S). These operations are performed during data read/write. 	
Display control 	      0 	0 	  0 	0 	0 	0 	1 	D 	C 	B 	37 μs     Sets on/off of all display (D), cursor on/off (C), and blink of cursor position character (B). 	
display shift 	        0 	0 	  0 	0 	0 	1 	S/C	R/L	* 	* 	37 μs     Sets cursor-move or display-shift (S/C), shift direction (R/L). DDRAM content remains unchanged. 	
Function set 	          0 	0 	  0 	0 	1 	DL 	N 	F 	* 	* 	37 μs     Sets interface data length (DL), number of display line (N), and character font (F). 	
Set CGRAM address       0 	0 	  0 	1 	CGRAM address 	        37 μs     Sets the CGRAM address. CGRAM data are sent and received after this setting. 	
Set DDRAM address       0 	0 	  1 	DDRAM address 	            37 μs     Sets the DDRAM address. DDRAM data are sent and received after this setting. 	
Read busy flag  	      0 	1 	  BF 	CGRAM/DDRAM address 	      0 μs      Reads busy flag (BF) indicating internal operation being performed and reads CGRAM or DDRAM address counter contents (depending on previous instruction). 	
Write RAM 	            1 	0 	  Write Data 	                    37 μs     Write data to CGRAM or DDRAM. 	
Read from CG/DDRAM 	    1 	1 	  Read Data 	                    37 μs     Read data from CGRAM or DDRAM. 	

Instruction bit names —

I/D - 0 = decrement cursor position, 1 = increment cursor position;

S - 0 = no display shift, 1 = display shift;
D - 0 = display off, 1 = display on;
C - 0 = cursor off, 1 = cursor on;
B - 0 = cursor blink off, 1 = cursor blink on ;
S/C - 0 = move cursor, 1 = shift display;
R/L - 0 = shift left, 1 = shift right;
DL - 0 = 4-bit interface, 1 = 8-bit interface;
N - 0 = 1/8 or 1/11 duty (1 line), 1 = 1/16 duty (2 lines);
F - 0 = 5×8 dots, 1 = 5×10 dots;
BF - 0 = can accept instruction, 1 = internal operation in progress.


Example commands:

1	Function Set: 8-bit, 1 Line, 5x7 Dots	0x30	48
2	Function Set: 8-bit, 2 Line, 5x7 Dots	0x38	56
3	Function Set: 4-bit, 1 Line, 5x7 Dots	0x20	32
4	Function Set: 4-bit, 2 Line, 5x7 Dots	0x28	40
5	Entry Mode	0x06	6
6	Display off Cursor off
(clearing display without clearing DDRAM content)	0x08	8
7	Display on Cursor on	0x0E	14
8	Display on Cursor off	0x0C	12
9	Display on Cursor blinking	0x0F	15
10	Shift entire display left	0x18	24
11	Shift entire display right	0x1C	30
12	Move cursor left by one character	0x10	16
13	Move cursor right by one character	0x14	20
14	Clear Display (also clear DDRAM content)	0x01	1
15	Set DDRAM address or coursor position on display	  0x80 + address*	 128 + address*
16	Set CGRAM address or set pointer to CGRAM location	0x40 + address   64  + address

* DDRAM address

00 01 02 03 04 05 06 07 ..... 32 33 34 35 36 37 38 39   - Character position (DEC)
-----------------------------------------------------
00 01 02 03 04 05 06 07 ..... 20 21 22 23 24 25 26 27   - Row 0 DDRAM address (HEX)
40 41 42 43 44 45 46 47 ..... 60 61 62 63 64 65 66 67   - Row 1 DDRAM address (HEX)


https://www.8051projects.net/lcd-interfacing/initialization.php
*/

#define DEBUG  

#include <SPI.h>
#include <MFRC522.h>

#ifdef DEBUG
   #define LOG(...)    Serial.print(__VA_ARGS__) 
   #define LOGLN(...)  Serial.println(__VA_ARGS__)
#else
   #define LOG(...)
   #define LOGLN(...)
#endif

#define SERIAL_BAUD 115200

#define RFID_RST_PIN 12  // Configurable, see typical pin layout above
#define RFID_SS_PIN 53   // Configurable, see typical pin layout above
#define NEW_RFID_UID { 0xDE, 0xAD, 0xBE, 0xEF }


int LCD_REGISTER_SELECT = 12;
int LCD_RW = 11;
int LCD_PINS[] = { 3, 4, 5, 6, 7, 8, 9, 10 };  
int LCD_ENABLE = 2;


MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN); 
MFRC522::MIFARE_Key key;


void LcdCommandWrite(int value) {
  size_t pinLength = sizeof(LCD_PINS) / sizeof(int) - 1;
  
  for (int i = LCD_PINS[0]; i <= LCD_PINS[pinLength]; i++) {
    digitalWrite(i, value & 01);  // Invert signal because 1602 LCD signal identification is D7-D0 (not D0-D7)
    value >>= 1;
  } 

  digitalWrite(LCD_RW, LOW);
  digitalWrite(LCD_REGISTER_SELECT, LOW);

  digitalWrite(LCD_ENABLE, LOW);
  delayMicroseconds(1);

  digitalWrite(LCD_ENABLE, HIGH);
  delayMicroseconds(1);

  digitalWrite(LCD_ENABLE, LOW);
  delayMicroseconds(1);
}

void LcdDataWrite(int value) {
  digitalWrite(LCD_REGISTER_SELECT, HIGH);
  digitalWrite(LCD_RW, LOW);

  for (int i = LCD_PINS[0]; i <= LCD_PINS[7]; i++) {
    digitalWrite(i, value & 01);
    value >>= 1;
  }

  digitalWrite(LCD_ENABLE, LOW);
  delayMicroseconds(1);

  digitalWrite(LCD_ENABLE, HIGH);
  delayMicroseconds(1);

  digitalWrite(LCD_ENABLE, LOW);
  delayMicroseconds(1);
}

void setup() {
  Serial.begin(SERIAL_BAUD);  // Initialize serial communications with the PC

  while (!Serial);  // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  /*
    Configure RFID
  */

  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Configure LCD
  for (int i = LCD_ENABLE; i <= LCD_REGISTER_SELECT; i++) {
    pinMode(i, OUTPUT);
  }

  delay(100);

  // Initialize the LCD
  LcdCommandWrite(0x38);  // Set to 8-bit interface, 2 lines display, 5x7 text size
  delay(64);

  LcdCommandWrite(0x06);  // Input method setting Auto increment, no shift is displayed
  delay(20);

  LcdCommandWrite(0x0E);  // Display on, Cursor on
  delay(20);

  LcdCommandWrite(0x01);  // Blank screen
  delay(100);

  LcdCommandWrite(0x80);  // Set DDRAM address to beginning of line
  delay(20);
}

void clearDisplay() {
  LcdCommandWrite(0x01);  // The screen is empty and the cursor position is zeroed
  delay(10);
}

void firstLine(unsigned int offset) {
  LcdCommandWrite(0x80 + offset);  // Define the cursor position
  delay(10);
}

void secondLine(unsigned int offset) {
  LcdCommandWrite(0xc0 + offset);  // Define the cursor position
  delay(10);
}


void writeText(unsigned char *text) {
  if (NULL != text) {
    for (int i = 0; i < strlen(text); i++) {
      LcdDataWrite(text[i]);
    }

    delay(10);
  }
}

// Setting the UID can be as simple as this:
//void loop() {
//  byte newUid[] = NEW_UID;
//  if ( mfrc522.MIFARE_SetUid(newUid, (byte)4, true) ) {
//    Serial.println("Wrote new UID to card.");
//  }
//  delay(2000);
//}


bool isNewCard() {
  return mfrc522.PICC_IsNewCardPresent();
}

bool canReadSerial() {
  return mfrc522.PICC_ReadCardSerial();
}

bool willRead() {
  return isNewCard() && canReadSerial();
}


void dumpPicType() {
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print(F("PICC type: "));
  Serial.print(mfrc522.PICC_GetTypeName(piccType));
  Serial.print(F(" (SAK "));
  Serial.print(mfrc522.uid.sak);
  Serial.print(")\r\n");

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      && piccType != MFRC522::PICC_TYPE_MIFARE_1K
      && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("This sample only works with MIFARE Classic cards."));
    return;
  }
}

void toHex(unsigned char *in, size_t in_sz, unsigned char *out) {
  const char *hex = "0123456789ABCDEF";

  unsigned char *pin = in;
  unsigned char *pout = out;

  for (; pin < in + in_sz; pin++, pout += 3) {
    pout[0] = hex[(*pin >> 4) & 0xF];
    pout[1] = hex[ *pin       & 0xF];
    pout[2] = ' ';
  }
  
  pout[-1] = 0;
}

void dumpHex(unsigned char *in, size_t insz) {
  clearDisplay();
  firstLine(0);

  char hexValue[insz * 3];
  toHex(in, insz, hexValue);
  writeText(hexValue);
}

void dumpUID() {
  LOG(F("Card UID: "));
  dumpHex(mfrc522.uid.uidByte, mfrc522.uid.size);
  LOGLN("");
}

void setUID(byte *newUid, byte uidSize) {
  if (mfrc522.MIFARE_SetUid(newUid, uidSize, true)) {
    LOGLN(F("Wrote new UID to card."));
  } else {
    LOGLN(F("Failed to write UID."));
  }
}

void dumpContents() {
  LOGLN(F("UID and contents:"));
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

void loop() {
  if (!willRead()) {
    delay(1000);
    return;
  }

  dumpUID();
  dumpPicType();
  dumpContents();

  // Halt PICC and re-select it so DumpToSerial doesn't get confused
  mfrc522.PICC_HaltA();

  if (!willRead()) {
    LOGLN(F("Nothing new"));
    return;
  }

  dumpContents();

  delay(3000);
}
