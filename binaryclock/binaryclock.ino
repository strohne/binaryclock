/*
 * Binary Clock with ATtiny 44
 * 
 * Jakob Jünger (jakobjuenger@freenet.de)
 * 
 * Programming ATTiny 44: https://42bots.com/tutorials/programming-attiny84-attiny44-with-arduino-uno/
 * Pinout: see link above 
 * 
 */

#include "DCF77Sync.h"

#define DCF_PIN 8          // Connection pin to DCF 77 device
#define DCF_POWER_PIN 7    // Connection to power on pin of DCF 77 device (HIGH = OFF; LOW = ON);
#define DCF_INTERRUPT 0    // Interrupt number associated with pin

DCF77Sync DCF = DCF77Sync(DCF_PIN,DCF_POWER_PIN,DCF_INTERRUPT);

#define PIN_MINUTE_POSITION 4
#define PIN_MINUTE_ZERO 1
#define PIN_MINUTE_MOTOR 3

#define PIN_HOUR_POSITION 10
#define PIN_HOUR_ZERO 0
#define PIN_HOUR_MOTOR 9

#define PIN_LED_SIGNAL 2

boolean state_minute_position = false;
boolean state_minute_zero = false;
boolean state_minute_turn = false;

boolean event_minute_position = false;
boolean event_minute_zero = false;
boolean event_minute_turn = false;

boolean state_hour_position = false;
boolean state_hour_zero = false;
boolean state_hour_turn = false;

boolean event_hour_position = false;
boolean event_hour_zero = false;
boolean event_hour_turn = false;

volatile byte time_seconds= 0;
byte time_minutes= 0;
byte time_hours= 0;
int time_minuteofday_prev = 0;

boolean event_second = false;
boolean event_minute = false;
boolean event_hour = false;

boolean state_timesync = false;
boolean event_timesync = false;
byte count_dcfvalid = 0;

byte pos_minutes = 0;
byte pos_hours = 0;
boolean pos_minutes_search = false;
boolean pos_hours_search = false;

//unsigned long time_next_second = 0;

unsigned long time_event_minute_stop = 0;
unsigned long time_event_hour_stop = 0;
unsigned long time_event_timesync = 0;

unsigned long time_debounce_event_minute_position = 0;
unsigned long time_debounce_event_hour_position = 0;

unsigned long time_led = 0;
boolean state_led = false;

//Interrupt service routine called every second
ISR(TIM1_COMPA_vect)
{
  time_seconds++;
}

void setup() {
  // Calibrate frequency to get better accuracy, see datasheet 
  // and http://becomingmaker.com/tuning-attiny-oscillator/
  // For using timer interrupts see http://www.engblaze.com/we-interrupt-this-program-to-bring-you-a-tutorial-on-arduino-interrupts/
  // and http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/
  
  OSCCAL += 2;
  const int calibrate = 65+39;

  //Init timer 1 
  cli();             // disable global interrupts
  
  TCCR1A = 0;        // set entire TCCR1A register to 0 (clear pin bindings etc.)
  TCCR1B = 0;        // set entire TCCR1A register to 0 (clear pin bindings etc.)
  
  //Target time: 1s
  //timer resolution (1 tick) = (prescaler / (clock speed)) s
  //timer resolution (1 tick) = 64 / 1*10^6 = 0,000064s = 64 microseconds
  
  //(target time) = (timer resolution) * (# timer counts + 1)
  //(# timer counts) = ((target time) / (timer resolution)) -1
  //(# timer counts) = ((target time) / [prescaler / (clock speed)]) -1
  //(# timer counts) = ([(target time) * (clock speed)] / prescaler)]) -1
  //(# timer counts) = (1s / [64 / 1000000Hz])-1 = 15624
  //(# timer counts) = ((1s * 1000000Hz) / 64) -1 = 15624 //minus one because it takes one cycle to compare?

  OCR1A = 15624-calibrate; //set compare match register to desired timer count
  TCCR1B |= (1 << WGM12);  //turn on CTC mode    
  TCCR1B |= (1 << CS10);   // Set CS10 and CS11 bit  for prescaler of 64
  TCCR1B |= (1 << CS11);   // Set CS10 and CS11 bit  for prescaler of 64
  TIMSK1 = (1 << OCIE1A);  // enable Timer1 compare interrupt  
  sei();                   // enable global interrupts: 

  //Pin configuration
  pinMode(PIN_MINUTE_POSITION,INPUT);
  digitalWrite(PIN_MINUTE_POSITION,HIGH);

  pinMode(PIN_MINUTE_ZERO,INPUT);
  digitalWrite(PIN_MINUTE_ZERO,HIGH);

  pinMode(PIN_MINUTE_MOTOR,OUTPUT);  
  digitalWrite(PIN_MINUTE_MOTOR,LOW);

  pinMode(PIN_HOUR_POSITION,INPUT);
  digitalWrite(PIN_HOUR_POSITION,HIGH);

  pinMode(PIN_HOUR_ZERO,INPUT);
  digitalWrite(PIN_HOUR_ZERO,HIGH);

  pinMode(PIN_HOUR_MOTOR,OUTPUT);  
  digitalWrite(PIN_HOUR_MOTOR,LOW);

  pinMode(PIN_LED_SIGNAL,OUTPUT);  
  digitalWrite(PIN_LED_SIGNAL,LOW);

  //Starting configuration
  pos_minutes_search = true;
  pos_hours_search = true;  
  time_event_minute_stop = millis() + 3000;
  time_event_hour_stop = millis() + 3000;
}

void loop() {
  static boolean state = false;

  //Zero position event
  state = ! digitalRead(PIN_MINUTE_ZERO);
  event_minute_zero = state != state_minute_zero;
  state_minute_zero = state;

  state = ! digitalRead(PIN_HOUR_ZERO);
  event_hour_zero = state != state_hour_zero;
  state_hour_zero = state;      

  //Zero events
  if (event_minute_zero && state_minute_zero) { 
    pos_minutes = 63;

    if (pos_minutes_search == true)
      time_minutes = 0;
    pos_minutes_search = false;
  }
  if (event_hour_zero && state_hour_zero) { 
    pos_hours = 31;
    
    if (pos_hours_search == true)
      time_hours = 0;
    pos_hours_search = false;
  }
  
  //Position event
  if ((long) (millis() - time_debounce_event_minute_position) >= 0 ) {  
    state = ! digitalRead(PIN_MINUTE_POSITION);
    event_minute_position = state != state_minute_position;
    state_minute_position = state;    
  } else {
     event_minute_position = false;
  }
  if (event_minute_position) 
    time_debounce_event_minute_position = millis()+100;

  if ((long) (millis() - time_debounce_event_hour_position) >= 0 ) {  
    state = ! digitalRead(PIN_HOUR_POSITION);
    event_hour_position = state != state_hour_position;
    state_hour_position = state;
  } else {
     event_hour_position = false;
  }
  if (event_hour_position) 
    time_debounce_event_hour_position = millis()+100;
  

  //Count events (falling edge)
  if (event_minute_position && !state_minute_position) {
    pos_minutes += 1;
    if (pos_minutes == 64 )
      pos_minutes = 0;   
  }
  
  if (event_hour_position && !state_hour_position) { 
    pos_hours += 1;
    if (pos_hours == 32 )
      pos_hours = 0;
  }


  //Time events
  if (time_seconds >= 60) {
    time_seconds = 0;
    time_minutes += 1;
    event_minute = true;
  } else event_minute = false;
  
  if (event_minute && (time_minutes >= 60)) {
    time_minutes = 0;
    time_hours += 1;
    if (time_hours == 24)
      time_hours = 0;
    event_hour = true;
  } else event_hour= false;


  //
  // Minutes --
  //
  
  //Allow turning only when events triggered (clock is working)
  if (event_minute || event_minute_position || event_timesync) { 
    time_event_minute_stop = millis() + 3000;
  }

  //Emergency stop when no events triggered any more
  if ((long) (millis() - time_event_minute_stop) >= 0) {
    state_minute_turn = false;
    digitalWrite(PIN_MINUTE_MOTOR,LOW);
  }  

  //Pause when searching time signal (noise reduction)
  else if (state_timesync && !pos_minutes_search) {
    state_minute_turn = false;
    digitalWrite(PIN_MINUTE_MOTOR,LOW);    
  }
  
  //Start
  else if ((pos_minutes != time_minutes)  || pos_minutes_search) {
    state_minute_turn = true;
    digitalWrite(PIN_MINUTE_MOTOR,HIGH);
  }

  //Stop
  else  {
      state_minute_turn = false;
      digitalWrite(PIN_MINUTE_MOTOR,LOW);    
  }  


  //
  // Hours --
  //
  
  //Allow turning only when events triggered (clock is working)
  if (event_hour || event_hour_position || event_timesync) {  
    time_event_hour_stop = millis() + 3000;    
  } 

  //Emergency stop  
  if (((long) (millis() - time_event_hour_stop) >= 0)) {
    state_hour_turn = false;
    digitalWrite(PIN_HOUR_MOTOR,LOW);
  }

  //Pause when searching time signal (noise reduction)
  else if (state_timesync && !pos_hours_search) {
    state_hour_turn = false;
    digitalWrite(PIN_HOUR_MOTOR,LOW);
  }
  
  //Start
  else if ((pos_hours != time_hours) || pos_hours_search) {
    digitalWrite(PIN_HOUR_MOTOR,HIGH);
    state_hour_turn = true;
  }  
  
  //Stop
  else {
    state_hour_turn = false;
    digitalWrite(PIN_HOUR_MOTOR,LOW);    
  }

 //
 // DCF sync events --
 //
 
  event_timesync = false;
  if ((long) (millis() - time_event_timesync) >= 0 ) {
    if (!state_timesync && !state_minute_turn && !state_hour_turn) {
      
      DCF.start();
      event_timesync = true;
      count_dcfvalid = 0;
      time_minuteofday_prev = 0;
      time_event_timesync = millis()+ 300000; //Check max. 5 minutes
      state_timesync = true;
      time_led = millis()+1;
    } 
    else if (state_timesync && !state_minute_turn && !state_hour_turn) {
       DCF.stop();
       state_timesync = false;    
      
       time_event_timesync = millis()+ 900000; //Try again in 15 minutes
    }
  } 
    
  if (state_timesync)
  {
     if (DCF.isValid()) {
       //check for consecutive valid values
       if ((DCF.getMinuteOfDay() - time_minuteofday_prev) == 1) 
         count_dcfvalid += 1;         
       else
          count_dcfvalid = 1;
          
       time_minuteofday_prev = DCF.getMinuteOfDay();
       event_timesync = true;
       
       if (count_dcfvalid > 0) {
         time_seconds = DCF.getSecond();
         time_minutes = DCF.getMinute();
         time_hours = DCF.getHour();         
       }

       if (count_dcfvalid > 0) {
         DCF.stop();
         time_event_timesync = millis()+ 3600000 - (time_seconds * 1000) - 5000; //Check every 60 minutes, align to minute, 5 seconds earlier to hit data stream
         state_timesync = false;    
       }
     }

    if (DCF.hasSignal()) {
     state_led= true;
    }     
    else if ((long) (millis() - time_led) >= 0 ) {      
     state_led= !state_led;     
     time_led = millis() + 200;
    }
       
  } else {
    state_led= false;  
  }  

  //Signal indicator (off = standby, blinking = searching signal, on = downloading data)
  digitalWrite(PIN_LED_SIGNAL,state_led);     
           
  delay(1);
}


