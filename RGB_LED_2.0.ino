/*
 * RGB_LED 
 * Arduino sketch ver 2.1
 * for WinApp ver 1.1.2 or high
 * Author: Stanislav TEEN Eshkov
 */
// USER SETTINGS BLOCK
#define PIN 4
#define MaxPixels 68
bool RandomColorInTest = true;
// SYSTEM SETTINGS BLOCK
bool isDebug = false;
bool isDebugAll = true; // when false - debug CRC errors only
int random_int;
// END OF BLOCK

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip = Adafruit_NeoPixel(MaxPixels, PIN, NEO_GRB + NEO_KHZ800);
byte data[64];
byte data_length;
byte data_buf[64];
byte buf_offset;
byte command[5];
byte last_command_offset;
unsigned long bytes_total;


bool isWait = true;
//byte MaxBright;
void setup() {
  randomSeed(analogRead(0));
  Serial.begin(115200);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  if (isWait) //wait for first serial data
  {
      CheckTest();
  }
  // put your main code here, to run repeatedly:
  data_length = Serial.available();
  if (data_length != 0)
  {
    Serial.readBytes(data,data_length);
    bytes_total = bytes_total + data_length;
    if (isDebug && isDebugAll) 
    {
      Serial.print("T>");
      Serial.println(bytes_total); 
    }
    buf_append_new_data(data,data_length);
  }
  last_command_offset = buf_check_commands(data_buf, buf_offset);
  if (isDebug && isDebugAll && last_command_offset != 0)
  {
    Serial.print("last command end at: "); Serial.println(last_command_offset);   
    Serial.print("checking buffer content: ");
    for (byte i = 0; i < 64; i++)
    {
      Serial.print(data_buf[i],HEX);
      Serial.print(",");
    }
    Serial.println();  
  }
  if (last_command_offset != 0)
  {
    buf_offset = buf_clean(data_buf, last_command_offset);
    if (isDebug && isDebugAll && buf_offset != 0)
    {
      Serial.print("buf clean. New buf offset: "); Serial.println(buf_offset);      
    }
  }
}

// append new any data to buffer
void buf_append_new_data(byte c_data[], byte i_len)
{
  for (byte i = 0; i < i_len; i++)
  {
    data_buf[buf_offset + i] = c_data[i];
  }
  buf_offset = buf_offset + i_len;
  memset(c_data, 0, sizeof(*c_data));
}

// find commands in buffer
byte buf_check_commands(byte i_buf[], byte i_offset)
{
  byte last_offset = 0;
  // check standard command
  for (byte i = 0; i < i_offset; i++)
  {
    // if command founded at
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x41 )  //A
        //i_buf[i+0x07] != 0x00 )
    {
      //check crc for command
      if (check_crc(i_buf,i) == true)
      {
        //executing command
        if (isDebug && isDebugAll)
        {
          Serial.print("executing command. At: "); Serial.println(i);      
        }
        buf_execute_command(i_buf,i);
        last_offset = i+0x08;
      }
      else
      {
        if (isDebug)
        {
          Serial.print("Error executing command at: "); Serial.println(i); 
          Serial.print("in buffer content: ");
          for (byte e = 0; e < 64; e++)
          {
            Serial.print(i_buf[e],HEX);
            Serial.print(",");
          }
          Serial.println();               
        }
      }
    }
  }
  // check system command
  for (byte i = 0; i < i_offset; i++)
  {
    // if command founded at
    // reset arduino by command
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x42 && //B
        i_buf[i+0x02] == 0xFB && //ы
        i_buf[i+0x03] == 0xFB)   //ы
    {
      if(isDebug && isDebugAll) Serial.println("Reseting...");
      delay(100);
      SoftReset();
    }

    // enable/disable debug mode
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x42 && //B
        i_buf[i+0x02] == 0xFA && //ъ
        i_buf[i+0x03] == 0xFA)   //ъ
    {
      isDebug = !isDebug;
      Serial.print("Debug = ");
      Serial.println(isDebug);
      last_offset = i + 0x05;
    }

    // update led
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x42 && //B
        i_buf[i+0x02] == 0xFC && //ь
        i_buf[i+0x03] == 0xFC)   //ь
    {
      strip.show();
      last_offset = i + 0x05;
    }

    // rainbow preset
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x42 && //B
        i_buf[i+0x02] == 0xFD && //э
        i_buf[i+0x03] == 0xFD)   //э
    {
      rainbow(i_buf[i+0x04]);
      last_offset = i + 0x06;
    }

    // rainbow "cycle" preset
    if (i_buf[i]      == 0x40 && //@
        i_buf[i+0x01] == 0x42 && //B
        i_buf[i+0x02] == 0xFE && //ю
        i_buf[i+0x03] == 0xFE)   //ю
    {
      rainbowCycle(i_buf[i+0x04]);
      last_offset = i + 0x06;
    }
  }
  return last_offset;
}

// executing command for LED
byte buf_execute_command(byte i_buf[], byte offset)
{
  if (i_buf[offset + 0x02] == 0xFF)//я
  {
    for (byte l = 0; l<=MaxPixels; l++)
    {
      strip.setPixelColor(l, strip.Color(i_buf[offset + 0x03], 
                                         i_buf[offset + 0x04], 
                                         i_buf[offset + 0x05]));
    }
  }
  else if(i_buf[offset + 0x02] >= 0 && 
          i_buf[offset + 0x02] <= MaxPixels)
  {
    strip.setPixelColor(MaxPixels - i_buf[offset + 0x02], strip.Color(i_buf[offset + 0x03], 
                                                                      i_buf[offset + 0x04], 
                                                                      i_buf[offset + 0x05]));
  }
  
  if (i_buf[offset + 0x06] != 0)
  {
    strip.show();
  }
}

byte buf_clean(byte c_buf[], byte i_last_offset)
{
  byte temp_buf[64];
  memset(temp_buf, 0, sizeof(temp_buf));
  byte last_pos = 0;
  for (byte i = i_last_offset; i < 64; i++)
  {
    temp_buf[i - i_last_offset] = c_buf[i];
  }
  memset(c_buf, 0, sizeof(*c_buf));
  memcpy(c_buf,temp_buf,sizeof(temp_buf)) ;
  if (isDebug && isDebugAll)
  {
    Serial.print("new buf content: "); 
    for (byte i = 0; i < 64; i++)
    {
      Serial.print(c_buf[i],HEX);
      Serial.print(",");
    }
    Serial.println();      
  }
  if (buf_offset - i_last_offset >= 0)
  {
    return buf_offset - i_last_offset; 
  }
  else
  {
    return 0;
  }
}

// check CRC sum for command
bool check_crc(byte data[],byte offset)
{
  int crc = 0x00;
  for (byte i = 0; i <= 4; i++)
  {
    crc += (int)data[offset + 0x02 + i];
  }
  crc = ( (int)(crc / 5) + 1 ) % 0xFF;
  if ((int)data[offset + 0x07] == crc)
  {
    if (isDebug && isDebugAll)
    {
      Serial.print("crc correct: Calc(");
      Serial.print(crc,HEX);
      Serial.print(") = Data(");
      Serial.print((int)data[offset + 0x07],HEX);
      Serial.println(")");
    }
    return true;
  }
  else
  {
    if (isDebug)
    {
      Serial.print("crc wrong: Calc(");
      Serial.print(crc,HEX);
      Serial.print(") != Data(");
      Serial.print((int)data[offset + 0x07],HEX);
      Serial.println(")");
    }
    return false;
  }
}

void CheckTest()
{
  uint32_t p_color = strip.Color(255, 255, 255);
  
  if (RandomColorInTest)
  {
    p_color = get_random_color();
  }
  for (int i=0; i<=MaxPixels; i++)
  {
    strip.setPixelColor(i, p_color);
    if(i>3) strip.setPixelColor(i-4, 0);
    strip.show();
    if (Serial.available() != 0)
    {
      isWait = false;
      return;
    }
    else
    {
      delay(15);
    }
  }
  
  if (RandomColorInTest)
  {
    p_color = get_random_color();
  }
  for (int i=MaxPixels; i>=0; i--)
  {
    strip.setPixelColor(i, p_color);
    if(i<MaxPixels-3) strip.setPixelColor(i+4, 0);
    strip.show();
    if (Serial.available() != 0)
    {
      isWait = false;
      return;
    }
    else
    {
      delay(15);
    }
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  while (1)
  {
    for(j=0; j<256; j++) {
      for(i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel((i+j) & 255));
        if (Serial.available() != 0)
          return;
      }
      strip.show();
      delay(wait);
    }
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  while (1)
  {
    for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
      for(i=0; i< strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        if (Serial.available() != 0)
          return;
      }
      strip.show();
      delay(wait);
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void SoftReset()
{
  asm volatile ("  jmp 0");
}

uint32_t get_random_color()
{
  return strip.Color((uint8_t)(random(255)), 
                     (uint8_t)(random(255)), 
                     (uint8_t)(random(255)));
}

