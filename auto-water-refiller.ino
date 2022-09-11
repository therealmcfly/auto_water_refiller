#include <DS1302.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
DS1302 rtc(6,5,4);
Time t;

//define IO pins
#define WS_PIN A1 
#define LBTN_PIN 2
#define RBTN_PIN 3
#define RLY_PIN 8

//res mode time variable
int reservedTime = 11;

//function declaration
void startDisplay(); //display phrase at startup
void leftBtn(); // ISR function for left button
void rightBtn(); // ISR function for right button
void runSetState();
void autoMode();
void runRunState();
void runResMode();
void tankIsFull();
void fillWaterTank();
void standBy();

//Mode States

const int STATESTATUS_DEFAULT = 1;
const int SETSEL_DEFAULT = 0;

volatile int stateStatus; // run state 0, set state 1
volatile int modeStatus; // autoMode 0, resMode 1, manualMode 2

volatile int setSelect;

int setModeLoopCount;
int resModeRefillCount;
int refillReq;

// LCD display txt arrays
char setModeTxt[3][17] = {
  "1. Auto Mode",
  "2. Res Mode ",
  "3. Manual Mode"
  };







void setup () {
  pinMode(LBTN_PIN, INPUT);
  pinMode(RBTN_PIN, INPUT);
  pinMode(RLY_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(LBTN_PIN), leftBtn, FALLING);
  attachInterrupt(digitalPinToInterrupt(RBTN_PIN), rightBtn, FALLING);

  lcd.init();
  lcd.backlight();
  lcd.begin(16,2);

  rtc.halt(false);
  rtc.writeProtect(true); //switch to false to set rtc
//  rtc.setTime(11,40,10);
//  rtc.setDate(11,9,2022);
  
  digitalWrite(RLY_PIN, 1);
  lcd.begin(16,2);
  
  stateStatus = STATESTATUS_DEFAULT;
  setSelect = SETSEL_DEFAULT;

  resModeRefillCount = 0;
  
  setModeLoopCount = 0;

  refillReq = 0;

  Serial.begin(9600);
  
  startDisplay();;
  
}

void loop() {
  Serial.print(stateStatus);
  Serial.print(modeStatus);
  Serial.println();
  
  switch (stateStatus) {
    
    case 0 :
      runRunState();
      break;
    case 1 :
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Entering");
      lcd.setCursor(0,1);
      lcd.print("Set Mode");
      delay(300);
      for(int i = 0;i<5; i++) {
        lcd.setCursor((8+i),1);
        lcd.print(".");
        delay(300);
      }
      runSetState();
      break;
      
  }
  delay(100);
}









//functions

void startDisplay () {
  lcd.setCursor(0,0);
  lcd.print("#Water Refiller#");
  lcd.setCursor(0,1);
  lcd.print("ver. 2.02       ");
  delay(3000);
}

void runSetState () {
  lcd.clear();
  
  while(stateStatus >0) {
    if (stateStatus == 1) {
      lcd.setCursor(0,0);
      lcd.print("Select a mode");
      lcd.setCursor(0,1);
      lcd.print(setModeTxt[setSelect]);
      delay(300);
      lcd.setCursor(0,1);
      lcd.print("                ");
      delay(300);

      if(setModeLoopCount <= 30) setModeLoopCount++;
      else if(setModeLoopCount > 30) {
        stateStatus = 0;
        setModeLoopCount = 0;
      }
      if(stateStatus != 1) setModeLoopCount = 0;
    }
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Saved!          ");
  delay(2000);
  lcd.clear();
}

void runRunState() {
  if (refillReq > 0) {
    fillWaterTank();
    refillReq = 0;
  }
  switch (modeStatus) {
    case 0 :
      fillWaterTank();
      break;
    case 1 :
      switch (resModeRefillCount) {
        case 0:
          runResMode();
        case 1:
          standBy();
        default :
          break;
      }
      break;
    case 2:
      standBy();
    default :
      break;
  } 
}

void runResMode() {
  t = rtc.getTime();
  Serial.println(rtc.getTimeStr());
//  Serial.println(rtc.getDateStr());
  if (t.hour != reservedTime) {
    resModeRefillCount = 0;
    standBy();
  } else {
    if(resModeRefillCount > 0) standBy();
    else {
      lcd.clear();
      for (int i = 0; i<3; i++) {
        lcd.setCursor(0,0);
        lcd.print("Reserved time   ");
        lcd.setCursor(0,1);
        lcd.print("Auto Mode Init  ");
        delay(1000);
        lcd.clear();
        delay(300);
      }
      fillWaterTank();
      resModeRefillCount++;
    }
  }
  delay(200);
}

void tankIsFull() {
  lcd.clear();   
  for (int i = 0; i<3; i++) 
  {
  lcd.setCursor(0,0);
  lcd.print("Tank is Full!      ");
  delay(1000);
  lcd.clear();
  delay(500);
  }
}

void fillWaterTank() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sensing");
  lcd.setCursor(0,1);
  lcd.print("Water level:    ");
  for (int i = 0; i<5; i++)
  {
    lcd.setCursor(7+i,0);
    lcd.print(".");
    lcd.setCursor(13,1);
    lcd.print(analogRead(WS_PIN));
    delay(1000);
  }
  
  if(analogRead(WS_PIN) < 300) 
  {
//    detachInterrupt(digitalPinToInterrupt(LBTN_PIN));
//    detachInterrupt(digitalPinToInterrupt(RBTN_PIN));
    digitalWrite(RLY_PIN, 0);
    lcd.setCursor(0,0);
    lcd.print("Water level:    ");
    lcd.setCursor(0,1);
    lcd.print("Filling water   ");
    int dotCount = 0;
    while (analogRead(WS_PIN) < 500) 
    {
      if (digitalRead(RBTN_PIN) == 0) break;
      lcd.setCursor(13,0);
      lcd.print(analogRead(WS_PIN));
      
      if (dotCount==0) {
        lcd.setCursor(12+1,1);
        lcd.print("   ");
        dotCount++;
      } 
      else if (dotCount > 0 && dotCount <3) {
        lcd.setCursor(12+dotCount,1);
        lcd.print(".");
        dotCount++;
      }
      else {
        lcd.setCursor(12+dotCount,1);
        lcd.print(".");
        dotCount= 0;
      }
      
      delay(300);
    }
    digitalWrite(RLY_PIN, 1);
    tankIsFull();
//    attachInterrupt(digitalPinToInterrupt(LBTN_PIN), leftBtn, FALLING);
//    attachInterrupt(digitalPinToInterrupt(RBTN_PIN), rightBtn, FALLING);
  } else {
    tankIsFull();
  }
}

void standBy() {
  lcd.setCursor(0,0);
  lcd.print("WaterTank Filler");
  lcd.setCursor(0,1);
  lcd.print("Standing by.....");
}

//ISR functions 
void leftBtn() {
  switch(stateStatus) {
    case 0 :
      refillReq = 1;
      break;
    case 1 :
        if (setSelect<2) setSelect++;
      else setSelect = 0;
      break;
    default :
      break;
  }
}

void rightBtn () {
  switch(stateStatus) {
    case 0 : 
      stateStatus = 1;
      setSelect = modeStatus;
      break;
    case 1 :
      switch(setSelect) {
            case 0 :
                modeStatus = 0;
                stateStatus = 0;
                break;
            case 1 :
                modeStatus = 1;
                stateStatus = 0;
                break;
            case 2 : 
                modeStatus = 2;
                stateStatus = 0;
                break;
            default :
              break;
          }
      break;
      break;
  }
}
