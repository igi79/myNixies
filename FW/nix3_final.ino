#include <avr/wdt.h>
#include <Wire.h>
#include "RTClib.h"

PCF8563 rtc;

DateTime n;


#define LED 2

volatile unsigned long lastupdate;
volatile unsigned long lastrefresh;
volatile unsigned long lastpress;
bool forcerefresh = false;
uint8_t flashtik = 0;
bool flashing = false;
uint8_t confirm = 0;

int column_pins[6] = {
  8,9,10,11,12,13
};

int plate_pins[4] = {
  7,6,5,4
};

void init_pins(void)
{
  for(int i=0;i<6;i++){
    pinMode(column_pins[i], OUTPUT);
    digitalWrite(column_pins[i], LOW);
  }
  for(int i=0;i<4;i++){
    pinMode(plate_pins[i], OUTPUT);
    digitalWrite(plate_pins[i], LOW);
  }
}

long last_time;
uint8_t column;
unsigned long int ix = 0;
uint8_t nix[6] = {3,1,4,1,5,9};
boolean plates = true;

#define FIRST_SCREEN 0
#define LAST_SCREEN 3
#define SETUP_SCREEN 3

int dayScreen = 0;

#define KEY_NONE 0
#define KEY_PREV 1
#define KEY_NEXT 2
#define KEY_SELECT 3
#define KEY_BACK 4

uint8_t uiKeyPrev = A2;
uint8_t uiKeyNext = A1;
uint8_t uiKeySelect = A0;

uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;
uint8_t last_key_code = KEY_NONE;

uint8_t inSetup = 0;
uint8_t setupItem = 0;

int sDateTime[6] = {2018, 12, 1, 15, 1, 0};
int sDateTimeMin[6] = {2000, 1, 1, 0, 0, 0};
int sDateTimeMax[6] = {3000, 12, 31, 23, 59, 1};

void updaten(){
   if(millis() - lastupdate >= 995 && inSetup == 0){
      n = rtc.now();
      lastupdate = millis();
   }
}

void uiStep(void) {
  uiKeyCodeSecond = uiKeyCodeFirst;
  if ( analogRead(uiKeyPrev) < 1000 )
    uiKeyCodeFirst = KEY_PREV;
  else if ( analogRead(uiKeyNext) < 1000 )
    uiKeyCodeFirst = KEY_NEXT;
  else if ( analogRead(uiKeySelect) < 1000 )
    uiKeyCodeFirst = KEY_SELECT;
  else 
    uiKeyCodeFirst = KEY_NONE;
  
  if ( uiKeyCodeSecond == uiKeyCodeFirst )
    uiKeyCode = uiKeyCodeFirst;
  else
    uiKeyCode = KEY_NONE;
}


void controller(void){
  if (( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode)) {
    return;
  }
  last_key_code = uiKeyCode;

  if(inSetup == 1){
    if(setupItem < 6){
      flashing = setupItem < 5 ? false : true;      
      switch (uiKeyCode){
        case KEY_PREV:
          if(sDateTime[setupItem] > sDateTimeMin[setupItem])sDateTime[setupItem]--;
          forcerefresh = true;
          break;
        case KEY_NEXT:
          if(sDateTime[setupItem] < sDateTimeMax[setupItem])sDateTime[setupItem]++;
          forcerefresh = true;
          break;
        case KEY_SELECT:
          if(setupItem < 5){
            setupItem++;
          } else {
            if(sDateTime[setupItem] == 1){
              DateTime newtime;
              newtime.setyear(sDateTime[0]);
              newtime.setmonth(sDateTime[1]);
              newtime.setday(sDateTime[2]);
              newtime.sethour(sDateTime[3]);
              newtime.setminute(sDateTime[4]);
              newtime.setsecond(0);
              rtc.adjust(newtime);
            }
            inSetup = 0;
            dayScreen = 0;
          }
          forcerefresh = true;
          break;          
      }
    } else {
    }     
  } else {
    flashing = false;
    switch (uiKeyCode){
      case KEY_PREV:
        if(dayScreen > FIRST_SCREEN)dayScreen--;
        forcerefresh = true;
        break;
      case KEY_NEXT:
        if(dayScreen < LAST_SCREEN)dayScreen++;
        forcerefresh = true;
        break;
      case KEY_SELECT:
        if(dayScreen == SETUP_SCREEN){
          inSetup = 1;
          setupItem = 0;
          sDateTime[0] = n.year();
          sDateTime[1] = n.month();
          sDateTime[2] = n.day();
          sDateTime[3] = n.hour();
          sDateTime[4] = n.minute();
          sDateTime[5] = 0;
          forcerefresh = true;
        }        
        break;        
    }    
  }
}

void setup()
{
  cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A = 27;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10);  
    TIMSK1 |= (1 << OCIE1A);
  sei();
  
  wdt_enable(WDTO_1S);
  
  init_pins();
  pinMode(LED, OUTPUT);
  last_time = millis();
  column  = 0;

  Wire.setClock(400000);
  Wire.begin();
  rtc.begin();
  updaten();  
}

void dayScreen1(){
  clr_nix(0);
  set_nix(n.second(), 0);
  set_nix(n.minute(), 2);
  set_nix(n.hour(), 4);
}

void dayScreen2(){
  clr_nix(255);
  set_nix(n.month(), 0);
  set_nix(n.day(), 2);
}

void dayScreen3(){
  clr_nix(255);
  set_nix(n.year(), 0);
}

void setup0(){
  clr_nix(255);
  set_nix(12345, 0);
}

void setupDateTime(){
  clr_nix(255);
  nix[0] = setupItem + 1;
  set_nix(sDateTime[setupItem], 0);
}

void setupConfirmation(){
  clr_nix(255);
  set_nix(sDateTime[setupItem], 0);
  switch(flashtik){
    case 0:
      nix[0] = 1;
      flashtik = 1;
      break;      
    case 1:
      nix[0] = 0;      
      flashtik = 0;
      break;      
  }
}

void view(){

    if((flashing && (millis() - lastrefresh >= 300)) ||forcerefresh && inSetup == 1){
      if(setupItem < 5){
        setupDateTime();      
      } else {
        setupConfirmation();
      }
      lastrefresh = millis();
      forcerefresh = false;
    }
    
  
    if((millis() - lastrefresh >= 1000 || forcerefresh) && inSetup != 1){
      switch(dayScreen){
        case 0:
          dayScreen1();
          break;
        case 1:
          dayScreen2();
          break;      
        case 2:
          dayScreen3();
          break;    
        case 3:
          setup0();
          break;    
        default:
          break;
      }
      lastrefresh = millis();
      forcerefresh = false;
    }

    digitalWrite(LED, dayScreen == 3 || inSetup == 1 ? HIGH : LOW);
}

void loop()
{
  wdt_reset();
  uiStep();
  updaten();
  controller();
  view();
}

void set_nix(long int value, uint8_t p){
  uint8_t i = 5 - p;
  if(value == 0){
      nix[i] = 0;
  }
  while (value > 0) {
   long int digit = value % 10;
   nix[i] = digit;
   value /= 10;
   if(i==0)break;
   i--;
  }
}

void clr_nix(uint8_t v){
  nix[0] = nix[1] = nix[2] = nix[3] = nix[4] = nix[5] = v;
}

ISR(TIMER1_COMPA_vect) {
  if(!plates){
    if(column > 5) column = 0;
    PORTB = 255;
    plates = true;
  } else {
    if(nix[column]<10)digitalWrite(column_pins[column], LOW);
    digitalWrite(plate_pins[3], (nix[column] & 0x8));
    digitalWrite(plate_pins[2], (nix[column] & 0x4));
    digitalWrite(plate_pins[1], (nix[column] & 0x2));
    digitalWrite(plate_pins[0], (nix[column] & 0x1));
    plates = false;
    column++;
  }
}
