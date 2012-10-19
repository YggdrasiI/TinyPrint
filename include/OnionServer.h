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

#include "JsonConfig.h"
#include "B9CreatorSettings.h"
//#include "PrintSettings.h"


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
		OnionServer(B9CreatorSettings &b9CreatorSettings ):
			//m_ponion( onion_new(O_THREADED|O_DETACH_LISTEN) ),
			m_ponion( onion_new(O_THREADED) ),
			m_pthread(),
			m_b9CreatorSettings(b9CreatorSettings)
		{
			//start_server();
		}
		~OnionServer()
		{
			if(m_ponion != NULL) stop_server();
		}

		int start_server();
		int stop_server();
		int updateSetting(onion_request *req, onion_response *res, std::string &reply);
};

#endif
