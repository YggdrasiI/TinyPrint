#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <signal.h>

//#include <boost/signals2/signal.hpp>
#include <boost/bind.hpp>

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

static bool die(false);

static void end_main(int _){
	die = true;
}

int main(int argc, char **argv) {
	//signal(SIGINT,end_main);//segfault...
	//signal(SIGTERM,end_main);

	//bool die(false);
	int k(0);

	//Create arguments for DisplayManager.
	std::vector<std::string> dfbOptions;

	/* DirectFBInit ignores first arg. Thus
	 * the program name should added, too */
	for( int i=0; i<argc; ++i){
		std::string s(argv[i]);
		dfbOptions.push_back(s);
	}
	//Add no cursor option.
	std::string nocur("--dfb:no-cursor");
	dfbOptions.push_back(nocur);

	//Load & Create settings
	//B9CreatorSettings *b9CreatorSettings = new B9CreatorSettings();
	B9CreatorSettings b9CreatorSettings;
	if( b9CreatorSettings.init("b9CreatorSettings.json") ){
		//Config file did not exist. Create it.
		printf("Create b9CreatorSettings.json\n");
		b9CreatorSettings.saveConfigFile("b9CreatorSettings.json");
	}

	//init onion server thread
	OnionServer onion(b9CreatorSettings); 
	onion.start_server();

	SerialManager serialManager(b9CreatorSettings);

	DisplayManager displayManager(b9CreatorSettings, dfbOptions);
	JobManager jobManager(b9CreatorSettings, displayManager);

	//connect signals
	onion.updateSignal.connect(
					boost::bind(&JobManager::webserverSetState,&jobManager, _1, _2, _3)
					);
	onion.updateSignal.connect(
					boost::bind(&DisplayManager::getDisplayedImage, &displayManager, _1, _2, _3)
					);
	onion.updateSignal.connect(
					boost::bind(&JobManager::getJobTimings, &jobManager, _1, _2, _3)
					);


	bool autostartdisplay(false);
	for( int i=0; i<argc; ++i ){
		if( strcmp("--display",argv[i]) == 0 ){
			autostartdisplay = true;
			break;
		}
	}

	if(autostartdisplay){
		b9CreatorSettings.m_display = true;
	}

	//jobManager.loadJob("puzzle.svg");

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
	onion.stop_server();
	for(int i=0;i<1;++i){
		std::cout << "Wait..." << std::endl;
		sleep(1);
	}

	usleep(10000);

	return EXIT_SUCCESS;
}

