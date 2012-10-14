#include "B9CreatorSettings.h"

/*
 * Special properties. This values can modified with the web interface.
 * I.e. angle of kinect, nmbr of areas, position of areas, minimal blob size.
 */
cJSON* B9CreatorSettings::loadDefaults()
{
	cJSON* root = cJSON_CreateObject();	
	/* Kind only used to distinct different json structs. */
	cJSON_AddItemToObject(root, "kind", cJSON_CreateString("b9CreatorSettings"));

	cJSON_AddItemToObject(root, "host", cJSON_CreateString("0.0.0.0"));
	cJSON_AddItemToObject(root, "port", cJSON_CreateString("9090"));
	cJSON_AddItemToObject(root, "jobDir", cJSON_CreateString("job_files"));
	cJSON_AddItemToObject(root, "comPort", cJSON_CreateString("/dev/ttyACM0"));
	cJSON_AddItemToObject(root, "comBaudrate", cJSON_CreateNumber(115200));
	cJSON_AddItemToObject(root, "gridColor", cJSON_CreateString("A00000"));//hex string

	/* sub node. This values will transmitted to web interface */
	cJSON* html = cJSON_CreateArray();	
	cJSON_AddItemToArray(html, jsonIntField("stepsPerRevolution",200,36,1000,100,1) );
	cJSON_AddItemToArray(html, jsonIntField("threadPerInch",20,1,100,10,1) );
	cJSON_AddItemToArray(html, jsonDoubleField("breathTime",2,1,300,10) );
	cJSON_AddItemToArray(html, jsonCheckbox("gridShow",true) );
	cJSON_AddItemToArray(html, jsonStateField("currentLayer",1) );
	cJSON_AddItemToArray(html, jsonStateField("vatOpen",0,"percent","percent") );//in Percent
	cJSON_AddItemToArray(html, jsonStateField("projectorStatus",0,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("printerStatus",0,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("zHeight",0.0,"mm","mm") ); // height in mm.

	cJSON_AddItemToObject(root, "html", html);

	return root;
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
		if( updateState(nhtml,ohtml,"printerStatus",&m_printerStatus) ) changes|=MARGIN;
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
	VPRINT("update of %s:",id);				
	double nval=0.0, oval=*val;
	if( jsonOld != NULL && NULL != (otmp=getArrayEntry(jsonOld,id)) ){
		oval = getNumber(otmp,"val");
		nval = getNumber(ntmp,"val");
		if(oval!=nval) ret=true;
	}else if( ntmp != NULL){
		nval = getNumber(ntmp,"val");
		ret = true;
	}
	VPRINT(" %f\n",nval);				
	*val = nval;
	return ret;
}
