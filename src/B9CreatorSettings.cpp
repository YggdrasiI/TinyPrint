#include "B9CreatorSettings.h"

/*
 * Special properties. This values can modified with the web interface.
 * I.e. angle of kinect, nmbr of areas, position of areas, minimal blob size.
 */
cJSON* B9CreatorSettings::genJson()
{
	cJSON* root = cJSON_CreateObject();	
	/* Kind only used to distinct different json structs. */
	cJSON_AddItemToObject(root, "kind", cJSON_CreateString("b9CreatorSettings"));

	cJSON_AddItemToObject(root, "host", cJSON_CreateString(m_host.c_str() ));
	cJSON_AddItemToObject(root, "port", cJSON_CreateString(m_port.c_str() ));
	cJSON_AddItemToObject(root, "jobDir", cJSON_CreateString(m_b9jDir.c_str() ));
	cJSON_AddItemToObject(root, "comPort", cJSON_CreateString(m_comPort.c_str() ));
	cJSON_AddItemToObject(root, "comBaudrate", cJSON_CreateNumber(115200));

	char gcol[6];
	sprintf(gcol,"%X%X%X",m_gridColor[0],m_gridColor[1],m_gridColor[2]);
	cJSON_AddItemToObject(root, "gridColor", cJSON_CreateString(gcol));//hex string

	/* sub node. This values will transmitted to web interface */
	cJSON* html = cJSON_CreateArray();	
	cJSON_AddItemToArray(html, jsonIntField("stepsPerRevolution",m_spr,36,1000,100,1) );
	cJSON_AddItemToArray(html, jsonIntField("threadPerInch",m_tpi,1,100,10,1) );
	cJSON_AddItemToArray(html, jsonDoubleField("breathTime",m_breathTime,1,300,10) );
	cJSON_AddItemToArray(html, jsonCheckbox("gridShow",m_gridShow) );
	cJSON_AddItemToArray(html, jsonStateField("currentLayer",m_currentLayer) );
	cJSON_AddItemToArray(html, jsonStateField("vatOpen",m_vatOpen,"percent","percent") );//in Percent
	cJSON_AddItemToArray(html, jsonStateField("projectorStatus",m_projectorStatus,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("resetStatus",m_resetStatus,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("zHeight",m_zHeight,"mm","mm") ); // height in mm.

	cJSON_AddItemToObject(root, "html", html);

	return root;
};

void B9CreatorSettings::loadDefaults()
{
	m_host = "0.0.0.0";
	m_port = "9090";
	m_b9jDir = "job_files";
	m_comPort = "/dev/ttyACM0";
	m_comBaudrate = 115200;
	m_gridColor[0] = 200; m_gridColor[1] = 0; m_gridColor[2] = 0;

	/* sub node. This values will transmitted to web interface */
	m_spr = 200;
	m_tpi = 20;
	m_breathTime = 2;
	m_gridShow = true;
	m_currentLayer = 1;
	m_vatOpen = 0;
	m_projectorStatus = 0;
	m_resetStatus = 1;
	m_zHeight = 0.0;

};


int B9CreatorSettings::update(cJSON* jsonNew, cJSON* jsonOld, int changes=NO){
	cJSON* nhtml = cJSON_GetObjectItem(jsonNew,"html");
	cJSON* ohtml = jsonOld==NULL?NULL:cJSON_GetObjectItem(jsonOld,"html");

	lock();
	if( nhtml != NULL){

		if( JsonConfig::update(nhtml,ohtml,"stepsPerRevolution",&m_spr) 
		|| JsonConfig::update(nhtml,ohtml,"threadPerInch",&m_tpi) ){
			m_PU = 1000 * 254 / (m_spr * m_tpi) ;
			changes|=MARGIN;
		}
		if( JsonConfig::update(nhtml,ohtml,"breathTime",&m_breathTime) ) changes|=MARGIN;
		if( JsonConfig::updateCheckbox(nhtml,ohtml,"gridShow",&m_gridShow) ) changes|=MARGIN;

		if( updateState(nhtml,ohtml,"currentLayer",&m_currentLayer) ) changes|=MARGIN;
		if( updateState(nhtml,ohtml,"vatOpen",&m_vatOpen) ) changes|=MARGIN;
		if( updateState(nhtml,ohtml,"projectorStatus",&m_projectorStatus) ) changes|=MARGIN;
		if( updateState(nhtml,ohtml,"resetStatus",&m_resetStatus) ) changes|=MARGIN;
		if( updateState(nhtml,ohtml,"zHeight",&m_zHeight) ) changes|=MARGIN;
		//call signal
		//updateSig(this,changes);
		
	}
	unlock();

	return changes!=NO?1:0;
}

/* Update of val without argument checking. */
bool B9CreatorSettings::updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val){
	double tmp=*val;
	bool ret = updateState(jsonNew,jsonOld,id,&tmp);
	*val = (int)tmp;
	return ret;
}
bool B9CreatorSettings::updateState(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val){
	cJSON* ntmp = getArrayEntry(jsonNew,id);
	cJSON* otmp;
	bool ret(false);
	//VPRINT("update of %s:",id);				
	double nval=0.0, oval=*val;
	if( jsonOld != NULL && NULL != (otmp=getArrayEntry(jsonOld,id)) ){
		oval = getNumber(otmp,"val");
		nval = getNumber(ntmp,"val");
		if(oval!=nval) ret=true;
	}else if( ntmp != NULL){
		nval = getNumber(ntmp,"val");
		ret = true;
	}
	//VPRINT(" %f\n",nval);				
	*val = nval;
	return ret;
}
