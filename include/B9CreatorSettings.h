#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <queue>

#include <onion/onion.h>
#include <boost/signal.hpp>
//#include <boost/bind.hpp>

#include "JsonConfig.h"
#include "JsonMessage.h"

class JobFile;

struct PrintProperties{
		double m_breathTime;
		double m_releaseCycleTime;
		double m_exposureTime;
		double m_exposureTimeAL;
		int m_nmbrOfAttachedLayers;
		int m_currentLayer;
		int m_nmbrOfLayers; //chaged after file loading
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
//		bool m_redraw; //displayed image require redraw.
		bool m_shutterEquipped;
		bool m_projectorEquipped;
		bool m_readyForNextCycle; // Set on true if "F" recieved.
		int m_lampHours;
		unsigned char m_gridColor[3]; //rgb value
		bool m_die; // flag indicate end of main loop
		bool m_connected; // flag indicate serial connecton.
		JobState m_jobState; //updated by JobManager Thread.
		std::vector<JobFile*> m_files;

		/* This object owns his own mutexes.
			This could cause deadlocks if some mutexes will enwinded... 
			*/
		Messages m_queues; 

	public:
		B9CreatorSettings();
		~B9CreatorSettings();
	
		void loadDefaults();
		cJSON* genJson();
		int update(cJSON* root, cJSON* update, int changes=NO);

		/* Will called if website send data */
		void webserverUpdateConfig(onion_request *req, int actionid, std::string &reply);

		/* Call this method to eval the highest layer number
		 * for current list of m_files.
		 * Set numberOfLayers on 10 if list of files is empty.*/
		int updateMaxLayer();

		/* Update signal. Will send at the end of update(...) */
		boost::signal<void (int changes)> updateSettings;

		int loadJob(const std::string filename);

	private:
		//similar to updateIntField in JsonConfig.
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);

		/* Generate json struct for m_files vector. */
		/* Structure:
		 * { 	type: "filesField", 
		 * 		id	:	string,
		 * 		format: string,
		 * 		parse	: string,
		 * 		filearray : array[
		 * 			{ filename : string,
		 * 				description : string,
		 * 				html : array[
		 * 					{ maxLayer : intField,
		 * 						minLayer : intField,
		 * 						positionX : intField,
		 * 						positionY : intField
		 * 					}
		 * 				]
		 * 			}
		 * 		]
		 * */
		cJSON *jsonFilesField(const char* id, std::vector<JobFile*> files);

		/* Update m_files with values of json struct.
		 * For add==true will m_files expand with
		 * the given files.
		 * */
		int updateFiles(cJSON *jsonNew, cJSON *jsonOld, const char* id, std::vector<JobFile*> &files, bool add );
};



#endif
