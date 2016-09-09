#include <Wire.h>
#include <Eeprom24C32_64.h>

/******************************************************************************
 * Private macro definitions.
 ******************************************************************************/
//define sumchk addr
#define eeprom_size         4096                                              // 4K bytes
#define sumchk_addr_16u     0x02FF                                            // 0x02FF
#define count               16                                                // 16 bytes for reading
/**************************************************************************//**
 * \def EEPROM_ADDRESS
 * \brief Address of EEPROM memory on TWI bus.
 ******************************************************************************/
#define EEPROM_ADDRESS  0x50

/******************************************************************************
 * Private variable definitions.
 ******************************************************************************/

static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
byte readbuffer[count] = { 0 };                                               // eeprom data buffer for continue reading
char buf[128];                                                                // serial print buffer
byte eeprom_state;
unsigned int sum = 0;
byte checksumEE1;
byte checksumEE2;
byte data;
unsigned int sumchk_in_EE;
unsigned int sumchk_in_Calc;
/******************************************************************************
 * Public function definitions.
 ******************************************************************************/
void EepromWriteByteWithDelay (int addr, byte value, int delay_t = 5);
void EepromWriteBitWithDelay (int addr, byte bitno, byte value, int delay_t = 5);
unsigned int hexToDec(String hexString);
/**************************************************************************//**
 * \fn void setup()
 *
 * \brief
 ******************************************************************************/
void setup() {
  // put your setup code here, to run once:
    // Initialize serial communication.
    Serial.begin(9600);
        
    // Initialize EEPROM library.
    eeprom.initialize();
    eeprom_state = 1;
}

void loop() 
{
  // put your main code here, to run repeatedly:
  switch ( eeprom_state ) {
      case 0:// idle
          break;
      case 1:/* write section */
          // Write a byte at address 0 in EEPROM memory.
          /*  setting example
           *  EepromWriteByteWithDelay(0x0000, 0x1D);                               // write data = 0x1D to address 0x0000 in eeprom
           *  EepromWriteBitWithDelay(0x0000, 0, 1);                                // write bit0 = 1 to address 0x0000 in eeprom
          */
          Serial.println("Write setting table to EEPROM memory...");
          
          
          delay(1000);
          eeprom_state = 2;
          break;
      case 2:/* reading eeprom after write */
          // Read a byte at address 0 in EEPROM memory.
          Serial.println("Read byte from EEPROM memory...");
          for ( int  i = 0; i < eeprom_size/count; i++ ) {                          // row index i
              eeprom.readBytes(i*count, count, readbuffer);                         // read count(=16) bytes at once
              sprintf(buf, "%04d: ", i);
              Serial.print(buf);
              for ( int j = 0; j < count; j++ ) {
                  sprintf(buf, "0x%02X, ", readbuffer[j]);
                  Serial.print(buf);
              }
              if ( i == (sumchk_addr_16u/count) ) {
                  Serial.print("<---sumchk is in this row");
              }
              Serial.println("");
          }
          eeprom_state = 3;
          break;
      case 3:/* sumchk calculation */
          sum = 0;
          for (int i = 0; i < sumchk_addr_16u; i++) {
            data = eeprom.readByte(i);
            sum += (unsigned int)data;
          }
          checksumEE1 = eeprom.readByte(sumchk_addr_16u);
          checksumEE2 = eeprom.readByte(sumchk_addr_16u + 1);
          
          sumchk_in_EE = (unsigned int)checksumEE1 * 256 + (unsigned int)checksumEE2;
          sumchk_in_Calc = 0xffff - sum;
      
          sprintf(buf, "read eeprom sumchk(sumchk_addr_16u) = 0x%02X", sumchk_in_EE);
          Serial.println(buf);
          sprintf(buf, "calculate eeprom sumchk after write = 0x%02X", sumchk_in_Calc);
          Serial.println(buf);
          
          if ( sumchk_in_EE != sumchk_in_Calc ) {
              eeprom_state = 4;
          }
          else {
              Serial.println("sumchk_16u in eeprom wasn't changed.");
              eeprom_state = 5;
          }
          break;
      case 4:/* update sumchk of eeprom */
          sprintf(buf, "new checksum = 0x%04X ( %d, %d )", sumchk_in_Calc, sumchk_in_Calc/256, sumchk_in_Calc%256);
          Serial.println(buf);
    
          EepromWriteByteWithDelay(sumchk_addr_16u, (byte)(sumchk_in_Calc/256));    //sumchk high byte
          EepromWriteByteWithDelay(sumchk_addr_16u+1, (byte)(sumchk_in_Calc%256));  //sumchk low byte
    
          checksumEE1 = eeprom.readByte(sumchk_addr_16u);
          checksumEE2 = eeprom.readByte(sumchk_addr_16u + 1);
          sprintf(buf, "a new sumchk update ok...0x%02X%02X", checksumEE1, checksumEE2);
          Serial.println(buf);
          eeprom_state = 5;
      default:
          Serial.println("get into standby...");
          eeprom_state = 0;
      break;
  }
}

void EepromWriteByteWithDelay (int addr, byte value, int delay_t)
{
  char buf[64];
  eeprom.writeByte(addr, value);
  sprintf(buf, "setting eeprom(0x%04X) = 0x%02X", addr, value);
  Serial.print(buf);
  Serial.println("");
  delay(delay_t);
}

void EepromWriteBitWithDelay (int addr, byte bitno, byte value, int delay_t)
{
  char buf[128];
  byte value2;
  byte data = eeprom.readByte(addr);
  switch ( value ) {
    case 0:
      value2 = data & (~((byte)0x01 << bitno));
      break;
    case 1:
      value2 = data | (((byte)0x01 << bitno));
      break;
    default:
      break;
  }
  eeprom.writeByte(addr, value2);
  sprintf(buf, "setting eeprom(0x%04X) = 0x%02X --> 0x%02X", addr, data, value2);
  Serial.print(buf);
  Serial.println("");
  delay(delay_t);
}

unsigned int hexToDec(String hexString) 
{  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt>= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt>= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt>= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}
