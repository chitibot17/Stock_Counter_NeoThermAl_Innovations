#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#include <EEPROM.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>

#include <SPI.h>
#include "HX711.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define LOADCELL_DOUT_PIN 5 //D1
#define LOADCELL_SCK_PIN 4 //D2

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;

HX711 scale; //Declare object scale to call various operations like get values and tare
WiFiManager wifiManager;
ESP8266WiFiMulti WiFiMulti;

float calibration_factor = 227850; //Calibration factor obtained by previous testing
int contrast=40;

int menuitem = 1; //For main menu items
int submenuitem = 2; //For submenu items of ADD ITEMS
int frame = 1; //For scrolling through the 6 menu options
int page = 1; //For menu and its contents inside
int lastMenuItem = 1;
int positem = 1; //For right and left scrolling in count
int uidcur = 1; //For right and left scrolling in uid
int odbitem = 1;
int othermenuitem =1;
int wifimenuitem=1;
int cnfrmext=1; 

String menuItem1 = "WEIGHT";
String menuItem2 = "ADD PART";
String menuItem3 = "VIEW STOCK";
String menuItem4 = "ONLINE DB";
String menuItem5 = "WIFI";
String menuItem6 = "OTHER";


int u1 = 0;//UID digit 1
int u2 = 0;//UID digit 2
int u3 = 0;//UID digit 3
int uid = 0;//Final 3 digit uid is stored in this variable
int c1 = 0;//Count digit 1
int c2 = 0;//Count digit 2
int c3 = 0;//Count digit 3
int c4 = 0;//Count digit 4
int c5 = 0;//Count digit 5
int ct = 0;//Final 5 digit count is stored in this variable

float wt = 0;
float reading;
float w = reading;
float fct;//String float from eeprom

volatile boolean up = false;
volatile boolean down = false;
volatile boolean middle = false;
volatile boolean left = false;
volatile boolean right = false;

int downButtonState = 0;
int upButtonState = 0;  
int selectButtonState = 0; 
int leftButtonState = 0;   
int rightButtonState = 0;      

int lastDownButtonState = 0;
int lastSelectButtonState = 0;
int lastUpButtonState = 0;
int lastLeftButtonState = 0;
int lastRightButtonState = 0;
int analogState = 0;

//String Uid = "BP";

int prevuid;

String uidString = String(uid);
String wtString = String(wt);
String ctString = String(ct);
String uidString2 = String(uid);

int uidaddr;
int pwtaddr = uidaddr + sizeof(uidString);
int pctaddr = pwtaddr + sizeof(wtString);

String uidaddrS = String(uidaddr);
String pctaddrS = String(pctaddr);
String pwtaddrS = String(pwtaddr);

int eestate=0; ///Need to remove this 
int i =0;
int k =0;
int m =0;
int flag;
String gtud;
String gtwt;
String gtct;
int stockscroll=1;
int wifistate=1;
int onwifiitem=1;
String gtuidaddrS;
String gtpwtaddrS;
String gtpctaddrS;
String frstentryS;
String clreeprm;
int test=0;

const char* ssid = "ESP Cam";
const char* serverNamecam = "http://192.168.4.1/cam";
String uidcam;

float h ;
float t;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000; 

#define AWS_IOT_PUBLISH_TOPIC   "Stockmanager/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "Stockmanager/sub"

WiFiClientSecure net;
 
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient client(net);
 
time_t now;
time_t nowish = 1510592825;


/* Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);*/
Adafruit_PCD8544 display = Adafruit_PCD8544(14, 13, 16, 15, 1); /*D5, D7, D0, D8, Txpin*/

void setup() {

  EEPROM.begin(4000);
  /*Display Initialization*/
  display.begin();      
  display.clearDisplay();
  setContrast();  
  display.display(); 
  delay(100);

  //Serial.begin(115200);
  //connectAWS();
  /*Initializing hx117 ADC*/
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-227850); //Calibration Factor obtained from first sketch
  scale.tare(); //Reset the scale to 0
  
    /*Input declaration*/
  pinMode(0, INPUT_PULLUP); //D3 //Down button
  pinMode(2, INPUT_PULLUP); //D4 //Select button
  pinMode(3, FUNCTION_3); //RX //Up button //Do not initiate serial communication as it will disable the pin  
  pinMode(3, INPUT); //sd3
  pinMode(12, INPUT_PULLUP); //D6 //new up button
  pinMode(A0, INPUT); //A0 //Left button

  /*Stock save Cursor*/
  String uidaddrS = String(uidaddr);
  String pctaddrS = String(pctaddr);
  String pwtaddrS = String(pwtaddr);
  EEPROM.get(3840, gtuidaddrS);
  EEPROM.get(3852, gtpwtaddrS);
  EEPROM.get(3864, gtpctaddrS);
  EEPROM.get(0, frstentryS);
}

   
void loop() {
  
  drawMenu();
  
  readbuttonstate();
 
  checkIfDownButtonIsPressed();
  checkIfUpButtonIsPressed();
  checkIfSelectButtonIsPressed();
  checkIfRightButtonIsPressed();
  checkIfLeftButtonIsPressed();

  upbuttonpage();
  downbuttonpage();
  middlebuttonpage();
  rightbuttonpage();
  leftbuttonpage();
  //uidposiselect(int mi);
  countposiselect();
  //reading = (scale.get_units(10)*1000);

}

void readbuttonstate(){
  downButtonState = digitalRead(0);
  selectButtonState = digitalRead(2);
  upButtonState = digitalRead(3);
  rightButtonState = digitalRead(12);
  analogState = analogRead(A0);
  if (analogState >= 500){
    leftButtonState = 1;
  }
    else if(analogState <= 500){
      leftButtonState = 0;
      }
}


void checkIfDownButtonIsPressed() {
    if (downButtonState != lastDownButtonState) {
      if (downButtonState == 0) {
        down=true;
      }
    delay(50);
    }
    lastDownButtonState = downButtonState;
  }


void checkIfUpButtonIsPressed() {
    if (upButtonState != lastUpButtonState) {
      if (upButtonState == 0) {
         up=true;
      }
    delay(50);
    }
   lastUpButtonState = upButtonState;
}


void checkIfSelectButtonIsPressed() {
    if (selectButtonState != lastSelectButtonState) {
      if (selectButtonState == 0) {
         middle=true;
       }
    delay(50);
    }
   lastSelectButtonState = selectButtonState;
}


void checkIfRightButtonIsPressed() {
    if (rightButtonState != lastRightButtonState) {
      if (rightButtonState == 0) {
         right=true;
       }
    delay(50);
    }
   lastRightButtonState = rightButtonState;
}


void checkIfLeftButtonIsPressed() {
    if (leftButtonState != lastLeftButtonState) {
      if (leftButtonState == 0) {
         left=true;
       }
    delay(50);
    }
   lastLeftButtonState = leftButtonState;
}


void upbuttonpage() {
   if (up && page == 1 ) {
     
    up = false;
    if(menuitem==2 && frame ==2)//main menu page scrolls frame up to view menuitem 123 from 234
    {
      frame--; 
    }
     if(menuitem==3 && frame ==3)//main menu page scrolls frame up to view menuitem 234 from 345
    {
      frame--;
    }
     if(menuitem==4 && frame ==4)//main menu page scrolls frame up to view menuitem 345 from 456
    {
      frame--;
    }
    lastMenuItem = menuitem;
    menuitem--;
    if (menuitem==0)
    {
      menuitem=1;
    } 
  }
  else if (up && page == 2 && menuitem==3 ) { //To increase display contrast
    up = false;
    stockscroll--;
    if(stockscroll==0){
      stockscroll=35;
    }
  }
  else if (up && page == 4 && menuitem==3 ) { //To increase display contrast
    up = false;
    page=3;
    menuitem=3;
  }
  else if (up && page==2 && menuitem==2 && uidcur==1){ //To change first digit of UID
    up = false;
    u1++;
    if (u1==10){
      u1=0;
    }
  }
    else if (up && page==2 && menuitem==2 && uidcur==2){ //To change second digit of UID
    up = false;
    u2++;
    if (u2==10){
      u2=0;
    }
  }
  else if (up && page==2 && menuitem==2 && uidcur==3){ //To change third digit of UID
    up = false;
    u3++;
    if (u3==10){
      u3=0;
    }
  }
  else if (up && page == 7 && menuitem == 2 && cnfrmext==2){
    up = false;
    cnfrmext=1;
  }
  else if (up && page == 9 && menuitem==2 ) { //scrolls up through submenu of "ADD ITEMS"
    up = false;
    submenuitem--;
    if (submenuitem==1)
    {
      submenuitem=3;
    }
  }
    else if (up && page==10 && submenuitem==2 && positem==1){ //To change first digit of Count
    up = false;
    c1++;
    if (c1==10){
      c1=0;
    }
  }
  else if (up && page==10 && submenuitem==2 && positem==2){ //To change second digit of Count
    up = false;
    c2++;
    if (c2==10){
      c2=0;
    }
  }
  else if (up && page==10 && submenuitem==2 && positem==3){ //To change third digit of Count
    up = false;
    c3++;
    if (c3==10){
      c3=0;
    }
  }
  else if (up && page==10 && submenuitem==2 && positem==4){ //To change fourth digit of Count
    up = false;
    c4++;
    if (c4==10){
      c4=0;
    }
  }
  else if (up && page==10 && submenuitem==2 && positem==5){ //To change fifth digit of Count
    up = false;
    c5++;
    if (c5==10){
      c5=0;
    }
  }
  else if (up && page==2 && menuitem==4){
    up = false;
    odbitem--;
    if(odbitem==0){
      odbitem=3;
    }
  }
  else if(up && page == 2 && menuitem==5){
    up=false;
    wifimenuitem--;
    if(wifimenuitem==0){
      wifimenuitem=2;
     }
   }
  else if(up && page == 3 && menuitem==5 && wifimenuitem==1 && wifistate==2){
    up=false;
    onwifiitem--;
    if(onwifiitem==0){
      onwifiitem=3;
     }
   }
  else if (up && page==2 && menuitem==6){
    up = false;
    othermenuitem--;
    if(othermenuitem==0){
      othermenuitem=3;
    }
  }
  else if (up && page == 3 && menuitem==6 && othermenuitem==1) { //To decrease display contrast
    up = false;
    contrast++;
    setContrast();
  }

}


void downbuttonpage() {
  if (down && page == 1) 
  {
    down = false;
    if(menuitem==3 && lastMenuItem == 2) //main menu page scrolls frame down to view menuitem 234 from 123
    {
      frame ++;
    }
    else  if(menuitem==4 && lastMenuItem == 3) //main menu page scrolls frame down to view menuitem 345 from 234
    {
      frame ++;
    }
     else  if(menuitem==5 && lastMenuItem == 4 && frame!=4) //main menu page scrolls frame down to view menuitem 456 from 345
    {
      frame ++;
    }
    lastMenuItem = menuitem;
    menuitem++;  
    if (menuitem==7) 
    {
      menuitem--;
    }
  }
   else if (down && page == 2 && menuitem==3 ) { //To view stock
    down = false;
    stockscroll++;
    if(stockscroll==36){
      stockscroll=1;
    }

  }
  else if (down && page == 3 && menuitem==3){
    down = false;
    menuitem=3;
    page=4;
  }
  else if (down && page == 7 && menuitem == 2 && cnfrmext==1){
    down = false;
    cnfrmext=2;
  }
  else if (down && page == 9 && menuitem==2) { //scrolls down through submenu of "ADD ITEMS"
    down = false;
    submenuitem++;
    if (submenuitem==4)
    {
      submenuitem=2;
    }
  }
   else if (down && page==2 && menuitem==2 && uidcur==1){ //To decrease first digit of UID
    down = false;
    u1--;
    if (u1==-1){
      u1=9;
    }
  }
  else if (down && page==2 && menuitem==2 && uidcur==2){ //To decrease second digit of UID
    down = false;
    u2--;
    if (u2==-1){
      u2=9;
    }
  }
  else if (down && page==2 && menuitem==2 && uidcur==3){ //To decrease third digit of UID
    down = false;
    u3--;
    if (u3==-1){
      u3=9;
    }
  }
  else if (down && page==10 && submenuitem==2 && positem==1){ //To decrease first digit of Count
    down = false;
    c1--;
    if (c1==-1){
      c1=9;
    }
  }
  else if (down && page==10 && submenuitem==2 && positem==2){ //To decrease second digit of Count
    down = false;
    c2--;
    if (c2==-1){
      c2=9;
    }
  }
  else if (down && page==10 && submenuitem==2 && positem==3){ //To decrease third digit of Count
    down = false;
    c3--;
    if (c3==-1){
      c3=9;
    }
  }
  else if (down && page==10 && submenuitem==2 && positem==4){ //To decrease fourth digit of Count
    down = false;
    c4--;
    if (c4==-1){
      c4=9;
    }
  }
  else if (down && page==10 && submenuitem==2 && positem==5){ //To decrease fifth digit of Count
    down = false;
    c5--;
    if (c5==-1){
      c5=9;
    }
  }
  else if (down && page==2 && menuitem==4){
    down = false;
    odbitem++;
    if(odbitem==4){
      odbitem=1;
    }
  }
  else if(down && page == 2 && menuitem==5){
     down=false;
     wifimenuitem++;
     if(wifimenuitem==3){
      wifimenuitem=1;
     }
   }
  else if(down && page == 3 && menuitem==5 && wifimenuitem==1 && wifistate==2){
     down=false;
     onwifiitem++;
     if(onwifiitem==4){
      onwifiitem=1;
     }
   }
  else if (down && page==2 && menuitem==6){
    down = false;
    othermenuitem++;
    if(othermenuitem==4){
      othermenuitem=1;
    }
  }
  else if (down && page == 3 && menuitem==6 && othermenuitem==1) { //To decrease display contrast
    down = false;
    contrast--;
    setContrast();
  }
    
}

void middlebuttonpage(){
   if (middle) //Middle Button is Pressed
  {
    middle = false;
   
    if(page == 1 && menuitem ==6)// Reset is pressed
    {
      //eestate++;
      page=2;
      //resetDefaults();
    }
    else if (page == 1 && menuitem<6) {// To toggle to page 2 of all menu items
      page=2;
     }
    else if (page == 2 && menuitem==1) {// To fix the menuitem 1 jump to page 3 bug
      tarescale();
     }
    else if (page == 2 && menuitem==2 ) { //To toggle to page 3 of ADD ITEMS menu
      menuitem=2;
      page=3;
     }
    else if (page == 4 && menuitem==2 ) { //To toggle to page 3 of ADD ITEMS menu
      menuitem=2;
      page=5;
     }
    else if (page == 5 && menuitem==2) { //To save changes of contents in submenu and return to ADD ITEM Menu 
      menuitem=2;
      page=6;
     }
    else if (page == 6 && menuitem==2) { //To save changes of contents in submenu and return to ADD ITEM Menu 
      menuitem=2;
      page=8;
     }
    else if (page == 7 && menuitem == 2 && cnfrmext == 1){
       menuitem=2;
       page=6;
     }
     else if (page == 7 && menuitem == 2 && cnfrmext == 2){
       page=1;
     }
     else if (page == 8 && menuitem==2) { //To save changes of contents in submenu and return to ADD ITEM Menu 
      page=1;
     }
     else if (page == 9 && menuitem==2 && submenuitem == 2) { //To toggle to page 3 of ADD ITEMS menu
      menuitem = 2;
      submenuitem = 2;
      page=10;
     }
     else if (page == 10 && menuitem==2 && submenuitem == 2) { //To save changes of contents in submenu and return to ADD ITEM Menu 
      menuitem=2;
      page=9;
     }
     else if (page == 9 && menuitem==2 && submenuitem == 3) { //To toggle to page 3 of ADD ITEMS menu
      menuitem = 2;
      submenuitem = 3;
      page=11;
     }
     else if (page == 11 && menuitem==2 && submenuitem == 3) { //To toggle to page 3 of ADD ITEMS menu
      menuitem = 2;
      page=6;
     }
     else if (page == 2 && menuitem==4 && odbitem==1){
      menuitem=4;
      odbitem=1;
      page=3;
     }
     else if (page == 2 && menuitem==4 && odbitem==2){
      menuitem=4;
      odbitem=2;
      page=4;
     }
     else if (page == 2 && menuitem==4 && odbitem==3){
      menuitem=4;
      odbitem=3;
      page=6;
      WiFi.begin("ESP Cam");
     }
     else if (page == 2 && menuitem==5 && wifimenuitem==1){
      menuitem=5;
      wifimenuitem=1;
      page=3;
     }
     else if (page == 2 && menuitem==5 && wifimenuitem==2){
      menuitem=5;
      wifimenuitem=2;
      page=5;
     }
     else if (page == 3 && menuitem==5 && wifimenuitem==1){
      if(wifistate==1){
        wifistate=2;
        onwifiitem=1;
      }
      else if(wifistate==2 && onwifiitem==1){
        wifistate=1;
      }
      else if(wifistate==2 && onwifiitem==2){
        WiFi.begin(WiFi.SSID(), WiFi.psk());
        page=4;
        menuitem=5;
        wifimenuitem=1;
        wifistate=2;
        onwifiitem=2;
      }
      else if(wifistate==2 && onwifiitem==3){
        page=6;
        menuitem=5;
        wifimenuitem=1;
        wifistate=2;
        onwifiitem=3;
      }
     }
     else if (page == 2 && menuitem==6 && othermenuitem==1){
      menuitem=6;
      othermenuitem=1;
      page=3;
     }
     else if (page == 3 && menuitem==6 && othermenuitem==1){
      menuitem=6;
      othermenuitem=1;
      page=2;
     }
     else if (page == 2 && menuitem==6 && othermenuitem==2){
      menuitem=6;
      othermenuitem=2;
      page=4;
     }
     else if (page == 2 && menuitem==6 && othermenuitem==3){
      menuitem=6;
      othermenuitem=3;
      page=5;
     }
   }   
} 


void rightbuttonpage(){ //To shift through the digits of Uid, count, weight
   if(right && menuitem==2 && page==2){
    right=false;
    uidcur++;
    if(uidcur==4){
      uidcur=1;
      }
    }
   else if(right && menuitem==2 && page==8){
    right=false;
    menuitem=3;
    page=2;
  }
   else if(right && menuitem==2 && submenuitem==2 && page==10){
    right=false;
    positem++;
    if(positem==6){
      positem=1;
      }
    }
   else if(right && menuitem==1 && page==2){
    right=false;
    menuitem=2;
    page=2; 
    }
   else if(right && menuitem==4 && odbitem==2 && page==4){
    right=false;
    menuitem=4;
    odbitem==2;
    page=5; 
    }
   else if(right && page==5 && menuitem==5 && wifimenuitem==2){
    right=false;
    menuitem=5;
    wifimenuitem=2;
    page=7; 
    }
   else if(right && page==7 && menuitem==5 && wifimenuitem==2){
    right=false;
    menuitem=5;
    wifimenuitem=2;
    page=8; 
    }
   else if(right && page==8 && menuitem==5 && wifimenuitem==2){
    right=false;
    menuitem=5;
    wifimenuitem=2;
    page=9; 
    }
  }

  
void leftbuttonpage(){ //To go back
   if (left) //Left Button is Pressed
  {
    left = false;
    if((page == 2 && menuitem == 1) || (page == 2 && menuitem > 3)){
      page=1;
    }

    else if(page >= 2 && menuitem==3){
      page=1;
    }
    
    else if(menuitem==2 && page==2){
    uidcur--;
    if(uidcur==0){
      uidcur=3;
      }
    }
    else if(menuitem==2 && submenuitem==2 && page==10){
    positem--;
    if(positem==0){
      positem=5;
      }
    }
    else if (menuitem==2 && page==3){
      //menuitem=3;
      page=1;
    }
    else if (menuitem==2 && page==4){
      //menuitem=3;
      page=1;
    }
     else if (menuitem==2 && page==5){
      menuitem=3;
      page=4;
    }
     else if (menuitem==2 && page==6){
      menuitem=3;
      page=7 ;
    }
     else if (menuitem==2 && page==9){
      menuitem=3;
      page=1 ;
    }
    else if ((page == 3 && menuitem==4 && odbitem==1)||(page == 4 && menuitem==4 && odbitem==2)||(page == 6 && menuitem==4 && odbitem==3)){
      menuitem=4;
      page=2 ;
    }
    else if (page == 5 && menuitem==4 && odbitem==2){
      menuitem=4;
      page=4 ;
    }
    else if ((page ==3 && menuitem==5 && wifimenuitem==1)||(page==4 && menuitem==5 && wifimenuitem==1)){
      menuitem=5;
      page=2;
    }
    else if (page ==5 && menuitem==5 && wifimenuitem==2){
      menuitem=5;
      page=2;
    }
    else if (page ==7 && menuitem==5 && wifimenuitem==2){
      menuitem=5;
      wifimenuitem==2;
      page=5;
    }
    else if (page ==8 && menuitem==5 && wifimenuitem==2){
      menuitem=5;
      wifimenuitem==2;
      page=7;
    }
    else if (page ==9 && menuitem==5 && wifimenuitem==2){
      menuitem=5;
      wifimenuitem==2;
      page=8;
    }
    else if (page == 4 && menuitem==6 && othermenuitem==2){
      menuitem=6;
      othermenuitem=2;
      page=2;
     }
    else if (page==5 && menuitem==6 && othermenuitem==3 ){
      menuitem=6;
      page=2;
    }
  }
}

void uidposiselect(int pg, int mi){ //To darken selected digit of uid and then printing the entire digit in main add item menu
   if (page == pg && menuitem == mi && uidcur==1){
    display.setCursor(15, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(u1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(35, 30);
    display.print(u2);
    display.setCursor(55, 30);
    display.print(u3);
    uid=u1*100 + u2*10 + u3;
    display.display();
   }
    else if (page == pg && menuitem == mi && uidcur==2){
    display.setCursor(35, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(u2);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 30);
    display.print(u1);
    display.setCursor(55, 30);
    display.print(u3);
    uid=u1*100 + u2*10 + u3;
    display.display();
    }
    else if (page == pg && menuitem == mi && uidcur==3){
    display.setCursor(55, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(u3);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 30);
    display.print(u1);
    display.setCursor(35, 30);
    display.print(u2);
    uid=u1*100 + u2*10 + u3;
    display.display();
    }
}

void countposiselect(){ //To darken selected digit of count and then printing the entire digit in main add item menu
   if (page == 10 && submenuitem == 2 && positem==1){
    display.setCursor(5, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(c1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 30);
    display.print(c2);
    display.setCursor(35, 30);
    display.print(c3);
    display.setCursor(50, 30);
    display.print(c4);
    display.setCursor(65, 30);
    display.print(c5);
    ct=c1*10000 + c2*1000 + c3*100 + c4*10 + c5;
    display.display();
   }
    else if (page == 10 && submenuitem == 2 && positem==2){
    display.setCursor(20, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(c2);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 30);
    display.print(c1);
    display.setCursor(35, 30);
    display.print(c3);
    display.setCursor(50, 30);
    display.print(c4);
    display.setCursor(65, 30);
    display.print(c5);
    ct=c1*10000 + c2*1000 + c3*100 + c4*10 + c5;
    display.display();
    }
    else if (page == 10 && submenuitem == 2 && positem==3){
    display.setCursor(35, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(c3);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 30);
    display.print(c1);
    display.setCursor(20, 30);
    display.print(c2);
    display.setCursor(50, 30);
    display.print(c4);
    display.setCursor(65, 30);
    display.print(c5);
    ct=c1*10000 + c2*1000 + c3*100 + c4*10 + c5;
    display.display();
    }
    else if (page == 10 && submenuitem == 2 && positem==4){
    display.setCursor(50, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(c4);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 30);
    display.print(c1);
    display.setCursor(20, 30);
    display.print(c2);
    display.setCursor(35, 30);
    display.print(c3);
    display.setCursor(65, 30);
    display.print(c5);
    ct=c1*10000 + c2*1000 + c3*100 + c4*10 + c5;
    display.display();
    }
    else if (page == 10 && submenuitem == 2 && positem==5){
    display.setCursor(65, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(c5);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 30);
    display.print(c1);
    display.setCursor(20, 30);
    display.print(c2);
    display.setCursor(35, 30);
    display.print(c3);
    display.setCursor(50, 30);
    display.print(c4);
    ct=c1*10000 + c2*1000 + c3*100 + c4*10 + c5;
    display.display();
    }
}

void displayIntMenuPage(String menuItem, int value) //Function for Contrast adjustment
{
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    wifilogo();
    display.setCursor(15, 0);
    display.print(menuItem);
    display.drawFastHLine(0,10,83,BLACK);
    display.setCursor(25, 20);
    display.print("Value");
    display.setTextSize(2);
    display.setCursor(25, 30);
    display.print(value);
    display.setTextSize(2);
    display.display();
}

void setContrast(){
    display.setContrast(contrast);
    display.display();
  }

void displayMenuItem(String item, int position, boolean selected) //Function for menu item highlight
{
    if(selected)
    {
      display.setTextColor(WHITE, BLACK);
    }else
    {
      display.setTextColor(BLACK, WHITE);
    }
    display.setCursor(0, position);
    display.print(">"+item);
}

void tarescale(){
    scale.tare();
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 10);
    display.print("Tare Done!"); 
    display.setCursor(5, 25);
    display.print("Scale weight resetted to 0");
    display.display();
    delay(1000);
    menuitem=1;
    page=2;
}

void getstock(int ad1, int ad2, int ad3, int ad4, int ad5, int ad6, int ad7, int ad8, int ad9){
    String uidString = String(uid);
    String wtString = String(wt);
    String ctString = String(ct);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    wifilogo();
    //display.setCursor(0, 0);
    //display.print(uidaddr);
    display.setCursor(20,0);
    display.print("Page:");
    display.print(stockscroll);
    //display.setCursor(20,0);
    //display.print(pwtaddr);
    //display.setCursor(65,0);
    //display.print(pctaddr);
    display.setCursor(0, 10);
    display.print("UID");
    display.setCursor(25,10);
    display.print("PtWT");
    display.setCursor(60, 10);
    display.print("PtCT");
    
     display.setCursor(0, 20);
     display.print(EEPROM.get(ad1,uidString));
     display.setCursor(25, 20);
     display.print(EEPROM.get(ad2,wtString));
     display.setCursor(60, 20);
     display.print(EEPROM.get(ad3,ctString));
     
     
     display.setCursor(0,30);
     display.print(EEPROM.get(ad4,uidString));
     display.setCursor(25, 30);
     display.print(EEPROM.get(ad5,wtString));
     display.setCursor(60, 30);
     display.print(EEPROM.get(ad6,ctString));
     
     display.setCursor(0,40);
     display.print(EEPROM.get(ad7,uidString));
     display.setCursor(25, 40);
     display.print(EEPROM.get(ad8,wtString));  
     display.setCursor(60, 40);
     display.print(EEPROM.get(ad9,ctString));
     display.display();
}

void viewstock(){
  
   if(stockscroll==1){ //1-3
       getstock(0,12,24,36,48,60,72,84,96);
       display.display();
     }
     else if(stockscroll==2){ //4-6
       getstock(108,120,132,144,156,168,180,192,204);
       display.display();
     }
     else if(stockscroll==3){ //7-9
       getstock(216,228,240,252,264,276,288,300,312);
       //getstock(4896,4908,4920,4932,4944,4956,4968,4980,4992);
       display.display();
     }
     else if(stockscroll==4){ //10-12
       getstock(324,336,348,360,372,384,396,408,420);
       display.display();
     }
     else if(stockscroll==5){ //13-15
       getstock(432,444,456,468,480,492,504,516,528);
       display.display();
     }
     else if(stockscroll==6){ //16-18
       getstock(540,552,564,576,588,600,612,624,648);
       display.display();
     }
     else if(stockscroll==7){ 
       getstock(660,672,684,696,708,720,732,744,756);
       display.display();
     }
     else if(stockscroll==8){ 
       getstock(768,780,792,804,816,828,840,852,864);
       display.display();
     }
     else if(stockscroll==9){ 
       getstock(876,888,900,912,924,948,960,972,984);
       display.display();
     }
     else if(stockscroll==10){ 
       getstock(996,1008,1020,1032,1044,1056,1068,1080,1092);
       display.display();
     }
     else if(stockscroll==11){ 
       getstock(1104,1116,1128,1140,1152,1164,1176,1188,1200);
       display.display();
     }
     else if(stockscroll==12){ 
       getstock(1212,1224,1248,1260,1272,1284,1296,1308,1320);
       display.display();
     }
     else if(stockscroll==13){ 
       getstock(1332,1344,1356,1368,1380,1392,1404,1416,1428);
       display.display();
     }
     else if(stockscroll==14){ 
       getstock(1440,1452,1464,1476,1488,1500,1512,1524,1548);
       display.display();
     }
     else if(stockscroll==15){ 
       getstock(1560,1572,1584,1596,1608,1620,1632,1644,1656);
       display.display();
     }
     else if(stockscroll==16){ 
       getstock(1668,1680,1692,1704,1716,1728,1740,1752,1764);
       display.display();
     }
     else if(stockscroll==17){ 
       getstock(1776,1788,1800,1812,1824,1848,1860,1872,1884);
       display.display();
     }
     else if(stockscroll==18){ 
       getstock(1896,1908,1920,1932,1944,1956,1968,1980,1992);
       display.display();
     }
     else if(stockscroll==19){ 
       getstock(2004,2016,2028,2040,2052,2064,2076,2088,2100);
       display.display();
     }
     else if(stockscroll==20){ 
       getstock(2112,2124,2148,2160,2172,2184,2196,2208,2220);
       display.display();
     }
     else if(stockscroll==21){ 
       getstock(2232,2244,2256,2268,2280,2292,2304,2316,2328);
       display.display();
     }
     else if(stockscroll==22){ 
       getstock(2340,2352,2364,2376,2388,2400,2412,2424,2448);
       display.display();
     }
     else if(stockscroll==23){ 
       getstock(2460,2472,2484,2496,2508,2520,2532,2544,2556);
       display.display();
     }
     else if(stockscroll==24){ 
       getstock(2568,2580,2592,2604,2616,2628,2640,2652,2664);
       display.display();
     }
     else if(stockscroll==25){ 
       getstock(2676,2688,2700,2712,2724,2748,2760,2772,2784);
       display.display();
     }
     else if(stockscroll==26){ 
       getstock(2796,2808,2820,2832,2844,2856,2868,2880,2892);
       display.display();
     }
     else if(stockscroll==27){ 
       getstock(2904,2916,2928,2940,2952,2964,2976,2988,3000);
       display.display();
     }
     else if(stockscroll==28){ 
       getstock(3012,3024,3048,3060,3072,3084,3096,3108,3120);
       display.display();
     }
     else if(stockscroll==29){ 
       getstock(3132,3144,3156,3168,3180,3192,3204,3216,3228);
       display.display();
     }
     else if(stockscroll==30){ 
       getstock(3240,3252,3264,3276,3288,3300,3312,3324,3348);
       display.display();
     }
     else if(stockscroll==31){ 
       getstock(3360,3372,3384,3396,3408,3420,3432,3444,3456);
       display.display();
     }
     else if(stockscroll==32){ 
       getstock(3468,3480,3492,3504,3516,3528,3540,3552,3564);
       display.display();
     }
     else if(stockscroll==33){ 
       getstock(3576,3588,3600,3612,3624,3636,3648,3660,3672);
       display.display();
     }
     else if(stockscroll==34){ 
       getstock(3684,3696,3708,3720,3732,3744,3756,3768,3780);
       display.display();
     }
     else if(stockscroll==35){ 
       display.clearDisplay();
       display.setTextSize(1);
       display.setTextColor(BLACK, WHITE);
       display.setCursor(20,0);
       display.print("Page:");
       display.print(stockscroll);
       display.setCursor(0, 10);
       display.print("EEPROM ADDRESS");
       display.setCursor(0,20);
       display.print("UID");
       display.setCursor(25,20);
       display.print("PtWT");
       display.setCursor(60, 20);
       display.print("PtCT");
       display.setCursor(0, 30);
       display.print(EEPROM.get(3840, gtuidaddrS));
       display.setCursor(25, 30);
       display.print(EEPROM.get(3852, gtpwtaddrS));
       //display.print(pwtaddrS);
       display.setCursor(60, 30);
       display.print(EEPROM.get(3864, gtpctaddrS));
       //display.print(pctaddrS);
       display.setCursor(0, 40);
       display.print(uidaddr);
       display.setCursor(25, 40);
       display.print(pwtaddr);
       display.setCursor(60, 40);
       display.print(pctaddr);
       display.display();
     }    
}

static const unsigned char wificonnected [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 
  0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char wifiweak [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

void wifilogo(){
  if(WiFi.status() == 3 && WiFi.RSSI()<0 && WiFi.RSSI()>-70){
    display.drawBitmap(0, 0,  wificonnected, 84, 48, BLACK);
  }
  else if(WiFi.status() == 3 && WiFi.RSSI()>-90){
    display.drawBitmap(0, 0,  wifiweak, 84, 48, BLACK);
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "--"; 
  
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
  }
  // Free resources
  http.end();

  return payload;
}

void NTPConnect(void)
{
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void messageReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connectAWS()
{
  //delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 5);
  display.print("Connecting to saved wifi...");
  display.setCursor(0, 25);
  display.print(WIFI_SSID);
  display.display();
    
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 5);
    display.print("Connecting to saved wifi...");
    display.setCursor(0, 25);
    display.print(WIFI_SSID);
    delay(1000);
    display.display();
  }
 
  NTPConnect();
 
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
 
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
 
  Serial.println("Connecting to AWS IOT");
  display.clearDisplay();
  wifilogo();
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 10);
  display.print("Connecting to AWS IOT...");
  display.setCursor(0, 30);
  display.print(THINGNAME);
  display.display();
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(1000);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    display.clearDisplay();\
    wifilogo();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 0);
    display.print("AWS IoT Timeout!");
    display.display();
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("Connected to AWS IOT!");
  display.clearDisplay();
  display.setTextSize(1);
  wifilogo();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 10);
  display.print("Connected to  AWS IOT!");
  display.setCursor(0, 30);
  display.print(THINGNAME);
  display.display();
  delay(2000);
}

void publishMessage()
{
  while(k <= (uidaddr)){

       EEPROM.get(k,gtud);
       EEPROM.get(k+12, gtwt);
       EEPROM.get(k+24, gtct);
       uid= gtud.toInt();
       wt = gtwt.toFloat();
       ct = gtct.toInt();
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     wifilogo();
     display.setCursor(0, 0);
     display.print("UID:");
     display.print(uid);
     display.setCursor(0, 10);
     display.print("PtWT:");
     display.print(wt);
     display.setCursor(0, 20);
     display.print("PtCT:");
     display.print(ct);
     display.setCursor(0, 25);
     //display.print(k);
     display.display();
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["UID"] = uid;
  doc["Part Weight"] = wt;
  doc["Part Count"] = ct;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

  if(k<(uidaddr)){
      display.setCursor(10, 35);
      display.setTextColor(WHITE, BLACK);
      display.print("Uploading...");
      //display.print(k);
      display.display();
  }
  if(k>=(uidaddr)){
      display.setCursor(10, 35);
      display.setTextColor(WHITE, BLACK);
      display.print("Upload Done!");
      display.display();
      delay(3000);
      ESP.restart();
     }
  else if(WiFi.status()!=3){
      k=4500;
      display.setCursor(15,30);
      display.setTextColor(WHITE, BLACK);
      display.print("ERROR!!!");
      display.setCursor(5,40);
      display.setTextColor(WHITE, BLACK);
      display.print("WIFI IS OFF");
      display.display();   
   }
    delay(2000);
    k=k+36;
  }
}


void drawMenu() //Main Menu function 
  {
    //Serial.println("Serial port is back!!");
  if (page==1) //Main page header
  {    
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("NeoThermal"); //Change to alter main page title
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    //display.setCursor(0, 15);

   if(menuitem==1 && frame ==1) //Weight option will be darkened
    {   
      displayMenuItem(menuItem1, 15,true);
      displayMenuItem(menuItem2, 25,false);
      displayMenuItem(menuItem3, 35,false);
    }
    else if(menuitem == 2 && frame == 1) //Tare scale option will be darkened
    {
      displayMenuItem(menuItem1, 15,false);
      displayMenuItem(menuItem2, 25,true);
      displayMenuItem(menuItem3, 35,false);
    }
    else if(menuitem == 3 && frame == 1) //Add item option will be darkened
    {
      displayMenuItem(menuItem1, 15,false);
      displayMenuItem(menuItem2, 25,false);
      displayMenuItem(menuItem3, 35,true);
    }
     else if(menuitem == 4 && frame == 2) //Calibration option will be darkened
    {
      displayMenuItem(menuItem2, 15,false);
      displayMenuItem(menuItem3, 25,false);
      displayMenuItem(menuItem4, 35,true);
    }

      else if(menuitem == 3 && frame == 2) //Add item option will be darkened
    {
      displayMenuItem(menuItem2, 15,false);
      displayMenuItem(menuItem3, 25,true);
      displayMenuItem(menuItem4, 35,false);
    }
    else if(menuitem == 2 && frame == 2) //Tare scale option will be darkened
    {
      displayMenuItem(menuItem2, 15,true);
      displayMenuItem(menuItem3, 25,false);
      displayMenuItem(menuItem4, 35,false);
    }
    
    else if(menuitem == 5 && frame == 3) //Contrast option will be darkened
    {
      displayMenuItem(menuItem3, 15,false);
      displayMenuItem(menuItem4, 25,false);
      displayMenuItem(menuItem5, 35,true);
    }

    else if(menuitem == 6 && frame == 4) //Reset option will be darkened
    {
      displayMenuItem(menuItem4, 15,false);
      displayMenuItem(menuItem5, 25,false);
      displayMenuItem(menuItem6, 35,true);
    }
    
      else if(menuitem == 5 && frame == 4) //Contrast option will be darkened
    {
      displayMenuItem(menuItem4, 15,false);
      displayMenuItem(menuItem5, 25,true);
      displayMenuItem(menuItem6, 35,false);
    }
      else if(menuitem == 4 && frame == 4) //Calibration option will be darkened
    {
      displayMenuItem(menuItem4, 15,true);
      displayMenuItem(menuItem5, 25,false);
      displayMenuItem(menuItem6, 35,false);
    }
    else if(menuitem == 3 && frame == 3) //Add item option will be darkened
    {
      displayMenuItem(menuItem3, 15,true);
      displayMenuItem(menuItem4, 25,false);
      displayMenuItem(menuItem5, 35,false);
    }
        else if(menuitem == 2 && frame == 2) //Tare scale option will be darkened
    {
      displayMenuItem(menuItem2, 15,true);
      displayMenuItem(menuItem3, 25,false);
      displayMenuItem(menuItem4, 35,false);
    }
    else if(menuitem == 4 && frame == 3) //Calibration option will be darkened
    {
      displayMenuItem(menuItem3, 15,false);
      displayMenuItem(menuItem4, 25,true);
      displayMenuItem(menuItem5, 35,false);
    }
    
    if(uid!=0 && ct!=0){
    uidaddr=gtuidaddrS.toInt();
    pwtaddr=gtpwtaddrS.toInt();
    pctaddr=gtpctaddrS.toInt();
    }
    else if(uid == 0 && ct == 0 && frstentryS == "0"){
    uidaddr=gtuidaddrS.toInt();
    pwtaddr=uidaddr+12;
    pctaddr=uidaddr+24;
    //pwtaddr=(gtpwtaddrS.toInt())+12;
    //pctaddr=(gtpctaddrS.toInt())+24;    
    }
    else if(uid == 0 && ct == 0 && frstentryS != "0"){
    uidaddr=(gtuidaddrS.toInt())+36;
    //pwtaddr=uidaddr+12;
    //pctaddr=uidaddr+24;
    pwtaddr=(gtpwtaddrS.toInt())+48;
    pctaddr=(gtpctaddrS.toInt())+60;    
    }
    display.display();
  }
  
  else if (page==2 && menuitem == 1) //This page displays current weight of object placed 
  {    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 0);
    display.print("Current Weight");
    display.drawFastHLine(0,10,83,BLACK);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 15);
    display.print("Value in grams");
    display.setTextSize(2);
    display.setCursor(0, 30);
    Serial.println("Measuring Weight");
    reading = (scale.get_units(10)*1000);
    display.print(reading); //Previously declared object scale is used to get the weight value where the number in bracket indicates number of values averaged and *1000 converts Kg to gms value
    display.display();  
    }
  
  else if (page==2 && menuitem == 2) //ADD UID
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 0);
    display.print("Add UID");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 15);
    display.print("Enter 3 digits");
    display.setTextSize(2);
    uidposiselect(2,2);
    display.display(); 
  }

  else if (page==3 && menuitem ==2){
    if(uid != 0){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 0);
    display.print("Add UID");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0,15);
    display.setTextColor(BLACK, WHITE);
    display.print(">UID: BP-"); 
    display.print(uid);
    
    String uidString=String(uid);
 
    while( i < 3781 && gtud != uidString){
      //for(i=0; i<=uidaddr; i=i+36){ 
       EEPROM.get(i,gtud);
       display.setCursor(65, 0);
       //display.print(i);
       flag=0;
       if((gtud == uidString)&&(uidString != "0")){
        EEPROM.get(i+12, gtwt);
        EEPROM.get(i+24, gtct);
        flag=1;
        break;
       }
       i=i+36;
       //delay(500);
  }
       if(flag==1){
       display.setCursor(0, 30);
       display.setTextColor(WHITE, BLACK);
       display.print("UID Found!:");
       display.print(gtud);
       /*display.setCursor(0, 40); //Shows eeprom address of found uid, uncomment for troubleshooting
       display.print(i);
       display.setCursor(30, 40);
       display.print(i+12);
       display.setCursor(60, 40);
       display.print(i+24);*/
       display.display();
       delay(2500);
       menuitem=2;
       page=4;
       }
       
       if(flag==0){
        i=0;
       display.setCursor(0, 30);
       display.setTextColor(WHITE, BLACK);
       display.print("UID not found");
       display.setCursor(10, 40);
       display.setTextColor(WHITE, BLACK);
       display.print("ADD PART");
       display.display();
       delay(2500);
       menuitem=2;
       page=9;
      }
   }
   else if(uid==0){
     
     display.clearDisplay();
     display.setTextSize(2);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(10, 5);
     display.print("ERROR!");
     display.setTextColor(WHITE, BLACK);
     display.setCursor(10, 25);
     display.setTextSize(1);
     display.print("Enter Valid");
     display.setCursor(30, 35);
     display.print("UID!");
     display.display();
     delay(3000);
     menuitem=2;
     page=2;   
   }
 }
  else if (page==4 && menuitem==2){
    wt = gtwt.toFloat();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 0);
    display.print("Add UID");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0,15);
    display.setTextColor(BLACK, WHITE);
    display.print(">UID: BP-"); 
    display.print(uid);
    display.setCursor(0,25);
    display.setTextColor(BLACK, WHITE);
    display.print(">Part WT:");
    display.print(wt);
    display.setCursor(0,35);
    display.setTextColor(WHITE, BLACK);
    display.print(">Find Part Cnt");
    display.display();
    reading = (scale.get_units(10)*1000);
  }
  else if (page==5 && menuitem==2){
    //reading = (scale.get_units(10)*1000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("PART COUNT");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0,15);
    display.print("Part Count is:");
    fct = reading/wt;
    ct = round(fct);
    display.setTextSize(2);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0,30);
    display.print(ct);
    display.display();
  }
  else if (page==6 && menuitem==2){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    wifilogo();
    display.setCursor(0,0);
    display.print(">UID: BP-"); 
    display.print(uid);
    display.setCursor(0,10);
    display.print(">Prt WT:");
    display.print(wt);
    display.setCursor(0,20);
    display.print(">Prt CNT:");
    display.print(ct);
    if(flag==1){
    display.setCursor(10,30);
    display.setTextColor(WHITE, BLACK);
    display.print("Press OK to");
    display.setCursor(0,40);
    display.setTextColor(WHITE, BLACK);
    display.print("UPDATE IN MEM");
    display.display();
    
    }
    else if(flag==0){
    display.setCursor(10,30);
    display.setTextColor(WHITE, BLACK);
    display.print("Press OK to");
    display.setCursor(10,40);
    display.setTextColor(WHITE, BLACK);
    display.print("ADD IN MEM");
    display.display();
    
    }
  }
  else if (page==7 && menuitem==2){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5,0);
    display.print("CONFIRM EXIT?");
    if (cnfrmext==2){
      display.setTextColor(WHITE, BLACK);
    }
    else{
      display.setTextColor(BLACK, WHITE);
    }
    display.setTextSize(2);
    display.setCursor(0,30);
    display.print(">YES");
    if (cnfrmext==1){
      display.setTextColor(WHITE, BLACK);
    }
    else{
      display.setTextColor(BLACK, WHITE);
    }
    display.setTextSize(2);
    display.setCursor(0,10);
    display.print(">NO");

    display.display();
  }
  else if (page==8 && menuitem==2){
   if(flag==1){
    String uidString = String(uid);
    String wtString = String(wt);
    String ctString = String(ct);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(10,0);
    display.print("UID: BP-");
    display.print(uid);
    EEPROM.put(i,uidString); 
    EEPROM.put(i+12,wtString);
    EEPROM.put(i+24,ctString); 
    display.setCursor(10,20);
    display.print("UID updated");
    display.setCursor(0,30);
    display.print("in EEPROM MEM");
    display.display();
  }
  else if(flag==0){
    String uidString = String(uid);
    String wtString = String(wt);
    String ctString = String(ct);
    String uidaddrS = String(uidaddr);
    String pctaddrS = String(pctaddr);
    String pwtaddrS = String(pwtaddr);
    if((prevuid != uid)&&(prevuid != 0)){
     uidaddr = pctaddr + sizeof(ctString);
     pwtaddr = uidaddr + sizeof(uidString);
     pctaddr = pwtaddr + sizeof(wtString);
    }
    else if((prevuid != uid)&&(prevuid == 0)){
     pwtaddr = uidaddr + sizeof(uidString);
     pctaddr = pwtaddr + sizeof(wtString);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(10,0);
    display.print("UID: BP-");
    display.print(uid);
    EEPROM.put(uidaddr,uidString); 
    EEPROM.put(pwtaddr,wtString);
    EEPROM.put(pctaddr,ctString);
    EEPROM.put(3840, uidaddrS);
    EEPROM.put(3852, pwtaddrS);
    EEPROM.put(3864, pctaddrS);
    EEPROM.commit(); 
    prevuid=uid;
    display.setCursor(15,20);
    display.print("UID added");
    display.setCursor(0,30);
    display.print("in EEPROM MEM");
    display.display();
  }
 }

  else if (page==9 && menuitem==2){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    wifilogo();
    display.setCursor(20, 0);
    display.print("Add Part");
    display.drawFastHLine(0,10,83,BLACK);
    display.setCursor(0,15);
    display.setTextColor(BLACK, WHITE);
    display.print(">UID: BP-"); 
    display.print(uid);
    display.setCursor(0, 25);
    reading = (scale.get_units(10)*1000);

    if (submenuitem==2) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }    
    display.print(">Part Cnt:");
    display.print(ct);
    display.setCursor(0, 25);
       
    if (submenuitem==3) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }  
    display.setCursor(0, 35);
    display.print(">Find Part WT");
    
    display.display();
  }
  
  else if (page==10 && menuitem==2 && submenuitem==2) //Add Count 
    {
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(20, 0);
     display.print("Add Count");
     display.drawFastHLine(0,10,83,BLACK);
     wifilogo();
     display.setTextColor(BLACK, WHITE);
     display.setCursor(0, 15);
     display.print("Enter 5 digits");
     display.setTextSize(2);
     countposiselect();//enter function for count here
     display.display();  
    }
     else if (page==11 && menuitem==2 && submenuitem==3) //Add Weight 
    {
     //reading = (scale.get_units(10)*1000);
      if(ct > 0){
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(10, 0);
     display.print("Part Weight");
     display.drawFastHLine(0,10,83,BLACK);
     wifilogo();
     display.setTextColor(BLACK, WHITE);
     display.setCursor(0, 15);
     display.print("Weight in gms:");
     display.setTextSize(2);
     wt = reading/ct;
     display.setCursor(0, 30);
     display.print(wt);
     display.display();
      }
      else if(ct <=0){
     display.clearDisplay();
     display.setTextSize(2);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(10, 5);
     display.print("ERROR!");
     display.setTextColor(WHITE, BLACK);
     display.setCursor(10, 25);
     display.setTextSize(1);
     display.print("Enter Valid");
     display.setCursor(20, 35);
     display.print("Count!");
     display.display();
     delay(3000);
     menuitem=2;
     submenuitem=2;
     page=10;   
      }
     //enter function for weight here
     display.display();  
    }    
 
  else if (page==2 && menuitem == 3) //VIEW STOCK
  {  
     viewstock();
  }
  
  else if(page==2 && menuitem == 4){
    k=0;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("Online DB");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    if (odbitem==1) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
    display.setCursor(0, 15);    
    display.print(">Upload Data");
       
    if (odbitem==2) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }  
    display.setCursor(0, 25);
    display.print(">Credentials");
    
    if (odbitem==3) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }  
    display.setCursor(0, 35);
    display.print(">Get UID");
    display.display();
  }

  else if (page == 3 && menuitem==4 && odbitem==1){
  //k=12;
    now = time(nullptr);
 
  if (!client.connected())
  {
    connectAWS();
  }
  else
  {
    client.loop();
    if (millis() - lastMillis > 2000)
    {
      lastMillis = millis();
      publishMessage();
    }
  }
  /*
  while(k <= (uidaddr)){

       EEPROM.get(k,gtud);
       EEPROM.get(k+12, gtwt);
       EEPROM.get(k+24, gtct);
       uid= gtud.toInt();
       wt = gtwt.toFloat();
       ct = gtct.toInt();
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     wifilogo();
     display.setCursor(0, 0);
     display.print("UID:");
     display.print(uid);
     display.setCursor(0, 10);
     display.print("PtWT:");
     display.print(wt);
     display.setCursor(0, 20);
     display.print("PtCT:");
     display.print(ct);
     display.setCursor(0, 25);
     //display.print(k);
     display.display();
     k=k+36;
     if (Firebase.ready() && (millis() - sendDataPrevMillis > 150 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    Firebase.setFloat(fbdo, F("/Test/0/UID"), uid)? "ok" : fbdo.errorReason().c_str();
    Firebase.setFloat(fbdo, F("/Test/0/Part Count"), ct)? "ok" : fbdo.errorReason().c_str();
    Firebase.setFloat(fbdo, F("/Test/0/Part Weight"), wt)? "ok" : fbdo.errorReason().c_str();
    if(k<(uidaddr)){
      display.setCursor(10, 35);
      display.setTextColor(WHITE, BLACK);
      display.print("Uploading...");
      //display.print(k);
      display.display();
    }
    else if(k>(uidaddr)){
      display.setCursor(10, 35);
      display.setTextColor(WHITE, BLACK);
      display.print("Upload Done!");
      display.display();
     }
  }
   else if(WiFi.status()!=3){
      k=4500;
      display.setCursor(15,30);
      display.setTextColor(WHITE, BLACK);
      display.print("ERROR!!!");
      display.setCursor(5,40);
      display.setTextColor(WHITE, BLACK);
      display.print("WIFI IS OFF");
      display.display();   
   }
   else{
      k=4500;
      display.setCursor(15,30);
      display.setTextColor(WHITE, BLACK);
      display.print("ERROR!!!");
      display.setCursor(5,40);
      display.setTextColor(WHITE, BLACK);
      display.print("RETRY UPLOAD");
      display.display();   
   }
   delay(1000);
  }*/
}
   
  else if (page == 4 && menuitem==4 && odbitem==2){
     display.setTextSize(1);
     display.clearDisplay();
     display.setTextColor(BLACK, WHITE);
     wifilogo();
     display.setCursor(25, 0);
     display.print("DB URL");
     display.drawFastHLine(0,10,83,BLACK);
     display.setCursor(0, 15);
     //display.print(DATABASE_URL);
     display.display();
  }
  else if (page == 5 && menuitem==4 && odbitem==2){
     display.setTextSize(1);
     display.clearDisplay();
     display.setTextColor(BLACK, WHITE);
     wifilogo();
     display.setCursor(20, 0);
     display.print("DB EMAIL");
     display.drawFastHLine(0,10,83,BLACK);
     display.setCursor(0, 15);
     //display.print(USER_EMAIL);
     display.display();
  }
    else if (page == 6 && menuitem==4 && odbitem==3){    
       
       unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval) {
     // Check WiFi connection status
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      uidcam = httpGETRequest(serverNamecam);  
      // save the last HTTP GET Request
      previousMillis = currentMillis;
    }
  }
     display.setTextSize(1);
     display.clearDisplay();
     display.setTextColor(BLACK, WHITE);
     wifilogo();
     display.setCursor(10, 0);
     display.print(WiFi.status());
     display.print("GET UID");
     display.drawFastHLine(0,10,83,BLACK);
     display.setCursor(0, 15);
     display.print("UID:");
     display.print(uidcam);
     display.display();
  }


  else if (page==2 && menuitem == 5){
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(30, 0);
    display.print("WIFI");
    //display.print(wifistate); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 35);
    //display.print(EEPROM.get(3792,gtwifissid));
    //display.print(EEPROM.get(4200,gtwifipsk));
    display.setCursor(0, 15);

    if (wifimenuitem==1) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(">WIFI STS:");
    if(wifistate==1){
      display.print("OFF");
    }
    else if(wifistate==2){
      display.print("ON"); 
    }
    display.setCursor(0, 25);
    
    if (wifimenuitem==2) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
        
    display.print(">WIFI CREDS");
    display.display();
  }

  else if (page==3 && menuitem==5 && wifimenuitem==1){
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    wifilogo();
    display.setCursor(0, 0);
    display.print("WIFI STATUS");
    //display.print(test);
    //display.print(wifistate); 
    display.drawFastHLine(0,10,83,BLACK);
    display.setTextColor(WHITE, BLACK);
    if(wifistate==1){
      
      display.setCursor(0, 15);
      display.print(">WIFI STS: OFF");
      WiFi.mode(WIFI_OFF);
      display.setTextColor(BLACK, WHITE);
      display.setCursor(30, 25);
      display.print("WIFI");
      display.setCursor(5, 35);
      display.print("DISCONNECTED!");
    }
    else if(wifistate==2){
      display.setTextColor(BLACK, WHITE);
      if(onwifiitem==1){
            display.setCursor(0, 15);
            display.setTextColor(WHITE, BLACK);
            display.print(">WIFI STS: ON");
            display.setTextColor(BLACK, WHITE);
            display.setCursor(0, 25);
            display.print(">SAVED N/W");
            display.setCursor(0, 35);
            display.print(">NEW N/W");
      }
      else if(onwifiitem==2){
            display.setCursor(0, 15);
            display.setTextColor(BLACK, WHITE);
            display.print(">WIFI STS: ON");
            display.setCursor(0, 35);
            display.print(">NEW N/W");
            display.setTextColor(WHITE, BLACK);
            display.setCursor(0, 25);
            display.print(">SAVED N/W");
      }
      else if(onwifiitem==3){
            display.setCursor(0, 15);
            display.setTextColor(BLACK, WHITE);
            display.print(">WIFI STS: ON");
            display.setCursor(0, 25);
            display.print(">SAVED N/W");
            display.setTextColor(WHITE, BLACK);
            display.setCursor(0, 35);
            display.print(">NEW N/W");
      }
 
    }
    display.display();
  }
  else if (page==4 && menuitem==5 && wifimenuitem==1 && wifistate==2 && onwifiitem==2){
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("SAVED N/W");
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    if(WiFi.status()==3){
       display.setTextColor(WHITE, BLACK);
       display.setCursor(5, 15);
       display.print("CONNECTED to:");
       display.setCursor(0, 25);
       display.print(WiFi.SSID());
    }
    else if(WiFi.status()!=3){
       display.setTextColor(BLACK, WHITE);
       display.setCursor(5, 15);
       display.print("CONNECTING to:");
       display.setCursor(0, 25);
       display.print(WiFi.SSID());
    }
    display.display();     
  }
  else if (page==6 && menuitem==5 && wifimenuitem==1 && wifistate==2 && onwifiitem==3){
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(WHITE, BLACK);
    display.setCursor(5, 0);
    display.print("WiFi Details");
    display.setCursor(15, 10);
    display.print("Cleared!");
    display.setCursor(0, 20);
    display.print("Reconnect ESP");
    display.setCursor(0, 30);
    display.print("to WiFi using");
    display.setCursor(10, 40);
    display.print("Web Portal");
    display.display(); 
    wifiManager.resetSettings();
    wifiManager.autoConnect("Stock Counter WiFi Manager");
    test=2;
    menuitem=5;
    page=2;
    wifimenuitem=1;
    wifistate=2;
  }
  else if (page==5 && menuitem==5 && wifimenuitem==2){//SSID
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("WIFI SSID"); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0, 15);
    display.setTextColor(BLACK, WHITE);
    display.print("N/W SSID:");
    display.setCursor(0, 25);
    display.print(WiFi.SSID());
    display.display();
  }
  else if (page==7 && menuitem==5 && wifimenuitem==2){//Password
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("WIFI PASS"); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0, 15);
    display.setTextColor(BLACK, WHITE);
    display.print("N/W PASSWORD:");
    display.setCursor(0, 25);
    display.print(WiFi.psk());
    display.display();
  }
   else if (page==8 && menuitem==5 && wifimenuitem==2){//RSSI
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(30, 0);
    display.print("RSSI"); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0, 15);
    display.setTextColor(BLACK, WHITE);
    display.print("RSSI:");
    display.print(WiFi.RSSI());
    display.setCursor(0, 25);
    display.print("Quality:");
    display.setCursor(0, 35);
    if(WiFi.RSSI()>0){
       display.print("WIFI OFF");
    }
    else if(WiFi.RSSI()>-30){
       display.print("EXCELLENT");
    }
    else if(WiFi.RSSI()>-67){
       display.print("VERY GOOD");
    }
    else if(WiFi.RSSI()>-70){
       display.print("AVERAGE");
    }
    else if(WiFi.RSSI()>-80){
       display.print("NOT GOOD");
    }
    else if(WiFi.RSSI()>-90){
       display.print("UNUSABLE");
    }
    display.display();
  }
   else if (page==9 && menuitem==5 && wifimenuitem==2){//Local IP
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 0);
    display.print("WIFI IP"); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setCursor(0, 15);
    display.setTextColor(BLACK, WHITE);
    display.print("Local IP:");
    display.setCursor(0, 25);
    display.print(WiFi.localIP());
    display.display();
  }
  else if (page==2 && menuitem == 6){
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(25, 0);
    display.print("OTHER"); 
    display.drawFastHLine(0,10,83,BLACK);
    wifilogo();
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 15);
   
    if (othermenuitem==1) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(">CONTRAST:"); 
    display.print(contrast);
    display.setCursor(0, 25);
   
    if (othermenuitem==2) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }    
    display.print(">CALIBRATION");
    display.setCursor(0, 35);
    
    if (othermenuitem==3) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
        
    display.print(">EEPROM CLEAR");
    display.display();
  }
    else if (page==3 && menuitem == 6 && othermenuitem==1){
       displayIntMenuPage("CONTRAST", contrast);
  }
    else if (page==4 && menuitem == 6 && othermenuitem==2){
       display.setTextSize(1);
       display.clearDisplay();
       display.setTextColor(BLACK, WHITE);
       display.setCursor(10, 0);
       display.print("CALIBRATION"); 
       display.drawFastHLine(0,10,83,BLACK);
       wifilogo();
       display.setCursor(0, 15);
       display.print("Calib fctr:"); 
       display.setCursor(0, 25);
       display.print("10kg max Load"); 
       display.setCursor(0, 35);
       display.print("227850"); 
       display.display();
  }
    else if (page==5 && menuitem == 6 && othermenuitem==3){
      String clreeprm = "0";
      display.setTextSize(1);
      display.clearDisplay();
      display.setTextColor(BLACK, WHITE);
      wifilogo();
      display.setCursor(5, 0);
      display.print("EEPROM CLEAR"); 
      display.drawFastHLine(0,10,83,BLACK);
      display.setTextColor(WHITE, BLACK);
      display.setCursor(0, 20);
      display.print("Memory Cleared");
      display.setCursor(0, 30);
      display.print("Successfully!");
      display.display();
      while(m<3999){
        display.print(m);
        EEPROM.put(m, clreeprm);
        EEPROM.put(3840, clreeprm);
        EEPROM.put(3852, clreeprm);
        EEPROM.put(3864, clreeprm);
        EEPROM.commit();
        m=m+12;
        uidaddr=pwtaddr=pctaddr=0;
      }
      delay(2000);
      menuitem=6;
      page=2;
  }
}





  
  

 
