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

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
