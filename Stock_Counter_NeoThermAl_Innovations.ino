/* The user defined add weight submenu item is removed from this version and instead, the single item weight by dividing scale weight with count is displayed. 
 * The system can now connect to wifi via entered credentials and upload scale readings, count, uid and single item weight. 
 * The data is uploaded and refreshed on a firebase server
*/

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

//Provide the token generation process info.
#include <addons/TokenHelper.h>
//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <SPI.h>
#include "HX711.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define LOADCELL_DOUT_PIN 5 //D1
#define LOADCELL_SCK_PIN 4 //D2

/* 1. Define the API Key */
#define API_KEY "AIzaSyCo_W2RkkjZYEtb0IXbyHU3uce73Qj2IG8"
/* 2. Define the RTDB URL */
#define DATABASE_URL "loadsensor-96033-default-rtdb.firebaseio.com"
/* 3. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "chaitanyahs17@gmail.com"
#define USER_PASSWORD "chaitanyahs17"

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;

HX711 scale; //Declare object scale to call various operations like get values and tare
WiFiManager wifiManager;

float calibration_factor = -227850; //Calibration factor obtained by previous testing
int contrast=50;

int menuitem = 1; //For main menu items
int submenuitem = 1; //For submenu items of ADD ITEMS
int frame = 1; //For scrolling through the 6 menu options
int page = 1; //For menu and its contents inside
int lastMenuItem = 1;
int positem = 1; //For right and left scrolling in uid, count, weight, calibration menus

String menuItem1 = "WEIGHT";
String menuItem2 = "TARE SCALE";
String menuItem3 = "ADD ITEM";
String menuItem4 = "CALIBRATION";
String menuItem5 = "CONTRAST";
String menuItem6 = "RESET";


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
float lastReading;
float w = reading;

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


const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

/* Adafruit_PCD8544(CLK,DIN,D/C,CE,RST);*/
Adafruit_PCD8544 display = Adafruit_PCD8544(14, 13, 16, 15, 1); /*D5, D7, D0, D8, Txpin*/

void setup() {

  /*Display Initialization*/
  display.begin();      
  display.clearDisplay();
  setContrast();  
  display.display(); 
  delay(100);
  
  /*Initializing hx117 ADC*/
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-227850); //Calibration Factor obtained from first sketch
  scale.tare(); //Reset the scale to 0

    /*Input declaration*/
  pinMode(0, INPUT_PULLUP); //D3 //Down button
  pinMode(2, INPUT_PULLUP); //D4 //Select button
  pinMode(3, FUNCTION_3); //RX //Up button //Do not initiate serial communication as it will disable the pin  
  pinMode(3, INPUT_PULLUP); //Rx
  pinMode(12, INPUT_PULLUP); //D6 //Right button
  pinMode(A0, INPUT); //A0 //Left button

  wifiManager.resetSettings();
  wifiManager.autoConnect("Stock Counter WiFi Manager");
  
   /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
 
  
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
  uidposiselect();
  countposiselect();

// if (scale.wait_ready_timeout(100)) {
    //reading = (scale.get_units(10)*1000);
    //Serial.print("Weight: ");
    //Serial.println(reading);
    //if (reading == lastReading){
      //displayWeight(reading); 
   //}
   // lastReading = reading;
  //}
  //else {
    //Serial.println("HX711 not found.");
  //}
 //} 
  //float w = reading;

}




void readbuttonstate(){
  downButtonState = digitalRead(0);
  selectButtonState = digitalRead(2);
  upButtonState = digitalRead(3);
  rightButtonState = digitalRead(12);
  analogState = analogRead(analogInPin);
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
  }else if (up && page == 2 && menuitem==3 ) { //scrolls up through submenu of "ADD ITEMS"
    up = false;
    submenuitem--;
    if (submenuitem==0)
    {
      submenuitem=3;
    }
  }
  //else if (up && page == 2 && menuitem==4 ) { //Modifications to be done here to insert calibration number
    //up = false;
  //}
  else if (up && page == 2 && menuitem==5 ) { //To increase display contrast
    up = false;
    contrast--;
    setContrast();
  }
  else if (up && page==3 && submenuitem==1 && positem==1){ //To change first digit of UID
    up = false;
    u1++;
    if (u1==10){
      u1=0;
    }
  }
  else if (up && page==3 && submenuitem==1 && positem==2){ //To change second digit of UID
    up = false;
    u2++;
    if (u2==10){
      u2=0;
    }
  }
  else if (up && page==3 && submenuitem==1 && positem==3){ //To change third digit of UID
    up = false;
    u3++;
    if (u3==10){
      u3=0;
    }
  }
    else if (up && page==3 && submenuitem==2 && positem==1){ //To change first digit of Count
    up = false;
    c1++;
    if (c1==10){
      c1=0;
    }
  }
  else if (up && page==3 && submenuitem==2 && positem==2){ //To change second digit of Count
    up = false;
    c2++;
    if (c2==10){
      c2=0;
    }
  }
  else if (up && page==3 && submenuitem==2 && positem==3){ //To change third digit of Count
    up = false;
    c3++;
    if (c3==10){
      c3=0;
    }
  }
  else if (up && page==3 && submenuitem==2 && positem==4){ //To change fourth digit of Count
    up = false;
    c4++;
    if (c4==10){
      c4=0;
    }
  }
  else if (up && page==3 && submenuitem==2 && positem==5){ //To change fifth digit of Count
    up = false;
    c5++;
    if (c5==10){
      c5=0;
    }
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
  else if (down && page == 2 && menuitem==3) { //scrolls down through submenu of "ADD ITEMS"
    down = false;
    submenuitem++;
    if (submenuitem==4)
    {
      submenuitem=1;
    }
  }
  //else if (down && page == 2 && menuitem==4) { //Modifications to be done here to insert calibration number
    //down = false;
  //}
   else if (down && page == 2 && menuitem==5 ) { //To decrease display contrast
    down = false;
    contrast++;
    setContrast();
  }
   else if (down && page==3 && submenuitem==1 && positem==1){ //To decrease first digit of UID
    down = false;
    u1--;
    if (u1==-1){
      u1=9;
    }
  }
  else if (down && page==3 && submenuitem==1 && positem==2){ //To decrease second digit of UID
    down = false;
    u2--;
    if (u2==-1){
      u2=9;
    }
  }
  else if (down && page==3 && submenuitem==1 && positem==3){ //To decrease third digit of UID
    down = false;
    u3--;
    if (u3==-1){
      u3=9;
    }
  }
  else if (down && page==3 && submenuitem==2 && positem==1){ //To decrease first digit of Count
    down = false;
    c1--;
    if (c1==-1){
      c1=9;
    }
  }
  else if (down && page==3 && submenuitem==2 && positem==2){ //To decrease second digit of Count
    down = false;
    c2--;
    if (c2==-1){
      c2=9;
    }
  }
  else if (down && page==3 && submenuitem==2 && positem==3){ //To decrease third digit of Count
    down = false;
    c3--;
    if (c3==-1){
      c3=9;
    }
  }
  else if (down && page==3 && submenuitem==2 && positem==4){ //To decrease fourth digit of Count
    down = false;
    c4--;
    if (c4==-1){
      c4=9;
    }
  }
  else if (down && page==3 && submenuitem==2 && positem==5){ //To decrease fifth digit of Count
    down = false;
    c5--;
    if (c5==-1){
      c5=9;
    }
  }
    
}

void middlebuttonpage(){
   if (middle) //Middle Button is Pressed
  {
    middle = false;
   
    if(page == 1 && menuitem ==6)// Reset is pressed
    {
      resetDefaults();
    }
    else if (page == 1 && menuitem<6) {// To toggle to page 2 of all menu items
      page=2;
     }
     else if (page == 2 && menuitem==1) {// To fix the menuitem 1 jump to page 3 bug
      page=1;
     }
     else if (page == 2 && menuitem==5) {// To fix the menuitem 5 jump to page 3 bug
      page=1;
     }
     else if (page == 2 && menuitem==3 && submenuitem <= 3) { //To toggle to page 3 of ADD ITEMS menu
      page=3;
     }
     else if (page == 3) { //To save changes of contents in submenu and return to ADD ITEM Menu 
      menuitem=3;
      page=2;
     }
   }   
} 

void rightbuttonpage(){ //To shift through the digits of Uid, count, weight
  if(right && submenuitem==1 && page==3){
    right=false;
    positem++;
    if(positem==4){
      positem=1;
      }
    }
    else if(right && submenuitem==2 && page==3){
    right=false;
    positem++;
    if(positem==6){
      positem=1;
      }
    }
  }

  
void leftbuttonpage(){ //To go back
   if (left) //Left Button is Pressed
  {
    left = false;
    if(page == 2){
      page=1;
    }
    else if(submenuitem==1 && page==3){
    positem--;
    if(positem==0){
      positem=3;
      }
    }
    else if(submenuitem==2 && page==3){
    positem--;
    if(positem==0){
      positem=5;
      }
    }
  }
}

void uidposiselect(){ //To darken selected digit of uid and then printing the entire digit in main add item menu
   if (page == 3 && submenuitem == 1 && positem==1){
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
    else if (page == 3 && submenuitem == 1 && positem==2){
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
    else if (page == 3 && submenuitem == 1 && positem==3){
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
   if (page == 3 && submenuitem == 2 && positem==1){
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
    else if (page == 3 && submenuitem == 2 && positem==2){
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
    else if (page == 3 && submenuitem == 2 && positem==3){
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
    else if (page == 3 && submenuitem == 2 && positem==4){
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
    else if (page == 3 && submenuitem == 2 && positem==5){
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




void setContrast(){
    display.setContrast(contrast);
    display.display();
  }

void resetDefaults(){
    contrast = 50;
    setContrast();
    scale.tare();
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 15);
    display.print("Reset Done!"); 
    display.display();
    delay(2500);
    page=1;
}
 // void display1() {
    //display.setCursor(column,row);
  //}

void displayIntMenuPage(String menuItem, int value) //Function for Contrast adjustment
{
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print(menuItem);
    display.drawFastHLine(0,10,83,BLACK);
    display.setCursor(5, 15);
    display.print("Value");
    display.setTextSize(2);
    display.setCursor(5, 25);
    display.print(value);
    display.setTextSize(2);
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

//void wificonnect(){
  //wifiManager.resetSettings();
  //wifiManager.autoConnect("Stock Counter WiFi Manager");
  //if 
  
//}


void drawMenu() //Main Menu function 
  {
    
  if (page==1) //Main page header
  {    
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("NeoThermal"); //Change to alter main page title
    display.drawFastHLine(0,10,83,BLACK);
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
    display.setCursor(5, 15);
    display.print("Value in gms");
    display.setTextSize(2);
    display.setCursor(0, 25);
    reading = (scale.get_units(10)*1000);
    display.print(reading); //Previously declared object scale is used to get the weight value where the number in bracket indicates number of values averaged and *1000 converts Kg to gms value
    display.display();  
    }
    
  else if (page==2 && menuitem == 2) //This page will reset weight to 0
  {
    scale.tare();
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("Tare Done!"); 
    display.setCursor(5, 15);
    display.print("Scale weight resetted to 0");
    display.display();
    delay(2500);
    page=1;
  }
  
  else if (page==2 && menuitem == 3) //Will display submenus of UID, Count, Weight
  {
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 0);
    display.print("ADD ITEM"); 
    display.drawFastHLine(0,10,83,BLACK);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(0, 15);
   
    if (submenuitem==1) 
    { 
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }
    display.print(">UID: BP-"); 
    display.print(uid);
    display.setCursor(0, 25);
   
    if (submenuitem==2) 
    {
      display.setTextColor(WHITE, BLACK);
    }
    else 
    {
      display.setTextColor(BLACK, WHITE);
    }    
    display.print(">COUNT:");
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
    display.print(">WT:");
    display.print(wt);
    display.display();
  }
  
  else if (page==2 && menuitem == 4) //Enter Calibration factor in this menu
  {
    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 0);
    display.print("Calibration");
    display.drawFastHLine(0,10,83,BLACK);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(5, 15);
    display.print("Enter Factor:"); 
    display.display(); 
    }
  else if (page==2 && menuitem == 5) //Vary Contrast
  {
   displayIntMenuPage(menuItem5, contrast);
  }
  else if (page==2 && menuitem == 6) //Reset device
  {
   resetDefaults();
  }
  else if (page==3 && submenuitem == 1) //Add uid menu
  {    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(20, 0);
    display.print("Add UID");
    display.drawFastHLine(0,10,83,BLACK);
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 15);
    display.print("Enter 3 digits");
    display.setTextSize(2);
    uidposiselect();
    display.display();  
    }
    else if (page==3 && submenuitem==2) //Add Count 
    {
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(20, 0);
     display.print("Add Count");
     display.drawFastHLine(0,10,83,BLACK);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(0, 15);
     display.print("Enter 5 digits");
     display.setTextSize(2);
     countposiselect();//enter function for count here
     display.display();  
    }
     else if (page==3 && submenuitem==3) //Add Weight 
    {
     if (Firebase.ready() && (millis() - sendDataPrevMillis > 150 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    //Firebase.setFloat(fbdo, F("/Test/Weight"), reading)? "ok" : fbdo.errorReason().c_str();
    Firebase.setFloat(fbdo, F("/Test/0/UID"), uid)? "ok" : fbdo.errorReason().c_str();
    Firebase.setFloat(fbdo, F("/Test/0/Part Count"), ct)? "ok" : fbdo.errorReason().c_str();
    Firebase.setFloat(fbdo, F("/Test/0/Part Weight"), wt)? "ok" : fbdo.errorReason().c_str();
  
  }
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(0, 0);
     display.print("Single Weight");
     display.drawFastHLine(0,10,83,BLACK);
     display.setTextColor(BLACK, WHITE);
     display.setCursor(5, 15);
     display.print("Value in gms:");
     display.setTextSize(2);\
     wt = reading/ct;
     display.setCursor(0, 25);
     display.print(wt);
     //enter function for weight here
     display.display();  
    }
    
    
   }



  
  

 
