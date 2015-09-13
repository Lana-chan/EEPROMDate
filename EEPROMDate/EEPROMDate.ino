/******************************************************************************
 *                                                                            *
 * EEPROMDate: EEPROM Programmer for AT29Cxxx ICs                             *
 *                                                                            *
 * Loosely based off EEPROMMER by mario and schematics from Tiido             *
 *                                                                            *
 * Special thanks to Atmel for making such a complicated memory to write to   *
 *                                                                            *
 * $Authors: Svetlana Tovarisch & Tiido Priimägi $                            *
 * $Date: 2014-08-04 $                                                        *
 * $Revision: 1.3 $                                                           *
 *                                                                            *
 * This code is provided with no warranty whatsoever and the authors will not *
 * be held liable for any damages it might occur                              *
 *                                                                            *
 ******************************************************************************/
 
#define VERSIONSTRING "EEPROMDate 1.3"

//IO Lines
#define D0 2
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D6 8
#define D7 9

//Chip lines
#define WE A0
#define OE A1
#define CE A2

//Flip-flop selector
#define F0 10
#define F1 11
#define F2 12

//ROM data buffer
#define BUFFERSIZE 1024 
byte buffer[BUFFERSIZE];

//Command buffer
#define COMMANDSIZE 32
char cmdbuf[COMMANDSIZE];

unsigned long startAddress,endAddress,dataLength;
byte lineLength;

//Burner commands
#define NOCOMMAND    0
#define VERSION      1
#define SET_ADDRESS  2
#define UNLOCK       3
#define LOCK         4

#define READ_BIN    10
#define READ_HEX    11

#define WRITE_BIN   20

/******************************************************************************
 *                                                                            *
 * Chip Control functions                                                     *
 *                                                                            *
 ******************************************************************************/

// switch IO lines of databus to INPUT state
void data_bus_input() {
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
}

//switch IO lines of databus to OUTPUT state
void data_bus_output() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
}

//read a complete byte from the bus
//be sure to set data_bus to input before
byte read_data_bus() {
  return (
    (digitalRead(D7) << 7) +
    (digitalRead(D6) << 6) +
    (digitalRead(D5) << 5) +
    (digitalRead(D4) << 4) +
    (digitalRead(D3) << 3) +
    (digitalRead(D2) << 2) +
    (digitalRead(D1) << 1) +
    digitalRead(D0)
  );
}

//write a byte to the data bus
//be sure to set data_bus to output before
void write_data_bus(byte data) {
  digitalWrite(D7, (data >> 7) & 0x01);
  digitalWrite(D6, (data >> 6) & 0x01);
  digitalWrite(D5, (data >> 5) & 0x01);
  digitalWrite(D4, (data >> 4) & 0x01);
  digitalWrite(D3, (data >> 3) & 0x01);
  digitalWrite(D2, (data >> 2) & 0x01);
  digitalWrite(D1, (data >> 1) & 0x01);
  digitalWrite(D0, data & 0x01);
}

//short function to set the OE(output enable line of the eeprom)
// attention, this line is LOW - active
void set_oe (byte state) {
  digitalWrite(OE, state);
}

//short function to set the CE(chip enable line of the eeprom)
// attention, this line is LOW - active
void set_ce (byte state) {
  digitalWrite(CE, state);
}

//short function to set the WE(write enable line of the eeprom)
// attention, this line is LOW - active
void set_we (byte state) {
  digitalWrite(WE, state);
}

//Selects which latch to pick for the address bus
void line_select(byte pin) {
  digitalWrite(pin,LOW);
  digitalWrite(pin,HIGH);
}

//write to address bus
//address is up to 24 bits
void set_address_bus(unsigned long address) {
  //byte Ad3 = address >> 24;
  byte Ad2 = address >> 16;
  byte Ad1 = address >> 8;
  byte Ad0 = address;
  write_data_bus(Ad0);
  line_select(F0);
  write_data_bus(Ad1);
  line_select(F1);
  write_data_bus(Ad2);
  line_select(F2);
  //write_data_bus(Ad3);
  //line_select(0x04);
}

/******************************************************************************
 *                                                                            *
 * High-level functions                                                       *
 *                                                                            *
 ******************************************************************************/

//highlevel function to read a byte from a given address
byte read_byte(unsigned long address) {
  byte data = 0;
  data_bus_output();         //set databus for output -- will write addr
  set_oe(HIGH);              //first disbale output
  set_ce(HIGH);              //disable chip select
  set_we(HIGH);              //disable write
  set_address_bus(address);  //set address bus
  set_oe(LOW);               //enable output
  set_ce(LOW);               //enable chip select
  data_bus_input();          //set databus for input
  data = read_data_bus();
  set_oe(HIGH);              //disable output
  set_ce(HIGH);              //disable chip select  
  return data;
}

void write_byte(unsigned long address, byte data) {
  data_bus_output();         //set databus for output -- will write addr
  set_oe(HIGH);              //first disbale output
  set_ce(HIGH);              //disable chip select
  set_we(HIGH);              //disable write
  set_address_bus(address);  //set address bus
  write_data_bus(data);
  set_we(LOW);               //enable write
  set_ce(LOW);               //enable chip select
  set_we(HIGH);              //disable write
  set_ce(HIGH);              //disable chip select  
}

/**
 * read a data block from eeprom and write out a hex dump 
 * of the data to serial connection
 * @param from       start address to read fromm
 * @param to         last address to read from
 * @param linelength how many hex values are written in one line
 **/
void read_block(unsigned long from, unsigned long to, int linelength) {
  //count the number fo values that are already printed out on the
  //current line
  int outcount = 0;
  //loop from "from address" to "to address" (included)
  for (unsigned long address = from; address <= to; address++) {
    if (outcount == 0) {
      //print out the address at the beginning of the line
      Serial.println();
      Serial.print("0x");
      printAddress(address);
      Serial.print(" : ");
    }
    //print data, separated by a space
    byte data = read_byte(address);
    printByte(data);
    Serial.print(" ");
    outcount = (++outcount % linelength);
  }
  //print a newline after the last data line
  Serial.println();
}

//Writes a block of arbitrary size, checks last byte
byte write_block(unsigned long address, byte* buffer, unsigned int len) {
  //AT29C040 unlock code
  /*write_byte(0x00005555,0xAA);
  write_byte(0x00002AAA,0x55);
  write_byte(0x00005555,0x80);
  write_byte(0x00005555,0xAA);
  write_byte(0x00002AAA,0x55);
  write_byte(0x00005555,0x20);*/
  //write buffer
  //for (unsigned long i = address; i < address+len; i++) write_byte(i, buffer[i]);
  for (unsigned int i = 0; i < len; i++) write_byte(address+i, buffer[i]);
    //Serial.print(char(buffer[i]));
  int timeout = 0;
  while (read_byte(address+len-1) != buffer[len-1]) {
    delay(1);
    if (timeout++ >= 1000) return 0xFF;
  }
  return 0x00;
}

//unlocks the chip for writing
byte unlock_chip() {
  //buffer first 256 bytes of the chip
  byte unlock_buffer[0x100];
  for (byte b = 0x00; b <= 0xFF; b++) unlock_buffer[b] = read_byte(0x00000000+b);
  //AT29C040 unlock code
  write_byte(0x00005555,0xAA);
  write_byte(0x00002AAA,0x55);
  write_byte(0x00005555,0x80);
  write_byte(0x00005555,0xAA);
  write_byte(0x00002AAA,0x55);
  write_byte(0x00005555,0x20);
  //writes a 256 byte page, necessary to unlock
  return write_block(0x00000000,unlock_buffer,0x100); 
}

//locks the chip for writing
byte lock_chip() {
  //buffer first 256 bytes of the chip
  byte lock_buffer[0x100];
  for (byte b = 0x00; b <= 0xFF; b++) lock_buffer[b] = read_byte(0x00000000+b);
  //AT29C040 lock code
  write_byte(0x00005555,0xAA);
  write_byte(0x00002AAA,0x55);
  write_byte(0x00005555,0xA0);
  //writes a 256 byte page, necessary to unlock
  return write_block(0x00000000,lock_buffer,0x100); 
}

/******************************************************************************
 *                                                                            *
 * Command parsing functions                                                  *
 *                                                                            *
 ******************************************************************************/

//waits for a string submitted via serial connection
//returns only if linebreak is sent or the buffer is filled
void readCommand() {
  //first clear command buffer
  for(int i=0; i< COMMANDSIZE;i++) cmdbuf[i] = 0;
  //initialize variables
  char c = ' ';
  int idx = 0;
  //now read serial data until linebreak or buffer is full
  do {
    if(Serial.available()) {
      c = Serial.read();
      cmdbuf[idx++] = c;
    }
  } 
  while (c != '\n' && idx < (COMMANDSIZE)); //save the last '\0' for string end
  //change last newline to '\0' termination
  cmdbuf[idx - 1] = 0;
}

//parse the given command by separating command character and parameters
//at the moment only 5 commands are supported
byte parseCommand() {
  //set ',' to '\0' terminator (command string has a fixed strucure)
  cmdbuf[1]  = 0;  //first string is the command character
  cmdbuf[10] = 0;  //second string is startaddress (8 bytes)
  cmdbuf[19] = 0;  //third string is endaddress (8 bytes)
  cmdbuf[22] = 0;  //fourth string is length (2 bytes)
  /*startAddress= hexLong((cmdbuf+2));
  dataLength  = hexLong((cmdbuf+11));
  lineLength  = hexByte(cmdbuf+20);*/
  startAddress = strtoul(cmdbuf+2, 0, 16);
  dataLength = strtoul(cmdbuf+11, 0, 16);
  lineLength = strtoul(cmdbuf+20, 0, 16);
  byte retval = 0;
  switch(cmdbuf[0]) {
  case 'V':
    retval = VERSION; break;
  case 'A':
    retval = SET_ADDRESS; break;
  case 'B':
    retval = READ_BIN; break;
  case 'R':
    retval = READ_HEX; break;
  case 'W':
    retval = WRITE_BIN; break;
  case 'U':
    retval = UNLOCK; break;
  case 'L':
    retval = LOCK; break;
  default:
    retval = NOCOMMAND; break;
  }
  return retval;
}

/************************************************************
 * convert a single hex digit (0-9,a-f) to byte
 * @param char c single character (digit)
 * @return byte represented by the digit 
 ************************************************************/
byte hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  else return 0; // getting here is bad: it means the character was invalid
}

/************************************************************
 * convert a hex byte (00 - ff) to byte
 * @param c-string with the hex value of the byte
 * @return byte represented by the digits 
 ************************************************************/
byte hexByte(char* a) {
  return ((hexDigit(a[0])*16) + hexDigit(a[1]));
}

/************************************************************
 * convert a hex word (0000 - ffff) to unsigned int
 * @param c-string with the hex value of the word
 * @return unsigned int represented by the digits 
 ************************************************************/
unsigned int hexWord(char* data) {
  return (
    (hexDigit(data[0])*0x1000)+
    (hexDigit(data[1])*0x100)+
    (hexDigit(data[2])*0x10)+
    (hexDigit(data[3]))
  ); 
}

/************************************************************
 * convert a hex long (00000000 - ffffffff) to unsigned long
 * @param c-string with the hex value of the long
 * @return unsigned long represented by the digits 
 ************************************************************/
unsigned long hexLong(char* data) {
  return (
    (hexDigit(data[0])*0x10000000)+
    (hexDigit(data[1])*0x1000000)+
    (hexDigit(data[2])*0x100000)+
    (hexDigit(data[3])*0x10000)+
    (hexDigit(data[4])*0x1000)+
    (hexDigit(data[5])*0x100)+
    (hexDigit(data[6])*0x10)+
    (hexDigit(data[7]))
  ); 
}

/**
 * print out a 32 bit address as 8 character hex value
 **/
void printAddress(unsigned long address) {
  if(address < 0x00000010) Serial.print("0");
  if(address < 0x00000100) Serial.print("0");
  if(address < 0x00001000) Serial.print("0");
  if(address < 0x00010000) Serial.print("0");
  if(address < 0x00100000) Serial.print("0");
  if(address < 0x01000000) Serial.print("0");
  if(address < 0x10000000) Serial.print("0");
  Serial.print(address, HEX);
}

/**
 * print out a byte as 2 character hex value
 **/
void printByte(byte data) {
  if(data < 0x10) Serial.print("0");
  Serial.print(data, HEX);  
}

/******************************************************************************
 *                                                                            *
 * Main program                                                               *
 *                                                                            *
 ******************************************************************************/

void setup() {
  pinMode(F0, OUTPUT);
  pinMode(F1, OUTPUT);
  pinMode(F2, OUTPUT);
  data_bus_output();

  //define the EEPROM Pins as output
  // take care that they are HIGH
  digitalWrite(OE, HIGH);
  pinMode(OE, OUTPUT);
  digitalWrite(CE, HIGH);
  pinMode(CE, OUTPUT);
  digitalWrite(WE, HIGH);
  pinMode(WE, OUTPUT);

  //Serial.begin(115200);
  Serial.begin(460800);
}

void loop() {
  readCommand();
  byte cmd = parseCommand();
  unsigned int bytes = 0;
  switch(cmd) {
  case VERSION:
    Serial.println(VERSIONSTRING);
    break;
  case SET_ADDRESS:
    // Set the address bus to an arbitrary value.
    // Useful for debugging shift-register wiring, byte-order.
    // e.g. A,000000FF
    Serial.print("Setting address bus to 0x");
    printAddress(startAddress);
    Serial.println();
    data_bus_output();
    set_address_bus(startAddress);
    break;
  case READ_BIN: //Raw binary output
    for(unsigned long p = 0; p < dataLength; p++) {
      Serial.print(char(read_byte(startAddress+p)));
    }
    Serial.print('\0');
    break;
  case READ_HEX: //"Pretty" hex dump
    //set a default if needed to prevent infinite loop
    if(lineLength==0) lineLength=32;
    endAddress = startAddress + dataLength -1;
    read_block(startAddress,endAddress,lineLength);
    Serial.println('%');
    break;
  case WRITE_BIN: //Raw binary input
    //take care for max buffer size
    if(dataLength > BUFFERSIZE) dataLength = BUFFERSIZE;
    //endAddress = startAddress + dataLength -1;
    while(bytes < dataLength) {
      if(Serial.available()) buffer[bytes++] = Serial.read();
    }
    if(write_block(startAddress,buffer,dataLength) != 0x00) Serial.println("Write failed.");
    Serial.println('%');
    break;
  case UNLOCK:
    if(unlock_chip() != 0x00) Serial.println("Unlock failed.");
    Serial.println('%');
    break;
  case LOCK:
    if(lock_chip() != 0x00) Serial.println("Lock failed.");
    Serial.println('%');
    break;
  default:
    break;    
  }
}
