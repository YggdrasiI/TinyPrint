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

/* Not used.
 * */
enum JobState {IDLE=0,INIT=1,FIRST_LAYER=2,MIDDLE_LAYER=4,LAST_LAYER=8,WAIT_ON_F_MESS=32};

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
