#ifndef B9CREATORSETTINGS_H
#define B9CREATORSETTINGS_H

#include "JsonConfig.h"


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

		int m_currentLayer;
		int m_vatOpen; //in percent
		int m_projectorStatus;
		int m_printerStatus;
		double m_zHeight;

	public:
		const char* m_host;
		const char* m_port;
		const char* m_comPort;
		const char* m_b9jDir;
		double m_breathTime;//?
		bool m_gridShow; //show grid
		int m_gridColor[3]; //rgb value
		bool m_die; // flag indicate end of main loop

	public:
		B9CreatorSettings() : 	JsonConfig(),
		m_spr(200), m_tpi(20),
		m_breathTime(-1.0), m_gridShow(true),
		m_currentLayer(1), m_vatOpen(-100),
		m_projectorStatus(0), m_printerStatus(0),
		m_zHeight(-1.0),
		m_die(false)
		{
			m_PU = 1000 * 254 / (m_spr * m_tpi) ;
		};

	
		cJSON* loadDefaults();
		int update(cJSON* root, cJSON* update, int changes);

	private:
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		bool updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);

};



#endif
