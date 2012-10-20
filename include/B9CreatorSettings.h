#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <queue>
#include <onion/onion.h>
#include "JsonConfig.h"
#include "JsonMessage.h"

struct PrintProperties{
		double m_breathTime;
		double m_releaseCycleTime;
		double m_exposureTime;
		double m_exposureTimeAL;
		int m_nmbrOfAttachedLayers;
		int m_currentLayer;
		int m_maxLayer; //chaged after file loading
		bool m_lockTimes; /*flag to set some fields 'readable' 
												on the web page 
												if printing is running */
		int m_zResolution; // in μm.
		int m_xyResolution; // in μm.
};

class B9CreatorSettings: public JsonConfig{
	private:
		/* Maschine dependent values:
		 * m_tpi: thread per inch of z axis screw.
		 * m_spr: Steps per revolution of z axis motor.
		 *
		 * Definition of m_PU:
		 * 1PU	= 1/spr * 1/tpi * inch 
		 * 			= 1/spr * 1/tpi * 254 mm
		 * 			= 1/spr * 1/tpi * 254000 μm
		 * 			=: m_PU * 10μm
		 * => m_PU is scaled by unit '10μm'!
		 * Remark: 1″ = 1 inch = 25.4 mm
		 * */
		int m_spr; 
		int m_tpi;

	public:
		int m_PU;
		int m_vatOpen; //in percent
		int m_projectorStatus;
		int m_resetStatus;
		int m_zHeight;// height in PU
		int m_zHeightLimit;
		int m_zHome;// height in PU
		PrintProperties m_printProp;

		std::string m_host;
		std::string m_port;
		std::string m_comPort;
		std::string m_b9jDir;
		int m_comBaudrate;
		bool m_gridShow; //show grid
		bool m_display; //display used
		bool m_shutterEquipped;
		bool m_projectorEquipped;
		bool m_readyForNextCycle; // Set on true if "F" recieved.
		int m_lampHours;
		unsigned char m_gridColor[3]; //rgb value
		bool m_die; // flag indicate end of main loop
		JobState m_jobState; //updated by JobManager Thread.

		/* This object owns his own mutexes.
			This could cause deadlocks if some mutexes will enwinded... 
			*/
		Messages m_queues; 

	public:
		B9CreatorSettings();
	
		void loadDefaults();
		cJSON* genJson();
		int update(cJSON* root, cJSON* update, int changes=NO);

		/* Will called if website send data */
		void webserverUpdateConfig(onion_request *req, int actionid, std::string &reply);

	private:
		//similar to updateIntField in JsonConfig.
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);

};



#endif
