#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <queue>

#include <onion/onion.hpp>
#include <onion/response.hpp>
#include <onion/request.hpp>

#include <boost/signal.hpp>
#include <boost/regex.hpp> 
//#include <boost/bind.hpp>

#include "JsonConfig.h"
#include "JsonMessage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class JobFile;

/* Wrapper struct for cycle settings. 
 * This will used to disticnt the values for 
 * the initial layers and all others.
 * */
struct CycleProperties{
		double m_breathTime;
		double m_settleTime;

		int m_shutterOpenSpeed; //in percent
		int m_shutterCloseSpeed; //in percent
		int m_zAxisRaiseSpeed; //in percent
		int m_zAxisLowerSpeed; //in percent
		int m_overlift; //in PU.
};

/* Wrapper struct for some properties. */
struct PrintProperties{
		double m_exposureTime;
		double m_exposureTimeAL;
		double m_overcureTime;
		//double m_releaseCycleTime;
		int m_nmbrOfAttachedLayers;
		int m_currentLayer;
		int m_nmbrOfLayers; //chaged after file loading
		bool m_lockTimes; /*flag to set some fields 'readable' 
												on the web page 
												if printing is running */
		double m_zResolution; // in μm.
		double m_xyResolution; // in μm.

		CycleProperties m_cycleProps[2]; /* Saves values for initial and normal cycles */
		int m_initialCutoff; // in PU.

		/* Return cycle props for current layer */
		CycleProperties &getCurrentProps(int heightInPU){
			std::cout << "Cutoff: " << m_initialCutoff << " Height: " << heightInPU << std::endl;
			if( heightInPU > m_initialCutoff ) return m_cycleProps[1];
			else return m_cycleProps[0];
		}
};


/* Helper functions */
static bool check_regex(const std::string &s, const std::string &pattern){
	boost::regex re;
	//std::string pattern("^[[:alnum:]]*\\.b9j$");
	//std::string s(filename);
	//std::cout << "String: " << s << std::endl << "Pattern: " << pattern << std::endl;
	try {
		re.assign(pattern, boost::regex_constants::icase);
	} catch (boost::regex_error& e) {
		std::cout << pattern << " is not a valid regular expression: \""
			<< e.what() << "\"" << std::endl;
	}

	if (boost::regex_match(s, re)) return true;
	return false;
}

static bool check_filename(const char *filename){
	const std::string s(filename);
	const std::string pattern("^[[:alnum:]]*\\.\\(svg|b9j|list\\)$");
	return check_regex(s,pattern);
}

static bool check_configFilename(const char *filename){
	const std::string s(filename);
	const std::string pattern("^.*\\.json$");
	return check_regex(s,pattern);
}

static bool check_b9jExtension(const char *filename){
	const std::string s(filename);
	const std::string pattern("^.*\\.b9j$");
	return check_regex(s,pattern);
}

static bool check_svgExtension(const char *filename){
	const std::string s(filename);
	const std::string pattern("^.*\\.svg$");
	return check_regex(s,pattern);
}

static bool check_listExtension(const char *filename){
	const std::string s(filename);
	const std::string pattern("^.*\\.list$");
	return check_regex(s,pattern);
}





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
		int m_shutterOpen; //in percent
		int m_projectorStatus;
		int m_resetStatus;
		int m_zHeight;// height in PU
		int m_zHeightLimit;
		int m_zHome;// height in PU
		int m_zOffset; //difference between expected home and found home position in PU.
		PrintProperties m_printProp;

		std::string m_host;
		std::string m_port;
		std::string m_comPort;
		std::string m_b9jDir;
		std::string m_firmwareVersion;
		std::string m_printerModel;
		int m_projectorXResolution;// Width of beamer resolution
		int m_projectorYResolution;// Height of beamer resolution
		/* Size of projected pixel in micron.
		 * The variable is redundant,  but part of DLP3DPAPI 1.1
		 * specification. */
		int m_projectorPixelSize;
		int m_hardDown; // in PU
		int m_upToFlush; // in PU
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
		/* Flag to flip slice orientation.
		 * This correct 'wrong' layer order where layer0 is on top */
		bool m_flipSprites; 
//		JobState m_jobState; //updated by JobManager Thread.
		std::vector<JobFile*> m_files;

		/* This object owns his own mutexes.
			This could cause deadlocks if some mutexes will enwinded... 
			*/
		Messages m_queues; 
		std::string m_configFilename;

		cv::Mat m_currentDisplayedImage; //will be updated by DisplayManager

	public:
		B9CreatorSettings();
		~B9CreatorSettings();
	
		void loadDefaults();
		cJSON* genJson();
		int update(cJSON* root, cJSON* update, int changes=NO);

		/* Will called if website send data */
		bool webserverUpdateConfig(Onion::Request *preq, int actionid, Onion::Response *pres);

		/* Call this method to eval the highest layer number
		 * for current list of m_files.
		 * Set numberOfLayers on 10 if list of files is empty.*/
		int updateMaxLayer();

		/* Update signal. Will send at the end of update(...) */
		boost::signal<void (int changes)> updateSettings;

		int loadJob(const std::string filename);
		int unloadJob(const int index);
		void clearJobs();

		/* Overwrite two methodes to save filename */
		int loadConfigFile(const char* filename){
			m_configFilename = filename;	
			return JsonConfig::loadConfigFile(filename);
		};
		int init(const char* filename="")
		{
			m_configFilename = filename;
			return JsonConfig::init(filename);
		};

		/* Get json struct of current open files. */
		cJSON *jsonFilesField(const char* id);

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
