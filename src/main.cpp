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
//#include "PrinterSettings.h"
#include "OnionServer.h"

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
	OnionServer* ponion = new OnionServer(b9CreatorSettings); 
	ponion->start_server();

	/* Local needs to be set to avoid errors with printf + float values.
	 * Gtk:Window changes locale...*/
	setlocale(LC_NUMERIC, "C");

	while( !die ){

		//char k = cvWaitKey(10);
		k = 0; usleep(100);
		if( k == 27 ){
			printf("End main loop\n");
			die = true;
			break;
		}
	}

	/* Clean up objects */
	delete ponion;

	//cvDestroyAllWindows();

	//wait some time to give img-window enought time to close.
	//cvWaitKey(10);
	usleep(100);

	return EXIT_SUCCESS;
}

