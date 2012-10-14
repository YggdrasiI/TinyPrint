/*
 * Mange the serial communication with the arduino board.
 *
 * */
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <queue>
#include <pthread.h>
#include <SerialStream.h>
#include "Mutex.h"
#include "B9CreatorSettings.h"

#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

using namespace std ;
using namespace LibSerial ;

static void* readSerial(void* arg);

class SerialManager {
	public: 
		bool m_die;
		queue<string> m_messageQueue;
		Mutex m_messageMutex;
	private:
		B9CreatorSettings &m_b9CreatorSettings;
		SerialStream m_serialStream;
		pthread_t m_pthread;
	public:
		SerialManager(B9CreatorSettings &b9CreatorSettings ) :
			m_b9CreatorSettings(b9CreatorSettings),
			m_die(false),
			m_messageQueue(),
			m_messageMutex(),
			m_serialStream()	{

				if( pthread_create( &m_pthread, NULL, &readSerial, this) ){
					std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
						<< "Error: Could not create thread for serial communication."
						<< std::endl ;
					exit(1) ;
				}

			}

		~SerialManager(){
			// kill loop in other thread
			m_die = true;
			//wait on other thread
	    pthread_join( m_pthread, NULL);

		}

		void run();

	private:
		/* Buffer serial input */
		void readLine();

};

/* function for serial communication thread */
static void* readSerial(void* arg){
	VPRINT("Start serial thread\n");
	((SerialManager*)arg)->run();
	VPRINT("Quit serial thread\n");
}


#endif
