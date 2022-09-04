#include <DS1302.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
DS1302 rtc(4,3,2);
Time t;

int autoRefillTime = 9;

#define WS_PIN A0
#define RLY_PIN 7
#define BTN_PIN 8



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
  lcd.print("Sensing tank... ");
  lcd.setCursor(0,1);
  lcd.print("Water level:    ");
  for (int i = 0; i<10; i++)
  {
    lcd.setCursor(13,1);
    lcd.print(analogRead(WS_PIN));
    delay(250);
  }
  
  if(analogRead(WS_PIN) < 300) 
  {
    digitalWrite(RLY_PIN, 0);
    lcd.setCursor(0,0);
    lcd.print("Water level:     ");
    lcd.setCursor(0,1);
    lcd.print("Filling water... ");
    while (analogRead(WS_PIN) < 500) 
    {
      if (digitalRead(BTN_PIN) == 1) break;
      lcd.setCursor(13,0);
      lcd.print(analogRead(WS_PIN));
      delay(200);
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
  lcd.print("Standing by...  ");
}


void setup() {
  lcd.init();
  lcd.backlight();

  rtc.halt(false);
  rtc.writeProtect(true);

  
  
  Serial.begin(9600);
  pinMode(WS_PIN, INPUT);
  pinMode(RLY_PIN, OUTPUT);
  digitalWrite(RLY_PIN, 1);

  lcd.setCursor(0,0);
  lcd.print("WaterTank Filler");
  lcd.setCursor(0,1);
  lcd.print("Start!          ");
  delay(1000);
}

void loop() {
  t = rtc.getTime();
//  Serial.println(t.hour);
  
  if(t.hour == autoRefillTime) {
    lcd.clear();
    for (int i = 0; i<3; i++) {
      lcd.setCursor(0,0);
      lcd.print("Auto-Refill: On");
      delay(1000);
      lcd.clear();
      delay(500);
    }
    fillWaterTank();
    while(t.hour == autoRefillTime && digitalRead(BTN_PIN) == 0) {
      lcd.setCursor(0,0);
      lcd.print("Auto-Refill: Off ");
      lcd.setCursor(0,1);
      lcd.print("Standing by...  ");
      delay(200);
    }
  }
  else if (digitalRead(BTN_PIN) == 1) fillWaterTank();
  else standBy();
  delay(200);
}
