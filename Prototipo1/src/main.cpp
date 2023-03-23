#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>

// SD
#include <SPI.h>
#include <SD.h>
#include <ESP32Time.h>

// Humidity Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
String h = "";
int humidity = 1;

// ADS1015/Current Sensor
#include <ADS1X15.h>

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// SH1106 OLED Screen
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
Adafruit_SH1106 display(21, 22);

#define LED_BUILTIN 2

// Funciones
void serialandwrite();
void wificonnect();
void serialEvent();
void tempload();
void screen(float temp1);
void currentsensor();
void humiditysensor();
void currentload();
void httppos();
void rtctime();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void openFile(fs::FS &fs, const char * path);
uint8_t findDevices();
String leertemperatura();

// OLED Screen Variables
float temp;
float temp1;
float tempmax;
float tempmin;
bool screenon = false;

// WiFi Variables
bool wifisearch = false;
bool wifion = false;
String ssid = "";
String password = "";

// NTP
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 60 * 60;
const int daylightOffset_sec = 0;

// RTC
ESP32Time rtc;
String timestamp = "";
bool rtchour = false;

// SD Name File
String filename = "";
const char *path = "";
String datalog = "";
bool datalogon = false;

// Data Variables
bool stringComplete = false;
String inputString = "";
String httpRequestData;

// DS18B20 Sensor Variables
bool tempsearch = false;
String tempsensors[6];
String day = "";

String tempsensor1 = "";
String tempsensor2 = "";
String tempsensor3 = "";
String tempsensor4 = "";
String tempsensor5 = "";

uint8_t sensor1[8] = {0};
uint8_t sensor2[8] = {0};
uint8_t sensor3[8] = {0};
uint8_t sensor4[8] = {0};
uint8_t sensor5[8] = {0};

uint8_t count = {};

// Current Sensor Variables
int sum = 0;
int current = 0;
bool currenton = false;
bool currentact = false;
int factor1 = 0;
int factor2 = 0;
int factor3 = 0;
String Irms1;
int I1[1000];
String Irms2;
int I2[1000];
String Irms3;
int I3[1000];

// API
String apiKey = "453OP6IXES3VEVOS";
const char *serverName = "http://api.thingspeak.com/update";

// Instancias
Preferences preferences;
OneWire oneWire(32);
DallasTemperature sensors(&oneWire);
ADS1015 ADS1(0x48);
ADS1015 ADS2(0x49);

// Tesla LTDA Logo
static const unsigned char PROGMEM tesla[] = {
    0xff, 0x6c, 0xff, 0x6c, 0x00, 0x6c, 0xff, 0x6c, 0xff, 0x6c, 0x00, 0x6c, 0xd8, 0x6c, 0xd8, 0x6c,
    0xd8, 0x00, 0xdb, 0xfc, 0xdb, 0xfc, 0xd8, 0x00, 0xdb, 0xfc, 0xdb, 0xfc};

// OLED Text
String texto = "";

// OLED Mode
int mode = 0;

// Time Variables
int lastrefresh = 0;
int lastsensor = 0;
int lastcurrent = 0;
int lastdatalog = 0;
int lasttext1 = 0;
int lasttext2 = 10000;
int lasttext3 = 15000;
const int timerDelay = 20000;
int timerrefresh = 17500;
int sensorrefresh = 16500;
int currentrefresh = 6000;
int datalogrefresh = 22000;

// HTTP
bool httpon = false;
String allsensor = "";

void setup()
{
  Serial.begin(115200);
  delay(1);

  SD.begin(5);
  delay(1);

  Wire.begin();
  Wire.setClock(400000);
  delay(1);

  ADS1.begin();
  ADS1.setGain(4);
  ADS1.setDataRate(7);
  delay(1);

  ADS2.begin();
  ADS2.setGain(4);
  ADS2.setDataRate(7);
  delay(1);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1);

  display.clearDisplay();
  display.setTextColor(WHITE);
  delay(1);

  // logo tesla
  display.drawBitmap(0, 50, tesla, 14, 64, 1);
  display.setTextSize(1);
  display.setCursor(16, 57);
  display.print("Tesla");
  display.setTextSize(1);
  display.setCursor(16, 27);
  display.print("Inicializando");
  display.display();
  delay(1);

  sensors.begin();
  delay(1);

  dht.begin();
  delay(1);
}

void loop()
{

  if (stringComplete == true)
  {
    inputString.trim();
    serialandwrite();
    inputString = "";
    stringComplete = false;
  }

  if (wifisearch == false)
  {
    wificonnect();
    wifisearch = true;
  }

  if (tempsearch == false)
  {
    tempload();
    tempsearch = true;
  }

  if (currentact == false)
  {
    currentload();
    currentact = true;
  }

  if ((WiFi.status() == WL_CONNECTED) && (rtchour == false))
  {
    rtctime();
    rtchour = true;
    filename = "/" + rtc.getTime("%d%m%y") + ".csv";
    path = filename.c_str();
    openFile(SD, path);
  }

  if ((millis() - lastcurrent) >= currentrefresh)
  {
    lastcurrent = millis();
    currentsensor();
    Serial.println("1");
    if (currenton == false)
    {
      currentrefresh = 60000;
      currenton = true;
    }
  }

  if ((millis() - lastsensor) >= sensorrefresh)
  {
    lastsensor = millis();
    leertemperatura();
    delay(1);
    humiditysensor();
    temp = tempsensor1.toFloat();
    Serial.println("2");
    if (screenon == false)
    {
      tempmin = temp;
      sensorrefresh = 60000;
      screenon = true;
    }
    delay(1);
    if (day != rtc.getTime("%d") && rtchour == true)
      {
        day = rtc.getTime("%d");
        tempmax = temp;
        tempmin = temp;
      }
  }

  if ((millis() - lastrefresh) >= timerrefresh)
  {
    lastrefresh = millis();
    Serial.println("3");
    if (WiFi.status() == WL_CONNECTED)
    {
      httppos();
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str());
      delay(1);
      if (WiFi.status() == WL_CONNECTED)
      {
        rtctime();
        rtchour = true;
      }
    }
    Serial.println("3.1");
    if (rtchour == true)
    {
      timestamp = rtc.getTime("%H:%M:%S");
    }
    if (httpon == false)
    {
      timerrefresh = 60000;
      httpon = true;
    }
    Serial.println("3.2");
  }

  if ((millis() - lastdatalog) >= datalogrefresh && rtchour == true)
  {
    lastdatalog = millis();
    allsensor = tempsensor1 + "," + tempsensor2 + "," + tempsensor3 + "," + tempsensor4 + "," + tempsensor5 + "," + Irms1 + "," + Irms2 + "," + Irms3 + "," + h;
    datalog = timestamp + "," + allsensor + "\r\n";
    filename = "/" + rtc.getTime("%d%m%y") + ".csv";
    path = filename.c_str();
    openFile(SD, path);
    appendFile(SD, path, datalog.c_str());
    Serial.println("4");
    if (datalogon == false)
    {
      datalogrefresh = 60000;
      datalogon = true;
    }
  }

  if ((millis() - lasttext1) >= timerDelay && mode == 0)
  {
    lasttext1 = millis();
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "ACTUAL";
    temp1 = temp;
    Serial.println("A");
    screen(temp1);
    mode = 1;
  }

  if ((millis() - lasttext1) >= lasttext2 && mode == 1)
  {
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "MAXIMO";
    if (temp > tempmax)
    {
      tempmax = temp;
    }
    temp1 = tempmax;
    Serial.println("B");
    screen(temp1);
    mode = 2;
  }

  if ((millis() - lasttext1) >= lasttext3 && mode == 2)
  {
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "MINIMO";
    if (temp < tempmin && temp != -127)
    {
      tempmin = temp;
    }
    temp1 = tempmin;
    Serial.println("C");
    screen(temp1);
    mode = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void wificonnect()
{
  preferences.begin("credentials", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
  WiFi.begin(ssid.c_str(), password.c_str());
}

void serialandwrite()
{
  if (inputString.startsWith("$W"))
  {
    ssid = inputString.substring(2, 34);
    ssid.trim();
    password = inputString.substring(34);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    preferences.begin("credentials", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    wifisearch = false;
  }
  if (inputString.startsWith("$ST"))
  {
    findDevices();
    Serial.println("");
    Serial.flush();
  }
  if (inputString.startsWith("$T"))
  {
    int StringCount = 0;

    for (int i = 0; i < 6; i++)
    {
      tempsensors[i] = "0";
    }

    while (inputString.length() > 0)
    {
      int index = inputString.indexOf('/');
      if (index == -1) // No space found
      {
        tempsensors[StringCount++] = inputString;
        break;
      }
      else
      {
        tempsensors[StringCount++] = inputString.substring(0, index);
        inputString = inputString.substring(index + 1);
      }
    }

    preferences.begin("sensor", false);
    preferences.putString("tempsensor1", tempsensors[1]);
    preferences.putString("tempsensor2", tempsensors[2]);
    preferences.putString("tempsensor3", tempsensors[3]);
    preferences.putString("tempsensor4", tempsensors[4]);
    preferences.putString("tempsensor5", tempsensors[5]);
    preferences.end();
    tempsearch = false;
  }
  if (inputString.startsWith("$C"))
  {
    current = (inputString.substring(2, 3)).toInt();
    factor1 = (inputString.substring(3, 5)).toInt();
    factor2 = (inputString.substring(5, 7)).toInt();
    factor3 = (inputString.substring(7, 9)).toInt();
    preferences.begin("current", false);
    preferences.putInt("current", current);
    preferences.putInt("factor1", factor1);
    preferences.putInt("factor2", factor2);
    preferences.putInt("factor3", factor3);
    preferences.end();
    currentact = false;
  }
}

void tempload()
{
  preferences.begin("sensor", false);
  tempsensor1 = preferences.getString("tempsensor1", "");
  tempsensor2 = preferences.getString("tempsensor2", "");
  tempsensor3 = preferences.getString("tempsensor3", "");
  tempsensor4 = preferences.getString("tempsensor4", "");
  tempsensor5 = preferences.getString("tempsensor5", "");
  count = preferences.getUInt("count", 0);
  preferences.end();

  int i;
  int addrv[8];

  const char *temp1 = tempsensor1.c_str();
  sscanf(temp1, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor1[i] = (__typeof__(sensor1[0]))addrv[i];
  }

  const char *temp2 = tempsensor2.c_str();
  sscanf(temp2, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor2[i] = (__typeof__(sensor2[0]))addrv[i];
  }

  const char *temp3 = tempsensor3.c_str();
  sscanf(temp3, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor3[i] = (__typeof__(sensor3[0]))addrv[i];
  }

  const char *temp4 = tempsensor4.c_str();
  sscanf(temp4, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor4[i] = (__typeof__(sensor4[0]))addrv[i];
  }

  const char *temp5 = tempsensor5.c_str();
  sscanf(temp5, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor5[i] = (__typeof__(sensor5[0]))addrv[i];
  }

  tempsensor1 = "0";
  tempsensor2 = "0";
  tempsensor3 = "0";
  tempsensor4 = "0";
  tempsensor5 = "0";
}

uint8_t findDevices()
{

  uint8_t address[8];
  count = 0;

  if (oneWire.search(address))
  {
    do
    {
      if (count > 0)
        Serial.print("/");
      count++;
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10)
          Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7)
          Serial.print(", ");
      }
    } while (oneWire.search(address));
  }
  preferences.begin("sensor", false);
  preferences.putUInt("count", count);
  preferences.end();
  return count;
}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }
}

String leertemperatura()
{
  sensors.requestTemperatures();

  if (sensor1[0] != 0 && sensor1[1] != 0)
  {
    tempsensor1 = sensors.getTempC(sensor1);
  }
  else
  {
    tempsensor1 = "null";
  }

  if (sensor2[0] != 0 && sensor2[1] != 0)
  {
    tempsensor2 = sensors.getTempC(sensor2);
  }
  else
  {
    tempsensor2 = "null";
  }

  if (sensor3[0] != 0 && sensor3[1] != 0)
  {
    tempsensor3 = sensors.getTempC(sensor3);
  }
  else
  {
    tempsensor3 = "null";
  }

  if (sensor4[0] != 0 && sensor4[1] != 0)
  {
    tempsensor4 = sensors.getTempC(sensor4);
  }
  else
  {
    tempsensor4 = "null";
  }

  if (sensor5[0] != 0 && sensor5[1] != 0)
  {
    tempsensor5 = sensors.getTempC(sensor5);
  }
  else
  {
    tempsensor5 = "null";
  }
  return ("0");
}

void screen(float temp1)
{
  String L;
  display.setTextSize(2);
  display.setCursor(29, 4);
  display.print(texto);
  display.setTextSize(3);
  L = String(temp1);
  switch (L.length()){
    case 4:
      display.setCursor(36, 24);
      break;
    case 5:
      display.setCursor(18, 24);
      break;
    case 6:
      display.setCursor(0, 24);
      break;
  }
  display.print(temp1);
  display.drawCircle(111, 34, 2, 1);
  display.setCursor(116, 32);
  display.setTextSize(2);
  display.print("C");
  display.display();
}

void currentload()
{
  preferences.begin("current", false);
  current = preferences.getInt("current", {});
  factor1 = preferences.getInt("factor1", {});
  factor2 = preferences.getInt("factor2", {});
  factor3 = preferences.getInt("factor3", {});
  preferences.end();
}

void currentsensor()
{
  if (current >= 1)
  {
    for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
    {
      I1[j] = ADS1.readADC_Differential_0_1(); // set element at location i
    }
    if (current >= 2)
    {
      for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
      {
        I2[j] = ADS1.readADC_Differential_2_3(); // set element at location i
      }
      if (current >= 3)
      {
        for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
        {
          I3[j] = ADS2.readADC_Differential_0_1(); // set element at location i
        }
      }
      else
      {
        Irms3 = "null";
      }
    }
    else
    {
      Irms2 = "null";
      Irms3 = "null";
    }
  }
  else
  {
    Irms1 = "null";
    Irms2 = "null";
    Irms3 = "null";
  }

  if (current >= 1)
  {
    for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
    {
      I1[j] *= I1[j]; // set element at location i
      sum += I1[j];
    }
    Irms1 = String(0.0005 * factor1 * sqrt(sum / 1000));
    sum = 0;
    if (current >= 2)
    {
      for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
      {
        I2[j] *= I2[j]; // set element at location i
        sum += I2[j];
      }
      Irms2 = String(0.0005 * factor2 * sqrt(sum / 1000));
      sum = 0;
      if (current >= 3)
      {
        for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
        {
          I3[j] *= I3[j]; // set element at location i
          sum += I3[j];
        }
        Irms3 = String(0.0005 * factor3 * sqrt(sum / 1000));
        sum = 0;
      }
    }
  }
}

void humiditysensor()
{
  if (humidity == 1)
  {
    h = String(dht.readHumidity());
  }
  else
  {
    h = "null";
  }
}

void httppos()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverName);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apiKey + "&field1=" + tempsensor1 + "&field2=" + tempsensor2 + "&field3=" + tempsensor3 + "&field4=" + tempsensor4;
    httpRequestData += "&field5=" + tempsensor5 + "&field6=" + Irms1 + "&field7=" + Irms2 + "&field8=" + Irms3;
    int httpResponseCode = http.POST(httpRequestData);
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void rtctime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    rtc.setTimeStruct(timeinfo);
    rtchour = true;
  }
  else
  {
    return;
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void openFile(fs::FS &fs, const char * path){
  Serial.printf("Open file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    writeFile(SD, path, "Hour, Temp1, Temp2, Temp3, Temp4, Temp5, Current1, Current2, Current3, Humidity \r\n");
    delay(1);
    return;
  }
  file.close();
}