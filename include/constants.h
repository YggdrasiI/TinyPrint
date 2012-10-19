#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VERBOSE 1

#ifdef VERBOSE
#define VPRINT(...) fprintf(stdout, __VA_ARGS__)
#else
#define VPRINT(...)
#endif

//#include <cv.h>

/* PARSE_AGAIN: Indicates that json string should
 * parsed again.
 * WEB_INTERFACE: Indicates call of setConfig/update via web interface.
 * 	Parsing of some variables could omttid in this case.
 * CONFIG: Indicates call by initial loading.
 *  Every variable should be parsed in this case.
 * */
enum Changes {NO=0,YES=1,PARSE_AGAIN=2,CONFIG=4,WEB_INTERFACE=8, ALL=1023};

/* List of possible states. If the serial manager recieve
 * error messages or unexpected messages it could
 * use this states.
 * Currently not used.
 * */
enum PrinterState {PRINTER_OK=0, PRINTER_ERROR=1023};

/* 
 * PAUSE - Printing job is paused.
 * WAIT_ON_F_MESS - Waiting on next 'F' message on serial port.
 * 	( Precice: This class has no direct connection to the serial thread.
 * 		Moreover, to keep it the program simple, no event handling is used.
 * 	  JobManager just watches if m_b9CreatorSettings.m_readyForNextCycle 
 * 	  will set true.
 * WAIT_ON_R_MESS - Same as above for 'R0' message (set m_b9CreatorSettings.m_resetStatus )
 * */
enum JobState {
	RESET=0,
	INIT=1,
	FIRST_LAYER=1<<1,
	NEXT_LAYER=1<<2,
	NEXT_LAYER_OVERCURING=1<<3, /* not used */
	BREATH=1<<4,
	WAIT_ON_F_MESS=1<<5,
	WAIT_ON_R_MESS=1<<6,
	IDLE=1<<7,
	PAUSE=1<<8,
	FINISHED=1<<9,
	CURING=1<<10
};

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
