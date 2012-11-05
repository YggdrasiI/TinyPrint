/*
 * Mange start, flow and end of 
 * printing jobs.
 * Works like an finite state machine. (See run())
 * See http://b9creator.com/downloads/DLP3DPAPI_1.0.htm
 * for a description of the printing loop.
 *
 * */
#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/time.h>
#include <onion/onion.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "constants.h"
#include "Mutex.h"
//#include "B9CreatorSettings.h"
//#include "DisplayManager.h"
class B9CreatorSettings;
class DisplayManager;

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
			//VPRINT("Diff1: %lli",diff);
			return (diff < timeval_diff(now, &begin));
		}

		/* Return time diff in ns */
		static long long timeval_diff(
				timeval_t *pEndTime,
				timeval_t *pStartTime) 
		{   
			timeval_t difference;	

			difference.tv_sec =pEndTime->tv_sec -pStartTime->tv_sec ;
			difference.tv_usec=pEndTime->tv_usec-pStartTime->tv_usec;


			//VPRINT("Start: %li End: %li Diff: %lli\n",pStartTime->tv_sec,pEndTime->tv_sec,1000000LL*difference.tv_sec);
			/* Using while instead of if below makes the code slightly more robust. */

			while(difference.tv_usec<0)
			{   
				difference.tv_usec+=1000000;
				difference.tv_sec -=1;
			}   

			return 1000000LL*difference.tv_sec+
				difference.tv_usec;

		}
};

class JobManager {
		static const long long MaxWaitR = 12E7; //120s. Maximal waiting time on 'Ri' in ns.
		//static const long long MaxWaitF = 5E6; //5s. Maximal waiting time on 'F' in ns.
		static const long long MaxWaitF = 40E6; //40s. 5s was to low. It limit maximal release cycle timeout! 
		//static const long long MaxWaitFfrist = MaxWaitR; // Maximal waiting time on 'F' for base layer.
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
		//int m_showedLayer; //save last displayed layer number.
		bool m_force_preload; //preload image of next layer

	public:
		JobManager(B9CreatorSettings &b9CreatorSettings, DisplayManager &displayManager );	 
		~JobManager();

		JobState getState() { return m_state; };

		/* Load b9j file */
		int loadJob(const std::string filename);
		/* Load image. (For testing) */
		int loadImg(const std::string filename);

		/* Init printer (read printer properties and set z-Table). */
		int initJob(bool withReset);
		/* Start job if none running */
		int startJob();
		int pauseJob();
		int resumeJob();
		int stopJob();

		void run();

		//int nextStep();

		/* Will called if website send data. */
		void webserverSetState(onion_request *req, int actionid, std::string &reply);
		/* Will called if m_b9CreatorSettings propagate settings change. */
		void updateSignalHandler(int changes);

	private:
		/* call getSlice for Jobfile. This call fill the
		 * generated image into the intern cache.
		 * The image generation is not threaded. Thus, it
		 * should call in idle states like 'BREATH', 'WAIT_*'.
		 * */
		void preload(int slice, SliceType type=RAW);

		void show(int slice, SliceType type=RAW);
};

/* wrapper function for job thread.*/
static void* jobThread(void* arg){
	VPRINT("Start job thread\n");
	((JobManager*)arg)->run();
	VPRINT("Quit job thread\n");
}

#endif
