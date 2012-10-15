#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <queue>
#include "JsonConfig.h"
#include "JsonMessage.h"


class B9CreatorSettings: public JsonConfig{
	private:
		/* Maschine dependent values:
		 * m_tpi: thread per inch of z axis screw.
		 * m_spr: Steps per revolution of z axis motor.
		 *
		 * PU = 10000 * 1/m_spr * 1/m_tpi inch / mm (PU is scaled by Metric unit!)
		 * 1″ = 254 mm */
		int m_spr; 
		int m_tpi;
		int m_PU;

	public:
		int m_currentLayer;
		int m_vatOpen; //in percent
		int m_projectorStatus;
		int m_resetStatus;
		double m_zHeight;

		std::string m_host;
		std::string m_port;
		std::string m_comPort;
		std::string m_b9jDir;
		int m_comBaudrate;
		double m_breathTime;
		bool m_gridShow; //show grid
		unsigned char m_gridColor[3]; //rgb value
		bool m_die; // flag indicate end of main loop

		/* This object owns his own mutexes.
			This could cause deadlocks if some mutexes will enwinded... 
			*/
		Messages m_queues; 

	public:
		B9CreatorSettings() : 	JsonConfig(),
		m_spr(200), m_tpi(20),
		m_breathTime(-1.0), m_gridShow(true),
		m_currentLayer(1), m_vatOpen(-100),
		m_projectorStatus(0), m_resetStatus(1),
		m_zHeight(-1.0),
		m_comBaudrate(115200),
		m_queues(),
		m_host(),
		m_port(),
		m_comPort(),
		m_b9jDir(),
		m_die(false)
		{
			m_PU = 1000 * 254 / (m_spr * m_tpi) ;
		};

	
		void loadDefaults();
		cJSON* genJson();
		int update(cJSON* root, cJSON* update, int changes);

	private:
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);

};



#endif
