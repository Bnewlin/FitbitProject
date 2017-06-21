/*
 * Required Data structures for Demo
 */
#include <Wire.h>
#include <SPI.h>
#define ADXL345_CS (7)
#define ADXL345_DEVICE (0x80)
#define ADXL345_DATAX0      0x32    // X-Axis Data 0
#define ADXL345_DATAX1      0x33    // X-Axis Data 1
#define ADXL345_DATAY0      0x34    // Y-Axis Data 0
#define ADXL345_DATAY1      0x35    // Y-Axis Data 1
#define ADXL345_DATAZ0      0x36    // Z-Axis Data 0
#define ADXL345_DATAZ1      0x37    // Z-Axis Data 1
#define ADXL345_DATA_FORMAT    0x31    // Data Format Control

#define RTCADDR B1101111//page11 datasheet
#define RTCSEC 0x00
#define CONTROL 0x07


#define disk1 0x50    //Address of 24LC256 eeprom chip


typedef struct
{
  uint16_t x, y, z;
} Accel;

typedef struct
{
  uint16_t month, day, year, weekday;
  uint16_t hour, minute, second;
} DateTime;
String weekDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
/*
 * Setup and Initialization functions
 */

/*
* Function implements any configuration necessary to operate
* on the SPI bus
*/
void setupSPI(){
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);//mode for ADXL345
  
  }

/*
* Function implements any configuration necessary to operate
* on the i2c bus
*/
void setupI2C(){
   Wire.begin(); 
}

/*
* Conduct any initialization necessary of the ADXL345
* either internally to Arduino or external in the peripheral
*/
void setupADXL345(){
  //gains[0] = 0.00376390;
  //gains[1] = 0.00376009;
  //gains[2] = 0.00349265;
  
  pinMode(ADXL345_CS, OUTPUT);
  digitalWrite(ADXL345_CS, HIGH);

    //Set the ADXL345 DATA_FORMAT to 4 wire mode
  digitalWrite(ADXL345_CS, LOW);
  SPI.transfer(ADXL345_DATA_FORMAT);
  SPI.transfer(0x01);
  digitalWrite(ADXL345_CS, HIGH);
}

/*
* Conduct any initialization necessary of the RTC
* either internally to Arduino or external in the peripheral
* Initialize the current date/time in MM/DD/YY HH:MM:SS format
*/
void setupRTC(DateTime* init){
  Wire.beginTransmission(RTCADDR);
  Wire.write(CONTROL);
  Wire.write(B00000000);//clear out the entire control register
  Wire.endTransmission();

  //let's go change the time:
  Wire.beginTransmission(RTCADDR);
  Wire.write(RTCSEC);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.beginTransmission(RTCADDR);//set the time/date
  Wire.write(RTCSEC);
  Wire.write(dec_to_bcd(init->second));
  Wire.write(dec_to_bcd(init->minute));
  Wire.write(dec_to_bcd(init->hour));
  Wire.write(dec_to_bcd(init->weekday));
  Wire.write(dec_to_bcd(init->day));
  Wire.write(dec_to_bcd(init->month));
  Wire.write(dec_to_bcd(init->year));
  Wire.endTransmission();

  Wire.beginTransmission(RTCADDR);//start RTC
  Wire.write(RTCSEC);
  Wire.endTransmission();
  Wire.requestFrom(RTCADDR, 1);
  delay(1);
  byte rtcSecondRegister = Wire.read();
  rtcSecondRegister |= 0x80;//start!
  Wire.beginTransmission(RTCADDR);
  Wire.write(RTCSEC);
  Wire.write(rtcSecondRegister);
  Wire.endTransmission();
  }

/*
* Conduct any initialization necessary of the EERPOM
* either internally to Arduino or external in the peripheral
*/
void setupEEPROM(){}

/**
 * Data read/write functions
 */

/*
 * Function takes a pointer to an Accel data object
 * and populates the contents with the X, Y, and Z acceleration values.
 * Value may be raw readings or normalized relative to 0g.
 */
void readAcceleration(Accel* data){
  byte _buff[6] ;    //  6 Bytes Buffer
  
  // Read: Most Sig Bit of Reg Address Set
  char address = 0x80 | ADXL345_DATAX0;
  address = address | 0x40;//multi-byte read
  
  digitalWrite(ADXL345_CS, LOW);
  SPI.transfer(address);    // Transfer Starting Reg Address To Be Read  
  for(int i=0; i<6; i++){
    _buff[i] = SPI.transfer(0x00);
  }
  digitalWrite(ADXL345_CS, HIGH);
   
  data->x = (int)((((int)_buff[1]) << 8) | _buff[0]);
  data->y = (int)((((int)_buff[3]) << 8) | _buff[2]);
  data->z = (int)((((int)_buff[5]) << 8) | _buff[4]);         

  }

/*
 * Function takes a pointer to a DateTime object and
 * populates with the current DateTime in MM/DD/YYYY HH:MM:SS
 * format. Return values must be human readable
 */
void readDateTime(DateTime* data){
  Wire.beginTransmission(RTCADDR);
  Wire.write(RTCSEC);
  Wire.endTransmission();
  Wire.requestFrom(RTCADDR, 7);//pull out all timekeeping registers
  delay(1);//little delay

  //now read each byte in and clear off bits we don't need, hence the AND operations
  data->second = bcd_to_dec(Wire.read() & 0x7F);
  data->minute = bcd_to_dec(Wire.read() & 0x7F);
  data->hour = bcd_to_dec(Wire.read() & 0x3F); 
  data->weekday = bcd_to_dec(Wire.read() & 0x03);
  data->day = bcd_to_dec(Wire.read() & 0x3F);
  data->month = bcd_to_dec(Wire.read() & 0x3F);
  data->year = bcd_to_dec(Wire.read());

  }

/**
 * Function writes a series of uint16_t values to the EEPROM
 * from the data array. Length of array is specified by the len parameter.
 * All writing is assumed to occur at EEPROM storage address 0x0.
 * Any existing data should be overwritten.
 */
void writeDataToEEPROM(uint16_t* data, uint16_t len){
  unsigned int address = 0x0;
  
  for(int i = 0; i<len; i++){
  Wire.beginTransmission(disk1);
  Wire.write((int)(i >> 8));   // MSB
  Wire.write((int)(i & 0xFF)); // LSB
  Wire.write(data[i]);
  
  Wire.endTransmission();

  delay(5);
  }
  }

/**
 * Function reads a sequence of uint16_t values from the EEPROM
 * and stores the result in the buffer array. Length of buffer is
 * specified by the len parameter. All reads are assumed to begin at EEPROM storage
 * address 0x0.
 */
void readDataFromEEPROM(uint16_t* buffer, uint16_t len){
  unsigned int address = 0;
  for(int i = 0; i<len; i++){
    Wire.beginTransmission(disk1);
    Wire.write((int)(i >> 8));   // MSB
    Wire.write((int)(i & 0xFF)); // LSB
    Wire.endTransmission();
  
    Wire.requestFrom(disk1,1);
    buffer[i] = Wire.read();
  }
  

}

uint16_t dec_to_bcd(uint16_t num){
  return (num/10) << 4 | (num%10) & 0xFF;
}

uint16_t bcd_to_dec(uint16_t num){
  return (num >> 4) * 10 + (num & 0x0F);
}


/**
  * typical sample code for your milestone. Particular
  * implementations may be different if you are in a different
  * development environment
  */
void setup() {

  /*
   * Configure all buses
   */
  setupI2C();
  setupSPI();

  /**
  * Initialize the EEPROM
  */
  setupEEPROM();

  /**
   * Configure the ADXL345
   */
  setupADXL345();

  /**
   * Initialize configuration data for the RTC. Modify
   * the contents of the initConfig object such that the
   * RTC will begin with the correct values
   */
  DateTime initConfig;
  initConfig.month = 06;
  initConfig.day = 8;
  initConfig.year = 17;
  initConfig.weekday = 5;
  initConfig.hour = 9;
  initConfig.minute = 23;
  initConfig.second = 10;
  setupRTC(&initConfig);

  Serial.begin(9600);


}

Accel currentReading;
DateTime currentDateTime;

void loop() 
{
  /**
   * Test #1: Try to read from the ADXL345 and
   * print out the results
   */
  readAcceleration(&currentReading);
  Serial.print(currentReading.x);
  Serial.print(",");
  Serial.print(currentReading.y);
  Serial.print(",");
  Serial.println(currentReading.z);
  

  /**
   * Test #2: Try to read out the time of day and
   * print out the results
   *
   readDateTime(&currentDateTime);
   Serial.print(weekDay[currentDateTime.weekday - 1]);
   Serial.print(" ");
   Serial.print(currentDateTime.month);
   Serial.print("/");
   Serial.print(currentDateTime.day);
   Serial.print("/");
   Serial.print(currentDateTime.year);
   Serial.print("\t");
   Serial.print(currentDateTime.hour);
   Serial.print(":");
   Serial.print(currentDateTime.minute);
   Serial.print(":");
   Serial.println(currentDateTime.second);
   */
   /**
    * Test #3: Try to write and read back some
    * random data
    
    uint16_t array[5];
    for(int i=0;i<5;i++)
    {
      array[i]=random(32);
    }
    Serial.print("Original Data ");
    for(int i = 0; i<5;i++){
    
    Serial.print(array[i]);
    Serial.print(", ");
    }
    Serial.println("");
    writeDataToEEPROM(array,5);



    uint16_t Readarray[5];

    readDataFromEEPROM(Readarray, 5);
    Serial.print("Read Data     ");
    for(int i = 0; i<5;i++){
    Serial.print(Readarray[i]);
    Serial.print(", ");
    }
    Serial.println("");
    Serial.println("");
    */
    
}
