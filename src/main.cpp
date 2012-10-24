#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
//#include <cv.h>
//#include <cxcore.h>
//#include <highgui.h>

//#include <boost/signal.hpp>
//#include <boost/bind.hpp>

//for usleep
#include <unistd.h>

#include "constants.h"
#include "JsonConfig.h"
#include "B9CreatorSettings.h"
#include "SerialManager.h"
#include "OnionServer.h"
#include "DisplayManager.h"
#include "JobManager.h"

#include <locale.h>
//#include <time.h>
#include <sys/time.h>

int main(int argc, char **argv) {
	bool die(false);
	int k(0);

	if( argc > 1){
	}

	//Load & Create settings
	//B9CreatorSettings *b9CreatorSettings = new B9CreatorSettings();
	B9CreatorSettings b9CreatorSettings;
	if( b9CreatorSettings.init("b9CreatorSettings.ini") ){
		//Config file did not exist. Create it.
		printf("Create b9CreatorSettings.ini\n");
		b9CreatorSettings.saveConfigFile("b9CreatorSettings.ini");
	}

	//init onion server thread
	OnionServer onion(b9CreatorSettings); 
	onion.start_server();

	SerialManager serialManager(b9CreatorSettings);

	DisplayManager displayManager(b9CreatorSettings);
	JobManager jobManager(b9CreatorSettings, displayManager);

	//connect signals
	onion.updateSignal.connect(
					boost::bind(&JobManager::webserverSetState,&jobManager, _1, _2, _3)
					);


	//displayManager.start();
	//jobManager.loadImg("../test.png");
	jobManager.loadJob("puzzle.svg");

	/* Local needs to be set to avoid errors with printf + float values.
	 * Gtk:Window changes locale...*/
	setlocale(LC_NUMERIC, "C");

	/* Main Thread Loop */
	while( !die ){

		//block setting object
		b9CreatorSettings.lock();
		//send serial messages

		//read&handle serial messages

		//update job/image
		//jobManager.run();

		//unblock setting object
		b9CreatorSettings.unlock();

		//give other threads some time to react.
		usleep(100);

		//char k = cvWaitKey(10);
		k = 0; usleep(100);
		if( k == 27 ){
			printf("End main loop\n");
			die = true;
			break;
		}
		if( b9CreatorSettings.m_die ) die = true;

		/*
		usleep(4000000);
		die = true;
		b9CreatorSettings.m_die = true;
		*/
	}

	/* Clean up objects */
	//none

	usleep(10000);

	return EXIT_SUCCESS;
}

