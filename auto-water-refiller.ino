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


// ISR function prototype
void ISR_leftBtn(); // ISR function for left button
void ISR_rightBtn(); // ISR function for right button


//function 
void runRunState();
void runSetState();
void runResSetState();

void runrunAutoMode();
void runResMode();
void runManualMode();

void startDisplay();//display phrase at startup
void standBy();
void tankIsFull();
void fillWaterTank();

//Mode States

const int STATE_DEFAULT = 0;
const int MODE_DEFAULT = 2;

volatile int stateStatus; // run state 0, set state 1, res time set state 2
volatile int modeStatus; // autoMode 0, resMode 1, manualMode 2

volatile int modeSelect;

//res mode time variables
int reservedHour;
int reservedMinute;

volatile int resSetSelect;
volatile int resHourSelect;
volatile int resMinuteSelect;

// other variables
int setModeLoopCount;
int resModeRefillCount;
int refillReq;

bool selectCount = 0;

bool interruptStandBy;

// LCD display text arrays
char setModeTxt[3][17] = {
  "1. Auto Mode",
  "2. Res Mode ",
  "3. Manual Mode"
  };


void setup () {
	
  pinMode(LBTN_PIN, INPUT);
  pinMode(RBTN_PIN, INPUT);
  pinMode(RLY_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.begin(16,2);

  rtc.halt(false);
  rtc.writeProtect(true); //switch to false to set rtc
  //rtc.setTime(22,14,10);
  //rtc.setDate(18,9,2022);
  
  digitalWrite(RLY_PIN, 1);
  lcd.begin(16,2);

  
  attachInterrupt(digitalPinToInterrupt(LBTN_PIN), ISR_leftBtn, FALLING);
  attachInterrupt(digitalPinToInterrupt(RBTN_PIN), ISR_rightBtn, FALLING);
  
  stateStatus = STATE_DEFAULT;
  modeStatus = MODE_DEFAULT;
  modeSelect = modeStatus;

  resModeRefillCount = 0;
  setModeLoopCount = 0;
  refillReq = 0;

  interruptStandBy = false;

  reservedHour = 8;
  reservedMinute = 00;

  resSetSelect = 0;
  resHourSelect = reservedHour;
  resMinuteSelect = reservedMinute;

  Serial.begin(9600);
  
  startDisplay();;
  
}

void loop() {
	Serial.print("State Status : ");
	Serial.print(stateStatus);
	Serial.println();
	Serial.print("Mode Status : ");
	Serial.print(modeStatus);
	Serial.println();
	Serial.print("reservedHour");
	Serial.print(reservedHour);
	Serial.println();
	Serial.print("reservedMinute");
	Serial.print(reservedMinute);
	Serial.println();
	Serial.print("Res Set Select");
	Serial.print(resSetSelect);
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
			lcd.print("Set State");
			delay(300);
			for(int i = 0;i<5; i++) {
				lcd.setCursor((9+i),1);
				lcd.print(".");
				delay(300);
			}
		  
			runSetState();
			break;
		
		case 2:
			runResSetState();
			break;
      
	}
	if(interruptStandBy) interruptStandBy = false;
	delay(100);
}









//functions

void startDisplay () {
  lcd.setCursor(0,0);
  lcd.print("#Water Refiller#");
  lcd.setCursor(0,1);
  lcd.print("   Ver. 2.02    ");
  delay(3000);
}

void runSetState () {
  lcd.clear();
  while(stateStatus == 1) {
    lcd.setCursor(0,0);
    lcd.print("Select a mode");
    lcd.setCursor(0,1);
   
    lcd.print(setModeTxt[modeSelect]);
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
	if(interruptStandBy) interruptStandBy = false;
  }
  if(stateStatus == 0) {
	  lcd.clear();
	  lcd.setCursor(0,0);
	  lcd.print("Entering        ");
	  lcd.setCursor(0,1);
	  lcd.print(setModeTxt[modeStatus]);
	  delay(2000);
	  lcd.clear();
  }
}

void runResSetState() {
	
	selectCount = !selectCount;
	
	lcd.setCursor(0,0);
	lcd.print("Set reserve time");
	lcd.setCursor(7,1);
	lcd.print(":");
	if(selectCount) {
		if(resHourSelect > 9) {
			lcd.setCursor(5,1);
			lcd.print(resHourSelect);
		} else {
			lcd.setCursor(5,1);
			lcd.print("0");
			lcd.setCursor(6,1);
			lcd.print(resHourSelect);
		}
		if(resMinuteSelect > 9) {
			lcd.setCursor(8,1);
			lcd.print(resMinuteSelect);
		} else {
			lcd.setCursor(8,1);
			lcd.print("0");
			lcd.setCursor(9,1);
			lcd.print(resMinuteSelect);
		}
	} else if (!selectCount) {
		if (resSetSelect == 0) {
				lcd.setCursor(5,1);
				lcd.print("  ");
		} else if (resSetSelect == 1) {
				lcd.setCursor(8,1);
				lcd.print("  ");
		}
	}
	if (resSetSelect == 2) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("Entering        ");
		lcd.setCursor(0,1);
		lcd.print(setModeTxt[modeStatus]);
		stateStatus = 0;
		resSetSelect = 0;
		delay(1800);
	}
	delay(200);
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
      t = rtc.getTime();
      Serial.println(rtc.getTimeStr());
      // Serial.println(rtc.getDateStr());
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

  if (t.hour != reservedHour && t.min != reservedMinute) {
    resModeRefillCount = 0;
    standBy();
  } else {
    lcd.clear();
    for (int i = 0; i<3; i++) {
      lcd.setCursor(0,0);
      lcd.print("Reserve time met");
      lcd.setCursor(0,1);
      lcd.print("Auto Mode Init  ");
      delay(1000);
    }
    fillWaterTank();
    resModeRefillCount++;
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
  
  if(analogRead(WS_PIN) < 300) //this loop statement loops until water tank is full
  {
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
void ISR_leftBtn() {
  if(!interruptStandBy) {
    switch(stateStatus) {
      case 0 : //in run state -> manual refill
        refillReq = 1;
        break;
      case 1 : //set state -> choosing mode to select
          if (modeSelect<2) modeSelect++;
        else modeSelect = 0;
        break;
      case 2: //res time set state -> choosing time to set
        switch (resSetSelect) {
          case 0 : // choosing hour
            if(resHourSelect < 23) resHourSelect++;
            else resHourSelect = 0;
            break;
          case 1 : // choosing minute
            if(resMinuteSelect < 59) resMinuteSelect++;
            else resMinuteSelect = 0;
            break;
        }
      default :
        break;
    }
  interruptStandBy = true;
  }
}
void ISR_rightBtn () {
  if(!interruptStandBy) {
    switch(stateStatus) {
      case 0 : //run State
        stateStatus = 1;
        modeSelect = modeStatus;
        break;
      case 1 : //set state
        switch(modeSelect) {
            case 0 : //select auto mode
                modeStatus = 0;
                stateStatus = 0;
                break;
            case 1 : //select res mode and enter res time set state
                modeStatus = 1;
                stateStatus = 2;
                break;
            case 2 : //select manual mode
                modeStatus = 2;
                stateStatus = 0;
                break;
            default :
              break;
        }
		break;
      case 2 : //res time set state
        switch (resSetSelect) {
          case 0 : //go to minute set
            reservedHour = resHourSelect;
            resSetSelect = 1;
            break;
          case 1 : // time set complete enter res mode
            reservedMinute = resMinuteSelect;
			resSetSelect = 2;
            break;
          default : 
            break;
        }
      break;
    }
  interruptStandBy = true;
  }
}
