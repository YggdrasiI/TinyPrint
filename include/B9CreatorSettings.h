#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <queue>
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
		 * 			= 1/spr * 1/tpi * 254000 nm
		 * 			=: m_PU nm
		 * => m_PU is scaled by Metric unit nm!
		 * 1″ = 25.4 mm
		 * */
		int m_spr; 
		int m_tpi;
		PrintProperties m_printProp;

	public:
		int m_PU;
		int m_vatOpen; //in percent
		int m_projectorStatus;
		int m_resetStatus;
		int m_zHeight;// height in PU
		int m_zHeightLimit;
		int m_zHome;// height in PU

		std::string m_host;
		std::string m_port;
		std::string m_comPort;
		std::string m_b9jDir;
		int m_comBaudrate;
		bool m_gridShow; //show grid
		bool m_display; //display used
		bool m_shutterEquipped;
		bool m_projectorEquipped;
		bool m_layerFinished; // Set on true if "F" recieved.
		int m_lampHours;
		unsigned char m_gridColor[3]; //rgb value
		bool m_die; // flag indicate end of main loop

		/* This object owns his own mutexes.
			This could cause deadlocks if some mutexes will enwinded... 
			*/
		Messages m_queues; 

	public:
		B9CreatorSettings() : 	JsonConfig(),
		m_spr(200), m_tpi(20),
		m_gridShow(true),
		m_display(false),
		m_vatOpen(-100),
		m_projectorStatus(2), m_resetStatus(1),
		m_zHeight(-1),
		m_zHeightLimit(100000000),
		m_zHome(-1),
		m_comBaudrate(115200),
		m_queues(),
		m_host(),
		m_port(),
		m_comPort(),
		m_projectorEquipped(false),
		m_shutterEquipped(false),
		m_lampHours(-1),
		m_b9jDir(),
		m_layerFinished(true),
		m_printProp(),
		m_die(false)
		{
			m_PU = 100 * 254 / (m_spr * m_tpi) ;

			m_printProp.m_breathTime = 2.0;
			m_printProp.m_releaseCycleTime = 1.75;
			m_printProp.m_exposureTime = 12;
			m_printProp.m_exposureTimeAL = 40;
			m_printProp.m_nmbrOfAttachedLayers = 4;
			m_printProp.m_currentLayer = 1;
			m_printProp.m_maxLayer = 1;
			m_printProp.m_lockTimes = false;
		};

	
		void loadDefaults();
		cJSON* genJson();
		int update(cJSON* root, cJSON* update, int changes=NO);

	private:
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);

};



#endif
