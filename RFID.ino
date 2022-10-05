/*

Instruction                          Code                            Execution  Description
                   RS  R/W    B7   B6   B5   B4   B3   B2   B1   B0  time
                   ------------------------------------------------
Clear display      0    0     0    0    0    0    0    0    0    1    1.52 ms   Clears display and returns cursor to the home position (address 0).
Cursor home        0    0     0    0    0    0    0    0    1    *    1.52 ms   Returns cursor to home position. Also returns display being shifted to the original position. DDRAM content remains unchanged.    
Entry mode set     0    0     0    0    0    0    0    1    I/D  S    37 μs     Sets cursor move direction (I/D); specifies to shift the display (S). These operations are performed during data read/write.    
Display control    0    0     0    0    0    0    1    D    C    B    37 μs     Sets on/off of all display (D), cursor on/off (C), and blink of cursor position character (B).    
display shift      0    0     0    0    0    1    S/C   R/L *    *    37 μs     Sets cursor-move or display-shift (S/C), shift direction (R/L). DDRAM content remains unchanged.    
Function set       0    0     0    0    1    DL    N    F   *    *    37 μs     Sets interface data length (DL), number of display line (N), and character font (F).    
Set CGRAM address  0    0     0    1    CGRAM address                 37 μs     Sets the CGRAM address. CGRAM data are sent and received after this setting.    
Set DDRAM address  0    0     1    DDRAM address                      37 μs     Sets the DDRAM address. DDRAM data are sent and received after this setting.    
Read busy flag     0    1     BF    CGRAM/DDRAM address               0 μs      Reads busy flag (BF) indicating internal operation being performed and reads CGRAM or DDRAM address counter contents (depending on previous instruction).    
Write RAM          1    0     Write Data                              37 μs     Write data to CGRAM or DDRAM.    
Read from CG/DDRAM 1    1     Read Data                               37 μs     Read data from CGRAM or DDRAM.    

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

1   Function Set: 8-bit, 1 Line, 5x7 Dots   0x30   48
2   Function Set: 8-bit, 2 Line, 5x7 Dots   0x38   56
3   Function Set: 4-bit, 1 Line, 5x7 Dots   0x20   32
4   Function Set: 4-bit, 2 Line, 5x7 Dots   0x28   40
5   Entry Mode   0x06   6
6   Display off Cursor off (clearing display without clearing DDRAM content)   0x08   8
7   Display on Cursor on   0x0E   14
8   Display on Cursor off   0x0C   12
9   Display on Cursor blinking   0x0F   15
10   Shift entire display left   0x18   24
11   Shift entire display right   0x1C   30
12   Move cursor left by one character   0x10   16
13   Move cursor right by one character   0x14   20
14   Clear Display (also clear DDRAM content)   0x01   1
15   Set DDRAM address or coursor position on display     0x80 + address*    128 + address*
16   Set CGRAM address or set pointer to CGRAM location   0x40 + address   64  + address

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

#define SERIAL_BAUD 115200 // Baud rate for serial debugging


#define RFID_RESET_PIN 12  
#define RFID_SS_PIN 53   // Signal Input (when SPI enabled), Serial Data (when I2C enabled), Data input (When UART enabled)

#define NEW_RFID_UID { 0xDE, 0xAD, 0xBE, 0xEF }


int LCD_REGISTER_SELECT = 12;
int LCD_RW = 11;
int LCD_PINS[] = { 3, 4, 5, 6, 7, 8, 9, 10 };  
int LCD_ENABLE = 2;

int ERROR_LED = 23;
int SUCCESS_LED = 22;

MFRC522 mfrc522(RFID_SS_PIN, RFID_RESET_PIN); 
MFRC522::MIFARE_Key key;


void error() {
  digitalWrite(ERROR_LED, LOW);
  digitalWrite(SUCCESS_LED, HIGH);
}

void success() {
  digitalWrite(ERROR_LED, HIGH);
  digitalWrite(SUCCESS_LED, LOW);
}

void neutral() {
  digitalWrite(ERROR_LED, HIGH);
  digitalWrite(SUCCESS_LED, HIGH);
}

// LCD helper functions
/**
  Write command to LCD interface, each bit of the supplied value is sent to one of the data 
  pins on the 1602 chip. The value needs to be inverted because the signal identification 
  goes from D7 -> D0 instead of D0 -> D7

  value:  Command to send to LCD chip.
  return: none
**/
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

/**
  Write data to LCD interface, each bit of the supplied value is sent to one of the data 
  pins on the 1602 chip. The value needs to be inverted because the signal identification 
  goes from D7 -> D0 instead of D0 -> D7

  value:  Data to send to LCD chip.
  return: none

**/
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

/**
  Returns cursor to home position. Also returns display being shifted to the original position. DDRAM content remains unchanged.    

  return: none

**/
void clearDisplay() {
  LcdCommandWrite(0x01);  
  delay(10);
}


/**
  Set cursor position to be the first line with an optional offset.

  offset: column offset    

  return: none

**/
void firstLine(unsigned int offset) {
  LcdCommandWrite(0x80 + offset);  // Define the cursor position
  delay(10);
}

/**
  Set cursor position to be the second line with an optional offset.

  offset: column offset    

  return: none

**/
void secondLine(unsigned int offset) {
  LcdCommandWrite(0xc0 + offset);  // Define the cursor position
  delay(10);
}

/**
  Write a string of text to currently defined line on the LCD

  text: A string of text to write to the LCD
**/
void writeText(unsigned char *text) {
  if (NULL != text) {
    for (int i = 0; i < strlen(text); i++) {
      LcdDataWrite(text[i]);
    }

    delay(10);
  }
}

// RFID helper functions

/**
  Determine if a new card has been presented.

  return: True if the presented card has a differernt UID than the previously presented card
**/
bool isNewCard() {
  return mfrc522.PICC_IsNewCardPresent();
}


/**
  Determine if a the card reader is active.

  return: True if the card reader is active.
**/
bool canReadSerial() {
  return mfrc522.PICC_ReadCardSerial();
}


/**
  Determine if a new card has been presented and the card reader is active.

  return: True if the presented card has a differernt UID than the previously presented card and the card reader is active.
**/

bool willRead() {
  return isNewCard() && canReadSerial();
}


bool authenticate(byte blockAddr, MFRC522::MIFARE_Key *key, MFRC522::Uid *uid) {
  MFRC522::StatusCode status;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, key, uid);
  
  if (status != MFRC522::STATUS_OK) {
    LOG(F("PCD_Authenticate() failed: "));
    LOG(MFRC522::GetStatusCodeName(status));
    return false;
  }

  return true;
}

/**
  Read the memory contents of a MIFARE Classic sector.
  Note: Memory is read in reverse, where

**/
void readClassicSector(MFRC522::Uid uid, MFRC522::MIFARE_Key *key, byte sector) {
  MFRC522::StatusCode status;
	byte firstBlock;		  // Address of the first block
	byte no_of_blocks;		// Number of blocks in a sector
	bool authenticated;	
	
	// The access bits are stored in a peculiar fashion.
	
  // There are four groups:
	//		g[3]	Access bits for the sector trailer, block 3 (for sectors 0-31) or block 15 (for sectors 32-39)
	//		g[2]	Access bits for block 2 (for sectors 0-31) or blocks 10-14 (for sectors 32-39)
	//		g[1]	Access bits for block 1 (for sectors 0-31) or blocks 5-9 (for sectors 32-39)
	//		g[0]	Access bits for block 0 (for sectors 0-31) or blocks 0-4 (for sectors 32-39)


	// Each group has access bits [C1 C2 C3]. In this code C1 is MSB and C3 is LSB.
	// The four CX bits are stored together in a nible cx and an inverted nible cx_.

  
	byte c1, c2, c3;		// Nibbles
	byte c1_, c2_, c3_;		// Inverted nibbles
	bool invertedError;		// True if one of the inverted nibbles did not match
	byte g[4] = {0, 0, 0, 0};				// Access bits for each of the four groups.
	byte group;				// 0-3 - active group for access bits
	bool firstInGroup;		// True for the first block dumped in the group
	
	// Determine position and size of sector.
	if (sector < 32) { // Sectors 0..31 has 4 blocks each
		no_of_blocks = 4;
		firstBlock = sector * no_of_blocks;
	}
	else if (sector < 40) { // Sectors 32-39 has 16 blocks each
		no_of_blocks = 16;
		firstBlock = 128 + (sector - 32) * no_of_blocks;
	}
	else { // Illegal input, no MIFARE Classic PICC has more than 40 sectors.
		return;
	}
		
	// Dump blocks, highest address first.
	byte byteCount;
	byte buffer[18];
	byte blockAddr;
	authenticated = false;
	invertedError = false;	
	
  for (int8_t blockOffset = 0; blockOffset < no_of_blocks; blockOffset++) {
		blockAddr = firstBlock + blockOffset;
		
    // Sector number
		
    if (blockOffset == no_of_blocks - 1) {
			if(sector < 10)
				LOG(F("   ")); 
			else
				LOG(F("  ")); 
			LOG(sector);
			LOG(F("   "));
		}
		else {
			LOG(F("       "));
		}

		// Block number
		if(blockAddr < 10)
			LOG(F("   ")); // Pad with spaces
		else {
			if(blockAddr < 100)
				LOG(F("  ")); // Pad with spaces
			else
				LOG(F(" ")); // Pad with spaces
		}
    
		LOG(blockAddr);
		LOG(F("  "));
		
    // Establish encrypted communications before reading the first block
		if (!authenticated) {
      if (false == authenticate(firstBlock, key, &(mfrc522.uid))) {
				return;
      }
      authenticated = true;
		}
		
    // Read block
		byteCount = sizeof(buffer);
    
		status = mfrc522.MIFARE_Read(blockAddr, buffer, &byteCount);
		
    if (status != MFRC522::STATUS_OK) {
			LOG(F("MIFARE_Read() failed: "));
			LOG(mfrc522.GetStatusCodeName(status));
			continue;
		}

		// Dump data
		for (byte index = 0; index < 16; index++) {
			if(buffer[index] < 0x10)
				LOG(F(" 0"));
			else
				LOG(F(" "));
			LOG(buffer[index], HEX);
			if ((index % 4) == 3) {
				LOG(F(" "));
			}
		}
		// Parse access bits
		if (blockOffset == no_of_blocks - 1) {
			c1  = buffer[7] >> 4;
			c2  = buffer[8] & 0xF;
			c3  = buffer[8] >> 4;
			c1_ = buffer[6] & 0xF;
			c2_ = buffer[6] >> 4;
			c3_ = buffer[7] & 0xF;
			invertedError = (c1 != (~c1_ & 0xF)) || (c2 != (~c2_ & 0xF)) || (c3 != (~c3_ & 0xF));
			g[0] = ((c1 & 1) << 2) | ((c2 & 1) << 1) | ((c3 & 1) << 0);
			g[1] = ((c1 & 2) << 1) | ((c2 & 2) << 0) | ((c3 & 2) >> 1);
			g[2] = ((c1 & 4) << 0) | ((c2 & 4) >> 1) | ((c3 & 4) >> 2);
			g[3] = ((c1 & 8) >> 1) | ((c2 & 8) >> 2) | ((c3 & 8) >> 3);
		}
		
		// Which access group is this block in?
		if (no_of_blocks == 4) {
			group = blockOffset;
			firstInGroup = true;
		}
		else {
			group = blockOffset / 5;
			firstInGroup = (group == 3) || (group != (blockOffset + 1) / 5);
		}
		
		if (firstInGroup) {
			// Print access bits
			LOG(F(" [ "));
			LOG((g[group] >> 2) & 1, DEC); LOG(F(" "));
			LOG((g[group] >> 1) & 1, DEC); LOG(F(" "));
			LOG((g[group] >> 0) & 1, DEC);
			LOG(F(" ] "));
			if (invertedError) {
				LOG(F(" Inverted access bits did not match! "));
			}
		}
		
		if (group != 3 && (g[group] == 1 || g[group] == 6)) { // value block
			int32_t value = (int32_t(buffer[3])<<24) | (int32_t(buffer[2])<<16) | (int32_t(buffer[1])<<8) | int32_t(buffer[0]);
			LOG(F(" Value=0x")); Serial.print(value, HEX);
			LOG(F(" Adr=0x")); Serial.print(buffer[12], HEX);
		}
		
    LOGLN();
    
    if (blockOffset == no_of_blocks - 1) {
      LOGLN();
    }
	}
}

int8_t getSectors(MFRC522::PICC_Type piccType) {
  int8_t sectors = 0;
	switch (piccType) {
		case MFRC522::PICC_TYPE_MIFARE_MINI:
			// Has 5 sectors * 4 blocks/sector * 16 bytes/block = 320 bytes.
			sectors = 5;
			break;
			
		case MFRC522::PICC_TYPE_MIFARE_1K:
			// Has 16 sectors * 4 blocks/sector * 16 bytes/block = 1024 bytes.
			sectors = 16;
			break;
			
		case MFRC522::PICC_TYPE_MIFARE_4K:
			// Has (32 sectors * 4 blocks/sector + 8 sectors * 16 blocks/sector) * 16 bytes/block = 4096 bytes.
			sectors = 40;
			break;
			
		default:
			break;
	}

  return sectors;
}

/**
  Dumps memory contents of a MIFARE Classic PICC.
**/
void readClassic(MFRC522::Uid uid, MFRC522::PICC_Type piccType,	MFRC522::MIFARE_Key *key	) {
  int8_t sectors = getSectors(piccType);

  if (sectors) {
		Serial.println(F("Sector Block   0  1  2  3   4  5  6  7   8  9 10 11  12 13 14 15  AccessBits"));
		// for (int8_t i = sectors - 1; i >= 0; i--) {
    for (int8_t i = 0; i < sectors; i++) {
			readClassicSector(uid, key, i);
		}
	}
}


/**
   Dumps memory contents of a MIFARE Ultralight PICC.
**/

void readUltraLight(byte pages) {
  MFRC522::StatusCode status;
	byte byteCount;
	byte buffer[18];
	byte i;

  for (byte page = 0; page < pages; page +=4) { // Read returns data for 4 pages at a time.
    byteCount = sizeof(buffer);
    MFRC522 miFare;
    status = miFare.MIFARE_Read(page, buffer, &byteCount);

    if (status != MFRC522::STATUS_OK) {
			LOG(F("MIFARE_Read() failed: "));
			LOGLN(MFRC522::GetStatusCodeName(status));
      error();
			break;
		}

    for(byte offset = 0; offset < 4; offset++) {
      i = page + offset;
      if (i < 10) {
        LOG("  ");
      } else {
        LOG(" ");
      }
      LOG(i);
      LOG(" ");

      for (byte index = 0; index < 4; index++) {
				i = 4 * offset + index;
				if(buffer[i] < 0x10)
					LOG(F(" 0"));
				else
					LOG(F(" "));
				LOG(buffer[i], HEX);
			}
			LOGLN("");
      
    }

  }

}

/**
  Get the data stored on the RFID chip. 

  uid: UID of a PICC 
**/

void getContents(MFRC522::Uid uid) {
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(uid.sak);

  switch (piccType) {
		case MFRC522::PICC_TYPE_MIFARE_MINI:
		case MFRC522::PICC_TYPE_MIFARE_1K:
		case MFRC522::PICC_TYPE_MIFARE_4K:
			for (byte i = 0; i < 6; i++) {
				key.keyByte[i] = 0xFF;
			}

      readClassic(uid, piccType, &key);
			break;
			
		case MFRC522::PICC_TYPE_MIFARE_UL:
			readUltraLight(43 /*16*/);
			break;
			
		case MFRC522::PICC_TYPE_ISO_14443_4:
		case MFRC522::PICC_TYPE_MIFARE_DESFIRE:
		case MFRC522::PICC_TYPE_ISO_18092:
		case MFRC522::PICC_TYPE_MIFARE_PLUS:
		case MFRC522::PICC_TYPE_TNP3XXX:
			LOGLN("OTHER");
			break;
			
		case MFRC522::PICC_TYPE_UNKNOWN:
		case MFRC522::PICC_TYPE_NOT_COMPLETE:
		default:
      LOGLN("UNKNOWN");
			break; // No memory dump here
	}

}


/**
  Dump the PICC (Proximity Integrated Circuit) type and SAK (Select AcKnowledgement) to the serial port.
  This is for debugging and analysis.
  
**/
void dumpPiccType() {
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  LOG(F("PICC type: "));
  LOG(mfrc522.PICC_GetTypeName(piccType));
  LOG(F(" (SAK "));
  LOG(mfrc522.uid.sak);
  LOGLN(")");
}

/**
  Convert byte data to ASCII in the form of a hex string.

  in: Binary data
  in_sz: Size of the binary data
  out: A buffer to hold the converted data. This must be 3 times the size of the inout data
**/
void binToHex(byte *in, size_t in_sz, unsigned char *out) {
  const char *hex = "0123456789ABCDEF";

  byte *pin = in;
  unsigned char *pout = out;

  for (; pin < in + in_sz; pin++, pout += 3) {
    pout[0] = hex[(*pin >> 4) & 0xF];
    pout[1] = hex[ *pin       & 0xF];
    pout[2] = ' ';
  }
  
  pout[-1] = 0;
}


void getUID(byte *uidByte, size_t uidSize, unsigned char *out) {
  binToHex(uidByte, uidSize, out);
}

/**
 Dump the UID to the serial port for the currently presented card.
 
**/

void dumpUID() {
  size_t uidSize = mfrc522.uid.size;
  char hexValue[uidSize * 3];
  
  getUID(mfrc522.uid.uidByte, uidSize, hexValue);

  clearDisplay();
  firstLine(0);
  writeText(hexValue);

  LOG(F("Card UID: "));
  LOGLN(hexValue);
}

void setUID(byte *newUid, byte uidSize) {
  if (mfrc522.MIFARE_SetUid(newUid, uidSize, true)) {
    LOGLN(F("Wrote new UID to card."));
  } else {
    LOGLN(F("Failed to write UID."));
  }
  delay(2000);
}

void setup() {
  // Configure state LEDS
  pinMode(SUCCESS_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  
  #ifdef DEBUG
    Serial.begin(SERIAL_BAUD);  
    while (!Serial);  
  #endif

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

  neutral();
}



void loop() {
  // Wait for a card to be presented
  if (!willRead()) {
    neutral(); // Disable state LEDs
    delay(250);
    return;
  }

  // Emable 'success' LED
  success();

  dumpUID();
  dumpPiccType();
  getContents(mfrc522.uid);
  
  // Halt PICC and re-select it so DumpToSerial doesn't get confused
  mfrc522.PICC_HaltA();
  
}
