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

#undef LOGGING

#define LAUTSTAERKE 30

Timer timer;

SdFat sd;
SFEMP3Shield MP3player;

StackArray <String> stack;

void overTime();

#define NUMLEN  3
#define MAX_BUFFER  10

class NumberTracker{

  int waitFor[NUMLEN+1] = {1,1,8};
  int numbers[MAX_BUFFER+1];
  int index = 0;
  int timerId = -1;
  
  public:
  
  NumberTracker(){
   reset();
  }

  void stopTimer(){
    if( timerId > -1)
      timer.stop( timerId );
    timerId = -1;
    
  }
  
  void restartTimer(){
    stopTimer();
    timerId = timer.after(5000, overTime);
  }
  
  bool add( int number){
    if( index < MAX_BUFFER ){
      restartTimer();
      numbers[ index++ ] = number;
      return true;      
    }
    return false;
  }
  
  void reset(){
    stopTimer();    
    index = 0;
    for(int i=0; i< MAX_BUFFER; i++){
      numbers[i] = -1;
    }
  }

  bool checkIsCorrectNumber(){
    for(int i=0; i< NUMLEN; i++){      
      if( i > index){
        return false;
      }
        
      if( numbers[i] != waitFor[i])
        return false;
    }
    reset();
    return true;
  }

  bool checkIsWrongNumber(){
    if( !isDialing())
      return false;
      
    if( index < NUMLEN)
      return false;
        
    for(int i=0; i< index; i++){
      if( i > NUMLEN || numbers[i] != waitFor[i]){
        reset();
        return true;
      }
    }
    return false;   
  }

  bool isDialing(){
    return index>0 ? true : false;
  }
    
};

NumberTracker tracker = NumberTracker();

RotaryDialer dialer = RotaryDialer(PIN_READY, PIN_PULSE);

#define BUTTON_DEBOUNCE_PERIOD 20 //ms

Bounce b_Gabel  = Bounce();
Bounce b_Ready  = Bounce();

void setup() {

  dialer.setup();

  if(!sd.begin(9, SPI_HALF_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  MP3player.begin();
  MP3player.setVolume(LAUTSTAERKE,LAUTSTAERKE);

  pinMode(PIN_GABEL, INPUT_PULLUP);
  b_Gabel.attach(PIN_GABEL);
  b_Gabel.interval(BUTTON_DEBOUNCE_PERIOD); 
 
}

boolean bFlag = false;
boolean bTimerRunning = false;

void loop() {
  timer.update();
  
  b_Gabel.update();

  // Get the updated value :
  int gabeValue = b_Gabel.read();
    
  if( b_Gabel.fell()){
     stack.push( String("hallo.mp3") );
  }
  
  if( b_Gabel.rose()){
    tracker.reset();     
  }
  
  if (dialer.update()) {  
    int theNumber = dialer.getNextNumber();       

#ifdef LOGGING    
    String msg = "zahl" + String(theNumber) + ".mp3";
    stack.push( msg )
#endif

    tracker.add(theNumber);
  }

  if( tracker.checkIsCorrectNumber() == true){
    stack.push( String("korrekt.mp3") );    
  }

  if( tracker.checkIsWrongNumber() ){
    stack.push( String("falsch.mp3") );    
  }
 
  if( stack.count() > 0 && bTimerRunning == false ){
    timer.after( 300, nextSong);
    bTimerRunning = true;
   }
   
}

void overTime(){   
   stack.push( String("timeout.mp3") );
   tracker.reset();
}
  
void nextSong(){
   if( stack.count() > 0){
      String song = stack.pop();
      MP3player.playMP3((char*)song.c_str());
   }
   bTimerRunning = false;
}
 
