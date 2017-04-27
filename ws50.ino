#include <Event.h>
#include <Timer.h>



#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>
#include <StackArray.h>

#include <RotaryDialer.h>
#include <Bounce2.h> 

#define PIN_READY A0
#define PIN_PULSE A1
#define PIN_GABEL A2

Timer timer;

SdFat sd;

SFEMP3Shield MP3player;

class Msg{
  
  public:
    Msg(int delay=300){
    timeout = delay;
  };
  
  int timeout;
  String name;
};

StackArray <String> stack;


RotaryDialer dialer = RotaryDialer(PIN_READY, PIN_PULSE);

#define BUTTON_DEBOUNCE_PERIOD 20 //ms

Bounce b_Gabel  = Bounce();

void setup() {
//  Serial.begin(38400);
  dialer.setup();

  if(!sd.begin(9, SPI_HALF_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  MP3player.begin();
  MP3player.setVolume(0,0);

  pinMode(PIN_GABEL, INPUT_PULLUP);
  b_Gabel.attach(PIN_GABEL);
  b_Gabel.interval(BUTTON_DEBOUNCE_PERIOD);  
}

boolean bFlag = false;
boolean bTimerRunning = false;
void loop() {
  timer.update();
  
  b_Gabel.update();
  if( b_Gabel.fell()){
    if( bFlag == false){  
     stack.push( String("welcome.mp3") );
     bFlag = true;
    }    
  }
  if( b_Gabel.rose()){
    bFlag = false;
  }
  
  if (dialer.update()) {
    // Serial.println(dialer.getNextNumber());
    
    // char fileName[10];
    // sprintf(fileName, "zahl%d.mp3",dialer.getNextNumber()) ;
    
    String msg = "zahl" + String(dialer.getNextNumber()) + ".mp3";
    
     // MP3player.playMP3(msg.c_str());
     stack.push( msg );
  }

  if( stack.count() > 0 && bTimerRunning == false ){
    // MP3player.playMP3(stack.pop().c_str());
    stack.peek();
    timer.after( 300, nextSong);
    bTimerRunning = true;
   }
}


void nextSong(){
    if( stack.count() > 0){
    MP3player.playMP3(stack.pop().c_str());
   }
   bTimerRunning = false;
 }
