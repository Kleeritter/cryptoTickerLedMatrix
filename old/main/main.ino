/* Basiert auf 
 * ESP8266 Crypto 
 * Conor Walsh 2021-02-27
 * conor.engineer
 * und der guten PxMatrix
 */
 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <PxMatrix.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <CertStoreBearSSL.h>
#include <time.h>
#include <FS.h>
#include <LittleFS.h>

/**** Setup ****/

// Uncomment the next line if you are using an ESP-01
// #define ESP01

// WIFI Network name and password
#define WIFI_SSID "SSID"
#define WIFI_PASS "Password"

// Setup time in seconds
#define SETUP_TIME 4

// Update screen every __ seconds also wie lange der Bums angezeigt wird
#define UPDATE_TIME 10

// Currency for crypto, tested: eur usd gbp
#define CURRENCY_CODE "eur"
// Currency symbol for crypto, tested: letters and $
#define CURRENCY_SYM 'E'


//NTP

void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

// API Bums
// Fingerprint for api.cryptonator.com - expires 6 Feb 2022
const uint8_t fingerprint_crypto[20] = {0x10, 0x76, 0x19, 0x6B, 0xE9, 0xE5, 0x87, 0x5A, 0x26, 0x12, 0x15, 0xDE, 0x9F, 0x7D, 0x3B, 0x92, 0x9A, 0x7F, 0x30, 0x13};
// Fingerprint for finnhub.io - expires 27 Aug 2021
const uint8_t fingerprint_stock[20] = {0x2B, 0x34, 0xAD, 0x3F, 0x51, 0x8D, 0xA8, 0x5A, 0xD5, 0x53, 0xA3, 0x5D, 0x48, 0x13, 0x85, 0xC4, 0xA1, 0xA2, 0x60, 0x43};

// API Key for finnhub.io, get your free API key at finnhub.io
const String stocksApiKey = "XXXXXXXXXXXXXXXXXXXX";

/** End Setup **/

ESP8266WiFiMulti WiFiMulti;

// Screen Definitions, wichtig für Darstellung auf Display
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C


// Einstellungen für die Matrix

// Pins
#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 16
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif

#define matrix_width 64
#define matrix_height 32

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=30; //30-70 is usually fine

//PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E); // Wichtig für den Typ von Matrix
//PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myORANGE = display.color565(255, 128, 0);
uint16_t myGREENE = display.color565(128, 255, 0);
uint16_t myGREENER = display.color565(0, 128, 255);
uint16_t myLIGHTBLUE = display.color565(255, 128, 0);
uint16_t myPURPLE = display.color565(127, 0, 255);
uint16_t myROSE = display.color565(255, 0, 127);
uint16_t myCOLORS[13]={myRED,myGREEN,myBLUE,myWHITE,myYELLOW,myCYAN,myMAGENTA,myORANGE,myGREENE,myGREENER,myLIGHTBLUE,myPURPLE,myROSE};


// This draws the icons

void drawImage(int x, int y, uint16_t num)
{
  int imageHeight = 12;
  int imageWidth = 9;
  int counter = 0;
  uint16_t ccl = num;
  uint16_t  btcbild[] = {
  0x0000, 0x0000, ccl, 0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000, ccl, 0x0000, 0x0000,   // 0x0010 (16) pixels
  0x0000, 0x0000, ccl, ccl, ccl, ccl, ccl, ccl, ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000, 0x0000,   // 0x0020 (32) pixels
  0x0000, 0x0000, ccl, 0x0000, 0x0000, 0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000, 0x0000, 0x0000, ccl,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x0000, 0x0000, ccl, ccl, 0x0000, 0x0000, ccl, ccl, ccl, ccl, ccl, ccl, ccl, 0x0000,   // 0x0040 (64) pixels
  0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl, ccl, 0x0000, 0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl,   // 0x0050 (80) pixels
  0x0000, 0x0000, 0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000, ccl, ccl, ccl, ccl, ccl, ccl,   // 0x0060 (96) pixels
  ccl, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000, ccl, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, ccl, 0x0000,   // 0x0070 (112) pixels
  };  
  //uint16_t picturecolor = myColors[num]
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    {
      display.drawPixel(xx + x , yy + y, btcbild[counter]);
      counter++;
    }
  }
}
//DOGE Bild
void drawImager(int x, int y,uint16_t number)
{
  int imageHeight = 12;
  int imageWidth = 9;
  int counter = 0;
  uint16_t ccd = number;
  uint16_t  doger[] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0010 (16) pixels
  0x0000, 0x0000, 0x0000, ccd, ccd, ccd, ccd, ccd, 0x0000, 0x0000, 0x0000, 0x0000, ccd, 0x0000, 0x0000, 0x0000,   // 0x0020 (32) pixels
  ccd, ccd, 0x0000, 0x0000, 0x0000, ccd, 0x0000, 0x0000, 0x0000, 0x0000, ccd, 0x0000, 0x0000, 0x0000, ccd, 0x0000,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x0000, 0x0000, ccd, 0x0000, ccd, ccd, ccd, ccd, 0x0000, 0x0000, 0x0000, ccd, 0x0000, 0x0000,   // 0x0040 (64) pixels
  ccd, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, ccd, 0x0000, 0x0000, ccd, 0x0000, 0x0000, 0x0000, 0x0000, ccd, 0x0000,   // 0x0050 (80) pixels
  0x0000, 0x0000, ccd, 0x0000, 0x0000, 0x0000, ccd, ccd, 0x0000, 0x0000, 0x0000, ccd, ccd, ccd, ccd, ccd,   // 0x0060 (96) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0070 (112) pixels
  };

  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    {
      display.drawPixel(xx + x , yy + y, doger[counter]);
      counter++;
    }
  }
}
//Ether Bild
void drawImagerino(int x, int y,uint16_t number)
{
  int imageHeight = 12;
  int imageWidth = 9;
  int counter = 0;
  uint16_t cce = number;
  uint16_t  ether[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0010 (16) pixels
0x0000, 0x0000, cce, cce, cce, cce, cce, cce, cce, cce, cce, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0000 (32) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0030 (48) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, cce, cce, cce, cce, cce, cce, cce, 0x0000, 0x0000,   // 0x0040 (64) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, cce, cce, cce, cce, cce, cce,   // 0x0060 (96) pixels
cce, cce, cce, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0070 (112) pixels
};
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    {
      display.drawPixel(xx + x , yy + y, ether[counter]);
      counter++;
    }
  }
}
#ifdef ESP8266
// ISR for display refresh
void display_updater()
{
  display.display(display_draw_time);
}
#endif
String pricer;
#ifdef ESP32
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif


void display_update_enable(bool is_enable)
{
 
#ifdef ESP8266
  if (is_enable)
    display_ticker.attach(0.004, display_updater);
  else
    display_ticker.detach();
#endif

#ifdef ESP32
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
#endif
}



// Struct to store an asset
struct asset { 
   bool isCrypto;
   String assetName;
   String url;
   const unsigned char logo [288];
};

// Declare all assests used
// To add a new asset
//  1: Add a new setup similar to one shown in the examples
//  2: Update the name and isCrypto flag for the asset
//  3: Find the api.cryptonator.com or finnhub.io api url for the asset
//  4: Create a 48x48 bitmap logo for the asset using http://javl.github.io/image2cpp/
asset assets[] = {
    // Doge setup
    {true, "Doge",
     "https://api.cryptonator.com/api/ticker/doge-"+String(CURRENCY_CODE),
      {0x03}
    },
    // BTC setup
    {true, "BTC",
     "https://api.cryptonator.com/api/ticker/btc-"+String(CURRENCY_CODE),
      {0x00}
    },
    //ETH
    {true, "ETH",
     "https://api.cryptonator.com/api/ticker/eth-"+String(CURRENCY_CODE),
      0x03}
    };


// Variable to store what asset should be displayed
int currentAsset = 0;
// Const to store how many assets are configured
const int maxAssets = sizeof(assets)/sizeof(assets[0]);
int icon=0;
int hanno;
int rando;
int kd;
int tor;
float price;
float change;
float pricebtc;
float changebtc;
float priceeth;
float changeeth;
float changePercent;
float changePercentbtc;
float changePercenteth;
void setup() {
  // put your setup code here, to run once:
WiFiManager wifiManager;
//first parameter is name of access point, second is the password
  Serial.begin(115200);
  Serial.print("Wifi Connect");
wifiManager.autoConnect("cryptoLED", "crpassword");
    setClock();
    SPIFFS.begin();
    HTTPClient http;

   // Initialize display
  display.begin(16);
  //display.setMuxDelay(0,1,0,0,0);
  display.setTextColor(myCYAN);
  display_update_enable(true);
  //display.setFastUpdate(true);
  display.setBrightness(50);
  #ifdef ESP01
    // I2C for ESP-01 is different
    Wire.begin(2, 0);
  #endif
  // Try to start display if that doesn't work halt program and inform user
//  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
//    Serial.println(F("HALT: OLED allocation failed"));
//    while(1);
  
  // Set text size to 1 
 display.setTextSize(1);

  // Allow esp wifi time to setup before attempting connection
  for (uint8_t t = SETUP_TIME; t > 0; t--) {
    // Print info to serial
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    // and print info to OLED
    display.clearDisplay();
    display.setCursor(0, display.height()/4);
    display.print(F("Boot V5.3  "));
    display.print(t);
    display.display();
    delay(1000);
  }
   

  // Add Wifi network to ESP
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
tor=0;
}

void loop() {
  // Wait for WiFi connection
    currentAsset=0;
    if ((WiFiMulti.run() == WL_CONNECTED)) {
    // Set the correct fingerprint
     uint8_t fingerprint[20];
      if(assets[currentAsset].isCrypto)
      // In C a memcopy needs to be used as an array can't be directly assigned to another array
      memcpy(fingerprint, fingerprint_crypto, 20);
       else
      memcpy(fingerprint, fingerprint_stock, 20);

    // Setup a https client
    HTTPClient http;
    BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure();
    BearSSL::CertStore certStore;
    int numCerts = certStore.initCertStore(SPIFFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    client->setCertStore(&certStore);
    Serial.println(numCerts);
    
      // DOGE
    // Connect to API
    Serial.print("HTTPS begin... bit\n");
    if (http.begin(dynamic_cast<WiFiClient&>(*client),"https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd")) {  // HTTPS

      // start connection and send HTTP header
      Serial.print("HTTPS GET...\n");
      display.clearDisplay();
      int httpCode = http.GET();
      //Serial.print("Balla...\n");
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("HTTPS GET... code: %d\n", httpCode);

        // If the HTTP code is valid process and display the data
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // Store the API payload in a string
          String payload = http.getString();
          Serial.print(payload);
         // Calculate the price, change and percentage change
          
          if(assets[currentAsset].isCrypto){
            price = payload.substring(payload.indexOf("price")+8,payload.indexOf("volume")-3).toFloat();
            change = payload.substring(payload.indexOf("change")+9,payload.indexOf("timestamp")-4).toFloat();
          }
          else{
            price = payload.substring(payload.indexOf("c")+3,payload.indexOf("h")-2).toFloat();
            change = price - payload.substring(payload.indexOf("pc")+4,payload.indexOf("timestamp")-2).toFloat();
           
          }
          Serial.print("Alles Goose 1");
               float changePercent = change/price*100;   
                 Serial.println(changePercent);
        }

      } else {
        // Warn user using serial and OLED that call failed
        Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, display.height()/4);
        display.println(F("HTTPS GET failed"));
        display.print(F("Will retry in a few seconds"));
        display.setTextSize(1);
      }

      // Must close the https connection
      http.end();
    }
  
  
 
       currentAsset=1;
 
   
         //BTC
          // Connect to API
    Serial.print("HTTPS begin...\n");
    if (http.begin(*client, assets[currentAsset].url)) {  // HTTPS

      // start connection and send HTTP header
      Serial.print("HTTPS GET...\n");
      display.clearDisplay();
      int httpCode = http.GET();
      //Serial.print("Balla...\n");
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //Serial.printf("HTTPS GET... code: %d\n", httpCode);

        // If the HTTP code is valid process and display the data
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // Store the API payload in a string
          String payloadbtc = http.getString();
         // Calculate the price, change and percentage change
          
          if(assets[currentAsset].isCrypto){
            pricebtc = payloadbtc.substring(payloadbtc.indexOf("price")+8,payloadbtc.indexOf("volume")-3).toFloat();
            changebtc = payloadbtc.substring(payloadbtc.indexOf("change")+9,payloadbtc.indexOf("timestamp")-4).toFloat();
          }
          else{
            pricebtc = payloadbtc.substring(payloadbtc.indexOf("c")+3,payloadbtc.indexOf("h")-2).toFloat();
           changebtc = pricebtc - payloadbtc.substring(payloadbtc.indexOf("pc")+4,payloadbtc.indexOf("timestamp")-2).toFloat();
           
          }
          Serial.print("Alles Goose 2");
          float changePercentbtc = changebtc/pricebtc*100;
            Serial.println(changePercentbtc);
          currentAsset=2;
   }

      } else {
        // Warn user using serial and OLED that call failed
        Serial.printf("[HTTPS] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        display.clearDisplay();
        display.setCursor(0, display.height()/4);
        display.println(F("HTTPS GET failed"));
        display.print(F("Will retry in a few seconds"));
       
      }

      // Must close the https connection
      http.end();
    }
  
  
  

    currentAsset=2;
   // ETH
  
   
              // Connect to API
    Serial.print("HTTPS begin...\n");
    if (http.begin(*client, assets[currentAsset].url)) {  // HTTPS

      // start connection and send HTTP header
      Serial.print("HTTPS GET...\n");
      int httpCode = http.GET();
      //Serial.print("Balla...\n");
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //Serial.printf("HTTPS GET... code: %d\n", httpCode);

        // If the HTTP code is valid process and display the data
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // Store the API payload in a string
          String payloadeth = http.getString();
         // Calculate the price, change and percentage change
          
          if(assets[currentAsset].isCrypto){
            priceeth = payloadeth.substring(payloadeth.indexOf("price")+8,payloadeth.indexOf("volume")-3).toFloat();
            changeeth = payloadeth.substring(payloadeth.indexOf("change")+9,payloadeth.indexOf("timestamp")-4).toFloat();
          }
          else{
            priceeth = payloadeth.substring(payloadeth.indexOf("c")+3,payloadeth.indexOf("h")-2).toFloat();
            changeeth = priceeth - payloadeth.substring(payloadeth.indexOf("pc")+4,payloadeth.indexOf("timestamp")-2).toFloat();
           
          }
          Serial.print("Alles Goose 3");
          float changePercenteth = changeeth/priceeth*100;
          Serial.println(changePercenteth);
   }
      } 

      // Must close the https connection
      http.end();
    }
  
  Serial.println(changePercenteth);
          int tim=0;
          while(tim<60){
            //DOGE
            Serial.println(changePercenteth);
            rando = rand() % 13;
            display.clearDisplay();
            display.setTextColor(myCOLORS[rando]);
            drawImager(0,0,myCOLORS[rando]);
            hanno= 7;
            kd=-10;
            printPrice(price);
            printChange(change/price*100);
            delay(UPDATE_TIME*1000);
            //BTC
            Serial.println(changePercenteth);
            rando = rand() % 13;
            display.clearDisplay();
            display.setTextColor(myCOLORS[rando]);
            drawImage(0,0,myCOLORS[rando]);
            kd=1;
            hanno= 9;
            printPrice(pricebtc);
            printChange(changebtc/pricebtc*100);
            delay(UPDATE_TIME*1000);
            //ETH
             rando = rand() % 13;
            display.clearDisplay();
            display.setTextColor(myCOLORS[rando]);
            drawImagerino(0,0,myCOLORS[rando]);
            kd=0;
            hanno= 8;
            printPrice(priceeth);
            printChange(changeeth/priceeth*100);
            delay(UPDATE_TIME*1000);
            tim= tim+3*UPDATE_TIME;
            
        }
        tor=0;
        tim=0;
      }
     
}
   
// Print the price on the Matrix alla
void printPrice(float price){
    char priceBuf[20];
    // Get correct currency symbol
    char currencySymbol = ' ';
      if(kd<0){
        price=price;
        currentAsset=0;
    }
      else if(kd>0){
        price=pricebtc;
        currentAsset=1;
    }
      else {
        price=priceeth;
        currentAsset=2;
    }
    if(price<10){
      sprintf(priceBuf, "%c%.5f",currencySymbol,  price); //currencySymbol,
    }
    else {
      sprintf(priceBuf, "%c%.3f",currencySymbol,  price); //currencySymbol
    }
    display.setCursor(display.width()/2 -10,2);
    display.print(assets[currentAsset].assetName);
    display.setCursor(-9 +(64 -(5* hanno))/2,display.height()/4+6);
    // Print the string to the OLED
    display.print(priceBuf);
   
   
}

// Print the price change percentage to the Matrix alla
void printChange(float change) {
  display.setCursor(display.width()/2 -16, display.height()/4 +16);
  // Manually print a + for positive changes
  if(change>=0){
    display.print(F("+"));
  }
  // If the change is less than 10 use a decimal point if not dont
  if(abs(change)<10){
    display.print(change,1);
  }
  else{
    display.print(change,0);
  }
  display.print(F("%"));
 // display.display();
}
      
