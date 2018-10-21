/*
 * @author  jakobjuenger@freenet.de
 * @date    2017-04-21
 * @license MIT
 *
 * DCF77 library, inspired by:
 * http://thijs.elenbaas.net/downloads/?did=1
 * https://www.freimann.eu/eliphi/phi/textuhr/
 * 
 */

#include <Arduino.h>

#ifndef DCF77SYNC
#define DCF77SYNC

#define DCF_VALUES_COUNT 61 //seconds (including leap-second) to store
#define DCF_OUTDATED_MILLIS 300000 // Time is outdated and invalid after 5 minutes

#define DCF_PULSEDISTANCE_MIN 700   // Minimum distance between two pulse starts
#define DCF_PULSEWIDTH_MIN 50  		// Minimal pulse width
#define DCF_SPLITTIME 180        	// Specifications distinguishes pulse width 100 ms and 200 ms. In practice we see 130 ms and 230
#define DCF_SYNCTIME 1500        	// Specifications defines 2000 ms pulse distance for end of sequence
#define DCF_PULSEDISTANCE_MAX 2500   // Maximum distance between two pulse starts

class DCF77Sync {
	public:

		DCF77Sync (int dcf_pin, int dcf_powerpin, int dcf_interrupt);

		static void start();		
		static void stop();
		static void restart();	
		static void int0handler();
		
		static bool isValid();
		static bool hasSignal();
		
		static int  getSecond();
		static int  getMinute();
		static int  getHour  ();
		static int getMinuteOfDay();
		// static int getValidMinutes();
		
		
	private:
		static int _dcf_pin;
		static int _dcf_powerpin;
		static int _dcf_interrupt;	
		static int _minuteofday;
		static int _validminutes;
	
		static unsigned long _time_pulse_start;
		static unsigned long _time_pulse_start_previous;
		static unsigned long _time_pulse_end;
		
		static bool _dcf_signal_previous;
		static bool _dcf_signal_ok;
	
		static volatile unsigned long  _time_sync;     //time (millis) the dcf signal started or 0
		static volatile unsigned long  _time_outdated; //time (millis) the dcf signal will be too old

		static volatile bool  _dcf_values[DCF_VALUES_COUNT];
		static volatile int   _dcf_value_pos; // 0..(DCF_VALUES_COUNT-1)
		
		static void _clearValues();
		static void _calculateTime();
		// static void _calculateValidMinutes();
		static bool _checkParity();		
}; 


#endif
