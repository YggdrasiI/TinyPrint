#include "B9CreatorSettings.h"

/*
 * Special properties. This values can modified with the web interface.
 * I.e. angle of kinect, nmbr of areas, position of areas, minimal blob size.
 */
cJSON* B9CreatorSettings::loadDefaults()
{
	cJSON* root = cJSON_CreateObject();	
	/* Kind only used to distinct different json structs. */
	cJSON_AddItemToObject(root, "kind", cJSON_CreateString("settingKinectGrid"));

	cJSON_AddItemToObject(root, "host", cJSON_CreateString("0.0.0.0"));
	cJSON_AddItemToObject(root, "port", cJSON_CreateString("9090"));
	cJSON_AddItemToObject(root, "jobDir", cJSON_CreateString("job_files"));
	cJSON_AddItemToObject(root, "comPort", cJSON_CreateString("/dev/ttyACM0"));
	cJSON_AddItemToObject(root, "gridColor", cJSON_CreateString("A00000"));//hex string

	/* sub node. This values will transmitted to web interface */
	cJSON* html = cJSON_CreateArray();	
	cJSON_AddItemToObject(html, "stepsPerRevolution", cJSON_CreateNumber(200));
	cJSON_AddItemToObject(html, "threadPerInch", cJSON_CreateNumber(20));
	cJSON_AddItemToArray(html, jsonDoubleField("breathTime",2,1,300,10) );
	cJSON_AddItemToArray(html, jsonCheckbox("gridShow",true) );

	cJSON_AddItemToObject(root, "html", html);

	return root;
};


int B9CreatorSettings::update(cJSON* jsonNew, cJSON* jsonOld, int changes=NO){
	cJSON* nhtml = cJSON_GetObjectItem(jsonNew,"html");
	cJSON* ohtml = jsonOld==NULL?NULL:cJSON_GetObjectItem(jsonOld,"html");

	lock();
	if( nhtml != NULL){

		if( JsonConfig::update(nhtml,ohtml,"stepsPerRevolution",&m_spr) ) changes|=MARGIN;
		if( JsonConfig::update(nhtml,ohtml,"threadPerInch",&m_tpi) ) changes|=MARGIN;
		if( JsonConfig::update(nhtml,ohtml,"breathTime",&m_breathTime) ) changes|=MARGIN;
		if( JsonConfig::updateCheckbox(nhtml,ohtml,"gridShow",&m_gridShow) ) changes|=MARGIN;

		//call signal
		//updateSig(this,changes);
		
	}
	unlock();

	return changes!=NO>0?1:0;
}
