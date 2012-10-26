#ifndef ONIONSERVER_H
#define ONIONSERVER_H

#include <string>
#include <iostream>
#include <pthread.h>
//#include <png.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/dict.h>
//#include <onion/extras/png.h>

#include <boost/signal.hpp>
//#include <boost/bind.hpp>

#include "JsonConfig.h"
//#include "B9CreatorSettings.h"
class B9CreatorSettings;


/* Declare jSON data object here
 *
 *
 */


//thread function
static void* start_myonion_server(void* arg){
	printf("Onion server: Start listening.\n");
	onion_listen((onion*)arg);//loop
	printf("Onion server: Stop listening.\n");
}

class OnionServer{
	public:	
		onion* m_ponion;
		pthread_t m_pthread;
		//PrinterSetting* m_pprintSetting const;
		B9CreatorSettings &m_b9CreatorSettings;
	public:
		OnionServer(B9CreatorSettings &b9CreatorSettings );
		
		~OnionServer()
		{
			if(m_ponion != NULL) stop_server();
		}

		int start_server();
		int stop_server();

		/* Update signal. Called by sending of data by js script.
		 *	Every signal handler gain access to
		 *	- the raw request req,
		 *	-the get param 'actionid' and,
		 *	-the string 'reply' which will send back to the client.
		 *	For each actionid should only one signal handler wrote into reply.
		 *	*/
		boost::signal<void (onion_request *req,int actionid, std::string &reply)> updateSignal;

		/* Update signal handler of this class.*/
		void updateWebserver(onion_request *req, int actionid, std::string &reply);
};

#endif
