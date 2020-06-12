/*
 * ESP-01 ESP8266 IS USED AS A STAND-ALONE DEVICE. IT GETS AND SHOWS TIME INFORMATION,
 * WEATHER INFORMATION ON LED MATRIX 13X15 FROM NTP SERVER AND OPENWEATHERMAP.
 * BY TUENHIDIY - 11 JUNE, 2020.
 */
/*
// REAL PINS ON ESP-01 ESP8266

#define DATA_Pin          1  // GPIO1 - TX
#define CLOCK_Pin         2  // GPIO2
#define LATCH_Pin         0  // GPIO0
#define BLANK_Pin         3  // GPIO3 - RX

// CONNECTION
-------------------------------------------------
NAME    |  ESP-01     |  74HC595  |  TPIC6B595N |
-------------------------------------------------
/OE       GPIO3 - RX        13            9
LAT       GPIO0             12            12
CLK       GPIO2             11            13
SER       GPIO1 - TX        14            3
*/

// ESP8266 WiFi Main Library
#include <ESP8266WiFi.h>

// Libraries for Internet NTP Time
#include <NTPClient.h>          // https://github.com/arduino-libraries/NTPClient
#include <WiFiUdp.h>
#include <time.h>

// Libraries for Internet Weather
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

// Libraries for Font
#include "font3x5.h"
#include "font5x7.h"
#include "font8x8.h"
#include "font8x16.h"
#include "vnfont8x16.h"

#define DATA_Pin        1 // ESP-01 ESP8266 - GPIO1 - TX
#define CLOCK_Pin       2 // ESP-01 ESP8266 - GPIO2
#define LATCH_Pin       0 // ESP-01 ESP8266 - GPIO0
#define BLANK_Pin       3 // ESP-01 ESP8266 - GPIO3 - RX

#define FONT3x5         0
#define FONT5x7         1
#define FONT8x8         2
#define FONTVN8x16      3
#define FONTEN8x16      4

// WIFI and TIME
const char *WIFI_NETWORK_NAME = "FPT-Telecom"; // Change to your wifi network name
const char *WIFI_PASSWORD     = "12345678";   // Change to your wifi password

#define NTP_OFFSET    25200               // In seconds - Time offset (VIETNAM)
#define NTP_INTERVAL  60 * 1000           // In miliseconds
#define NTP_ADDRESS   "1.asia.pool.ntp.org"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
time_t rawtime;
struct tm *timeinfo;

uint16_t year;
uint8_t month;
uint8_t day;
uint8_t wday;
uint8_t hour;
uint8_t mins;
uint8_t sec;
uint8_t prevmins;

wchar_t YYYYMMDD[34];
wchar_t HHMMSS[32];
wchar_t WEATHER[106];

// Set openweathermap location and API key
String Location = "Vietnam,VN";
String API_Key  = "xxxxxxxxxxxxxxxxxxxxxxxxxx";

// Led Matrix Properties
// BAM resolution with 16 level brightness
#define BAM_RESOLUTION  4
byte row, level;
// Bit Angle Modulation variables to keep track of things
int BAM_Bit, BAM_Counter=0;
// Cathode array with BAM for column scanning
byte matrixBuffer[BAM_RESOLUTION][26];
// Anode array for row scanning
byte anode[16][2]= {{B11111111, B11111110}, {B11111111, B11111101}, {B11111111, B11111011}, {B11111111, B11110111},
                    {B11111111, B11101111}, {B11111111, B11011111}, {B11111111, B10111111}, {B11111111, B01111111}, 
                    {B01111110, B11111111}, {B11111101, B11111111}, {B11111011, B11111111}, {B11110111, B11111111},
                    {B11101111, B11111111}, {B11011111, B11111111}, {B10111111, B11111111}, {B01111111, B11111111}};


void __attribute__((optimize("O0"))) DIY_SPI(uint8_t DATA);
void ICACHE_RAM_ATTR timer1_ISR(void);   
void LED(uint8_t X, uint8_t Y, uint8_t RR);
void fillTable(uint8_t R);
void colorMorph(int time);
void clearscreen();

byte getPixelChar(uint8_t x, uint8_t y, wchar_t ch, uint8_t font);
byte getPixelHString(uint16_t x, uint16_t y, wchar_t *p,uint8_t font);
unsigned int lenString(wchar_t *p);
void printChar(uint8_t x, uint8_t y, uint8_t For_color, uint8_t Bk_color, char ch);
void hScroll(uint8_t y, byte For_color, byte Bk_color, wchar_t *mystring, uint8_t font, uint8_t delaytime, uint8_t times, uint8_t dir);

void setup () 
{      
  WiFi.begin(WIFI_NETWORK_NAME, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED)
  {
   hScroll(0, 15, 0, L"     CONNECTING TO WIFI......     ", FONTEN8x16, 30, 1, 1);
   delay(500);
  }
   
  timeClient.begin();

  row = 0;
  level = 0;
  //Serial.begin(9600,SERIAL_8N1,SERIAL_TX_ONLY);  
  pinMode(DATA_Pin, OUTPUT);
  pinMode(CLOCK_Pin, OUTPUT);
  pinMode(LATCH_Pin, OUTPUT);
  //pinMode(BLANK_Pin, OUTPUT);
  
  timer1_isr_init();
  timer1_attachInterrupt(timer1_ISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(500);
  clearscreen();

  if (WiFi.status() == WL_CONNECTED)
    {       
      //hScroll(0, 15, 0, L"     ESP8266-01 IS CONNECTED TO NTP SERVER...     ", FONTEN8x16, 30, 1, 1);
      hScroll(0, 15, 0, L"     HAVE FUN WITH ESP-01 ESP8266 & LED MATRIX 13x15 - INTERNET CLOCK AND WEATHER INFORMATION...     ", FONTEN8x16, 30, 1, 1);
      //hScroll(0, 15, 0, L"     Chúc các bạn vui !!! - Welcome to #tuenhidiy# YOUTUBE Channel...     ", FONTVN8x16, 30, 1, 1);
    }
    clearscreen();
}    

void loop()
{
  timeClient.update();
  rawtime = timeClient.getEpochTime();
  timeinfo = localtime (&rawtime);
   
  year = timeinfo->tm_year + 1900;
  month = timeinfo->tm_mon + 1;
  day   = timeinfo->tm_mday;
  wday  = timeinfo->tm_wday;
  hour  = timeinfo->tm_hour;
  mins  = timeinfo->tm_min;
  sec   = timeinfo->tm_sec;
  
  YYYYMMDD[0] = ' ' ;
  YYYYMMDD[1] = ' ' ;
  YYYYMMDD[2] = ' ' ;
  YYYYMMDD[3] = ' ' ;
  YYYYMMDD[4] = ' ' ;
  YYYYMMDD[5] = ' ' ;
  YYYYMMDD[6] = ' ' ;
  YYYYMMDD[7] = ' ' ;
  YYYYMMDD[8] = 'D' ;
  YYYYMMDD[9] = 'a' ;
  YYYYMMDD[10] = 't' ;
  YYYYMMDD[11] = 'e' ;
  YYYYMMDD[12] = ':' ;
  YYYYMMDD[13] = ' ' ;
  YYYYMMDD[14] = ((year/1000) % 10) + 48 ;
  YYYYMMDD[15] = ((year/ 100) % 10) + 48 ;
  YYYYMMDD[16] = ((year/10) %10) + 48 ;
  YYYYMMDD[17] = (year %10) + 48 ;
  YYYYMMDD[18] = '-' ;
  YYYYMMDD[19] = ((month/10) %10) + 48 ;
  YYYYMMDD[20] = (month %10) + 48 ;
  YYYYMMDD[21] = '-' ;
  YYYYMMDD[22] = ((day/10) %10) + 48 ;
  YYYYMMDD[23] = (day %10) + 48 ;
  YYYYMMDD[24] = ' ' ;
  YYYYMMDD[25] = ' ' ;
  YYYYMMDD[26] = ' ' ;
  YYYYMMDD[27] = ' ' ;
  YYYYMMDD[28] = ' ' ;
  YYYYMMDD[29] = ' ' ;
  YYYYMMDD[30] = ' ' ;
  YYYYMMDD[31] = ' ' ;
  YYYYMMDD[32] = ' ' ;
  YYYYMMDD[33] = '\0' ;
  
  HHMMSS[0] = ' ' ;
  HHMMSS[1] = ' ' ;
  HHMMSS[2] = ' ' ;
  HHMMSS[3] = ' ' ;
  HHMMSS[4] = ' ' ;
  HHMMSS[5] = ' ' ;
  HHMMSS[6] = ' ' ;
  HHMMSS[7] = ' ' ;
  HHMMSS[8] = 'H' ;
  HHMMSS[9] = 'o' ;
  HHMMSS[10] = 'u' ;
  HHMMSS[11] = 'r' ;
  HHMMSS[12] = '-' ;
  HHMMSS[13] = ' ' ;
  HHMMSS[14] = ((hour/10) %10) + 48 ;
  HHMMSS[15] = (hour%10) + 48 ;
  HHMMSS[16] = ':' ;
  HHMMSS[17] = ((mins/10) %10) + 48 ;
  HHMMSS[18] = (mins%10) + 48 ;
  HHMMSS[19] = ':' ;
  HHMMSS[20] = ((sec/10) %10) + 48 ;
  HHMMSS[21] = (sec%10) + 48 ;
  HHMMSS[22] = ' ' ;
  HHMMSS[23] = ' ' ;
  HHMMSS[24] = ' ' ;
  HHMMSS[25] = ' ' ;
  HHMMSS[26] = ' ' ;
  HHMMSS[27] = ' ' ;
  HHMMSS[28] = ' ' ;
  HHMMSS[29] = ' ' ;
  HHMMSS[30] = ' ' ;
  HHMMSS[31] = '\0' ;

  hScroll(0, 15, 0, YYYYMMDD, 4, 40, 1, 1); // Print "Date: YYYY-MM-DD"
  display_wday();                           // Print Day Of Week, ex: "TUESDAY"
  hScroll(0, 15, 0, HHMMSS, 4, 40, 1, 1);   // Print "Hour -  HH:MM:SS"
  
  if (prevmins != mins) // Updating weather information every 1 minute
    {
      HTTPClient http;  // Declare an object of class HTTPClient
 
      // specify request destination
      http.begin("http://api.openweathermap.org/data/2.5/weather?q=" + Location + "&APPID=" + API_Key);  // !!
 
      int httpCode = http.GET();  // Sending the request
 
      if (httpCode > 0)  // Checking the returning code
      {
        String payload = http.getString();   // Getting the request response payload
 
        DynamicJsonBuffer jsonBuffer(512);
 
        // Parse JSON object
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (!root.success())
        {
          hScroll(0, 15, 0, L"     Failed!     ", 4, 40, 1, 1);
          return;
        }
 
        int temp        = (int)(root["main"]["temp"]) - 273.15; // Get temperature in °C
        int humidity    = root["main"]["humidity"];             // Get humidity in %
        int pressure    = (int)(root["main"]["pressure"]);      // Get pressure in bar
        int wind_speed  = root["wind"]["speed"];                // Get wind speed in m/s
        int wind_degree = root["wind"]["deg"];                  // Get wind degree in °
        
        WEATHER[0] = ' ';
        WEATHER[1] = ' ';
        WEATHER[2] = ' ';
        WEATHER[3] = ' ';
        WEATHER[4] = ' ';
        WEATHER[5] = ' ';
        WEATHER[6] = ' ';
        WEATHER[7] = ' ';
        WEATHER[8] = 'T';
        WEATHER[9] = 'e';
        WEATHER[10] = 'm';
        WEATHER[11] = 'p';
        WEATHER[12] = 'e';
        WEATHER[13] = 'r';
        WEATHER[14] = 'a';
        WEATHER[15] = 't';
        WEATHER[16] = 'u';
        WEATHER[17] = 'r';
        WEATHER[18] = 'e';
        WEATHER[19] = ':';
        WEATHER[20] = ' ';
        WEATHER[21] = ((temp/10) %10) + 48;
        WEATHER[22] = (temp%10) + 48;
        WEATHER[23] = '*';
        WEATHER[24] = 'C';
        WEATHER[25] = ' ';
        WEATHER[26] = '-';
        WEATHER[27] = 'H';
        WEATHER[28] = 'u';
        WEATHER[29] = 'm';
        WEATHER[30] = 'i';
        WEATHER[31] = 'd';
        WEATHER[32] = 'i';
        WEATHER[33] = 't';
        WEATHER[34] = 'y';
        WEATHER[35] = ':';
        WEATHER[36] = ' ';
        WEATHER[37] = ((humidity/10) %10) + 48;
        WEATHER[38] = (humidity%10) + 48;
        WEATHER[39] = '%';
        WEATHER[40] = ' ';
        WEATHER[41] = '-';
        WEATHER[42] = 'P';
        WEATHER[43] = 'r';
        WEATHER[44] = 'e';
        WEATHER[45] = 's';
        WEATHER[46] = 's';
        WEATHER[47] = 'u';
        WEATHER[48] = 'r';
        WEATHER[49] = 'e';
        WEATHER[50] = ':';
        WEATHER[51] = ' ';
        WEATHER[52] = ((pressure/1000) % 10) + 48;
        WEATHER[53] = ((pressure/ 100) % 10) + 48;
        WEATHER[54] = ((pressure/10) %10) + 48;
        WEATHER[55] = (pressure%10) + 48;
        WEATHER[56] = 'h';
        WEATHER[57] = 'p';
        WEATHER[58] = 'a';
        WEATHER[59] = ' ';
        WEATHER[60] = '-';
        WEATHER[61] = 'W';
        WEATHER[62] = 'i';
        WEATHER[63] = 'n';
        WEATHER[64] = 'd';
        WEATHER[65] = ' ';
        WEATHER[66] = 'S';
        WEATHER[67] = 'p';
        WEATHER[68] = 'e';
        WEATHER[69] = 'e';
        WEATHER[70] = 'd';
        WEATHER[71] = ' ';
        WEATHER[72] = ((wind_speed/10) %10) + 48;
        WEATHER[73] = (wind_speed%10) + 48;
        WEATHER[74] = 'm';
        WEATHER[75] = '/';
        WEATHER[76] = 's';
        WEATHER[77] = ' ';
        WEATHER[78] = '-';
        WEATHER[79] = 'W';
        WEATHER[80] = 'i';
        WEATHER[81] = 'n';
        WEATHER[82] = 'd';
        WEATHER[83] = ' ';
        WEATHER[84] = 'D';
        WEATHER[85] = 'e';
        WEATHER[86] = 'g';
        WEATHER[87] = 'r';
        WEATHER[88] = 'e';
        WEATHER[89] = 'e';
        WEATHER[90] = ':';
        WEATHER[91] = ' ';
        WEATHER[92] = ((wind_degree/10) %10) + 48;
        WEATHER[93] = (wind_degree%10) + 48;
        WEATHER[94] = 'd';
        WEATHER[95] = 'e';
        WEATHER[96] = 'g';
        WEATHER[97] = ' ';
        WEATHER[98] = ' ';
        WEATHER[99] = ' ';
        WEATHER[100] = ' ';
        WEATHER[101] = ' ';
        WEATHER[102] = ' ';
        WEATHER[103] = ' ';
        WEATHER[104] = ' ';
        WEATHER[105] = '\0';
        
        hScroll(3, 15, 0, WEATHER, 2, 40, 1, 1); // Print "Temperature: 'AA'*C -Humidity: 'BB'% -Pressure: 'CCCC'hpa -Wind Speed: 'DD'm/s -Wind Degree: 'EE'deg"
      }
      http.end();   // Close connection
      prevmins = mins;
    }
}

void LED(uint8_t X, uint8_t Y, uint8_t BB)
{  
  uint8_t whichbyte = ((Y*2)+X/8);
  uint8_t whichbit = 7-(X % 8);
  
  for (byte BAM = 0; BAM < BAM_RESOLUTION; BAM++) 
  {
    bitWrite(matrixBuffer[BAM][whichbyte], whichbit, bitRead(BB, BAM));
  }
}

void fillTable(uint8_t BB)
{
    for (byte x=0; x<15; x++)
    {
      for (byte y=0; y<13; y++)
      {
        LED(x, y, BB);
      }
    }
}

void colorMorph(int timex)
{
  int keepColorTime = timex * 30;
  for(int blue = 0; blue <= 15; blue++)
  {
    fillTable(blue);
    delay(timex);
  }
  delay(keepColorTime);
  for(int blue = 15; blue >= 0; blue--)
  {
    fillTable(blue);
    delay(timex);
  }
  delay(keepColorTime);
}

void clearscreen()
{
  memset(matrixBuffer, 0, sizeof(matrixBuffer[0][0]) * BAM_RESOLUTION * 26);
}

void __attribute__((optimize("O0"))) DIY_SPI(uint8_t DATA)
{
    uint8_t i;
    uint8_t val;
    for (i = 0; i<8; i++)  
    {
      digitalWrite(DATA_Pin, !!(DATA & (1 << (7 - i))));
      digitalWrite(CLOCK_Pin,HIGH);
      digitalWrite(CLOCK_Pin,LOW);                
    }
}

void ICACHE_RAM_ATTR timer1_ISR(void)
{   
  digitalWrite(BLANK_Pin, HIGH);

  if(BAM_Counter==8)    // Bit weight 2^0 of BAM_Bit, lasting time = 8 ticks x interrupt interval time
  BAM_Bit++;
  else
  if(BAM_Counter==24)   // Bit weight 2^1 of BAM_Bit, lasting time = 24 ticks x interrupt interval time
  BAM_Bit++;
  else
  if(BAM_Counter==56)   // Bit weight 2^3 of BAM_Bit, lasting time = 56 ticks x interrupt interval time
  BAM_Bit++;
  BAM_Counter++;
  switch (BAM_Bit)
    {
    case 0:
        DIY_SPI(matrixBuffer[0][level + 0]);
        DIY_SPI(matrixBuffer[0][level + 1]);
      break;
    case 1:
        DIY_SPI(matrixBuffer[1][level + 0]);
        DIY_SPI(matrixBuffer[1][level + 1]);     
      break;
    case 2:
        DIY_SPI(matrixBuffer[2][level + 0]);
        DIY_SPI(matrixBuffer[2][level + 1]);       
      break;
    case 3:
        DIY_SPI(matrixBuffer[3][level + 0]);
        DIY_SPI(matrixBuffer[3][level + 1]);              
    if(BAM_Counter==120)    //Bit weight 2^3 of BAM_Bit, lasting time = 120 ticks x interrupt interval time
    {
    BAM_Counter=0;
    BAM_Bit=0;
    }
    break;
  }
  
  // Row scanning shifting out for 2 bytes
  DIY_SPI(anode[row][0]);
  DIY_SPI(anode[row][1]);
  
  digitalWrite(LATCH_Pin, HIGH);
  //delayMicroseconds(2);
  digitalWrite(LATCH_Pin, LOW);
  //delayMicroseconds(2);
  digitalWrite(BLANK_Pin, LOW);
  row++;
  level = row * 2; 
  if (row == 13) row=0;
  if (level == 26) level=0;
  
  pinMode(BLANK_Pin, OUTPUT);
  timer1_write(500);
}

void display_wday()
{
  switch(wday)
  {
    case 0:  hScroll(0, 15, 0, L"        SUNDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 1:  hScroll(0, 15, 0, L"        MONDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 2:  hScroll(0, 15, 0, L"        TUESDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 3:  hScroll(0, 15, 0, L"        WEDNESDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 4:  hScroll(0, 15, 0, L"        THURSDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 5:  hScroll(0, 15, 0, L"        FRIDAY       ", FONTEN8x16, 40, 1, 1); break;
    case 6: hScroll(0, 15, 0, L"        SATURDAY       ", FONTEN8x16, 40, 1, 1);
  }
}

byte getPixelChar(uint8_t x, uint8_t y, wchar_t ch, uint8_t font)
{
  if (font==FONT3x5)
  {
    if (x > 2) return 0; // 2 = font Width -1
    return bitRead(pgm_read_byte(&font3x5[ch-32][4-y]), 2-x); // 2 = Font witdh -1  
  }
  
  else if (font==FONT5x7)
  {
    if (x > 4) return 0; // 4 = font Width -1
    return bitRead(pgm_read_byte(&font5x7[ch-32][6-y]), 4-x); // 4 = Font witdh -1  
  }
  
  else if (font==FONTVN8x16)
  {
    if (x > 7) return 0; // 7 = font Width -1
    
    if ((ch >=32) && (ch <= 127))
    return bitRead(pgm_read_byte(&font8x16[ch-32][15-y]), 7-x);
    
    if ((ch >=192) && (ch <= 273))
    return bitRead(pgm_read_byte(&font8x16[ch-96][15-y]), 7-x);
    
    if ((ch >=296) && (ch <= 297))
    return bitRead(pgm_read_byte(&font8x16[ch-118][15-y]), 7-x);
    
    if ((ch >=360) && (ch <= 361))
    return bitRead(pgm_read_byte(&font8x16[ch-180][15-y]), 7-x);

    if ((ch >=416) && (ch <= 417))
    return bitRead(pgm_read_byte(&font8x16[ch-234][15-y]), 7-x);
     
    if ((ch >=431) && (ch <= 432))
    return bitRead(pgm_read_byte(&font8x16[ch-247][15-y]), 7-x);
    
    if ((ch >=7840) && (ch <= 7929))
    return bitRead(pgm_read_byte(&vnfont8x16[ch-7840][15-y]), 7-x); // 7 = Font witdh -1  
  }
  else if (font==FONT8x8)
  {
    if (x > 7) return 0; // 7 = font Width -1
    return bitRead(pgm_read_byte(&font8x8[ch-32][7-y]), 7-x); // 7 = Font witdh -1  
  }
  else if (font==FONTEN8x16)
  {
    if (x > 7) return 0; // 7 = font Width -1
    return bitRead(pgm_read_byte(&font8x16[ch-32][15-y]), 7-x); // 7 = Font witdh -1  
  }  
}
byte getPixelHString(uint16_t x, uint16_t y, wchar_t *p,uint8_t font)

{
  if (font==FONT3x5)
  {
    p=p+x/4;
    return getPixelChar(x%4, y, *p, FONT3x5);
  }
  
  else if (font==FONT5x7)
  {
    p=p+x/6;
    return getPixelChar(x%6, y, *p, FONT5x7);
  }
  
  else if (font==FONTVN8x16)
  {
    p=p+x/9;
    return getPixelChar(x%9, y, *p, FONTVN8x16);
  }
  else if (font==FONT8x8)
  {
    p=p+x/8;
    return getPixelChar(x%8, y, *p, FONT8x8);  
  }

  else if (font==FONTEN8x16)
  {
    p=p+x/9;
    return getPixelChar(x%9, y, *p, FONTEN8x16); 
  }
}

unsigned int lenString(wchar_t *p)
{
  unsigned int retVal=0;
  while(*p!='\0')
  { 
   retVal++;
   p++;
  }
  return retVal;
}


void printChar(uint8_t x, uint8_t y, uint8_t For_color, uint8_t Bk_color, char ch)
{
  uint8_t xx,yy;
  xx=0;
  yy=0;
    
  for (yy=0; yy < 5; yy++)
    {
    for (xx=0; xx < 3; xx++)
      {
      if (bitRead(pgm_read_byte(&font3x5[ch-32][4-yy]),2-xx))
      
        {
            LED(x+xx, y+yy, For_color);
        }
      else
        {
            LED(x+xx, y+yy, Bk_color);      
        }
      }
    }
}

void hScroll(uint8_t y, byte For_color, byte Bk_color, wchar_t *mystring, uint8_t font, uint8_t delaytime, uint8_t times, uint8_t dir)
{
  int offset;
  byte color;
  // TINY FONT 3x5
  if (font == FONT3x5)
  {
  while (times)
    {
    for ((dir) ? offset=0 : offset=((lenString(mystring)-4)*4-1) ; (dir) ? offset <((lenString(mystring)-4)*4-1) : offset >0; (dir) ? offset++ : offset--)
      {
      for (byte xx=0; xx<15; xx++)
        {
        for (byte yy=0; yy<5; yy++)
            {            
              if (getPixelHString(xx+offset, yy, mystring, FONT3x5))
              {
                color = For_color;                
              }
              else 
              {
                color = Bk_color;
              }
                LED(xx, (yy+y), color);
            }
        }
        delay(delaytime);  
      }
    times--;
    }
  }

  // FONT 5x7
  
  else if (font == FONT5x7)
  {
  while (times)
    {
    for ((dir) ? offset=0 : offset=((lenString(mystring)-5)*6-1) ; (dir) ? offset <((lenString(mystring)-5)*6-1) : offset >0; (dir) ? offset++ : offset--)
      {
      for (byte xx=0; xx<15; xx++)
        {
        for (byte yy=0; yy<7; yy++)
            {            
              if (getPixelHString(xx+offset, yy, mystring, FONT5x7))
              {
                color = For_color;                
              }
              else 
              {
                color = Bk_color;
              }
                LED(xx, (yy+y), color);
            }
        }
        delay(delaytime);  
      }
    times--;
    }
  }
//VIETNAMESE FONT 8x16      
  else if (font == FONTVN8x16)
  {
  while (times)
    {
    for ((dir) ? offset=0 : offset=((lenString(mystring)-6)*9-1) ; (dir) ? offset <((lenString(mystring)-6)*9-1) : offset >0; (dir) ? offset++ : offset--)
      {
      for (byte xx=0; xx<15; xx++)
        {
        for (byte yy=0; yy<13; yy++)
            {            
              if (getPixelHString(xx+offset, yy, mystring, FONTVN8x16))
              {
                color = For_color;                
              }
              else 
              {
                color = Bk_color;
              }
                LED(xx, (yy+y), color);
            }
        }
        delay(delaytime);  
      }
    times--;
    }
  }
// FONT 8x8
  else if (font == FONT8x8)
    {
    while (times)
      {
      for ((dir) ? offset=0 : offset=((lenString(mystring)-6)*8-1); (dir) ? offset <((lenString(mystring)-6)*8-1): offset >0; (dir) ? offset++ : offset--)
        {
        for (byte xx=0; xx<15; xx++)
          {
            for (byte yy=0; yy<8; yy++)
              {
                if (getPixelHString(xx+offset, yy, mystring, FONT8x8)) 
                  {
                  color = For_color;
                  }
                else 
                {
                  color = Bk_color;
                }
                  LED(xx, (yy+y), color);
              }          
            }
      delay(delaytime);  
        }
      times--;
      }
    }
// ENGLISH FONT 8x16  
   else if (font == FONTEN8x16)
    {
    while (times)
    {
    for ((dir) ? offset=0 : offset=((lenString(mystring)-6)*9-1); (dir) ? offset <((lenString(mystring)-6)*9-1) : offset >0; (dir) ? offset++ : offset--)
      {
      for (byte xx=0; xx<15; xx++)
        {     
            for (byte yy=0; yy<13; yy++)
              {
                if (getPixelHString(xx+offset, yy, mystring, FONTEN8x16)) 
                  {
                  color = For_color;
                  }
                else 
                {
                  color = Bk_color;
                }
                  LED(xx, (yy+y), color);
              }   
          }
          delay(delaytime);  
        }
        times--;
      } 
    }
 }
