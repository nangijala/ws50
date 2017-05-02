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

  stopTimer(){
    if( timerId > -1)
      timer.stop( timerId );
    timerId = -1;
    
  }
  
  restartTimer(){
    stopTimer();
    timerId = timer.after(5000, overTime);
  }
  
  add( int number){
    if( index < MAX_BUFFER ){
      restartTimer();
      numbers[ index++ ] = number;
      return true;      
    }
    return false;
  }
  
  reset(){
    stopTimer();    
    index = 0;
    for(int i=0; i< MAX_BUFFER; i++){
      numbers[i] = -1;
    }
  }

  checkIsCorrectNumber(){
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

  checkIsWrongNumber(){
    if( !isDialing())
      return false;
      
    for(int i=0; i< index; i++){
      /*
      Serial.print("Vergleiche ");
      Serial.print( i );      
      Serial.print( numbers[i] );
      Serial.print( waitFor[i] );
      */
      if( i > NUMLEN || numbers[i] != waitFor[i]){
        reset();
        return true;
      }
    }
    return false;   
  }

  isDialing(){
    return index>0 ? true : false;
  }
    
};

NumberTracker tracker = NumberTracker();

RotaryDialer dialer = RotaryDialer(PIN_READY, PIN_PULSE);

#define BUTTON_DEBOUNCE_PERIOD 20 //ms

Bounce b_Gabel  = Bounce();
Bounce b_Ready  = Bounce();

void setup() {
  Serial.begin(38400);
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

  // Get the updated value :
  int gabeValue = b_Gabel.read();
    
  if( b_Gabel.fell()){
     stack.push( String("welcome.mp3") );
  }
  if( b_Gabel.rose()){
     Serial.println("Aufgelegt");
    tracker.reset();     
  }

  
  if (dialer.update()) {  
    int theNumber = dialer.getNextNumber();       
    String msg = "zahl" + String(theNumber) + ".mp3";
    stack.push( msg );
    tracker.add(theNumber);
  }

  if( tracker.checkIsCorrectNumber() == true){
    Serial.println( "Korrekt");
  }

  if( tracker.checkIsWrongNumber() ){
    Serial.println( "Falsch Verbunden");
  }
 
  if( stack.count() > 0 && bTimerRunning == false ){
    String a = stack.peek();
    timer.after( 300, nextSong);
    bTimerRunning = true;
   }
   
}

void overTime(){   
   Serial.println( "Abgelaufen");
   tracker.reset();
}
  
void nextSong(){
   if( stack.count() > 0){
    String song = stack.pop();
    MP3player.playMP3(song.c_str());
   }
   bTimerRunning = false;
 }
 
