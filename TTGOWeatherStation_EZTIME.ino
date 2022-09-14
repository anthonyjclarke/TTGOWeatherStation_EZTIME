#include "ani.h"
#include <SPI.h>
#include <TFT_eSPI.h>             // Hardware-specific library
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson.git
#include <WiFi.h>
#include <ezTime.h>               //https://github.com/ropg/ezTime
#include "Orbitron_Medium_20.h"
#include <WiFiUdp.h>
#include <HTTPClient.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB
#define lightblue 0x01E9
#define darkred 0xA041
#define blue 0x5D9B

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

//EDIT this section here with Wifi, Location, Openweathermap KEY (https://openweathermap.org/)

const char* ssid     = "xxxxxxxxxx";        // EDIT
const char* password = "xxxxxxxxxx";        // EDIT
String town="Sydney";                       // EDIT
String Country="AU";                        // EDIT
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
const String key = "xxxxxxxxxxxxxxxxxxxxx"; // EDIT

Timezone myTZ; 
  
String payload="";            //whole json 
String tmp="" ;               //temperature
String hum="" ;               //humidity
  
StaticJsonDocument<1000> doc;

// Variables to save date and time
String secStamp;
String dayStamp;
String timeStamp;

int backlight[5] = {10,30,60,120,220};
byte b=4;

void setup(void) {
  pinMode(0,INPUT_PULLUP);
  pinMode(35,INPUT);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);

  Serial.begin(115200);
  tft.print("Connecting to ");
  tft.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    tft.print(".");
  }

  waitForSync();

  // Provide official timezone names
  // https://en.wikipedia.org/wiki/List_of_tz_database_time_zones

  myTZ.setLocation(F("Australia/Sydney")); // EDIT for your location
  
  Serial.println();
  Serial.println("UTC:" + UTC.dateTime());
  Serial.print(F("Sydney:"));
  Serial.println(myTZ.dateTime());
  
  tft.println("");
  tft.println("WiFi connected.");
  tft.println("IP address: ");
  tft.println(WiFi.localIP());
  delay(1000);
  
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);   
  tft.setCursor(2, 232, 1);
  tft.println(WiFi.localIP());
  tft.setCursor(80, 204, 1);
  tft.println("BRIGHT:");
  tft.setCursor(80, 152, 2);
  tft.println("SEC:");
  tft.setTextColor(TFT_WHITE,lightblue);
  tft.setCursor(4, 152, 2);
  tft.println("TEMP:");

  tft.setCursor(4, 192, 2);
  tft.println("HUM: ");
  tft.setTextColor(TFT_WHITE,TFT_BLACK);

  tft.setFreeFont(&Orbitron_Medium_20);
  tft.setCursor(6, 82);
  tft.println(town);

  tft.fillRect(68,152,1,74,TFT_GREY);

  for(int i=0;i<b+1;i++)
    tft.fillRect(78+(i*7),216,3,10,blue);

  getData();
  
  delay(500);
  
}

int i=0;
String tt="";
int count=0;
bool inv=1;
int press1=0; 
int press2=0;
int frame=0;
String curSeconds="";
String curTime="";

void loop() {

  events();
  
  tft.pushImage(0, 88,  135, 65, ani[frame]);
   frame++;
   if(frame>=10)
   frame=0;
 
   if(digitalRead(35)==0){
   if(press2==0)
   {press2=1;
   tft.fillRect(78,216,44,12,TFT_BLACK);
 
   b++;
   if(b>=5)
   b=0;

   for(int i=0;i<b+1;i++)
   tft.fillRect(78+(i*7),216,3,10,blue);
   ledcWrite(pwmLedChannelTFT, backlight[b]);}
   }else press2=0;

   if(digitalRead(0)==0){
   if(press1==0)
   {press1=1;
   inv=!inv;
   tft.invertDisplay(inv);}
   }else press1=0;
   
   if(count==0)
   getData();
   count++;
   if(count>2000)
   count=0;
   
    tft.setFreeFont(&Orbitron_Medium_20);
    tft.setCursor(2, 187);
         tft.println(tmp);
         tft.setCursor(2, 227);
         tft.println(hum+"%");

           tft.setTextColor(TFT_ORANGE,TFT_BLACK);
           tft.setTextFont(2);
           tft.setCursor(6, 44);
           tft.println(dayStamp);
           tft.setTextColor(TFT_WHITE,TFT_BLACK);

dayStamp = myTZ.dateTime("D d-M-Y");  // Tue-Apr-2022
timeStamp = myTZ.dateTime("H:i");     // 12:01 24hour
secStamp = myTZ.dateTime("s");        // Seconds "s", with leading zero

// If no change then skip, wanted to also use ezTime secondschanged()? but not working

 if (curSeconds!=secStamp) {
    tft.fillRect(78,170,48,28,darkred);
    tft.setFreeFont(&Orbitron_Light_24);
    tft.setCursor(81, 192);
    tft.println(secStamp);       
    curSeconds=secStamp;
    Serial.println("Day - "+dayStamp+" Time - "+timeStamp+" Sec:"+secStamp); 
 }
 
// Any changes in Minutes? wanted to use the ezTime function minuteschanged() but doesnt seem to work
// Details here - https://github.com/ropg/ezTime/issues/91
      
  if (curTime != timeStamp) {  
    // now print Time and only do once
    
    tft.setFreeFont(&Orbitron_Light_32);
    tft.fillRect(3,8,120,30,TFT_BLACK);
    tft.setCursor(5, 34);
    tft.println(timeStamp);
    curTime = timeStamp;
    }
  delay(80); 
}

void getData()
{
    tft.fillRect(1,170,64,20,TFT_BLACK);
    tft.fillRect(1,210,64,20,TFT_BLACK);
   if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
         payload = http.getString();
       // Serial.println(httpCode);
        Serial.println(payload);
        
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
  
  char inp[1000];
  payload.toCharArray(inp,1000);
  deserializeJson(doc,inp);
  
  String tmp2 = doc["main"]["temp"];
  String hum2 = doc["main"]["humidity"];
  String town2 = doc["name"];
  tmp=tmp2;
  hum=hum2;
  
  Serial.println("Temperature"+String(tmp));
  Serial.println("Humidity"+hum);
  Serial.println(town);
   
 }
         

     
