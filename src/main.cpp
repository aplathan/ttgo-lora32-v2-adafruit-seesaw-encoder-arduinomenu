/********************
September 2023 Antti Plathan based on M. Smit - github@gangkast.nl

Aarduinomenu running in TTGO LORA32 V2 ESP32 board
output: SSD1306 OLED
input: Adafruit I2C Stemma QT Rotary Encoder Breakout with NeoPixel
  
***/

#include <menu.h>                 // https://github.com/neu-rah/ArduinoMenu
#include <menuIO/u8g2Out.h>
#include <menuIO/chainStream.h>
#include <menuIO/rotaryEventIn.h>
#include "Adafruit_seesaw.h"
#include <AceButton.h>            // https://github.com/bxparks/AceButton 

#define SS_SWITCH       24 // This is seesaw chip internal pin for push button on the encoder board, not on Arduino
#define SS_ADDR         0x36 // Default address for Adafruit I2C Stemma QT Rotary Encoder Breakout with NeoPixel

Adafruit_seesaw ss; // Ada Seesaw constructor
int32_t encoder_position; // Encoder initial position
//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 

const int ROTARY_PIN_BUT  = 12;

using namespace ::ace_button;
AceButton button(ROTARY_PIN_BUT); // button part

// Display
// LOLIN32 I2C SSD1306 128x64 display
// https://github.com/olikraus/u8g2
#define SDA 21 // TTGO LORA32 V2 I2C data pin
#define SCL 22 // TTGO LORA32 V2 I2C clock pin

#include <Wire.h>
#define fontName u8g2_font_7x13_mf
#define fontX 7
#define fontY 16
#define offsetX 0
#define offsetY 3
#define U8_Width 128
#define U8_Height 64
#define USE_HWI2C
#define fontMarginX 2
#define fontMarginY 2

// OLED driver constructor
U8G2_SSD1306_128X64_VCOMH0_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);//allow contrast change

const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{1,1},{1,0,0}},//titleColor
};

// ArduinoMenu 
// https://github.com/neu-rah/ArduinoMenu
#define MAX_DEPTH 1

unsigned int timeOn=10;
unsigned int timeOff=90;

using namespace Menu;
MENU(mainMenu, "Blink menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,FIELD(timeOn,"On","ms",0,1000,10,1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,FIELD(timeOff,"Off","ms",0,10000,10,1,Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,EXIT("<Back")
);

RotaryEventIn reIn(
  RotaryEventIn::EventType::BUTTON_CLICKED | // select
  RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED | // back
  RotaryEventIn::EventType::BUTTON_LONG_PRESSED | // also back
  RotaryEventIn::EventType::ROTARY_CCW | // up
  RotaryEventIn::EventType::ROTARY_CW // down
); // register capabilities, see AndroidMenu MenuIO/RotaryEventIn.h file
MENU_INPUTS(in,&reIn);

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
  ,NONE
);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);


// This is the handler/callback for button events
// We will convert/relay events to the RotaryEventIn object
// Callback config in setup()
void handleButtonEvent(AceButton* /* button */, uint8_t eventType, uint8_t buttonState) {

  switch (eventType) {
    case AceButton::kEventClicked:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
      break;
    case AceButton::kEventDoubleClicked:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_DOUBLE_CLICKED);
      break;
    case AceButton::kEventLongPressed:
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_LONG_PRESSED);
      break;
  }
}


void setup() {
  Serial.begin(115200);
  while(!Serial);

  Serial.println("Looking for seesaw!");
  
  if (!ss.begin(SS_ADDR)) {
    Serial.println("Couldn't find seesaw on default address");
    while(1) delay(10);
  }
  Serial.println("seesaw started");

  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 4991){
    Serial.print("Wrong firmware loaded?");
    Serial.println(version);
    while(1) delay(10);
  }
  Serial.println("Found Product 4991");

  // use a pin for the built in encoder switch
  ss.pinMode(SS_SWITCH, INPUT_PULLUP);

  // get starting position
  encoder_position = -ss.getEncoderPosition();

  Serial.println("Turning on interrupts");
  delay(10);
  ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  ss.enableEncoderInterrupt();

// setup rotary button
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleButtonEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  // setup OLED disaply
  Wire.begin(SDA,SCL);
  u8g2.begin();
  u8g2.setFont(fontName);

  do {
    u8g2.drawStr(0,fontY,"RotaryEventIn demo");
  } while(u8g2.nextPage());
  // appear
  for(int c=255;c>0;c--) {
    u8g2.setContrast(255-255.0*log(c)/log(255));
    delay(12);
  }
    
}

void loop() {
  button_time = millis();
  if (button_time - last_button_time > 150) {
    if (!ss.digitalRead(SS_SWITCH)) {
      Serial.println("Button pressed!");
      reIn.registerEvent(RotaryEventIn::EventType::BUTTON_CLICKED);
    }
    last_button_time = button_time;
  }

  button.check(); // acebutton check

  int32_t new_position = -ss.getEncoderPosition();
  // did we move around?
  if (encoder_position != new_position) {
    if (encoder_position < new_position) { 
      Serial.println(new_position);
      Serial.println("Myötäpäivään");
      reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CW);
    }
    else if (encoder_position > new_position) {
      Serial.println(new_position);
      Serial.println("Vastapäivään");
      reIn.registerEvent(RotaryEventIn::EventType::ROTARY_CCW);
    }
    encoder_position = new_position;
  }

  nav.doInput(); // menu check

  if (nav.changed(0)) { //only draw if menu changed for gfx device
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }

}