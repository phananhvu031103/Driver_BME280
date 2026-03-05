#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <asm/div64.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>

static struct task_struct *detect_thread;

#define DEV_NAME "bme280"
#define DEV_CLASS "BME280Class"

static struct i2c_adapter *driver_i2c_adapter = NULL;
static struct i2c_client *bme280_i2c_client = NULL;
static struct i2c_client *ssd1306_i2c_client = NULL;

#define I2C_BUS_AVAILABLE       (          1 )  // i2c bus available on RPI
#define BME280_DEVICE_NAME      (   "BME280" )  // device and driver name
#define BME280_SLAVE_ADDRESS    (       0x76 )  // i2c address

#define SSD1306_SLAVE_NAME      (  "SSD1306" )  // Device and Driver Name
#define SSD1306_SLAVE_ADDRESS   (       0x3c )  // SSD1306 OLED Slave Address
#define SSD1306_MAX_SEG         (        128 )  // Maximum segment
#define SSD1306_MAX_LINE        (          7 )  // Maximum line
#define SSD1306_DEF_FONT_SIZE   (          5 )  // Default font size

static struct i2c_board_info bme280_i2c_board_info = {
    I2C_BOARD_INFO(BME280_DEVICE_NAME, BME280_SLAVE_ADDRESS)};
    
static struct i2c_board_info ssd1306_i2c_board_info = {
    I2C_BOARD_INFO(SSD1306_SLAVE_NAME, SSD1306_SLAVE_ADDRESS)};

dev_t dev_num;
static struct class *dev_class;
static struct cdev bme280_cdev;

static int BME280_connected = 0;

static int bme280_open(struct inode *device_file, struct file *instance);
static int bme280_close(struct inode *device_file, struct file *instance);
static ssize_t bme280_read(struct file *fptr, char __user *buf, size_t len, loff_t *off);

#define CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb)

static void SSD1306_PrintChar(unsigned char c);
static void SSD1306_String( unsigned char *str );
static int  SSD1306_DisplayInit( void );
static void SSD1306_Fill( unsigned char data );
static void SSD1306_GoToNextLine( void );
static void SSD1306_SetCursor(uint8_t lineNo, uint8_t cursorPos);
static void SSD1306_Write(bool is_cmd, unsigned char data);

/*
** Variable to store Line Number and Cursor Position.
*/ 
static uint8_t SSD1306_LineNum   = 0;
static uint8_t SSD1306_CursorPos = 0;
static uint8_t SSD1306_FontSize  = SSD1306_DEF_FONT_SIZE;

/*
** Array Variable to store the letters.
*/ 
static const unsigned char SSD1306_font[][SSD1306_DEF_FONT_SIZE]= 
{
    {0x00, 0x00, 0x00, 0x00, 0x00},   // space
    {0x00, 0x00, 0x2f, 0x00, 0x00},   // !
    {0x00, 0x07, 0x00, 0x07, 0x00},   // "
    {0x14, 0x7f, 0x14, 0x7f, 0x14},   // #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12},   // $
    {0x23, 0x13, 0x08, 0x64, 0x62},   // %
    {0x36, 0x49, 0x55, 0x22, 0x50},   // &
    {0x00, 0x05, 0x03, 0x00, 0x00},   // '
    {0x00, 0x1c, 0x22, 0x41, 0x00},   // (
    {0x00, 0x41, 0x22, 0x1c, 0x00},   // )
    {0x14, 0x08, 0x3E, 0x08, 0x14},   // *
    {0x08, 0x08, 0x3E, 0x08, 0x08},   // +
    {0x00, 0x00, 0xA0, 0x60, 0x00},   // ,
    {0x08, 0x08, 0x08, 0x08, 0x08},   // -
    {0x00, 0x60, 0x60, 0x00, 0x00},   // .
    {0x20, 0x10, 0x08, 0x04, 0x02},   // /

    {0x3E, 0x51, 0x49, 0x45, 0x3E},   // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},   // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},   // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},   // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},   // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},   // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},   // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},   // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},   // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E},   // 9

    {0x00, 0x36, 0x36, 0x00, 0x00},   // :
    {0x00, 0x56, 0x36, 0x00, 0x00},   // ;
    {0x08, 0x14, 0x22, 0x41, 0x00},   // <
    {0x14, 0x14, 0x14, 0x14, 0x14},   // =
    {0x00, 0x41, 0x22, 0x14, 0x08},   // >
    {0x02, 0x01, 0x51, 0x09, 0x06},   // ?
    {0x32, 0x49, 0x59, 0x51, 0x3E},   // @

    {0x7C, 0x12, 0x11, 0x12, 0x7C},   // A
    {0x7F, 0x49, 0x49, 0x49, 0x36},   // B
    {0x3E, 0x41, 0x41, 0x41, 0x22},   // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C},   // D
    {0x7F, 0x49, 0x49, 0x49, 0x41},   // E
    {0x7F, 0x09, 0x09, 0x09, 0x01},   // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A},   // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F},   // H
    {0x00, 0x41, 0x7F, 0x41, 0x00},   // I
    {0x20, 0x40, 0x41, 0x3F, 0x01},   // J
    {0x7F, 0x08, 0x14, 0x22, 0x41},   // K
    {0x7F, 0x40, 0x40, 0x40, 0x40},   // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},   // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F},   // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E},   // O
    {0x7F, 0x09, 0x09, 0x09, 0x06},   // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E},   // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46},   // R
    {0x46, 0x49, 0x49, 0x49, 0x31},   // S
    {0x01, 0x01, 0x7F, 0x01, 0x01},   // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F},   // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F},   // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F},   // W
    {0x63, 0x14, 0x08, 0x14, 0x63},   // X
    {0x07, 0x08, 0x70, 0x08, 0x07},   // Y
    {0x61, 0x51, 0x49, 0x45, 0x43},   // Z

    {0x00, 0x7F, 0x41, 0x41, 0x00},   // [
    {0x55, 0xAA, 0x55, 0xAA, 0x55},   // Backslash (Checker pattern)
    {0x00, 0x41, 0x41, 0x7F, 0x00},   // ]
    {0x04, 0x02, 0x01, 0x02, 0x04},   // ^
    {0x40, 0x40, 0x40, 0x40, 0x40},   // _
    {0x00, 0x03, 0x05, 0x00, 0x00},   // `

    {0x20, 0x54, 0x54, 0x54, 0x78},   // a
    {0x7F, 0x48, 0x44, 0x44, 0x38},   // b
    {0x38, 0x44, 0x44, 0x44, 0x20},   // c
    {0x38, 0x44, 0x44, 0x48, 0x7F},   // d
    {0x38, 0x54, 0x54, 0x54, 0x18},   // e
    {0x08, 0x7E, 0x09, 0x01, 0x02},   // f
    {0x18, 0xA4, 0xA4, 0xA4, 0x7C},   // g
    {0x7F, 0x08, 0x04, 0x04, 0x78},   // h
    {0x00, 0x44, 0x7D, 0x40, 0x00},   // i
    {0x40, 0x80, 0x84, 0x7D, 0x00},   // j
    {0x7F, 0x10, 0x28, 0x44, 0x00},   // k
    {0x00, 0x41, 0x7F, 0x40, 0x00},   // l
    {0x7C, 0x04, 0x18, 0x04, 0x78},   // m
    {0x7C, 0x08, 0x04, 0x04, 0x78},   // n
    {0x38, 0x44, 0x44, 0x44, 0x38},   // o
    {0xFC, 0x24, 0x24, 0x24, 0x18},   // p
    {0x18, 0x24, 0x24, 0x18, 0xFC},   // q
    {0x7C, 0x08, 0x04, 0x04, 0x08},   // r
    {0x48, 0x54, 0x54, 0x54, 0x20},   // s
    {0x04, 0x3F, 0x44, 0x40, 0x20},   // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C},   // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C},   // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C},   // w
    {0x44, 0x28, 0x10, 0x28, 0x44},   // x
    {0x1C, 0xA0, 0xA0, 0xA0, 0x7C},   // y
    {0x44, 0x64, 0x54, 0x4C, 0x44},   // z

    {0x00, 0x10, 0x7C, 0x82, 0x00},   // {
    {0x00, 0x00, 0xFF, 0x00, 0x00},   // |
    {0x00, 0x82, 0x7C, 0x10, 0x00},   // }
    {0x00, 0x06, 0x09, 0x09, 0x06}    // ~ (Degrees)
};
 
static void SSD1306_Write(bool is_cmd, unsigned char data)
{
  unsigned char buf[2] = {0};
  int ret;

  if( is_cmd == true ){
      buf[0] = 0x00;
  }else{
      buf[0] = 0x40;
  }
  
  buf[1] = data;
  
  ret = i2c_master_send(ssd1306_i2c_client, buf, 2);
}

static void SSD1306_SetCursor( uint8_t lineNo, uint8_t cursorPos )
{
  /* Move the Cursor to specified position only if it is in range */
  if((lineNo <= SSD1306_MAX_LINE) && (cursorPos < SSD1306_MAX_SEG)){
    SSD1306_LineNum   = lineNo;             // Save the specified line number
    SSD1306_CursorPos = cursorPos;          // Save the specified cursor position

    SSD1306_Write(true, 0x21);              // cmd for the column start and end address
    SSD1306_Write(true, cursorPos);         // column start addr
    SSD1306_Write(true, SSD1306_MAX_SEG-1); // column end addr

    SSD1306_Write(true, 0x22);              // cmd for the page start and end address
    SSD1306_Write(true, lineNo);            // page start addr
    SSD1306_Write(true, SSD1306_MAX_LINE);  // page end addr
  }
}

static void  SSD1306_GoToNextLine( void ){
  /*
  ** Increment the current line number.
  ** roll it back to first line, if it exceeds the limit. 
  */
  SSD1306_LineNum++;
  SSD1306_LineNum = (SSD1306_LineNum & SSD1306_MAX_LINE);

  SSD1306_SetCursor(SSD1306_LineNum,0); /* Finally move it to next line */
}

static void SSD1306_PrintChar(unsigned char c){
  uint8_t data_byte;
  uint8_t temp = 0;

  /*
  ** If we character is greater than segment len or we got new line charcter
  ** then move the cursor to the new line
  */ 
  if(((SSD1306_CursorPos + SSD1306_FontSize ) >= SSD1306_MAX_SEG ) || ( c == '\n' )){
    SSD1306_GoToNextLine();
  }
  
  if (c != '\n'){
    c -= 0x20;  //or c -= ' ';
    do{
      data_byte= SSD1306_font[c][temp]; // Get the data to be displayed from LookUptable
      SSD1306_Write(false, data_byte);  // write data to the OLED
      SSD1306_CursorPos++;
      
      temp++;
    } while ( temp < SSD1306_FontSize);
    
    SSD1306_Write(false, 0x00);         //Display the data
    SSD1306_CursorPos++;
  }
}

static void SSD1306_String(unsigned char *str){
  while(*str) {
    SSD1306_PrintChar(*str++);
  }
}

static int SSD1306_DisplayInit(void){
  msleep(100);               // delay

  /*
  ** Commands to initialize the SSD_1306 OLED Display
  */
  SSD1306_Write(true, 0xAE); // Entire Display OFF
  SSD1306_Write(true, 0xD5); // Set Display Clock Divide Ratio and Oscillator Frequency
  SSD1306_Write(true, 0x80); // Default Setting for Display Clock Divide Ratio and Oscillator Frequency that is recommended
  SSD1306_Write(true, 0xA8); // Set Multiplex Ratio
  SSD1306_Write(true, 0x3F); // 64 COM lines
  SSD1306_Write(true, 0xD3); // Set display offset
  SSD1306_Write(true, 0x00); // 0 offset
  SSD1306_Write(true, 0x40); // Set first line as the start line of the display
  SSD1306_Write(true, 0x8D); // Charge pump
  SSD1306_Write(true, 0x14); // Enable charge dump during display on
  SSD1306_Write(true, 0x20); // Set memory addressing mode
  SSD1306_Write(true, 0x00); // Horizontal addressing mode
  SSD1306_Write(true, 0xA1); // Set segment remap with column address 127 mapped to segment 0
  SSD1306_Write(true, 0xC8); // Set com output scan direction, scan from com63 to com 0
  SSD1306_Write(true, 0xDA); // Set com pins hardware configuration
  SSD1306_Write(true, 0x12); // Alternative com pin configuration, disable com left/right remap
  SSD1306_Write(true, 0x81); // Set contrast control
  SSD1306_Write(true, 0x80); // Set Contrast to 128
  SSD1306_Write(true, 0xD9); // Set pre-charge period
  SSD1306_Write(true, 0xF1); // Phase 1 period of 15 DCLK, Phase 2 period of 1 DCLK
  SSD1306_Write(true, 0xDB); // Set Vcomh deselect level
  SSD1306_Write(true, 0x20); // Vcomh deselect level ~ 0.77 Vcc
  SSD1306_Write(true, 0xA4); // Entire display ON, resume to RAM content display
  SSD1306_Write(true, 0xA6); // Set Display in Normal Mode, 1 = ON, 0 = OFF
  SSD1306_Write(true, 0x2E); // Deactivate scroll
  SSD1306_Write(true, 0xAF); // Display ON in normal mode
  
  //Clear the display
  SSD1306_Fill(0x00);
  return 0;
}

static void SSD1306_Fill(unsigned char data){
  unsigned int total  = 128 * 8;  // 8 pages x 128 segments x 8 bits of data
  unsigned int i      = 0;
  
  //Fill the Display
  for(i = 0; i < total; i++)
  {
      SSD1306_Write(false, data);
  }
}

// calibration registers, read at initialization
// temperature calibration
unsigned short dig_T1;
signed short dig_T2;
signed short dig_T3;
unsigned short dig_P1;
signed short dig_P2;
signed short dig_P3;
signed short dig_P4;
signed short dig_P5;
signed short dig_P6;
signed short dig_P7;
signed short dig_P8;
signed short dig_P9;
unsigned char dig_H1;
signed short dig_H2;
unsigned char dig_H3;
signed short dig_H4;
signed short dig_H5;
signed char dig_H6;

// fine temperature, a global variable measured by read_temperature() and used
// by read_pressure()
long signed int t_fine;

// to store data with format
long signed int temp;
long signed int press64;
long signed int hum;
char tempStr[11];
char pressureStr[11];
char humidityStr[11];

// Returns pressure in Q24.8 format, divide by 256 to obtain answer accurate to
// 1 decimal place (e.g. 24674867/256=96386.2Pa)
// See p25 of datasheet for calculation formula
long unsigned int read_pressure_int64(void){
  long long signed int var1, var2, p;
  long long unsigned int var4;
  long signed int adc_P;

  u8 p1, p2, p3;
  p1 = (u8)(0xFF & i2c_smbus_read_byte_data(bme280_i2c_client, 0xF7));
  p2 = (u8)(0xFF & i2c_smbus_read_byte_data(bme280_i2c_client, 0xF8));
  p3 = (u8)(0xFF & i2c_smbus_read_byte_data(bme280_i2c_client, 0xF9));

  adc_P = ((p1 << 16) | (p2 << 8) | p3) >> 4;

  var1 = ((long long signed int)t_fine) - 128000;
  var2 = var1 * var1 * (long long signed int)dig_P6;
  var2 = var2 + ((var1 * (long long signed int)dig_P5) << 17);
  var2 = var2 + (((long long signed int)dig_P4) << 35);
  var1 = ((var1 * var1 * (long long signed int)dig_P3) >> 8) + ((var1 * (long long signed int)dig_P2) << 12);
  var1 = (((((long long signed int)1) << 47) + var1)) * ((long long signed int)dig_P1) >> 33;
  
  if (var1 == 0){
    return 0;
  }
  
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125);
  var4 = do_div(p, var1);
  var1 = (((long long signed int)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((long long signed int)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((long long signed int)dig_P7) << 4);
  return (long unsigned int)p;
}

// Returns temperature in Celsius with 0.01 resolution, e.g. 3001 -> 30.01degC
// See p50 of datasheet for calculation formula
long signed int read_temperature(void){
  int var1, var2;
  long signed int adc_T;
  long signed int T;
  long signed int d1, d2, d3;

  // read temperature registers (see p31)
  d1 = i2c_smbus_read_byte_data(bme280_i2c_client, 0xFA);
  d2 = i2c_smbus_read_byte_data(bme280_i2c_client, 0xFB);
  d3 = i2c_smbus_read_byte_data(bme280_i2c_client, 0xFC);
  
  // formula from datasheet page 25
  adc_T = ((d1 << 16) | (d2 << 8) | d3) >> 4;

  var1 = ((((adc_T >> 3) - (dig_T1 << 1))) * (dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - (dig_T1)) * ((adc_T >> 4) - (dig_T1))) >> 12) * (dig_T3)) >> 14;

  t_fine = (var1 + var2);
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

long signed int read_humidity(void){
  long signed int adc_H;
  long signed int h1, h2;
  long signed int v_x1_u32r;

  // read temperature registers (p31)
  h1 = (u8)(0xFF & i2c_smbus_read_byte_data(bme280_i2c_client, 0xFD));
  h2 = (u8)(0xFF & i2c_smbus_read_byte_data(bme280_i2c_client, 0xFE));

  adc_H = (h1 << 8) | h2;
  // end of datasheet page 25
  v_x1_u32r = (t_fine - ((long signed int)76800));
  v_x1_u32r = (((((adc_H << 14) - (((long signed int)dig_H4) << 20) - (((s32)dig_H5) * v_x1_u32r)) + ((long signed int)16384)) >> 15) * (((((((v_x1_u32r *
              ((long signed int)dig_H6)) >> 10) * (((v_x1_u32r * ((s32)dig_H3)) >> 11) + ((long signed int)32768))) >> 10) + ((s32)2097152)) * ((s32)dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((long signed int)dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (long signed int)(v_x1_u32r >> 12);
}

void read_calibration(void){
  // read registers
  #define DATA1_LEN 24
  #define DATA1_START 0x88
  #define DATA2_LEN 8
  #define DATA2_START 0xE1
  
  u8 reg_data1[DATA1_LEN];
  u8 reg_data2[DATA2_LEN];
  
  long signed int data_read;
  int16_t dig_h4_msb;
  int16_t dig_h4_lsb;
  int16_t dig_h5_msb;
  int16_t dig_h5_lsb;
  
  int i = 0;
  
  while (i < DATA1_LEN)
  {
    data_read = i2c_smbus_read_word_data(bme280_i2c_client, DATA1_START + i);
    reg_data1[i] = (u8)(data_read & 0xFF);
    i++;
  }
  
  i = 0;
  
  while (i < DATA2_LEN)
  {
    data_read = i2c_smbus_read_word_data(bme280_i2c_client, DATA2_START + i);
    reg_data2[i] = (u8)(data_read & 0xFF);
    i++;
  }

  // read calibration temperature registers (p. 24 of datasheet)
  dig_T1 = CONCAT_BYTES(reg_data1[1], reg_data1[0]);
  dig_T2 = CONCAT_BYTES(reg_data1[3], reg_data1[2]);
  dig_T3 = CONCAT_BYTES(reg_data1[5], reg_data1[4]);

  // read calibration pressure registers (p. 24 of datasheet)
  dig_P1 = CONCAT_BYTES(reg_data1[7], reg_data1[6]);
  dig_P2 = CONCAT_BYTES(reg_data1[9], reg_data1[8]);
  dig_P3 = CONCAT_BYTES(reg_data1[11], reg_data1[10]);
  dig_P4 = CONCAT_BYTES(reg_data1[13], reg_data1[12]);
  dig_P5 = CONCAT_BYTES(reg_data1[15], reg_data1[14]);
  dig_P6 = CONCAT_BYTES(reg_data1[17], reg_data1[16]);
  dig_P7 = CONCAT_BYTES(reg_data1[19], reg_data1[18]);
  dig_P8 = CONCAT_BYTES(reg_data1[21], reg_data1[20]);
  dig_P9 = CONCAT_BYTES(reg_data1[23], reg_data1[22]);

  // read calibration humidity registers
  dig_H1 = reg_data1[24];
  dig_H2 = CONCAT_BYTES(reg_data2[1], reg_data2[0]);
  dig_H3 = reg_data2[2];

  dig_h4_msb = (int16_t)(int8_t)reg_data2[3] * 16;
  dig_h4_lsb = (int16_t)(reg_data2[4] & 0x0F);
  dig_H4 = dig_h4_msb | dig_h4_lsb;

  dig_h5_msb = (int16_t)(int8_t)reg_data2[5] * 16;
  dig_h5_lsb = (int16_t)((reg_data2[4] & 0xF0) >> 4);
  dig_H5 = dig_h5_msb | dig_h5_lsb;
  dig_H6 = reg_data2[6];
}

static int bme280_open(struct inode *device_file, struct file *instance){
  pr_info("bme280: open was called\n");
  return 0;
}

static int bme280_close(struct inode *device_file, struct file *instance){
  pr_info("bme280: close was called\n");
  return 0;
}

static void bme280_update_data(void){
  read_calibration();

  temp = read_temperature();
  press64 = read_pressure_int64();
  hum = read_humidity();

  long tempScaled = temp;        
  long pressureScaled = press64; 
  long humidityScaled = hum;     
  
  sprintf(tempStr, "%ld.%02ld", tempScaled / 100, abs(tempScaled % 100));          
  sprintf(pressureStr, "%ld.%02ld", pressureScaled / 256, (pressureScaled % 256) * 100 / 256); 
  sprintf(humidityStr, "%ld.%01ld", humidityScaled / 1024, (humidityScaled % 1024) * 10 / 1024); 
};

  
static ssize_t bme280_read(struct file *fptr, char __user *buf, size_t len, loff_t *off){
  char retStr[31];
  int copyRes;
  
  bme280_update_data();
  
  pr_info("bme280 str: read temp: %s pressure: %s humidity: %s\n", tempStr, pressureStr, humidityStr);
  snprintf(retStr, sizeof(retStr), "T%sP%sH%s\n", tempStr, pressureStr, humidityStr);
  copyRes = copy_to_user(buf, retStr, strlen(retStr));
  
  return strlen(retStr);
};

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = bme280_open,
    .release = bme280_close,
    .read = bme280_read
};

static const struct i2c_device_id driver_id[] = {
  { BME280_DEVICE_NAME, 0 },
  { SSD1306_SLAVE_NAME, 1 },
  { }
};
MODULE_DEVICE_TABLE(i2c, driver_id);

static void bme280_probe(void)
{
    u8 id;
    // read id, which is always 0x60 for bme280
    id = i2c_smbus_read_byte_data(bme280_i2c_client, 0xD0);
  
    pr_info("id of bme280 is 0x%x\n", id);
  
    // set config register at 0xF5
    // set t_standby time
    i2c_smbus_write_byte_data(bme280_i2c_client, 0xf5, 0x3 << 5);
  
    // set ctrl_meas register at 0xF4
    // set temperature oversampling to max (b101)
    // set pressure oversampling to max (b101)
    // set normal mode (b11)
    i2c_smbus_write_byte_data(bme280_i2c_client, 0xf4, (0x5 << 5) | (0x5 << 2) | (0x3 << 0));
  
    // set ctrl_hum (humidity oversampling)
    i2c_smbus_write_byte_data(bme280_i2c_client, 0xf2, (0x3 << 0));
  
    // set the calibration registers
    read_calibration();
    
    pr_info("BME20 probed!\n");
}

static void ssd1306_probe(void)
{
  SSD1306_DisplayInit();

  SSD1306_SetCursor(0,0);  

  SSD1306_String("      TEAM 12\n");
  SSD1306_String("    BME280 Driver\n");
  SSD1306_String("\n");
  
  pr_info("OLED Probed!!!\n");
}

static void ssd1306_remove(void){
  SSD1306_SetCursor(0,0);  
  SSD1306_Fill(0x00);
  
  SSD1306_Write(true, 0xAE); // Entire Display OFF
  
  pr_info("OLED Removed!!!\n");
}

static struct i2c_driver bme280_i2c_driver = {
    .driver = {
        .name = BME280_DEVICE_NAME,
        .owner = THIS_MODULE,
    },
    .id_table = driver_id,
};

static void ssd1306_clear_info(void){
  SSD1306_SetCursor(3,0);
  for(int i = 0; i < 128*3; i++)
  {
      SSD1306_Write(false, 0);
  }
}

static void ssd1306_update_data(void){
  bme280_update_data();

  ssd1306_clear_info();
  
  SSD1306_SetCursor(3,0);
  SSD1306_String("Temperature: ");
  SSD1306_String(tempStr);
  SSD1306_String("C\n");
  
  SSD1306_String("Pressure: ");
  SSD1306_String(pressureStr);
  SSD1306_String("Pa\n");
  
  SSD1306_String("Humidity: ");
  SSD1306_String(humidityStr);
  SSD1306_String("%");
};

static void ssd1306_bme280_no_connect(void){
  SSD1306_SetCursor(3,0);
  SSD1306_String("BME280 not found!\n");
  SSD1306_String("No information!\n");
  SSD1306_String("Check connection!\n");
}

static int detect_device_thread(void *data) {
    while (!kthread_should_stop()) {
        if (i2c_smbus_read_byte(ssd1306_i2c_client) >= 0) {
            pr_info("SSD1306 device is connected.\n");
        } else {
            pr_info("SSD1306 device is disconnected.\n");
        }
        
        if (i2c_smbus_read_byte(bme280_i2c_client) >= 0) {
            pr_info("BME280 device is connected.\n");
            BME280_connected = 1;
            ssd1306_update_data();
        } else {
            pr_info("BME280 device is disconnected.\n");
            if (BME280_connected) {
                ssd1306_clear_info();
            }
            BME280_connected = 0;
            ssd1306_bme280_no_connect();
        }
        msleep(1500);
    }
    return 0;
}

static int init_driver(void){
  int ret = 0;
  pr_info("Driver initializing...\n");
  
  // Allocate major number
  if (alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME) < 0){
    pr_err("bme280: could not allocate major number\n");
    return -1;
  }

  // Create class
  dev_class = class_create(DEV_CLASS);
  if (IS_ERR(dev_class)){
    pr_err("bme280: failed to create class\n");
    goto ClassError;
  }

  // Create device
  if (IS_ERR(device_create(dev_class, NULL, dev_num, NULL, DEV_NAME))){
    pr_err("bme280: failed to create device\n");
    goto DeviceError;
  }

  // Initialize cdev and file operations on device file
  cdev_init(&bme280_cdev, &fops);

  // Add char device to system
  if (cdev_add(&bme280_cdev, dev_num, 1) == -1){
    pr_err("Failed to add device");
    goto KernelError;
  }

  // initialize i2c
  // client may already be registered. If so, skip
  driver_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
  
  if (driver_i2c_adapter != NULL){
    int ret;
    
    bme280_i2c_client = i2c_new_client_device(driver_i2c_adapter, &bme280_i2c_board_info);
    if (bme280_i2c_client == NULL) {
        pr_err("Failed to initialize bme280 I2C client\n");
        goto Bme280Error;
    }
    BME280_connected = 1;
    ssd1306_i2c_client = i2c_new_client_device(driver_i2c_adapter, &ssd1306_i2c_board_info);
    if (ssd1306_i2c_client == NULL) {
        pr_err("Failed to initialize SSD1306 I2C client\n");
        goto Ssd1306Error;
    }
    
    ret = i2c_add_driver(&bme280_i2c_driver);
    pr_info("Adding bme280 driver ret=%d\n", ret);
    if (ret < 0) {
        pr_err("Failed to add i2c driver \n");
        goto cleanup_i2c_driver;
    }
    detect_thread = kthread_run(detect_device_thread, NULL, "detect_thread");
    
    i2c_put_adapter(driver_i2c_adapter);
  }
  
  bme280_probe();
  ssd1306_probe();
  
  pr_info("Module init\n");
  return ret;

cleanup_i2c_driver:
  i2c_del_driver(&bme280_i2c_driver); 
Ssd1306Error:
  i2c_unregister_device(bme280_i2c_client);
Bme280Error:
  i2c_unregister_device(ssd1306_i2c_client);
KernelError:
  device_destroy(dev_class, dev_num);
DeviceError:
  class_destroy(dev_class);
ClassError:
  unregister_chrdev_region(dev_num, 1);
  return -1;
}

static void exit_driver(void){
  if (detect_thread) {
    kthread_stop(detect_thread);
    detect_thread = NULL;
  }
  
  ssd1306_remove();
  
  i2c_unregister_device(bme280_i2c_client);
  i2c_unregister_device(ssd1306_i2c_client);
  
  i2c_del_driver(&bme280_i2c_driver);
  
  cdev_del(&bme280_cdev);
  
  device_destroy(dev_class, dev_num);
  
  class_destroy(dev_class);
  
  unregister_chrdev_region(dev_num, 1);
  
  pr_info("bme280: Module removed \n");
  return;
}

module_init(init_driver);
module_exit(exit_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team 12");
MODULE_DESCRIPTION("BME280 Raspberrypi Driver");
