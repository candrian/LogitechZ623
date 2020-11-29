#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Arduino.h>
#include <EEPROM.h>

#define CW  LOW
#define CCW HIGH

#define ON  LOW
#define OFF HIGH

#define STEP          D1
#define STEPEN        D2
#define STEPDIRECTION D3
#define IRRECEIVER    D5
#define LED           D4

#define M0            D6
#define M1            D7
#define M2            D8

#define FULL_STEP     0
#define HALF_STEP     1
#define STEP4        2
#define STEP8        3
#define STEP16       4
#define STEP32       5

#define STEPMOTOR_STAY_LOCKED_TIME  500       // milliseconds
#define CHANGE_DIRECTION_CORRECTION_STEPS 5   // Mposika

#define UP            2     
#define DOWN          1
#define WAITING       0 
#define FAST          0
#define NORMAL        1

//1650 steps 1 cycle (55*30)
#define STEPS_PER_CLICK     30
#define STEP_DURATION       1000
#define FAST_STEP_DURATION  1000

char volumeUpOnce=0;
char volumeDownOnce=0;
char previousMove=CW;

int i;

int currentTime=millis();
// Replace with your network credentials
const char* ssid = "wifiSSID";
const char* password = "wifiPassword";

char   volumeState=WAITING;
IRrecv irrecv(IRRECEIVER);

decode_results results;

// Create AsyncWebServer object on port 80
ESP8266WebServer  server(80);

// Set your Static IP address
IPAddress local_IP(192, 168, x, x);
// Set your Gateway IP address
IPAddress gateway(192, 168, x, x);

IPAddress subnet(255, 255, x, x);

String webPage =
    "<html>\r\n" \
    "<head>\r\n" \
    "<title>Volume Control</title>\r\n" \
    "</head>\r\n" \
    "<style>\r\n" \
    "input[type=submit]{\r\n" \
    "background-color: #4CAF50;\r\n" \
    "color: white;\r\n" \
    "padding: 200px 150px;\r\n" \
    "margin: 4px 2px;\r\n" \
    "cursor: pointer;\r\n" \
    "text-decoration: none;\r\n" \
    "border: none;\r\n" \
    "font-size: 200;\r\n" \
    "}\r\n" \
    "</style>\r\n" \
    "<body>\r\n" \
    "<center>\r\n" \
    "<h1>Volume</h1>\r\n" \
    "<form method=\"post\">\r\n" \
    "<input type=\"submit\" name=\"volume\" value=\"+\">\r\n" \
    "<input type=\"submit\" name=\"volume\" value=\"-\">\r\n" \
    "</form>\r\n" \
    "</center>\r\n" \
    "</body>\r\n" \
    "</html>\r\n";
 
void handleRoot(){
    server.send(200, "text/html", webPage);
}

void handleVolume(){
    // *** Turn on or off the LED ***
    if (server.arg("volume") == "+")
        //increaseVolume();
        volumeState=UP;
    else if (server.arg("volume") == "-")
        //decreaseVolume();
        volumeState=DOWN;
 
    server.sendHeader("Location","/");
    server.send(303);
}

void setup(){

  pinMode(STEPDIRECTION, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(STEPEN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);

  digitalWrite(LED, OFF);
  digitalWrite(STEPDIRECTION, CW);
  digitalWrite(STEPEN,HIGH);  

  // Serial port for debugging purposes
  Serial.begin(115200);
  
  irrecv.enableIRIn();  // Start the receiver

  stepMode(FULL_STEP);
  currentTime=millis();
  
 // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  digitalWrite(LED, ON);
  
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("LogitechVolume");
  ArduinoOTA.setPasswordHash("xx");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
    // *** Start web server ***
    server.on("/", HTTP_GET, handleRoot);
    server.on("/", HTTP_POST, handleVolume);
    server.begin();
    Serial.println("Web server started at port 80\n");

}
  
void loop() {
   if (irrecv.decode(&results)) {

    if(results.value==0x768900FF) {
      // IR control sends the command twice. In order to move the step motor only once i check it here. 
      if (volumeDownOnce==0){
        volumeDownOnce=1;
      }else{
        volumeState=DOWN;
        volumeDownOnce=0;
      }
      //decreaseVolume();
    }
    if(results.value==0x7689807F) {
      // IR control sends the command twice. In order to move the step motor only once i check it here. 
      if (volumeUpOnce==0){
        volumeUpOnce=1;
      }else{
        volumeState=UP;
        volumeUpOnce=0;
      }
      //increaseVolume();
    }
    irrecv.resume();  // Receive the next value
  }
  if (volumeState!=WAITING){
    if (volumeState==UP)increaseVolume();
    if (volumeState==DOWN)decreaseVolume();
  }

  if (millis()-currentTime>STEPMOTOR_STAY_LOCKED_TIME){
    digitalWrite(STEPEN,HIGH);  
  }
  server.handleClient();
  ArduinoOTA.handle();
}

void increaseVolume(){
  currentTime=millis();
  digitalWrite(STEPEN,LOW);
  delay(1);
  volumeState=WAITING;
  Serial.println("Volume increased");
  digitalWrite(STEPDIRECTION,CCW);
  if (previousMove==CW){
      for(i=0;i<STEPS_PER_CLICK*CHANGE_DIRECTION_CORRECTION_STEPS;i++){
        makeAstep(FAST);
      }
  }else{
    for(i=0;i<STEPS_PER_CLICK;i++){
        makeAstep(NORMAL);
    }
  }
  previousMove=CCW;
}
void decreaseVolume(){
  currentTime=millis();
  digitalWrite(STEPEN,LOW);
  delay(1);
  volumeState=WAITING;
  Serial.println("Volume decreased");
  //digitalWrite(STEPEN,LOW);
  //delay(1);
  digitalWrite(STEPDIRECTION,CW);
  if (previousMove==CCW){
    for(i=0;i<STEPS_PER_CLICK*CHANGE_DIRECTION_CORRECTION_STEPS;i++){
      makeAstep(FAST);
    }
  }else{
    for(i=0;i<STEPS_PER_CLICK;i++){
        makeAstep(NORMAL);
    }
  }
  previousMove=CW;
}

void makeAstep(char turningSpeed){
  if (turningSpeed==FAST){
    digitalWrite(STEP,HIGH);
    delayMicroseconds(FAST_STEP_DURATION);
    digitalWrite(STEP,LOW);
    delayMicroseconds(FAST_STEP_DURATION);
  }else{
    digitalWrite(STEP,HIGH);
    delayMicroseconds(STEP_DURATION);
    digitalWrite(STEP,LOW);
    delayMicroseconds(STEP_DURATION);
  }
}

void stepMode(char modeType){
  switch (modeType){
  case FULL_STEP:
    digitalWrite(M0, LOW);
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
    break;
  case HALF_STEP:
    digitalWrite(M0, HIGH);
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
    break;
  case STEP4:
    digitalWrite(M0, LOW);
    digitalWrite(M1, HIGH);
    digitalWrite(M2, LOW);
    break;
  case STEP8:
    digitalWrite(M0, HIGH);
    digitalWrite(M1, HIGH);
    digitalWrite(M2, LOW);
    break;
  case STEP16:
    digitalWrite(M0, LOW);
    digitalWrite(M1, LOW);
    digitalWrite(M2, HIGH);
    break;
  case STEP32:
    digitalWrite(M0, HIGH);
    digitalWrite(M1, LOW);
    digitalWrite(M2, HIGH);
    break;
  default:
    digitalWrite(M0, LOW);
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
  break;
  }
}
