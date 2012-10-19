/*
 * Mange start, flow and end of 
 * printing jobs.
 * Works like an finite state machine. (See run())
 * See http://b9creator.com/downloads/DLP3DPAPI_1.0.htm
 * for a description of the printing loop.
 *
 * */
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <queue>
#include <pthread.h>
#include <sys/time.h>
#include <onion/onion.h>
#include "constants.h"
#include "Mutex.h"
//#include "B9CreatorSettings.h"
//#include "DisplayManager.h"
class B9CreatorSettings;
class DisplayManager;

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

// invoke thread loop.
static void* jobThread(void* arg);

typedef struct timeval timeval_t;

struct Timer{
		timeval_t begin;
		/* m_tDiff will used for if-statements of the type
		 * 'if( m_tDiff < difference(NULL,now, m_t[...]) )'
		 * My intention: I want omit thread blocking by usleep calls
		 * and also avoid an event based approach.
		 */
		long long diff;

		bool timePassed( timeval_t *now = NULL ){
			if( now == NULL ){
				timeval_t tmp;
				now = &tmp;
				gettimeofday( now, NULL );
			}
			return (diff < timeval_diff(now, &begin));
		}

		/* Return time diff in ns */
		static long long timeval_diff(
				timeval_t *end_time,
				timeval_t *start_time) 
		{   
			timeval_t *difference;	

			difference->tv_sec =end_time->tv_sec -start_time->tv_sec ;
			difference->tv_usec=end_time->tv_usec-start_time->tv_usec;

			/* Using while instead of if below makes the code slightly more robust. */

			while(difference->tv_usec<0)
			{   
				difference->tv_usec+=1000000;
				difference->tv_sec -=1;
			}   

			return 1000000LL*difference->tv_sec+
				difference->tv_usec;

		} /* timeval_diff() */
};

class JobManager {
		static const long long MaxWaitR = 5E10; //50s. Maximal waiting time on 'Ri' in ns.
		static const long long MaxWaitF = 2E9; //2s. Maximal waiting time on 'F' in ns.
	private:
		pthread_t m_pthread;
		bool m_die;
		B9CreatorSettings &m_b9CreatorSettings;
		DisplayManager &m_displayManager;
		JobState m_state;
		JobState m_pauseInState; //marks state which got paused.
		Mutex m_job_mutex;
		//save some timings
		Timer m_tTimer;
		Timer m_tPause;
		//until we need two timer paralell, all other
		//timer are just references to m_tTimer;
		Timer &m_tCuring;
		Timer &m_tCloseSlider;
		Timer &m_tProjectImage;
		Timer &m_tBreath;
		Timer &m_tFWait;
		Timer &m_tRWait;

	public:
		JobManager(B9CreatorSettings &b9CreatorSettings, DisplayManager &displayManager ) :
			m_pthread(),
			m_die(false),
			m_b9CreatorSettings(b9CreatorSettings),
			m_displayManager(displayManager),
			m_state(IDLE),
			m_pauseInState(IDLE),
			m_job_mutex(),
			m_tTimer(),
			m_tPause(),
			m_tCuring(m_tTimer),
			m_tCloseSlider(m_tTimer),
			m_tProjectImage(m_tTimer),
			m_tBreath(m_tTimer),
			m_tFWait(m_tTimer),
			m_tRWait(m_tTimer)
	{
		if( pthread_create( &m_pthread, NULL, &jobThread, this) ){
			std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
				<< "Error: Could not create thread for job manager."
				<< std::endl ;
			exit(1) ;
		}
	}

	~JobManager(){
			// kill loop in other thread
			m_die = true;
			//wait on other thread
	    pthread_join( m_pthread, NULL);
		}

		JobState getState() { return m_state; };

		/* Load b9j file */
		int loadJob(const std::string filename);
		/* Load image. (For testing) */
		int loadImg(const std::string filename);

		/* Init printer (read printer properties and set z-Table) */
		int initJob(bool withReset);
		/* Start job if none running */
		int startJob();
		int pauseJob();
		int resumeJob();
		int stopJob();

		void run();

		//int nextStep();

		/* Will called if website send data */
		void webserverSetState(onion_request *req, int actionid, std::string &reply);
};

/* wrapper function for job thread.*/
static void* jobThread(void* arg){
	VPRINT("Start job thread\n");
	((JobManager*)arg)->run();
	VPRINT("Quit job thread\n");
}

#endif
