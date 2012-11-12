#include <vector>
#include "B9CreatorSettings.h"
#include "JobFile.h"
#include "OnionServer.h"

using namespace std;

B9CreatorSettings::B9CreatorSettings() :
	JsonConfig(),
	m_spr(200), m_tpi(20),
	m_gridShow(true),
	m_display(false),
//	m_redraw(false),
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
	m_readyForNextCycle(false),
	m_printProp(),
	m_jobState(IDLE),
	m_connected(false),
	m_flipSprites(true),
	m_files(),
	m_currentDisplayedImage(),
	m_die(false)
{
	//m_PU = 100 * 254 / (m_spr * m_tpi) ;
	m_PU = 10000 * 254 / (m_spr * m_tpi) ;

	m_printProp.m_breathTime = 2.0;
	m_printProp.m_releaseCycleTime = 1.75;
	m_printProp.m_exposureTime = 12;
	m_printProp.m_exposureTimeAL = 40;
	m_printProp.m_nmbrOfAttachedLayers = 4;
	m_printProp.m_overcureTime = 2;
	m_printProp.m_currentLayer = 0;
	m_printProp.m_nmbrOfLayers = 10;
	m_printProp.m_lockTimes = false;
	m_printProp.m_zResolution = 50;
	m_printProp.m_xyResolution = 100;
};

B9CreatorSettings::~B9CreatorSettings(){
	clearJobs();
}


void B9CreatorSettings::clearJobs(){
	//clear job file vector.
	vector<JobFile*>::iterator it = m_files.begin();
	const vector<JobFile*>::const_iterator it_end = m_files.end();
	for( ; it<it_end ; ++it ){
		delete (*it);
	}
	m_files.clear();
}


/*
 * Generate json struct of properties. Use the subnode 'html'
 * for values which can be modified on the web interface.
 */
cJSON *B9CreatorSettings::genJson()
{
	cJSON *root = cJSON_CreateObject();	
	/* Kind only used to distinct different json structs. */
	cJSON_AddItemToObject(root, "kind", cJSON_CreateString("b9CreatorSettings"));

	cJSON_AddItemToObject(root, "host", cJSON_CreateString(m_host.c_str() ));
	cJSON_AddItemToObject(root, "port", cJSON_CreateString(m_port.c_str() ));
	cJSON_AddItemToObject(root, "jobDir", cJSON_CreateString(m_b9jDir.c_str() ));
	cJSON_AddItemToObject(root, "comPort", cJSON_CreateString(m_comPort.c_str() ));
	cJSON_AddItemToObject(root, "comBaudrate", cJSON_CreateNumber(m_comBaudrate));
//	cJSON_AddItemToObject(root, "shutterEquipped", cJSON_CreateNumber(0));
//	cJSON_AddItemToObject(root, "lampHours", cJSON_CreateNumber(-1));

	char gcol[20];
	sprintf(gcol,"%02X%02X%02X\0",m_gridColor[0],m_gridColor[1],m_gridColor[2]);
	//gcol[6] = '\0';
	cJSON_AddItemToObject(root, "gridColor", cJSON_CreateString(gcol));//hex string

	/* sub node. This values will transmitted to web interface */
	cJSON *html = cJSON_CreateArray();	
	cJSON_AddItemToArray(html, jsonIntField("stepsPerRevolution",m_spr,36,1000,100,1) );
	cJSON_AddItemToArray(html, jsonIntField("threadPerInch",m_tpi,1,100,10,1) );
	cJSON_AddItemToArray(html, jsonCheckbox("gridShow",m_gridShow) );

	cJSON_AddItemToArray(html, jsonDoubleField("breathTime",m_printProp.m_breathTime,0.1,300,0.2,0 ) );
	cJSON_AddItemToArray(html, jsonDoubleField("releaseCycleTime",m_printProp.m_releaseCycleTime,0.1,300,0.2, 0/*m_printProp.m_lockTimes*/ ) );
	cJSON_AddItemToArray(html, jsonDoubleField("exposureTime",m_printProp.m_exposureTime,0.1,300,0.2,0 ) );
	cJSON_AddItemToArray(html, jsonDoubleField("exposureTimeAL",m_printProp.m_exposureTimeAL,0.1,300,0.2,m_printProp.m_lockTimes ) );
	cJSON_AddItemToArray(html, jsonIntField("nmbrOfAttachedLayers",m_printProp.m_nmbrOfAttachedLayers,0,40,1,m_printProp.m_lockTimes ) );
	cJSON_AddItemToArray(html, jsonDoubleField("overcureTime",m_printProp.m_overcureTime,0.0,100,0.2,0 ) );
	cJSON_AddItemToArray(html, jsonIntField("zResolution",m_printProp.m_zResolution,25,200,10,m_printProp.m_lockTimes ) );
	cJSON_AddItemToArray(html, jsonIntField("xyResolution",m_printProp.m_xyResolution,25,200,10, 1) );
	cJSON_AddItemToArray(html, jsonIntField("currentLayer",
				min(m_printProp.m_currentLayer,m_printProp.m_nmbrOfLayers-1),0,m_printProp.m_nmbrOfLayers-1,1,m_printProp.m_lockTimes) );

	cJSON_AddItemToArray(html, jsonStateField("vatOpen",m_vatOpen,"percent","percent") );//in Percent
	cJSON_AddItemToArray(html, jsonStateField("projectorStatus",m_projectorStatus,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("resetStatus",m_resetStatus,"token","token") );
	cJSON_AddItemToArray(html, jsonStateField("zHeight_mm",m_zHeight*m_PU/1000.0,"mm","mm") ); // height in mm.
	cJSON_AddItemToArray(html, jsonStateField("jobState",m_jobState,"state","token") );
	cJSON_AddItemToArray(html, jsonStateField("displayStatus",m_display,"token","token") );

	cJSON_AddItemToArray(html, jsonFilesField("files",m_files) );
	cJSON_AddItemToArray(html, jsonCheckbox("flipSprites",m_flipSprites) );

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
	m_shutterEquipped = false;
	m_projectorEquipped= false;
	m_lampHours = -1;
	m_readyForNextCycle = false;

	/* sub node. This values will transmitted to web interface */
	m_spr = 200;
	m_tpi = 20;
	m_gridShow = true;
	m_vatOpen = -100;
	m_projectorStatus = 2;
	m_resetStatus = 1;
	m_zHeight = -1;
	m_zHome = -1;
	m_printProp.m_breathTime = 2.0;
	m_printProp.m_releaseCycleTime = 1.75;
	m_printProp.m_exposureTime = 12;
	m_printProp.m_exposureTimeAL = 40;
	m_printProp.m_nmbrOfAttachedLayers = 4;
	m_printProp.m_overcureTime = 2;
	m_printProp.m_currentLayer = 0;
	m_printProp.m_nmbrOfLayers = 10;
	m_printProp.m_zResolution = 50;
	m_printProp.m_xyResolution = 100;
	m_printProp.m_lockTimes = false;
	m_jobState = IDLE;
	m_flipSprites = true;
};

/*
 * replaces |=YES with |=XYZ to extend changes flag.
 * It's could be useful to detect special updates, conflicts...
 */
int B9CreatorSettings::update(cJSON *jsonNew, cJSON *jsonOld, int changes){
	cJSON *nhtml = cJSON_GetObjectItem(jsonNew,"html");
	cJSON *ohtml = jsonOld==NULL?NULL:cJSON_GetObjectItem(jsonOld,"html");

	lock();

	/*load values outside of the html node. This valus should only
	* read from config files.
	*/
	if( changes & CONFIG ){
		m_host = JsonConfig::getString(jsonNew,"host");
		m_port = JsonConfig::getString(jsonNew,"port");
		m_b9jDir = JsonConfig::getString(jsonNew,"jobDir");

		m_comPort = JsonConfig::getString(jsonNew,"comPort");
		m_comBaudrate =  (int) JsonConfig::getNumber(jsonNew,"comBaudrate"); 
	}

	if( nhtml != NULL){

		if(false && (changes & ALL) ){
			//This values are set up on other placed and should not read
			//from json structs. I added the lines just for your information.
			double tmp = m_zHeight*m_PU/1000.0;
			if( updateState(nhtml,ohtml,"zHeight_mm",&tmp) ){
				m_zHeight = tmp*1000.0/m_PU;
				changes|=YES;
			}
			if( updateState(nhtml,ohtml,"vatOpen",&m_vatOpen) ) changes|=YES;
			if( updateState(nhtml,ohtml,"projectorStatus",&m_projectorStatus) ) changes|=YES;
			if( updateState(nhtml,ohtml,"resetStatus",&m_resetStatus) ) changes|=YES;
		}

		if(changes & CONFIG ){		
			if( JsonConfig::update(nhtml,ohtml,"stepsPerRevolution",&m_spr) 
					|| JsonConfig::update(nhtml,ohtml,"threadPerInch",&m_tpi) ){
				m_PU = 10000 * 254 / (m_spr * m_tpi) ;
				changes|=YES;
			}
			if( updateState(nhtml,ohtml,"zHeightLimit",&m_zHeightLimit) ) changes|=YES;
		
			//parse color hex string
			int color;
			sscanf( JsonConfig::getString(jsonNew,"gridColor"), "%x ", &color );
			m_gridColor[0] = max(0,min( (color>>16) & 0xFF  ,255));//red bits
			m_gridColor[1] = max(0,min( (color>>8) & 0xFF  ,255));//green bits
			m_gridColor[2] = max(0,min( (color>>0) & 0xFF  ,255));//blue bits
			
		}

		if( JsonConfig::updateCheckbox(nhtml,ohtml,"gridShow",&m_gridShow) ){
			changes|=REDRAW;
		}

		if( JsonConfig::update(nhtml,ohtml,"breathTime",&m_printProp.m_breathTime) ) changes|=YES;
		if( JsonConfig::update(nhtml,ohtml,"exposureTime",&m_printProp.m_exposureTime) ) changes|=YES;
		if( JsonConfig::update(nhtml,ohtml,"overcureTime",&m_printProp.m_overcureTime) ) changes|=YES;
		if( JsonConfig::update(nhtml,ohtml,"releaseCycleTime",&m_printProp.m_releaseCycleTime) ){
			changes|=YES;
#ifdef VERBOSE
			std::cout << "Update release cycle time to " << m_printProp.m_releaseCycleTime << std::endl;
#endif
			std::ostringstream cmd_cycle;
			cmd_cycle << "D" << (int)(1000*m_printProp.m_releaseCycleTime);
			std::string cmd_cycleStr(cmd_cycle.str()); 
			m_queues.add_command(cmd_cycleStr);	
		}
		if(! m_printProp.m_lockTimes ){
			if( JsonConfig::update(nhtml,ohtml,"exposureTimeAL",&m_printProp.m_exposureTimeAL) ) changes|=YES;
			if( JsonConfig::update(nhtml,ohtml,"nmbrOfAttachedLayers",&m_printProp.m_nmbrOfAttachedLayers) ) changes|=YES;
			if( JsonConfig::update(nhtml,ohtml,"currentLayer",&m_printProp.m_currentLayer) ){
				//m_redraw = true;
				changes|=LAYER; //should force redraw by JobManager
			}
			if( JsonConfig::update(nhtml,ohtml,"zResolution",&m_printProp.m_zResolution) ) changes|=YES;
			if( JsonConfig::update(nhtml,ohtml,"xyResolution",&m_printProp.m_xyResolution) ) changes|=YES;

			if( JsonConfig::updateCheckbox(nhtml,ohtml,"flipSprites",&m_flipSprites) ){
				changes|=LAYER;
			}

			int c2;

			//unlock here because updateFiles can extend m_files vector.
			unlock();
			if( c2 = B9CreatorSettings::updateFiles(nhtml,ohtml,"files",
						m_files, (changes & CONFIG ) )
				){
				changes|=c2;
				updateMaxLayer();
				//m_redraw = true;
			}
			lock();

		} 

	}
	unlock();

	//call update signal
	updateSettings(changes);	

	return changes!=NO?1:0;
}

/* Update of val without argument checking. */
bool B9CreatorSettings::updateState(cJSON *jsonNew, cJSON *jsonOld,const char* id, int* val){
	double tmp=*val;
	bool ret = updateState(jsonNew,jsonOld,id,&tmp);
	*val = (int)tmp;
	return ret;
}
bool B9CreatorSettings::updateState(cJSON *jsonNew, cJSON *jsonOld,const char* id, double* val){
	cJSON *ntmp = getArrayEntry(jsonNew,id);
	cJSON *otmp;
	bool ret(false);
	//VPRINT("update of %s:",id);				
	double nval=0.0, oval=*val;
	if( jsonOld != NULL && NULL != (otmp=getArrayEntry(jsonOld,id)) ){
		oval = getNumber(otmp,"val");
		nval = getNumber(ntmp,"val");
		if(oval!=nval){
			/* Attention. The case 'oval!=*val' indicates changes of this property
			 * by an other thread/operation. nval OVERWRITE *val. This could
			 * be problematic in some special cases. */
			*val = nval;
			ret=true;
		}
	}else if( ntmp != NULL){
		nval = getNumber(ntmp,"val");
		*val = nval;
		ret = true;
	}
	//VPRINT(" %f\n",nval);				
	return ret;
}


bool B9CreatorSettings::webserverUpdateConfig(onion_request *req, int actionid, onion_response *res){
	if( actionid == 0 ){
		VPRINT("update b9CreatorSettings values\n");
		const char* json_str = onion_request_get_post(req,"b9CreatorSettings");
		if( json_str != NULL){
			setConfig(json_str, WEB_INTERFACE|PARSE_AGAIN);
			//hm, send update signal here with some flags which mark changes?
		}
		std::string reply = "ok";
		onion_response_write(res, reply.c_str(), reply.size() ); 
		return true;
	}
return false;
}

/* See Header for generated structure of json struct */
cJSON *B9CreatorSettings::jsonFilesField(const char* id, std::vector<JobFile*> files){

	cJSON *jsonf = cJSON_CreateObject();	

	cJSON_AddStringToObject(jsonf, "type", "filesField");
	cJSON_AddStringToObject(jsonf, "id", id);
	//cJSON_AddNumberToObject(jsonf, "val", val );
	cJSON_AddStringToObject(jsonf, "format", "files");
	cJSON_AddStringToObject(jsonf, "parse", "files");

	cJSON *fa = cJSON_CreateArray();	

	int i = 0;
	vector<JobFile*>::iterator it = files.begin();
	const vector<JobFile*>::const_iterator it_end = files.end();
	for( ; it<it_end ; ++it,++i ){

		cJSON *file = cJSON_CreateObject();	
		cJSON_AddStringToObject(file, "filename", (*it)->m_filename.c_str() );
		cJSON_AddStringToObject(file, "description", (*it)->m_description.c_str() );

		std::ostringstream maxL; maxL << "file" << i << "_maxLayer";
		std::ostringstream minL; minL << "file" << i << "_minLayer";
		std::ostringstream positionX; positionX << "file" << i << "_positionX";
		std::ostringstream positionY; positionY << "file" << i << "_positionY";
		std::ostringstream scaleId; scaleId << "file" << i << "_scale";

		cJSON *html = cJSON_CreateArray();
		cJSON_AddItemToArray(html, jsonIntField(maxL.str().c_str(),
					(*it)->m_maxLayer,
					(*it)->m_minLayer,
					(*it)->m_nmbrOfLayers-1,
					1,m_printProp.m_lockTimes )
				);
		cJSON_AddItemToArray(html, jsonIntField(minL.str().c_str(),
					(*it)->m_minLayer,
					0,
					(*it)->m_maxLayer,
					1,m_printProp.m_lockTimes )
				);
		cJSON_AddItemToArray(html, jsonIntField(positionX.str().c_str(),
					(*it)->m_position.x,
					-(*it)->m_size.width,
					1024+(*it)->m_size.width,
					10, m_printProp.m_lockTimes )
				);
		cJSON_AddItemToArray(html, jsonIntField(positionY.str().c_str(),
					(*it)->m_position.y,
					-(*it)->m_size.height,
					768+(*it)->m_size.height,
					10, m_printProp.m_lockTimes )
				);
		cJSON_AddItemToArray(html, jsonDoubleField(scaleId.str().c_str(),
					(*it)->getScale(),
					0.01,
					1000.0,
					1, m_printProp.m_lockTimes )
				);

		cJSON_AddItemToObject(file, "html", html);
		cJSON_AddItemToArray(fa, file );
	}

	cJSON_AddItemToObject(jsonf,"filearray", fa );
	return jsonf;
}

/*
 * Update files[i] with the i-th entry
 * of the json struct. (There is no check
 * if orders of the input the enties match)
 * */
/* See Header for generated structure of json struct */
int B9CreatorSettings::updateFiles(cJSON *jsonNew, cJSON *jsonOld, 
		const char* id,
		vector<JobFile*> &files,
		bool add ){

	int ret=NO;//Combination of values of enum Changes

	if( jsonNew == NULL ) return ret;
	if( jsonOld == NULL || add ) jsonOld = jsonNew;

	/* The desired data are four levels under jsonNew.
	 * Levels: 
	 * 	json[New|Old],
	 * 	jsonfiles[New|Old]
	 *	jsonfile[New|Old]
	 *	html[New|Old]
	 * */

	cJSON *jsonfilesNew = getArrayEntry(jsonNew,id);
	cJSON *jsonfilesOld = getArrayEntry(jsonOld,id);
	if( jsonfilesNew == NULL || jsonfilesOld == NULL ) return ret;

	cJSON *jsonfilearrayNew = cJSON_GetObjectItem(jsonfilesNew,"filearray");
	cJSON *jsonfilearrayOld = cJSON_GetObjectItem(jsonfilesOld,"filearray");
	if( jsonfilearrayNew == NULL || jsonfilearrayOld == NULL ) return ret;


	if( add ){
		int nsize = cJSON_GetArraySize( jsonfilearrayNew );
		printf("New files: %i\n",nsize);
		for( int i=0; i<nsize; ++i ){
			cJSON *jsonfileNew = cJSON_GetArrayItem(jsonfilearrayNew, i);
			if( jsonfileNew == NULL ) continue;

			std::string filename  = JsonConfig::getString(jsonfileNew,"filename");
			std::string description  = JsonConfig::getString(jsonfileNew,"description");

			if( loadJob( filename.c_str() ) != 0){
				//loading failed
				continue;
			}

			//get new element
			JobFile *jf = m_files.back();

			ret|=REDRAW|YES;

			//replace the default position, scale, ... with the
			//values in the json struct.
			jf->m_description = description;
			cJSON *htmlNew = cJSON_GetObjectItem(jsonfileNew, "html");

			if( htmlNew == NULL ) continue;

			std::ostringstream maxL; maxL << "file" << i << "_maxLayer";
			std::ostringstream minL; minL << "file" << i << "_minLayer";
			std::ostringstream positionX; positionX << "file" << i << "_positionX";
			std::ostringstream positionY; positionY << "file" << i << "_positionY";
			std::ostringstream scaleId; scaleId << "file" << i << "_scale";

			double scale;
			JsonConfig::update(htmlNew,NULL,maxL.str().c_str(),&jf->m_maxLayer);
			JsonConfig::update(htmlNew,NULL,minL.str().c_str(),&jf->m_minLayer);

			if( JsonConfig::update(htmlNew,NULL,scaleId.str().c_str(),&scale ) ){
				if( scale != jf->getScale() )
					jf->setScale(scale);
			}

			JsonConfig::update(htmlNew,NULL,positionX.str().c_str(),&jf->m_position.x);
			JsonConfig::update(htmlNew,NULL,positionY.str().c_str(),&jf->m_position.y);

		}

	}else{

		for( int i=0; i<files.size(); ++i ){
			JobFile *file = files[i];
			cJSON *jsonfileNew = cJSON_GetArrayItem(jsonfilearrayNew, i);
			cJSON *jsonfileOld = cJSON_GetArrayItem(jsonfilearrayOld, i);
			if( jsonfileNew == NULL || jsonfileOld == NULL ) continue;

			cJSON *htmlNew = cJSON_GetObjectItem(jsonfileNew, "html");
			cJSON *htmlOld = cJSON_GetObjectItem(jsonfileOld, "html");
			if( htmlNew == NULL || htmlOld == NULL ) continue;

			std::ostringstream maxL; maxL << "file" << i << "_maxLayer";
			std::ostringstream minL; minL << "file" << i << "_minLayer";
			std::ostringstream positionX; positionX << "file" << i << "_positionX";
			std::ostringstream positionY; positionY << "file" << i << "_positionY";
			std::ostringstream scaleId; scaleId << "file" << i << "_scale";

			if( JsonConfig::update(htmlNew,htmlOld,maxL.str().c_str(),&file->m_maxLayer) ) ret|=YES;
			if( JsonConfig::update(htmlNew,htmlOld,minL.str().c_str(),&file->m_minLayer) ) ret|=YES;

			// position shift need no evaluation of layer. It just moved the displayed sprites.
			if( JsonConfig::update(htmlNew,htmlOld,positionX.str().c_str(),&file->m_position.x) ) ret|=REDRAW;
			if( JsonConfig::update(htmlNew,htmlOld,positionY.str().c_str(),&file->m_position.y) ) ret|=REDRAW;

			double scale = file->getScale();
			if( JsonConfig::update(htmlNew,htmlOld,scaleId.str().c_str(),&scale ) ){
				file->setScale(scale);
				ret|=LAYER;
			}

			//for data consistency
			if( file->m_minLayer > file->m_maxLayer ) file->m_minLayer = 0;
		}
	}

	return ret;
}

int B9CreatorSettings::updateMaxLayer(){
	int nmbrOfLayers = 0;
	vector<JobFile*>::iterator it = m_files.begin();
	const vector<JobFile*>::const_iterator it_end = m_files.end();
	for( ; it<it_end ; ++it ){
		nmbrOfLayers = max(nmbrOfLayers,
				(*it)->m_maxLayer - (*it)->m_minLayer + 1 ); 
	}
	//lock();
	m_printProp.m_nmbrOfLayers = nmbrOfLayers;
	if( m_printProp.m_currentLayer  > nmbrOfLayers )
		m_printProp.m_currentLayer = nmbrOfLayers;
	//unlock();

}


int B9CreatorSettings::loadJob(const std::string filename){

	lock();

	std::string path(m_b9jDir);
	path.append("/");
	path.append(filename);

	JobFile *jf = NULL;

	try{
		if( check_svgExtension(filename.c_str()) ){
			jf = new JobFileSvg(path.c_str(), 1000/m_printProp.m_xyResolution);
		} else if( check_listExtension(filename.c_str()) ){
			jf = new JobFileList(path.c_str(), m_b9jDir.c_str() );
		} else if( check_b9jExtension(filename.c_str()) ){
			//TODO
		}
	}catch( Exceptions e){ //only possible exception here: load failed
		unlock();
		return -1;	
	}
	//check if loading failed
	if( jf == NULL ){
		unlock();
		return -1;
	}

	m_files.push_back(jf);

	//Substitute path with filename in jf
	jf->m_filename = filename;

	//Update the number layers which should
	//printed.
	updateMaxLayer();
	//update json
	regenerateConfig();

	unlock();

	return 0;
}

int B9CreatorSettings::unloadJob(const int index){

	if( index>= m_files.size() ) return -1; //wrong index
	if( m_printProp.m_lockTimes ) return -2; //currently printing

	lock();

	JobFile *jf = m_files[index];
	VPRINT("Unload %i. Job (%s)\n",index, jf->m_filename);
	delete(jf);
	m_files.erase( m_files.begin() + index );

	//Update the number layers which should
	//printed.
	updateMaxLayer();
	//update json
	regenerateConfig();

	unlock();

	return 0;
}
