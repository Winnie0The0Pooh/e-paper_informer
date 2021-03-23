/* The MIT License (MIT)
* 
* Copyright (c) 2020 Sergey Dronsky (Winnie_The_Pooh)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

// e-paper informer Sergey Dronsky(C) 2021
// The device read and show bmp picture from IIS running on home server
// Python script is running on the same server
// Python script makes a BMP picture (300*400, 3 colours: white, black, red) 
// Python script takes data from various sources: internal home sensors, weather server,  etc.
// the advantage of this solution is the ability to form a complex image (any vector fonts)
// and asynchronous use of data from any sources
// and much more user friendly environment with big screen, speed etc.

// GxEPD_WiFi_Example : Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: these e-papers require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: http://www.e-paper-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// BMP handling code extracts taken from: https://github.com/prenticedavid/MCUFRIEND_kbv/tree/master/examples/showBMP_kbv_Uno
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD

// Supporting Arduino Forum Topics:
// Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
// Good Dispay ePaper for Arduino : https://forum.arduino.cc/index.php?topic=436411.0

// mapping suggestion from Waveshare SPI e-Paper to Wemos D1 mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V

// mapping suggestion from Waveshare SPI e-Paper to generic ESP8266
// BUSY -> GPIO4, RST -> GPIO2, DC -> GPIO0, CS -> GPIO15, CLK -> GPIO14, DIN -> GPIO13, GND -> GND, 3.3V -> 3.3V

// mapping of Waveshare e-Paper ESP8266 Driver Board
// BUSY -> GPIO16, RST -> GPIO5, DC -> GPIO4, CS -> GPIO15, CLK -> GPIO14, DIN -> GPIO13, GND -> GND, 3.3V -> 3.3V


// include library, include base class, make path known
#include <GxEPD.h>

// select the display class to use, only one
#include <GxGDEW042Z15/GxGDEW042Z15.h>    // 4.2" b/w/r

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <EEPROM.h>

#if defined(ESP8266)

// for SPI pin definitions see e.g.:
// C:\Users\xxx\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.2\variants\generic\common.h

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io /*RST=D4*/ /*BUSY=D2*/); // default selection of D4(=2), D2(=4)

#elif defined(ESP32)

// for SPI pin definitions see e.g.:
// C:\Users\xxx\Documents\Arduino\hardware\espressif\esp32\variants\lolin32\pins_arduino.h

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4); // arbitrary selection of (16), 4

#endif

#if defined (ESP8266)
#include <ESP8266WiFi.h>
#define USE_BearSSL true
#endif

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

const char* ssid     = "........"; // enter your data here
const char* password = "........";


const char * headerKeys[] = {"date", "server"};
const size_t numberOfHeaders = 2;

const int httpPort  = 80;

void showBitmapFrom_HTTP(const char* host, const char* path, const char* filename, int16_t x, int16_t y, bool with_color = true);

ADC_MODE(ADC_VCC); //включить измерение напряжения питания

String curr_date;
bool batt_low;
uint8_t ct_starts_no_communication;
uint16_t batt_low_porog = 2700; //Порог отключения системы по разряду батареи
uint16_t batt_high_porog = 3000; //Порог включения системы по зарядке батареи

String f = String(__FILE__); //compiler predefined macro, returns file name
String t = String(__TIME__); //returns time when compile
String c = f.substring(f.lastIndexOf('\\')+1, f.indexOf('.')) + " " + String(__DATE__) + " " + t.substring(0, t.lastIndexOf(':'));

int upit = 0; //ESP.getVcc();
uint32_t chipid = ESP.getChipId();
String coreversion = "";
uint8_t cpuf = 0;
uint32_t sksize = 0;
uint32_t freesize = 0;
 
void setup()
{

  delay(100);
  Serial.begin(115200);
  Serial.setDebugOutput(1);

  upit = ESP.getVcc();
  
  EEPROM.begin(16); //два раза по 8 байт
  EEPROM.get(0,batt_low);
  EEPROM.get(8,ct_starts_no_communication);

//  flash(10);

  Serial.println();
  Serial.println("E-paper informer (c) Sergey Dronsky. Based on GxEPD_WiFi_Example");

  Serial.println();
  Serial.print("Version: "); Serial.println(c);
  Serial.print("OS path: "); Serial.println(f);
  
  // справочные данные 

  //int upit = ESP.getVcc();
  upit = ESP.getVcc();
  chipid = ESP.getChipId();
  coreversion = ESP.getCoreVersion();
  cpuf = ESP.getCpuFreqMHz();
  sksize = ESP.getSketchSize();
  freesize = ESP.getFreeSketchSpace();
  
  Serial.printf("Chip ID: %d CoreVersion: %s CPU freq: %d VCC: %d mV \nProg size: %d free space: %d Free memory %d\n\n",ESP.getChipId(), ESP.getCoreVersion().c_str(), ESP.getCpuFreqMHz(), ESP.getVcc(), ESP.getSketchSize(), ESP.getFreeSketchSpace(), ESP.getFreeHeap());
  //  os_printf("\nChip ID: %d CoreVersion: %s CPU freq: %d VCC: %d mV \nProg size: %d free space: %d Free memory %d\n\n",ESP.getChipId(), ESP.getCoreVersion().c_str(), ESP.getCpuFreqMHz(), ESP.getVcc(), ESP.getSketchSize(), ESP.getFreeSketchSpace(), ESP.getFreeHeap());
  
  Serial.print("Vcc:");
  Serial.println(upit);
  
//  Serial.println(upit);
//  Serial.println(batt_low);
//  Serial.println(ct_starts_no_communication);

  
  if(upit < batt_low_porog and batt_low) //батарея разряжена и уже предупредили
  {
    Serial.println("batt low, there was a warning");
    digitalWrite(5, HIGH); // Выключить питание
    while(1)
    {
      flash(1);
      delay(1000);
    }
  }

  if(upit < batt_low_porog and !batt_low) //обнаружили разряженную батарею в первый раз
  {
    Serial.println("batt low first time 1=======2");
    batt_low = true;
    flash(2);
  }

  if(upit > batt_high_porog and batt_low) //батарея заряжена после разрядки
  {
    Serial.println("batt charged after warning=======3");
    batt_low = false;
    ct_starts_no_communication=0;
    EEPROM.begin(16); //два раза по 32 байта
    EEPROM.put(0,batt_low);
    EEPROM.put(8,ct_starts_no_communication);
    EEPROM.commit();
    flash(3);

  }

  display.init(115200);

  if (!WiFi.getAutoConnect() || ( WiFi.getMode() != WIFI_STA) || ((WiFi.SSID() != ssid) && String(ssid) != "........"))
  {
    Serial.println();
    Serial.print("WiFi.getAutoConnect()=");
    Serial.println(WiFi.getAutoConnect());
    Serial.print("WiFi.SSID()=");
    Serial.println(WiFi.SSID());
    WiFi.mode(WIFI_STA); // switch off AP
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  }
  int ConnectTimeout = 60; // 15 seconds
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status());
    if (--ConnectTimeout <= 0)
    {
      Serial.println();
      
      Serial.println("WiFi connect timeout");
    
    while(1)
    {
      flash(4); // Этот код выполнится до первого перехода ноги D1 из 0 в 1
      delay(1000);
    }
      return; // goto loop. Will not work because previous flash() will switch off power
    }
  }
  Serial.println();
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());

  if(batt_low) //Обнаружена разряженная батарея в первый раз
  {
      Serial.println("batt low first time");
      
      showBitmapFrom_HTTP("192.168.1.74", "/", "batt.bmp", 0, 0, true);
      
      send_get("192.168.1.74", "/File/CreateOrRewrite/");
     
      EEPROM.begin(16); //два раза по 32 байта
      EEPROM.put(0,batt_low);
      EEPROM.put(8,ct_starts_no_communication);
      EEPROM.commit();

      delay(200); //commit в принципе должен все записать и потом выпасть, но лишние 200 мс не повредят
//      digitalWrite(5, HIGH); //выключить питание
      while(1)
      {
        flash(5);
        delay(1000);
      }
  }

    showBitmapFrom_HTTP("192.168.1.74", "/", "sample-out.bmp", 0, 0, true);
    
    send_get("192.168.1.74", "/File/CreateOrRewrite/");

    

      while(1)
      {
        flash(3);
        delay(1000);
      }

}

void loop(void)
{
     flash(10); //in principle this code must never run
}

void flash(int ct)
{
  digitalWrite(5, LOW);
  pinMode(5, OUTPUT); //D1 GPIO5
  digitalWrite(5, LOW);

  delay(200);
  for(int i=0; i < ct; i++)
  {
    digitalWrite(5, HIGH);
    delay(200);
    digitalWrite(5, LOW);
    delay(200);
  }
  digitalWrite(5, LOW);
  delay(200);
}

static const uint16_t input_buffer_pixels = 640; // may affect performance

static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

void drawBitmapFrom_HTTP_ToBuffer(const char* host, const char* path, const char* filename, int16_t x, int16_t y, bool with_color)
{
  WiFiClient client;
  bool connection_ok = false;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top
  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height())) return;
  
//  display.fillScreen(GxEPD_WHITE); =====================
  
  Serial.print("connecting to "); Serial.println(host);
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.print("requesting URL: ");
  Serial.println(String("http://") + host + path + filename);
  client.print(String("GET ") + path + filename + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: GxEPD_WiFi_Example\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("request sent");
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    
    if(line.startsWith("Date"))
    {
      curr_date = line;
      Serial.print("============ ");
      Serial.println(curr_date);
    }
    
    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok) Serial.println(line);
      //if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok) Serial.println(line);
//    Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }
  if (!connection_ok) return;
  // Parse BMP header
  if (read16(client) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(client);
    uint32_t creatorBytes = read32(client);
    uint32_t imageOffset = read32(client); // Start of image data
    uint32_t headerSize = read32(client);
    uint32_t width  = read32(client);
    uint32_t height = read32(client);
    uint16_t planes = read16(client);
    uint16_t depth = read16(client); // bits per pixel
    uint32_t format = read32(client);
    uint32_t bytes_read = 7 * 4 + 3 * 2; // read so far
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish, colored;
      if (depth == 1) with_color = false;
      if (depth <= 8)
      {
        if (depth < 8) bitmask >>= depth;
        //bytes_read += skip(client, 54 - bytes_read); //palette is always @ 54
        bytes_read += skip(client, imageOffset - (4 << depth) - bytes_read); // 54 for regular, diff for colorsimportant
        for (uint16_t pn = 0; pn < (1 << depth); pn++)
        {
          blue  = client.read();
          green = client.read();
          red   = client.read();
          client.read();
          bytes_read += 4;
          whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
          if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;
          if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
          color_palette_buffer[pn / 8] |= colored << pn % 8;
          //Serial.print("0x00"); Serial.print(red, HEX); Serial.print(green, HEX); Serial.print(blue, HEX);
          //Serial.print(" : "); Serial.print(whitish); Serial.print(", "); Serial.println(colored);
        }
      }
      
      display.fillScreen(GxEPD_WHITE);
      
      uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
      //Serial.print("skip "); Serial.println(rowPosition - bytes_read);
      bytes_read += skip(client, rowPosition - bytes_read);
      for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
      {
        if (!connection_ok || !(client.connected() || client.available())) break;
        delay(1); // yield() to avoid WDT
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0; // for depth <= 8
        uint8_t in_bits = 0; // for depth <= 8
        uint16_t color = GxEPD_WHITE;
        for (uint16_t col = 0; col < w; col++) // for each pixel
        {
          yield();
          if (!connection_ok || !(client.connected() || client.available())) break;
          // Time to read more pixel data?
          if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
          {
            uint32_t get = in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain;
            uint32_t got = read(client, input_buffer, get);
            while ((got < get) && connection_ok)
            {
              //Serial.print("got "); Serial.print(got); Serial.print(" < "); Serial.print(get); Serial.print(" @ "); Serial.println(bytes_read);
              uint32_t gotmore = read(client, input_buffer + got, get - got);
              got += gotmore;
              connection_ok = gotmore > 0;
            }
            in_bytes = got;
            in_remain -= got;
            bytes_read += got;
          }
          if (!connection_ok)
          {
            Serial.print("Error: got no more after "); Serial.print(bytes_read); Serial.println(" bytes read!");
            break;
          }
          switch (depth)
          {
            case 24:
              blue = input_buffer[in_idx++];
              green = input_buffer[in_idx++];
              red = input_buffer[in_idx++];
              whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              break;
            case 16:
              {
                uint8_t lsb = input_buffer[in_idx++];
                uint8_t msb = input_buffer[in_idx++];
                if (format == 0) // 555
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                  red   = (msb & 0x7C) << 1;
                }
                else // 565
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                  red   = (msb & 0xF8);
                }
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              }
              break;
            case 1:
            case 4:
            case 8:
              {
                if (0 == in_bits)
                {
                  in_byte = input_buffer[in_idx++];
                  in_bits = 8;
                }
                uint16_t pn = (in_byte >> bitshift) & bitmask;
                whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                in_byte <<= depth;
                in_bits -= depth;
              }
              break;
          }
          if (whitish)
          {
            color = GxEPD_WHITE;
          }
          else if (colored && with_color)
          {
            color = GxEPD_RED;
          }
          else
          {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.drawPixel(x + col, yrow, color);
        } // end pixel
      } // end line
    }
    Serial.print("bytes read "); Serial.println(bytes_read);
  }
  Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

void showBitmapFrom_HTTP(const char* host, const char* path, const char* filename, int16_t x, int16_t y, bool with_color)
{
  Serial.println(); Serial.print("downloading file \""); Serial.print(filename);  Serial.println("\"");
  drawBitmapFrom_HTTP_ToBuffer(host, path, filename, x, y, with_color);
  display.update();
}

uint16_t read16(WiFiClient& client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read(); // MSB
  return result;
}

uint32_t read32(WiFiClient& client)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = client.read(); // LSB
  ((uint8_t *)&result)[1] = client.read();
  ((uint8_t *)&result)[2] = client.read();
  ((uint8_t *)&result)[3] = client.read(); // MSB
  return result;
}


uint32_t skip(WiFiClient& client, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      int16_t v = client.read();
      remain--;
    }
    else delay(1);
    if (millis() - start > 2000) break; // don't hang forever
  }
  return bytes - remain;
}

uint32_t read(WiFiClient& client, uint8_t* buffer, int32_t bytes)
{
  int32_t remain = bytes;
  uint32_t start = millis();
  while ((client.connected() || client.available()) && (remain > 0))
  {
    if (client.available())
    {
      int16_t v = client.read();
      *buffer++ = uint8_t(v);
      remain--;
    }
    else delay(1);
    if (millis() - start > 2000) break; // don't hang forever
  }
  return bytes - remain;
}

void send_get(const char* host, const char* path)
{
    Serial.println("send_get()");

    String buffs="";
    int lb = 200;
    char buff[lb];
    os_sprintf(buff,"Chip ID=%d CoreVersion=%s CPU freq=%d VCC=%d mV Prog size=%d free space=%d Free memory=%d",ESP.getChipId(), ESP.getCoreVersion().c_str(), ESP.getCpuFreqMHz(), ESP.getVcc(), ESP.getSketchSize(), ESP.getFreeSketchSpace(), ESP.getFreeHeap());
    Serial.println(buff);
    buffs = String((char*)buff);

//    for (int i = 0; i < lb; i++) buffs += (char)buff[i]; //0==buff[i]

    Serial.println(buffs);
//    curr_date = "Date: Mon, 01 Feb 2021 10:38:58 GMT"

    String Vcc;
//    Vcc="vcc.txt/Vcc=" + String(upit) + " " + buffs;

    curr_date.trim();
    Vcc = curr_date + " Vcc=" + String(upit) + ".txt/Vcc=" + String(upit) + " Ver=" + c + " " + buffs;

//    Vcc += " Ver=" + c;
    //Vcc += " OS path=" + f;

    Vcc.replace(" GMT ", " ");
    Vcc.replace("  ", " ");
    Vcc.replace(":", "_");
    Vcc.replace(" ", "%20");
    
    Serial.println(Vcc);
    
      WiFiClient client;
  bool connection_ok = false;

  uint32_t startTime = millis();

  Serial.print("connecting to "); Serial.println(host);
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.print("requesting URL: ");
  
  //http://atom-pc/File/CreateOrRewrite/kek.txt/qwe123555

  
  Serial.println(String("http://") + host + path + Vcc);
  
  client.print(String("GET ") + path + Vcc + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: e-paper_informer\r\n" +
               "Connection: close\r\n\r\n");
  
  Serial.println("request sent");
  
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    
    
    if (!connection_ok)
    {
      connection_ok = line.startsWith("HTTP/1.1 200 OK");
      if (connection_ok) Serial.println(line);
      //if (!connection_ok) Serial.println(line);
    }
    if (!connection_ok) Serial.println(line);
//    Serial.println(line);
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }
}
