/*
 * Mange start, flow and end of 
 * printing jobs.
 * Works like an finite state machine.
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
#include "Mutex.h"
#include "B9CreatorSettings.h"

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

// invoke thread loop.
static void* jobThread(void* arg);


class JobManager {
	private:
		B9CreatorSettings &m_b9CreatorSettings;
		DisplayManager &m_displayManager;
		JobState state;
	public:
		JobManager(B9CreatorSettings &b9CreatorSettings, DisplayManager &displayManager ) :
			m_b9CreatorSettings(b9CreatorSettings),
			m_displayManager(displayManager),
			m_state(IDLE)
	{
	}

	~JobManager(){
		}

		JobState getState() { return state; };

		/* Load b9j file */
		int loadJob(const std::string filename);
		/* Load image. (For testing) */
		int loadImg(const std::string filename);

}


#endif
