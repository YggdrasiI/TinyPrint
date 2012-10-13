#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VERBOSE 1

#ifdef VERBOSE
#define VPRINT(...) fprintf(stdout, __VA_ARGS__)
#else
#define VPRINT(...)
#endif

//#include <cv.h>

enum Changes {NO=0,MASK=1,MARGIN=2,CONFIG=4, ALL=1023};

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
