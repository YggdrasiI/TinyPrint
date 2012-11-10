#ifndef CONSTANTS_H
#define CONSTANTS_H

//#define VERBOSE 1

#ifdef VERBOSE
#define VPRINT(...) fprintf(stdout, __VA_ARGS__)
#else
#define VPRINT(...)
#endif

// Print mutex locks/unlocks.
//#define VERBOSE_MUTEX 

//#include <cv.h>

/* Most relevant properties are stored
 * in a B9CreatorSettings object. To update these
 * properties over the webinterface call
 * B9CreatorSettings.update(..., [json-string],..., Changes)
 * with Changes = WEB_INTERFACE.
 * The update method analyse the json-string, update 
 * the values, and extend the changes flag.
 * 		Example: If the current layer was modified =>
 * 			Changes = Changes | REDRAW
 *
 * At the end of the update, a signal with 'Changes'
 * will propagate. This can used by other clases to 
 * react on property changes.
 * 		Example: The JobManager class should update the
 * 			displayed layer if 'Changes & LAYER' is true.
 *
/* 
 * NO:  No changes.
 * YES: Mark general changes which not fit in below 
 *      categories.
 * CONFIG: Indicates call by initial loading.
 * WEB_INTERFACE: Indicates call of setConfig/update via web interface.
 *                Parsing of some variables could omited in this case.
 * PARSE_AGAIN: Indicates that json string (which will
 *              send by the webserver) should parsed again.
 * REDRAW: DisplayManager should force redraw.
 * LAYER: JobManager should regenerate LAYER (and force redraw).
 * */
enum Changes {
	NO=0,
	YES=1, /* If no other flag match. */
	PARSE_AGAIN=2,
	CONFIG=4,
	WEB_INTERFACE=8,
	REDRAW=16,
	LAYER=32,
	ALL=1023

};

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
	OVERCURING=1<<3, 
	BREATH=1<<4,
	WAIT_ON_F_MESS=1<<5,
	WAIT_ON_R_MESS=1<<6,
	IDLE=1<<7,
	PAUSE=1<<8,
	FINISH=1<<9,
	CURING=1<<10,
	START_STATE = 1<<11,
	ERROR = 1<<12, /* React like FINISH. TODO: Just blank image, (optional)close vat and (optional) power off projektor. */
	WAIT_ON_ZERO_HEIGHT = 1<<13
};
/* Remark: Mostly or all if-statements just checks one state at once. (No 'val & A|B' args)
 * It should be possible switch 1,2,3,4,5,... if #state numbers > #bits
 * */

// maximal number of images into the cache. (independent of image size)
static const int JOBFILE_CACHE_MAX_NMBR_OF_IMAGES = 10;
// maximal number of bytes (not used)
static const long JOBFILE_CACHE_SIZE = -1;

/* This type will used to distict different images of
 * the same slice in the cache.
 * Map of (slicenr, slicetype): slicenr<<4 + slicetype.
 * */
enum SliceType{
	RAW=0, /* raw svg/b9j pixel data */
	OVERCURE1=1 /* first type of overcuring */
};


enum Exceptions {
	JOB_LOAD_EXCEPTION
};

inline double min(double a,double b){return a<b?a:b;};
inline double max(double a,double b){return a>b?a:b;};

#endif
