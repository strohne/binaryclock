/**
 * @author  jakobjuenger@freenet.de
 * @date    2017-04-21
 * @license MIT
 *
 * DCF77 library, inspired by:
 * http://thijs.elenbaas.net/downloads/?did=1
 * https://www.freimann.eu/eliphi/phi/textuhr/
 * 
 */

#include "DCF77Sync.h"
#include <Arduino.h>


DCF77Sync::DCF77Sync(int dcf_pin,int dcf_powerpin, int dcf_interrupt) {
	_dcf_pin = dcf_pin;
	_dcf_powerpin = dcf_powerpin;
	_dcf_interrupt = dcf_interrupt;
	_clearValues();

  pinMode(_dcf_powerpin,OUTPUT);  
  digitalWrite(_dcf_powerpin,HIGH);	
}

/**
 * Start receiving DCF77 information
 */
void DCF77Sync::start(void) {
	digitalWrite(_dcf_powerpin,LOW);
	
	_clearValues();
	attachInterrupt(_dcf_interrupt, int0handler, CHANGE);
}

/**
 * Stop receiving DCF77 information
 */
void DCF77Sync::stop(void) {    
	detachInterrupt(_dcf_interrupt);
	_clearValues();	
	digitalWrite(_dcf_powerpin,HIGH);		
}

/**
 * Restart receiving DCF77 information
 */
void DCF77Sync::restart(void) {
	_clearValues();	
}


bool DCF77Sync::hasSignal() {
    unsigned long time_current = millis();
	
	// Reject if pulse distance is too long (incorrect signal)
	if ((time_current-_time_pulse_start) > DCF_PULSEDISTANCE_MAX) {
	  _dcf_signal_ok = false;
	} 	
	
	return _dcf_signal_ok;
}
	
bool DCF77Sync::isValid() {
	//no timestamp yet
	if(0 == _time_sync) {
		return false;
	}
	
	//timestamp is old
	if ((long) (millis() - _time_outdated) >= 0) {
		return false;
	}
	
	//not enough data yet
	if (_dcf_value_pos < 35 ) {
		return false;
	}

	//Check parity	
	return _checkParity();
}




int  DCF77Sync::getHour() {
	_calculateTime();
	return (_minuteofday / 60) % 24;
}


int DCF77Sync::getMinute() {
	_calculateTime();
	return (_minuteofday + 60) % 60;
}

int DCF77Sync::getMinuteOfDay() {
	_calculateTime();
	return _minuteofday;
}

int DCF77Sync::getSecond() {
	return _dcf_value_pos;
}


// void DCF77Sync::_calculateValidMinutes() {
	// static int _minuteofday_prev = 0;
	// static bool _isvalid_prev = false;
	
	// if (_isvalid_prev != isValid()) {
		
		// if (not _isvalid_prev) {
			// _calculateTime();
			
			 // if ((_minuteofday - _minuteofday_prev) == 1) 
			   // _validminutes += 1;         
			 // else
				// _validminutes = 1;
		// }
        
     // _minuteofday_prev = _minuteofday;
    // _isvalid_prev = not _isvalid_prev;
  // }
// }	
	
// int DCF77Sync::getValidMinutes() {	 
	// return _validminutes;
// }	
	
void DCF77Sync::_clearValues() {
	_time_sync = 0;
	_time_outdated = 0;	
	
	_time_pulse_start  = 0;
	_time_pulse_start_previous = 0;
	_time_pulse_end  = 0;

	_dcf_signal_previous = false;
	_dcf_signal_ok = false;
	_dcf_value_pos = 0;
	
	_validminutes = 0;
}


void DCF77Sync::_calculateTime() {
	int hour   =  1*_dcf_values[29] +  2*_dcf_values[30] +  4*_dcf_values[31] +
	               8*_dcf_values[32] + 10*_dcf_values[33] + 20*_dcf_values[34];	
	int minute =  1*_dcf_values[21] +  2*_dcf_values[22] +  4*_dcf_values[23] +
	             8*_dcf_values[24] + 10*_dcf_values[25] + 20*_dcf_values[26] +
	             40*_dcf_values[27];	

	_minuteofday = (hour*60) + minute -1;
	if (_minuteofday < 0)
		_minuteofday = 23*60+59;
}

/**
 * Interrupt handler that processes pulses
 */
void DCF77Sync::int0handler() {
	unsigned long time_change = millis();
	byte dcf_signal = digitalRead(_dcf_pin);

	// Reject if pulse distance is too short (incorrect signal)
	if ((time_change-_time_pulse_start_previous) < DCF_PULSEDISTANCE_MIN) {
	  _dcf_signal_ok = false;
	  return;
	}
	
    // Reject if pulse is too short (incorrect signal)	
	if ((time_change-_time_pulse_start) < DCF_PULSEWIDTH_MIN) {	  
	   _dcf_signal_ok = false;
		return;
	}
	
	_dcf_signal_ok = true;
	
	//Pulse start: raising edge
	if (dcf_signal && !_dcf_signal_previous) {
	    _dcf_signal_previous = true;
	    _time_pulse_start_previous = _time_pulse_start;
	    _time_pulse_start=time_change;

		//sync
		if ((_time_pulse_start-_time_pulse_start_previous) > DCF_SYNCTIME) {
			_dcf_value_pos = 0;
			
			_time_sync = time_change;
			_time_outdated = time_change + DCF_OUTDATED_MILLIS;
			
		} else {
	       _dcf_value_pos = (_dcf_value_pos + 1) % DCF_VALUES_COUNT;
		}   
	} 
	
	//Pulse end: falling edge
	else if (!dcf_signal && _dcf_signal_previous) {
		_dcf_signal_previous = false;
		_time_pulse_end=time_change;
		
		//long vs. short pulses
		int difference = _time_pulse_end - _time_pulse_start;
		_dcf_values[_dcf_value_pos] = difference > DCF_SPLITTIME;
		// _calculateValidMinutes();
	}  
}

boolean DCF77Sync::_checkParity() {
	bool parity_ok = true;
	int parity = 0;
	
	//Fixed values for second 0 and 20
	if(_dcf_values[0]) { parity_ok = false; }
	if(!_dcf_values[20]) { parity_ok = false; }

	//Minute parity
	parity = _dcf_values[21] + _dcf_values[22] + _dcf_values[23] +
	           _dcf_values[24] + _dcf_values[25] + _dcf_values[26] + _dcf_values[27];
	parity = 1 & parity;	
	if(_dcf_values[28] != parity) {parity_ok = false; }

	//Hour parity
	parity = _dcf_values[29] + _dcf_values[30] + _dcf_values[31] +
	            _dcf_values[32] + _dcf_values[33] + _dcf_values[34];
	parity = 1 & parity;
	if(_dcf_values[35] != parity) {parity_ok = false; } 

	//Date parity is ignored
	
	return parity_ok;
}


/**
 * Initialize parameters
 */
int DCF77Sync::_dcf_pin=0;
int DCF77Sync::_dcf_powerpin=0;
int DCF77Sync::_dcf_interrupt=0;

unsigned long DCF77Sync::_time_pulse_start = 0;
unsigned long DCF77Sync::_time_pulse_start_previous = 0;
unsigned long DCF77Sync::_time_pulse_end = 0;

bool DCF77Sync::_dcf_signal_previous = false;
bool DCF77Sync::_dcf_signal_ok = false;
int DCF77Sync::_minuteofday = 0;
int DCF77Sync::_validminutes = 0;

volatile unsigned long  DCF77Sync::_time_sync = 0;    
volatile unsigned long  DCF77Sync::_time_outdated = 0;

volatile bool  DCF77Sync::_dcf_values[DCF_VALUES_COUNT];
volatile int   DCF77Sync::_dcf_value_pos = 0; 
