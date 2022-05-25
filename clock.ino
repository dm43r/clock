#include <FastLED.h>
#include <microDS3231.h>
#include <EncButton2.h>
#include <EEPROM.h>

#define LED_PIN 13
#define BTN1_PIN 2
#define BTN2_PIN 3
#define BTN3_PIN 4
#define TEMP_PIN A0

#define ADDRESS_BRIGHTNESS 30
#define ADDRESS_COLOR 31
#define ADDRESS_EFFECT 32

#define TIME_UPDATE_INTERVAL 1000
#define TEMPERATURE_UPDATE_INTERVAL 1000
#define BLICK_POINT_INTERVAL_ON 3000
#define BLICK_POINT_INTERVAL_OFF 400
#define BLICK_POINT_INTERVAL_CHANGE_ON 700
#define BLICK_POINT_INTERVAL_CHANGE_OFF 300
#define EFFECT1_INTERVAL 60
#define EFFECT2_INTERVAL 60
#define EFFECT3_INTERVAL 1000
#define NUM_OF_COLORS 85
#define NUM_OF_EFFECTS 3

CRGB leds[58];
MicroDS3231 rtc;
char time[8];
EncButton2<EB_BTN> btn[3];

enum {
  TIME_SHOW,
  TIME_SETTING_H,
  TIME_SETTING_M,
  BRIGHTNESS_SETTING,
  COLOR_SETTING,
  TEMPERATURE_SHOW,
} mode;

byte h, m;
byte temp;

byte indexCurrentBrightness = 4;
const byte listBrightness[] = {20, 50, 75, 100, 125, 150, 175, 200, 225, 255};
const byte sizeOfListBrightness = sizeof(listBrightness)/sizeof(byte);

byte indexCurrentColor = 0;
const CRGB listColor[] = {0xFFFFFF, 0xFF0000, 0xFF8000, 0xFFFF00, 0x80FF00, 0x00FF00, 0x00FF80, 0x00FFFF, 0x0080FF, 0x0000FF, 0x8000FF, 0xFF00FF, 0xFF0080};
const byte sizeOfListColor = sizeof(listColor)/sizeof(CRGB);

byte indexCurrentEffect = 0;
const byte numOfEffects = NUM_OF_EFFECTS;

const byte numOfColors = NUM_OF_COLORS;
byte k = 1530/numOfColors;
unsigned long changer = 0;
CRGB listColorsForEffect[numOfColors];
CRGB randomColors[58];

bool blickTime = false;
bool blickChangeTime = false;
unsigned long timerForPointsTime, timerForPointsTime2, timerForUpdateTime, timerForUpdateTemp, timerForTimeSetting, timerForTimeSetting2, timerForEffect;

void setup() {
  if (EEPROM.read(ADDRESS_BRIGHTNESS) > sizeOfListBrightness - 1) EEPROM.update(ADDRESS_BRIGHTNESS, indexCurrentBrightness);
  else indexCurrentBrightness = EEPROM.read(ADDRESS_BRIGHTNESS);
  if (EEPROM.read(ADDRESS_COLOR) > sizeOfListColor - 1) EEPROM.update(ADDRESS_COLOR, indexCurrentColor);
  else indexCurrentColor = EEPROM.read(ADDRESS_COLOR);
  if (EEPROM.read(ADDRESS_EFFECT) > numOfEffects) EEPROM.update(ADDRESS_EFFECT, indexCurrentEffect);
  else indexCurrentEffect = EEPROM.read(ADDRESS_EFFECT);
  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, 58);
  FastLED.setBrightness(listBrightness[indexCurrentBrightness]);

  pinMode(TEMP_PIN, INPUT);
  btn[0].setPins(INPUT_PULLUP, BTN1_PIN);
  btn[1].setPins(INPUT_PULLUP, BTN2_PIN);
  btn[2].setPins(INPUT_PULLUP, BTN3_PIN);

  rtc.getTimeChar(time);
  temp = getTemp();
  fillRandomColors();
  mode = TIME_SHOW;
  h = rtc.getHours();
  m = rtc.getMinutes();

  for (unsigned int i = 1; i <= 1530; i++)
    if (i>=255*0+1 && i<=255*1 && i%k==0) listColorsForEffect[i/k-1] = CRGB(0, 255, i-255*0-1);
    else if (i>=255*1+1 && i<=255*2 && i%k==0) listColorsForEffect[i/k-1] = CRGB(0, 255-(i-255*1-1), 255);
    else if (i>=255*2+1 && i<=255*3 && i%k==0) listColorsForEffect[i/k-1] = CRGB(i-255*2-1, 0, 255);
    else if (i>=255*3+1 && i<=255*4 && i%k==0) listColorsForEffect[i/k-1] = CRGB(255, 0, 255-(i-255*3-1));
    else if (i>=255*4+1 && i<=255*5 && i%k==0) listColorsForEffect[i/k-1] = CRGB(255, i-255*4-1, 0);
    else if (i>=255*5+1 && i<=255*6 && i%k==0) listColorsForEffect[i/k-1] = CRGB(255-(i-255*5-1), 255, 0);
  
  timerForPointsTime = millis();
  timerForPointsTime2 = 0;
  timerForUpdateTime = millis();
  timerForUpdateTemp = millis();
  timerForTimeSetting = millis();
  timerForTimeSetting2 = 0;
  timerForEffect = millis();
}

void loop() {
  for (byte i = 0; i < 3; i++) btn[i].tick();
  bool btn1click = btn[0].click();
  bool btn2click = btn[1].click();
  bool btn3click = btn[2].click();
  bool btn1hold = btn[0].held();
  bool btn2hold = btn[1].held();
  bool btn3hold = btn[2].held();

  applyEffect();
  
  //выбор режима
  switch (mode) {
    
    case TIME_SHOW:
      if (btn1click) mode = TEMPERATURE_SHOW;
      else if (btn1hold) {
        h = rtc.getHours();
        m = rtc.getMinutes();
        mode = TIME_SETTING_H;
      }
      else if (btn2hold) mode = BRIGHTNESS_SETTING;
      else if (btn3hold) mode = COLOR_SETTING;
      else if (btn2click) {
          if (indexCurrentEffect == numOfEffects) indexCurrentEffect = 0;
          else indexCurrentEffect++;
          EEPROM.update(ADDRESS_EFFECT, indexCurrentEffect);
      }
      break;
      
    case TIME_SETTING_H:
      if (btn1click) mode = TIME_SETTING_M;
      break;
      
    case TIME_SETTING_M:
      if (btn1click) {
        DateTime now1;
        now1.second = 0;
        now1.minute = m;
        now1.hour = h;
        now1.date = 1;
        now1.month = 6;
        now1.year = 2022;
        rtc.setTime(now1);
        mode = TIME_SHOW;
      }
      break;
      
    case BRIGHTNESS_SETTING:
      if (btn1click) {
        EEPROM.update(ADDRESS_BRIGHTNESS, indexCurrentBrightness);
        mode = TIME_SHOW;
      }
      break;
      
    case COLOR_SETTING:
      if (btn1click) {
        EEPROM.update(ADDRESS_COLOR, indexCurrentColor);
        mode = TIME_SHOW;
      }
      break;
      
    case TEMPERATURE_SHOW:
      if (btn1click) mode = TIME_SHOW;
      break;  
  }

  //настройка кнопками в выбранном режиме
  switch (mode) {
      
    case TIME_SETTING_H:
      if (btn3click) h = (h + 1) % 24;
      else if (btn2click) {
        if (h == 0) h = 23;
        else h--;
      }
      break;
      
    case TIME_SETTING_M:
      if (btn3click) m = (m + 1) % 60;
      else if (btn2click) {
        if (m == 0) m = 59;
        else m--;
      }
      break;
      
    case BRIGHTNESS_SETTING:
      if (btn3click) {
        if (indexCurrentBrightness != sizeOfListBrightness - 1) {
          indexCurrentBrightness++;
          FastLED.setBrightness(listBrightness[indexCurrentBrightness]);
        } 
      }
      else if (btn2click) {
        if (indexCurrentBrightness != 0) {
          indexCurrentBrightness--;
          FastLED.setBrightness(listBrightness[indexCurrentBrightness]);
         }
      }   
      break;
      
    case COLOR_SETTING:
      if (btn3click) indexCurrentColor = (indexCurrentColor + 1) % sizeOfListColor;
      else if (btn2click) {
        if (indexCurrentColor == 0) indexCurrentColor = sizeOfListColor - 1;
        else indexCurrentColor--;
      }
      break; 
  }
  
    //отображение режима
    switch (mode) {
      
    case TIME_SHOW:
      if (millis() - timerForUpdateTime > TIME_UPDATE_INTERVAL) {
        rtc.getTimeChar(time);
        timerForUpdateTime = millis();
      }
      showTime();
      break;
      
    case TIME_SETTING_H:
      showTimeSettingH();
      break;
      
    case TIME_SETTING_M:
      showTimeSettingM();
      break;
      
    case BRIGHTNESS_SETTING:
      showBrightnessSetting();
      break;
      
    case COLOR_SETTING:
      showColorSetting();
      break;

    case TEMPERATURE_SHOW:
        if (millis() - timerForUpdateTemp > TEMPERATURE_UPDATE_INTERVAL) {
          temp = getTemp();
          timerForUpdateTemp = millis();
        }
      showTemperature();
      break;  
  }

  FastLED.show();
}

void showTime() {
  displaySymbol(time[0], 3); //первая цифра часа
  displaySymbol(time[1], 2); //вторая цифра часа
  displaySymbol(time[3], 1); //первая цифра минут
  displaySymbol(time[4], 0); //вторая цифра минут

  if(!blickTime)
    if (millis() - timerForPointsTime > BLICK_POINT_INTERVAL_ON) {
      blickTime = true;
      timerForPointsTime2 = millis();
    }
  if(blickTime)
    if (millis() - timerForPointsTime2 > BLICK_POINT_INTERVAL_OFF) {
      blickTime = false;
      timerForPointsTime = millis();
    }
  
  if (blickTime) {
    leds[28] = CRGB::Black;
    leds[29] = CRGB::Black; 
  }
}

void showTimeSettingH() {    
  if (cnt_digits(h) == 2) {
    displaySymbol(String(h)[0], 3);
    displaySymbol(String(h)[1], 2);
  }
  else if (cnt_digits(h) == 1) {
    displaySymbol('0', 3);
    displaySymbol(String(h)[0], 2);
  }
  else {
    displaySymbol('0', 3);
    displaySymbol('0', 2);
  }
  if (cnt_digits(m) == 2) {
    displaySymbol(String(m)[0], 1);
    displaySymbol(String(m)[1], 0);
  }
  else if (cnt_digits(m) == 1) {
    displaySymbol('0', 1);
    displaySymbol(String(m)[0], 0);
  }
  else {
    displaySymbol('0', 1);
    displaySymbol('0', 0);
  }

  if(!blickChangeTime)
    if (millis() - timerForTimeSetting > BLICK_POINT_INTERVAL_CHANGE_ON) {
      blickChangeTime = true;
      timerForTimeSetting2 = millis();
    }
  if(blickChangeTime)
    if (millis() - timerForTimeSetting2 > BLICK_POINT_INTERVAL_CHANGE_OFF) {
      blickChangeTime = false;
      timerForTimeSetting = millis();
    }
    
  if (blickChangeTime) for(byte i = 30; i < 58; i++) leds[i] = CRGB::Black; 
}

void showTimeSettingM() {
  if (cnt_digits(h) == 2) {
    displaySymbol(String(h)[0], 3);
    displaySymbol(String(h)[1], 2);
  }
  else if (cnt_digits(h) == 1) {
    displaySymbol('0', 3);
    displaySymbol(String(h)[0], 2);
  }
  else {
    displaySymbol('0', 3);
    displaySymbol('0', 2);
  }
  if (cnt_digits(m) == 2) {
    displaySymbol(String(m)[0], 1);
    displaySymbol(String(m)[1], 0);
  }
  else if (cnt_digits(m) == 1) {
    displaySymbol('0', 1);
    displaySymbol(String(m)[0], 0);
  }
  else {
    displaySymbol('0', 1);
    displaySymbol('0', 0);
  }

  if(!blickChangeTime)
    if (millis() - timerForTimeSetting > BLICK_POINT_INTERVAL_CHANGE_ON) {
      blickChangeTime = true;
      timerForTimeSetting2 = millis();
    }
  if(blickChangeTime)
    if (millis() - timerForTimeSetting2 > BLICK_POINT_INTERVAL_CHANGE_OFF) {
      blickChangeTime = false;
      timerForTimeSetting = millis();
    }
      
  if (blickChangeTime) for (byte i = 0; i < 28; i++) leds[i] = CRGB::Black; 
}

void showBrightnessSetting() {
  displaySymbol('B', 3);
  displaySymbol('R', 2);
  byte level = indexCurrentBrightness + 1;
  if (cnt_digits(level) == 2) {
    displaySymbol(String(level)[0], 1);
    displaySymbol(String(level)[1], 0);
  }
  else {
    displaySymbol(' ', 1);
    displaySymbol(String(level)[0], 0);
  }
  leds[28] = CRGB::Black;
  leds[29] = CRGB::Black;
}

void showColorSetting() {
  displaySymbol('C', 3);
  displaySymbol('O', 2);
  byte level = indexCurrentColor + 1;
  if (cnt_digits(level) == 2) {
    displaySymbol(String(level)[0], 1);
    displaySymbol(String(level)[1], 0);
  }
  else {
    displaySymbol(' ', 1);
    displaySymbol(String(level)[0], 0);
  }
  leds[28] = CRGB::Black;
  leds[29] = CRGB::Black;
}

void showTemperature() {
  if (temp<0 || temp >99) {
    displaySymbol('-', 3);
    displaySymbol('-', 2);
    displaySymbol('-', 1);
    displaySymbol('-', 0);
  }
  else {
      if (cnt_digits(temp) == 2) {
        displaySymbol(String(temp)[0], 3);
        displaySymbol(String(temp)[1], 2);
      }
      else if (cnt_digits(temp) == 1) {
        displaySymbol(' ', 3);
        displaySymbol(String(temp)[0], 2);
       }
      else {
        displaySymbol(' ', 3);
        displaySymbol('0', 2);
      }

    displaySymbol('g', 1);
    displaySymbol('C', 0);
  }
  leds[28] = CRGB::Black;
  leds[29] = CRGB::Black;
}

void applyEffect() {
  switch (indexCurrentEffect) {
    case 0:
      for (byte i = 0; i < 58; i++) leds[i] = listColor[indexCurrentColor];
      break;
      
    case 1:
        if (millis() - timerForEffect > EFFECT1_INTERVAL) {
          changer--;
          timerForEffect = millis();
        }
        
        for (byte i = 0; i < 58; i++) {
          if (i==48 || i==49 || i==50 || i==51) leds[i] = listColorsForEffect[(0 + changer) % numOfColors];
          if (i==47 || i==57 || i==52) leds[i] = listColorsForEffect[(1 + changer) % numOfColors];
          if (i==46 || i==56 || i==53) leds[i] = listColorsForEffect[(2 + changer) % numOfColors];
          if (i==45 || i==44 || i==55 || i==54) leds[i] = listColorsForEffect[(3 + changer) % numOfColors];

          if (i==34 || i==35 || i==36 || i==37) leds[i] = listColorsForEffect[(4 + changer) % numOfColors];
          if (i==33 || i==43 || i==38) leds[i] = listColorsForEffect[(5 + changer) % numOfColors];
          if (i==32 || i==42 || i==39) leds[i] = listColorsForEffect[(6 + changer) % numOfColors];
          if (i==31 || i==30 || i==41 || i==40) leds[i] = listColorsForEffect[(7 + changer) % numOfColors];

          if (i==29 || i==28) leds[i] = listColorsForEffect[(8 + changer) % numOfColors];

          if (i==18 || i==19 || i==20 || i==21) leds[i] = listColorsForEffect[(9 + changer) % numOfColors];
          if (i==17 || i==27 || i==22) leds[i] = listColorsForEffect[(10 + changer) % numOfColors];
          if (i==16 || i==26 || i==23) leds[i] = listColorsForEffect[(11 + changer) % numOfColors];
          if (i==15 || i==14 || i==25 || i==24) leds[i] = listColorsForEffect[(12 + changer) % numOfColors];

          if (i==4 || i==5 || i==6 || i==7) leds[i] = listColorsForEffect[(13 + changer) % numOfColors];
          if (i==3 || i==13 || i==8) leds[i] = listColorsForEffect[(14 + changer) % numOfColors];
          if (i==2 || i==12 || i==9) leds[i] = listColorsForEffect[(15 + changer) % numOfColors];
          if (i==1 || i==0 || i==11 || i==10) leds[i] = listColorsForEffect[(16 + changer) % numOfColors];
        }
      break;
    case 2:
      if (millis() - timerForEffect > EFFECT2_INTERVAL) {
        changer--;
        timerForEffect = millis();
      }
        
      for (byte i = 0; i < 58; i++) {
        if (i==52 || i==53 || i==38 || i==39 || i==22 || i==23 || i==8 || i==9) leds[i] = listColorsForEffect[(0 + changer) % numOfColors];
        if (i==51 || i==54 || i==37 || i==40 || i==21 || i==24 || i==7 || i==10) leds[i] = listColorsForEffect[(1 + changer) % numOfColors];
        if (i==50 || i==55 || i==36 || i==41 || i==28 || i==20 || i==25 || i==6 || i==11) leds[i] = listColorsForEffect[(2 + changer) % numOfColors];
        if (i==57 || i==56 || i==43 || i==42 || i==27 || i==26 || i==13 || i==12) leds[i] = listColorsForEffect[(3 + changer) % numOfColors];
        if (i==49 || i==44 || i==35 || i==30 || i==29 || i==19 || i==14 || i==5 || i==0) leds[i] = listColorsForEffect[(4 + changer) % numOfColors];
        if (i==48 || i==45 || i==34 || i==31 || i==18 || i==15 || i==4 || i==1) leds[i] = listColorsForEffect[(5 + changer) % numOfColors];
        if (i==47 || i==46 || i==33 || i==32 || i==17 || i==16 || i==3 || i==2) leds[i] = listColorsForEffect[(6 + changer) % numOfColors];
      }
      break;
     case 3:
       if (millis() - timerForEffect > EFFECT3_INTERVAL) {
         fillRandomColors();
         timerForEffect = millis();
       }
        for (byte i = 0; i < 58; i++) leds[i] = randomColors[i];
        break;
   }
}

//зануляет сегментно массив leds, на вход треубемый символ и номер сегмента
void displaySymbol(char symbol, byte segment) {
  
  // сегменты слева направо: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = 14;
      break;
    case 2:
      startindex = 30;
      break;
    case 3:
      startindex = 44;
      break;    
  }

/*
Индексы светодиодов:
    47 46        33 32            17 16        3  2
  48     45    34     31        18     15    4      1
  49     44    35     30   29   19     14    5      0
    57 56        43 42            27 26       13 12
  50     55    36     41   28   20     25    6     11
  51     54    37     40        21     24    7     10 
    52 53        38 39            22 23        8  9
    
*/

unsigned int number;
switch (symbol) {
    case 'C':
      number = 0b00001111111100;
      break;
    case 'O':
      number = 0b00111111111111;
      break;
    case 'B':
      number = 0b11111111110000;
      break;
    case 'R':
      number = 0b11000011000000;
      break;
    case 'g':
      number = 0b11000000111111;
      break;
    case '0':
      number = 0b00111111111111;
      break;
    case '1':
      number = 0b00110000000011;
      break;
    case '2':
      number = 0b11001111001111;
      break;
    case '3':
      number = 0b11111100001111;
      break;
    case '4':
      number = 0b11110000110011;
      break;
    case '5':
      number = 0b11111100111100;
      break;
    case '6':
      number = 0b11111111111100;
      break;
    case '7':
      number = 0b00110000001111;
      break;
    case '8':
      number = 0b11111111111111;
      break;
    case '9':
      number = 0b11111100111111;
      break;
    case ' ':
      number = 0b00000000000000;
      break;
    case '-':
      number = 0b11000000000000;
      break;
  }

  for (byte i = 0; i < 14; i++)
    if ( (number & 1 << i) != 1 << i )
      leds[i + startindex] = CRGB::Black;
}

// подсчёт количества цифр числа, если число <= 0, то return 0
byte cnt_digits(byte num) {
  byte k = 0;
  while (num > 0)
    {
        num /= 10;
        k++;
    }
    return k;
}

byte getTemp() { 
  int Vo;
  float R1 = 10000; // значение R1 на модуле
  float logR2, R2, T;
  float c1 = 0.001129148, c2 = 0.000234125, c3 = 0.0000000876741;
  Vo = analogRead(TEMP_PIN);
  R2 = R1 * (1023.0 / (float)Vo - 1.0); //вычислите сопротивление на термисторе
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // температура в Кельвине
  T = T - 273.15; //преобразование Кельвина в Цельсия
  return round(T);
}

void fillRandomColors() {
  for (byte i = 0; i < 58; i++) {
    byte r = 0; byte g = 0; byte b = 0;
    while (r==0 && g==0 && b==0) {
      r = random8();
      g = random8();
      b = random8();
    }
    randomColors[i] = CRGB(r, g, b);
  }
}
