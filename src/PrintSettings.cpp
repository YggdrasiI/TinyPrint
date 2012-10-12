#include "PrintSettings.h"


/*
 * Special properties. This values can modified with the web interface.
 * I.e. angle of kinect, nmbr of areas, position of areas, minimal blob size.
 */
cJSON* PrintSettings::loadDefaults()
{
	cJSON* root = cJSON_CreateObject();	
	cJSON_AddStringToObject(root, "kind", "printSettings");

	cJSON* html = cJSON_CreateArray();	
	cJSON_AddItemToArray(html, jsonIntField("firstLayer",1,1,10000,100) );
	cJSON_AddItemToArray(html, jsonIntField("lastLayer",10000,1,10000,100) );
	cJSON_AddItemToArray(html, jsonDoubleField("normalLayerExposure",12,1,100,10) );
	cJSON_AddItemToArray(html, jsonIntField("nbrAttachLayers",3,1,100,10) );
	cJSON_AddItemToArray(html, jsonDoubleField("attachLayerExposure",60,1,300,10) );

	cJSON_AddItemToObject(root, "html", html);

	/* Point information for repoke 
	cJSON* areas = cJSON_CreateArray();	
	cJSON_AddItemToArray(areas, jsonArea(1,320.0,240.0,100.0) );
	cJSON_AddItemToObject(root, "areas", areas);
	*/
	return root;
}

int PrintSettings::update(cJSON* jsonNew, cJSON* jsonOld, int changes=NO){
	cJSON* nhtml = cJSON_GetObjectItem(jsonNew,"html");
	cJSON* ohtml = jsonOld==NULL?NULL:cJSON_GetObjectItem(jsonOld,"html");

	lock();
	if( nhtml != NULL){

		if( update(nhtml,ohtml,"firstLayer",&m_firstLayer) ) changes|=MARGIN;
		if( update(nhtml,ohtml,"lastLayer",&m_lastLayer) ) changes|=MARGIN;
		if( update(nhtml,ohtml,"normalLayerExposure",&m_normalLayerExposure) ) changes|=MARGIN;
		if( update(nhtml,ohtml,"nbrAttachLayers",&m_nbrAttchachLayers) ) changes|=MARGIN;
		if( update(nhtml,ohtml,"attachLayerExposure",&m_attachLayerExposure) ) changes|=MARGIN;

		//call signal
		//updateSig(this,changes);
	}
	unlock();

	return changes!=NO>0?1:0;
}

