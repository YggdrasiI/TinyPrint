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
#include "Mutex.h"
#include "B9CreatorSettings.h"

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

// invoke thread loop.
static void* jobThread(void* arg);

struct Timer{
		struct timeval begin;
		/* m_tDiff will used for if-statements of the type
		 * 'if( m_tDiff < difference(NULL,now, m_t[...]) )'
		 * My intention: I want omit thread blocking by usleep calls
		 * and also avoid an event based approach.
		 */
		long long diff;

		bool timePassed(struct timeval now = time(NULL) ){
			return (diff < timeval_diff(NULL, now, begin));
		}

		/* Return time diff in ns */
		static long long timeval_diff(
				struct timeval *difference,
				struct timeval *end_time,
				struct timeval *start_time) 
		{   
			struct timeval temp_diff;

			if(difference==NULL)
			{   
				difference=&temp_diff;
			}   

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
}

class JobManager {
		const long MaxWaitR = 5E10; //50s. Maximal waiting time on 'Ri' in ns.
		const long MaxWaitF = 2E9; //2s. Maximal waiting time on 'F' in ns.
	private:
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
	}

	~JobManager(){
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

		//int nextStep();



}


#endif
