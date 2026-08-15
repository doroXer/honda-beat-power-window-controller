/*
  pws_v0.0
  Honda Beat power window controller
  Honda Beat パワーウインドウコントローラー

  Target / 対象:
    Arduino Pro Mini 5V / 16MHz

  Main functions / 主な機能:
    - Manual UP/DOWN control for right and left power windows
      左右パワーウインドウの手動UP/DOWN制御
    - Both-window auto UP/DOWN by simultaneous switch operation
      左右スイッチ同時操作による両窓オートUP/DOWN
    - Current-based auto stop using ACS712 20A sensors
      ACS712 20A電流センサーを用いた電流ベースのオートストップ
    - Auto stop by "current threshold + flatness" detection
      「電流閾値＋平坦化」判定によるオートストップ
    - Door-open partial down and door-close return up
      ドア開時パーシャルダウン、ドア閉時復帰UP
    - Key-off power hold and EEPROM state retention
      キーOFF後の電源保持とEEPROM状態保持

  Notes / 注記:
    This sketch assumes MOSFET H-bridge motor drivers.
    本スケッチはMOSFET Hブリッジモータードライバを前提とする。
    Motor stop output is LOW / LOW.
    モーター停止出力はLOW / LOW。
*/

#include <EEPROM.h>

// ------------------------------------------------------------
// Pin assignment / ピン割り当て
// ------------------------------------------------------------
const byte PIN_R_UP_OUT=2, PIN_R_DOWN_OUT=3, PIN_L_UP_OUT=4, PIN_L_DOWN_OUT=5;
const byte PIN_R_DOOR=6, PIN_L_DOOR=7;
const byte PIN_R_UP_SW=8, PIN_R_DOWN_SW=9, PIN_L_UP_SW=10, PIN_L_DOWN_SW=11;
const byte PIN_IG_ACC=12, PIN_POWER_HOLD=13;
const byte PIN_R_CURRENT=A0, PIN_L_CURRENT=A1;

// Configuration / 設定
const bool DEBUG_SERIAL=false;
const unsigned long AUTO_HOLD_MS=700, AUTO_TIMEOUT_MS=7000;
const unsigned long POWER_HOLD_MS=180000, POWER_OFF_SETTLE_MS=50;
const unsigned long PARTIAL_DOWN_MS=300;
const unsigned long SWITCH_DEBOUNCE_MS=10, DOOR_DEBOUNCE_MS=100, BOTH_SW_DEBOUNCE_MS=100, SETUP_WAIT_MS=100;
const int CURRENT_THRESHOLD_ADC=700;
const unsigned long MOTOR_START_IGNORE_MS=300, CURRENT_SAMPLE_INTERVAL_MS=20;
const byte FLAT_SAMPLE_COUNT=5;
const int FLAT_DELTA_ADC=15;
const unsigned long AUTO_STOP_PUSH_R_MS=50, AUTO_STOP_PUSH_L_MS=50;
const int EEPROM_ADDR_SWR=0x20, EEPROM_ADDR_SWL=0x21, EEPROM_ADDR_END_R=0x22, EEPROM_ADDR_END_L=0x23;
const byte SW_UP=1, SW_DOWN=2, END_REACHED=0, END_UNKNOWN=1;
const int DIR_STOP=0, DIR_UP=1, DIR_DOWN=-1;

int currentR=0,currentL=0;
unsigned long timerUpR=0,timerDownR=0,timerUpL=0,timerDownL=0,powerHoldTime=0;
bool doorPartialR=false,doorPartialL=false;
byte endStateR=END_UNKNOWN,endStateL=END_UNKNOWN,lastSwitchR=SW_DOWN,lastSwitchL=SW_DOWN;
bool motorWasRunningR=false,motorWasRunningL=false;
int lastMotorDirR=DIR_STOP,lastMotorDirL=DIR_STOP;
unsigned long motorStartTimeR=0,motorStartTimeL=0,lastCurrentSampleTimeR=0,lastCurrentSampleTimeL=0;
int currentWindowR[FLAT_SAMPLE_COUNT],currentWindowL[FLAT_SAMPLE_COUNT];
byte currentWindowIndexR=0,currentWindowIndexL=0,currentWindowCountR=0,currentWindowCountL=0;

void setup(){
 pinMode(PIN_R_UP_OUT,OUTPUT);pinMode(PIN_R_DOWN_OUT,OUTPUT);pinMode(PIN_L_UP_OUT,OUTPUT);pinMode(PIN_L_DOWN_OUT,OUTPUT);
 pinMode(PIN_R_DOOR,INPUT_PULLUP);pinMode(PIN_L_DOOR,INPUT_PULLUP);
 pinMode(PIN_R_UP_SW,INPUT_PULLUP);pinMode(PIN_R_DOWN_SW,INPUT_PULLUP);pinMode(PIN_L_UP_SW,INPUT_PULLUP);pinMode(PIN_L_DOWN_SW,INPUT_PULLUP);
 pinMode(PIN_IG_ACC,INPUT);pinMode(PIN_POWER_HOLD,OUTPUT);stopRight();stopLeft();digitalWrite(PIN_POWER_HOLD,LOW);
 if(DEBUG_SERIAL)Serial.begin(115200);loadWindowState();normalizeWindowState();delay(SETUP_WAIT_MS);
}
void loop(){handlePowerHold();readCurrentAndPrintDebug();handleSwitchR();handleSwitchL();handleBothAutoUp();handleBothAutoDown();handleAutoStopR();handleAutoStopL();handleDoorPartialR();handleDoorPartialL();}

void loadWindowState(){EEPROM.get(EEPROM_ADDR_SWR,lastSwitchR);EEPROM.get(EEPROM_ADDR_SWL,lastSwitchL);EEPROM.get(EEPROM_ADDR_END_R,endStateR);EEPROM.get(EEPROM_ADDR_END_L,endStateL);}
void saveWindowState(){EEPROM.put(EEPROM_ADDR_SWR,lastSwitchR);EEPROM.put(EEPROM_ADDR_SWL,lastSwitchL);EEPROM.put(EEPROM_ADDR_END_R,endStateR);EEPROM.put(EEPROM_ADDR_END_L,endStateL);}
void normalizeWindowState(){if(lastSwitchR!=SW_UP&&lastSwitchR!=SW_DOWN)lastSwitchR=SW_DOWN;if(lastSwitchL!=SW_UP&&lastSwitchL!=SW_DOWN)lastSwitchL=SW_DOWN;if(endStateR!=END_REACHED&&endStateR!=END_UNKNOWN)endStateR=END_UNKNOWN;if(endStateL!=END_REACHED&&endStateL!=END_UNKNOWN)endStateL=END_UNKNOWN;}

void handlePowerHold(){if(digitalRead(PIN_IG_ACC)==HIGH){digitalWrite(PIN_POWER_HOLD,HIGH);powerHoldTime=millis();}else if(digitalRead(PIN_R_DOOR)==LOW){delay(DOOR_DEBOUNCE_MS);digitalWrite(PIN_POWER_HOLD,HIGH);powerHoldTime=millis();}else if(digitalRead(PIN_L_DOOR)==LOW){delay(DOOR_DEBOUNCE_MS);digitalWrite(PIN_POWER_HOLD,HIGH);powerHoldTime=millis();}else if((millis()-powerHoldTime)>POWER_HOLD_MS){saveWindowState();stopRight();stopLeft();delay(POWER_OFF_SETTLE_MS);digitalWrite(PIN_POWER_HOLD,LOW);}}
void readCurrentAndPrintDebug(){currentR=analogRead(PIN_R_CURRENT);currentL=analogRead(PIN_L_CURRENT);if(!DEBUG_SERIAL)return;Serial.print(currentR);Serial.print(',');Serial.print(currentL);Serial.print(',');Serial.print(0);Serial.print(',');Serial.print(doorPartialR);Serial.print(',');Serial.print(doorPartialL);Serial.print(',');Serial.print(endStateR);Serial.print(',');Serial.print(endStateL);Serial.print(',');Serial.println(1000);}

void handleSwitchR(){if(digitalRead(PIN_R_UP_SW)==LOW){lastSwitchR=SW_UP;upRight();delay(SWITCH_DEBOUNCE_MS);timerDownR=millis();}else if(digitalRead(PIN_R_DOWN_SW)==LOW){lastSwitchR=SW_DOWN;downRight();delay(SWITCH_DEBOUNCE_MS);timerUpR=millis();endStateR=END_UNKNOWN;}else if((millis()-timerUpR)>AUTO_HOLD_MS){timerDownR=millis();if((millis()-timerUpR)>AUTO_TIMEOUT_MS){timerUpR=millis();timerDownR=millis();stopRight();}}else if((millis()-timerDownR)>AUTO_HOLD_MS){timerUpR=millis();if((millis()-timerDownR)>AUTO_TIMEOUT_MS){timerUpR=millis();timerDownR=millis();stopRight();}}else{timerUpR=millis();timerDownR=millis();stopRight();}}
void handleSwitchL(){if(digitalRead(PIN_L_UP_SW)==LOW){lastSwitchL=SW_UP;upLeft();delay(SWITCH_DEBOUNCE_MS);timerDownL=millis();}else if(digitalRead(PIN_L_DOWN_SW)==LOW){lastSwitchL=SW_DOWN;downLeft();delay(SWITCH_DEBOUNCE_MS);timerUpL=millis();endStateL=END_UNKNOWN;}else if((millis()-timerUpL)>AUTO_HOLD_MS){timerDownL=millis();if((millis()-timerUpL)>AUTO_TIMEOUT_MS){timerUpL=millis();timerDownL=millis();stopLeft();}}else if((millis()-timerDownL)>AUTO_HOLD_MS){timerUpL=millis();if((millis()-timerDownL)>AUTO_TIMEOUT_MS){timerUpL=millis();timerDownL=millis();stopLeft();}}else{timerUpL=millis();timerDownL=millis();stopLeft();}}
void handleBothAutoUp(){if(digitalRead(PIN_R_UP_SW)==LOW&&digitalRead(PIN_L_UP_SW)==LOW){lastSwitchR=SW_UP;lastSwitchL=SW_UP;upRight();upLeft();timerUpR=millis()-1000;timerUpL=millis()-1000;timerDownR=millis();timerDownL=millis();delay(BOTH_SW_DEBOUNCE_MS);}}
void handleBothAutoDown(){if(digitalRead(PIN_R_DOWN_SW)==LOW&&digitalRead(PIN_L_DOWN_SW)==LOW){lastSwitchR=SW_DOWN;lastSwitchL=SW_DOWN;downRight();downLeft();timerUpR=millis();timerUpL=millis();timerDownR=millis()-1000;timerDownL=millis()-1000;endStateR=END_UNKNOWN;endStateL=END_UNKNOWN;delay(BOTH_SW_DEBOUNCE_MS);}}

void resetCurrentWindowR(){currentWindowIndexR=0;currentWindowCountR=0;}void resetCurrentWindowL(){currentWindowIndexL=0;currentWindowCountL=0;}
bool addCurrentSampleAndIsFlat(int v,int b[],byte&i,byte&c){b[i]=v;i++;if(i>=FLAT_SAMPLE_COUNT)i=0;if(c<FLAT_SAMPLE_COUNT)c++;if(c<FLAT_SAMPLE_COUNT)return false;int mn=b[0],mx=b[0];for(byte n=1;n<FLAT_SAMPLE_COUNT;n++){if(b[n]<mn)mn=b[n];if(b[n]>mx)mx=b[n];}return(mx-mn)<=FLAT_DELTA_ADC;}
void handleAutoStopR(){bool d=digitalRead(PIN_R_UP_OUT)||digitalRead(PIN_R_DOWN_OUT);int dir=DIR_STOP;if(digitalRead(PIN_R_UP_OUT)&&!digitalRead(PIN_R_DOWN_OUT))dir=DIR_UP;else if(digitalRead(PIN_R_DOWN_OUT)&&!digitalRead(PIN_R_UP_OUT))dir=DIR_DOWN;if(!d){motorWasRunningR=false;lastMotorDirR=DIR_STOP;resetCurrentWindowR();return;}if(!motorWasRunningR||dir!=lastMotorDirR){motorStartTimeR=millis();lastCurrentSampleTimeR=0;resetCurrentWindowR();motorWasRunningR=true;lastMotorDirR=dir;}currentR=analogRead(PIN_R_CURRENT);if(millis()-motorStartTimeR<MOTOR_START_IGNORE_MS){resetCurrentWindowR();return;}if(millis()-lastCurrentSampleTimeR<CURRENT_SAMPLE_INTERVAL_MS)return;lastCurrentSampleTimeR=millis();if(currentR<=CURRENT_THRESHOLD_ADC){resetCurrentWindowR();return;}if(addCurrentSampleAndIsFlat(currentR,currentWindowR,currentWindowIndexR,currentWindowCountR)){if(AUTO_STOP_PUSH_R_MS)delay(AUTO_STOP_PUSH_R_MS);stopRight();timerUpR=millis();timerDownR=millis();endStateR=END_REACHED;resetCurrentWindowR();}}
void handleAutoStopL(){bool d=digitalRead(PIN_L_UP_OUT)||digitalRead(PIN_L_DOWN_OUT);int dir=DIR_STOP;if(digitalRead(PIN_L_UP_OUT)&&!digitalRead(PIN_L_DOWN_OUT))dir=DIR_UP;else if(digitalRead(PIN_L_DOWN_OUT)&&!digitalRead(PIN_L_UP_OUT))dir=DIR_DOWN;if(!d){motorWasRunningL=false;lastMotorDirL=DIR_STOP;resetCurrentWindowL();return;}if(!motorWasRunningL||dir!=lastMotorDirL){motorStartTimeL=millis();lastCurrentSampleTimeL=0;resetCurrentWindowL();motorWasRunningL=true;lastMotorDirL=dir;}currentL=analogRead(PIN_L_CURRENT);if(millis()-motorStartTimeL<MOTOR_START_IGNORE_MS){resetCurrentWindowL();return;}if(millis()-lastCurrentSampleTimeL<CURRENT_SAMPLE_INTERVAL_MS)return;lastCurrentSampleTimeL=millis();if(currentL<=CURRENT_THRESHOLD_ADC){resetCurrentWindowL();return;}if(addCurrentSampleAndIsFlat(currentL,currentWindowL,currentWindowIndexL,currentWindowCountL)){if(AUTO_STOP_PUSH_L_MS)delay(AUTO_STOP_PUSH_L_MS);stopLeft();timerUpL=millis();timerDownL=millis();endStateL=END_REACHED;resetCurrentWindowL();}}

void handleDoorPartialR(){if(digitalRead(PIN_R_DOOR)==LOW){delay(DOOR_DEBOUNCE_MS);if(!doorPartialR&&endStateR==END_REACHED&&lastSwitchR==SW_UP){downRight();delay(PARTIAL_DOWN_MS);stopRight();endStateR=END_UNKNOWN;doorPartialR=true;}}else if(doorPartialR){doorPartialR=false;upRight();currentR=analogRead(PIN_R_CURRENT);timerUpR=millis()-1000;timerDownR=millis();}}
void handleDoorPartialL(){if(digitalRead(PIN_L_DOOR)==LOW){delay(DOOR_DEBOUNCE_MS);if(!doorPartialL&&endStateL==END_REACHED&&lastSwitchL==SW_UP){downLeft();delay(PARTIAL_DOWN_MS);stopLeft();endStateL=END_UNKNOWN;doorPartialL=true;}}else if(doorPartialL){doorPartialL=false;upLeft();currentL=analogRead(PIN_L_CURRENT);timerUpL=millis()-1000;timerDownL=millis();}}
void upRight(){digitalWrite(PIN_R_UP_OUT,HIGH);digitalWrite(PIN_R_DOWN_OUT,LOW);}void downRight(){digitalWrite(PIN_R_DOWN_OUT,HIGH);digitalWrite(PIN_R_UP_OUT,LOW);}void stopRight(){digitalWrite(PIN_R_UP_OUT,LOW);digitalWrite(PIN_R_DOWN_OUT,LOW);}void upLeft(){digitalWrite(PIN_L_UP_OUT,HIGH);digitalWrite(PIN_L_DOWN_OUT,LOW);}void downLeft(){digitalWrite(PIN_L_DOWN_OUT,HIGH);digitalWrite(PIN_L_UP_OUT,LOW);}void stopLeft(){digitalWrite(PIN_L_UP_OUT,LOW);digitalWrite(PIN_L_DOWN_OUT,LOW);}
