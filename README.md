# RFID Reader


## LCD Programming

Chip used is a 5x11 dot matrix 1602 display where each position is a single character. This chip can display 2 lines of 16 characters.


| Instruction        | Code                                       | Execution Time | Description |
|--------------------|--------------------------------------------|----------------|-------------|
|                    |  RS  R/W  B7  B6  B5  B4  B3  B2  B1  B0   |                |             |
| Clear display      |   0  0    0  0  0  0  0  0  0  1           | 1.52 ms        | Clears display and returns cursor to the home position (address 0). |
| Cursor home        |   0  0    0  0  0  0  0  0  1  *           | 1.52 ms        | Returns cursor to home position. Also returns display being shifted to the original position. DDRAM content remains unchanged. |
| Entry mode set     |   0  0    0  0  0  0  0  1  I/D S          | 37 μs          | Sets cursor move direction (I/D); specifies to shift the display (S). These operations are performed during data read/write. |
| Display control    |   0  0    0  0  0  0  1  D  C  B           | 37 μs          | Sets on/off of all display (D), cursor on/off (C), and blink of cursor position character (B). |  
| Display shift      |   0  0    0  0  0  1  S/C R/L *  *         | 37 μs          | Sets cursor-move or display-shift (S/C), shift direction (R/L). DDRAM content remains unchanged. |  
| Function set       |   0  0    0  0  1  DL  N  F  *  *          | 37 μs          | Sets interface data length (DL), number of display line (N), and character font (F). |  
| Set CGRAM address  |   0  0    0  1  CGRAM address              | 37 μs          | Sets the CGRAM address. CGRAM data are sent and received after this setting. |  
| Set DDRAM address  |   0  0    1  DDRAM address                 | 37 μs          | Sets the DDRAM address. DDRAM data are sent and received after this setting. |  
| Read busy flag     |   0  1    BF  CGRAM/DDRAM address          | 0 μs           | Reads busy flag (BF) indicating internal operation being performed and reads CGRAM or DDRAM address counter contents (depending on previous instruction). |
| Write RAM          |   1  0    Write Data                       | 37 μs          | Write data to CGRAM or DDRAM. | 
| Read from CG/DDRAM |   1  1    Read Data                        | 37 μs          | Read data from CGRAM or DDRAM. |  

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

1 Function Set: 8-bit, 1 Line, 5x7 Dots 0x30 48
2 Function Set: 8-bit, 2 Line, 5x7 Dots 0x38 56
3 Function Set: 4-bit, 1 Line, 5x7 Dots 0x20 32
4 Function Set: 4-bit, 2 Line, 5x7 Dots 0x28 40
5 Entry Mode 0x06 6
6 Display off Cursor off
(clearing display without clearing DDRAM content) 0x08 8
7 Display on Cursor on 0x0E 14
8 Display on Cursor off 0x0C 12
9 Display on Cursor blinking 0x0F 15
10 Shift entire display left 0x18 24
11 Shift entire display right 0x1C 30
12 Move cursor left by one character 0x10 16
13 Move cursor right by one character 0x14 20
14 Clear Display (also clear DDRAM content) 0x01 1
15 Set DDRAM address or coursor position on display   0x80 + address*  128 + address*
16 Set CGRAM address or set pointer to CGRAM location 0x40 + address   64  + address

* DDRAM address

00 01 02 03 04 05 06 07 ..... 32 33 34 35 36 37 38 39   - Character position (DEC)
-----------------------------------------------------
00 01 02 03 04 05 06 07 ..... 20 21 22 23 24 25 26 27   - Row 0 DDRAM address (HEX)
40 41 42 43 44 45 46 47 ..... 60 61 62 63 64 65 66 67   - Row 1 DDRAM address (HEX)


https://www.8051projects.net/lcd-interfacing/initialization.php