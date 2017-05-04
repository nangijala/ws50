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

#define LOGGING 

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
  
  boolean add( int number){
    if( index < MAX_BUFFER ){
      restartTimer();
      numbers[ index++ ] = number;
      return true;      
    }
    return false;
  }
  
  void reset(){
    Serial.println("Reset");
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

int gabelValue = LOW;

NumberTracker tracker = NumberTracker();

RotaryDialer dialer = RotaryDialer(PIN_READY, PIN_PULSE);

#define BUTTON_DEBOUNCE_PERIOD 20 //ms

Bounce b_Gabel  = Bounce();

void keepAlive(){
 if( stack.isEmpty()){
  if( gabelValue )
    stack.push( String("ring.mp3") );
   else
    stack.push( String("rauschen.mp3") );
 }
}

void setup() {
 #ifdef LOGGING
  Serial.begin(38400);
 #endif
  dialer.setup();

  if(!sd.begin(9, SPI_HALF_SPEED)) sd.initErrorHalt();
  if (!sd.chdir("/")) sd.errorHalt("sd.chdir");

  MP3player.begin();
  MP3player.setVolume(LAUTSTAERKE,LAUTSTAERKE);

  pinMode(PIN_GABEL, INPUT_PULLUP);
  b_Gabel.attach(PIN_GABEL);
  b_Gabel.interval(BUTTON_DEBOUNCE_PERIOD); 
  timer.every(10000, keepAlive);
  timer.every(300, nextSong);
}

boolean bFlag = false;

void loop() {
  timer.update();
  
  b_Gabel.update();

  // Get the updated value :
  gabelValue = b_Gabel.read();
    
  if( b_Gabel.fell()){
     MP3player.stopTrack();    
     stack.push( String("hallo.mp3") );
  }
  
  if( b_Gabel.rose()){
#ifdef LOGGING     
     Serial.println("Aufgelegt");   
#endif     
    MP3player.stopTrack();    
    if( tracker.isDialing())   
      tracker.reset();     
  }

  
  if (dialer.update()) {  
    int theNumber = dialer.getNextNumber();       

#ifdef LOGGING    
    Serial.println("Gewaehlt: "+ String(theNumber) );
#endif
    tracker.add(theNumber);
  }

  if( tracker.checkIsCorrectNumber() == true){
#ifdef LOGGING    
    Serial.println( "Korrekt");
#endif
    stack.push( String("korrekt.mp3") );
    
  }

  if( tracker.checkIsWrongNumber() ){
#ifdef LOGGING        
    //Serial.println( "Falsch Verbunden");
#endif
    stack.push( String("falsch.mp3") );
    
  }
 
   
}

void overTime(){   
#ifdef LOGGING  
   Serial.println( "Abgelaufen");
#endif   
   stack.push( String("timeout.mp3") );
   tracker.reset();
}
  
void nextSong(){
   if( MP3player.isPlaying())
    return;
    
#ifdef LOGGING  
   Serial.println( "Stacksize" + String(stack.count()));
#endif   
    
   if( stack.count() > 0){
      String song = stack.pop();
#ifdef LOGGING  
   Serial.println( "Now Playing " + song);
#endif      
      MP3player.playMP3((char*)song.c_str());
   }
}
 
