#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "JsonConfig.h"


void JsonConfig::lock(){
	m_block_all.lock();
}

void JsonConfig::unlock(){
	m_block_all.unlock();
}

int JsonConfig::clearConfig()
{
	m_json_mutex.lock();
	if( m_pjson_root != NULL){
		cJSON_Delete(m_pjson_root);
		m_pjson_root = NULL;
	}
	m_json_mutex.unlock();
	return 0;
}


int JsonConfig::regenerateConfig()
{
		clearConfig();
		m_json_mutex.lock();
		m_pjson_root = genJson(); 
		m_json_mutex.unlock();
		return 0;
}

int JsonConfig::setConfig(const char* json_str, int changes)
{
	cJSON* pNewRoot = cJSON_Parse(json_str);
	if( pNewRoot == NULL ) return -1;
	/* Not call setConfig from construtor. update is virtual. Use init() instead.*/
	update(pNewRoot,m_pjson_root, changes);

	if( changes & PARSE_AGAIN ){
		//pNewRoot can not be used if other threads changed a variable.
		cJSON_Delete( pNewRoot );
		regenerateConfig();
	}else{
		clearConfig();//delete m_pjson_root
		m_json_mutex.lock();
		m_pjson_root = pNewRoot;
		m_json_mutex.unlock();
	}

	return 0;
}

char* JsonConfig::getConfig(bool regenerate)//const
{
	if( regenerate ) regenerateConfig();
	return cJSON_Print(m_pjson_root);
	//cJSON html_node = cJSON_GetObjectItem(m_pjson_root,"html");
	//return cJSON_Print(html_node);
}

int JsonConfig::loadConfigFile(const char* filename)
{
	clearConfig();
	if( FILE *f=fopen(filename,"rb") ){
		fseek(f,0,SEEK_END);
		long len=ftell(f);
		fseek(f,0,SEEK_SET);
		char *data=(char*)malloc(len+1);
		fread(data,1,len,f);
		fclose(f);

		setConfig(data, CONFIG);
		free(data);
	}else{
		printf("File %s not found. Use default values.\n",filename);
		loadDefaults();
		regenerateConfig();
		//update(m_pjson_root,NULL, CONFIG);
		return 1;
	}
	return 0;
}

int JsonConfig::saveConfigFile(const char* filename)
{
	FILE *file;
	file = fopen(filename,"w");
	char* conf = getConfig();
	fprintf(file,"%s", conf );
	free(conf);
	fclose(file); 
	return 0;
}

void JsonConfig::loadDefaults(){
	VPRINT("Should not execute __FILE__ , __LINE__ .\n");
}

cJSON* JsonConfig::genJson()
{
cJSON* root = cJSON_CreateObject();	
cJSON_AddItemToObject(root, "kind", cJSON_CreateString("unknown"));
VPRINT("Should not execute __FILE__ , __LINE__ .\n");
return root;
}


int JsonConfig::update(cJSON* new_json, cJSON* old_json, int changes)
{
	VPRINT("Error (JsonConfig): Parent update method called.\n");
	return 0;
};

const char* JsonConfig::getString(cJSON* r, const char* string) const{
	cJSON* obj = 	cJSON_GetObjectItem(r,string);
	if( obj != NULL && obj->type == cJSON_String)
		return obj->valuestring;
	else{
		printf("JsonConfig: Object %s not found.\n",string);
		static const char* notfound = "not found";
		return notfound;
	}
}; 

double JsonConfig::getNumber(cJSON* r, const char* string) const{
	cJSON* obj = 	cJSON_GetObjectItem(r,string);
	if( obj != NULL && obj->type == cJSON_Number)
		return obj->valuedouble;
	else{
		printf("JsonConfig: Object %s not found.\n",string);
		return -1;
	}
};

cJSON* JsonConfig::getArrayEntry(cJSON* arr, const char* string) const{
	cJSON *tmp, *id;
	for(int i=0,n=cJSON_GetArraySize(arr);i<n;i++){
		tmp = 	cJSON_GetArrayItem(arr,i);
		id = 	cJSON_GetObjectItem(tmp,"id");
		if(id!=NULL && !strcmp(id->valuestring,string)) return tmp;
	}
	return NULL;
};
		

/*
 * json representation of extended html input field.
 * 
 * diff - deprached value. Controlls value change on click on "+","-"
 * readonly - Flag to set value readonly.
 * format - Set name of javascript function to format display style of val.
 * parse - Parse string to value (inverse operation to format)
 */
cJSON* JsonConfig::jsonDoubleField(const char* id,
		double val, double min,
		double max, double diff,
		bool readonly, const char* format , const char* parse 
		){
	cJSON* df = cJSON_CreateObject();
	cJSON_AddStringToObject(df, "type", "doubleField");
	cJSON_AddStringToObject(df, "id", id);
	cJSON_AddNumberToObject(df, "val", val );
	cJSON_AddNumberToObject(df, "min", min );
	cJSON_AddNumberToObject(df, "max", max );
	cJSON_AddNumberToObject(df, "diff", diff );
	cJSON_AddNumberToObject(df, "readonly", readonly?1:0 );
	cJSON_AddStringToObject(df, "format", format);
	cJSON_AddStringToObject(df, "parse", parse);

	return df;
}

cJSON* JsonConfig::jsonIntField(const char* id,
		int val, int min,
		int max, int diff,
		bool readonly, const char* format , const char* parse 
		){
	cJSON* df = cJSON_CreateObject();
	cJSON_AddStringToObject(df, "type", "intField");
	cJSON_AddStringToObject(df, "id", id);
	cJSON_AddNumberToObject(df, "val", val );
	cJSON_AddNumberToObject(df, "min", min );
	cJSON_AddNumberToObject(df, "max", max );
	cJSON_AddNumberToObject(df, "diff", diff );
	cJSON_AddNumberToObject(df, "readonly", readonly?1:0 );
	cJSON_AddStringToObject(df, "format", format);
	cJSON_AddStringToObject(df, "parse", parse);

	return df;
}

cJSON* JsonConfig::jsonCheckbox(const char* id,
		bool checked,
		bool readonly, const char* format , const char* parse 
		){
	cJSON* df = cJSON_CreateObject();
	cJSON_AddStringToObject(df, "type", "checkboxField");
	cJSON_AddStringToObject(df, "id", id);
	cJSON_AddNumberToObject(df, "val", checked?1:0 );
	cJSON_AddNumberToObject(df, "readonly", readonly?1:0 );
	cJSON_AddStringToObject(df, "format", format);
	cJSON_AddStringToObject(df, "parse", parse);

	return df;
}

/* Similar to jsonIntField, but for formatted values. */
cJSON* JsonConfig::jsonStateField(const char* id,
		double val,
		const char* format , const char* parse 
		){
	cJSON* df = cJSON_CreateObject();
	cJSON_AddStringToObject(df, "type", "stateField");
	cJSON_AddStringToObject(df, "id", id);
	cJSON_AddNumberToObject(df, "val", val );
	cJSON_AddStringToObject(df, "format", format);
	cJSON_AddStringToObject(df, "parse", parse);

	return df;
}

void JsonConfig::setString(const char* string, const char* value){
	m_json_mutex.lock();
	setString(m_pjson_root, string, value);
	m_json_mutex.unlock();
	return;
}

void JsonConfig::setString(cJSON* r,const char* string, const char* value){
	//cJSON* old = 	cJSON_GetObjectItem(r,string);
	cJSON_ReplaceItemInObject(r, string, cJSON_CreateString(value) );
	/*if( old != NULL ){
		cJSON_Delete(old); //already done in ReplaceItem
		old = NULL;
		}*/
	return;
}

/* Access to string child nodes of root node.*/
const char* JsonConfig::getString(const char* string) const{
	return getString(m_pjson_root, string);
}

/* Access to number child nodes of root node.*/
double JsonConfig::getNumber(const char* string) const{
	return getNumber(m_pjson_root, string);
}

double JsonConfig::doubleFieldValue(cJSON* ndf, cJSON* odf){
	return min(max(getNumber(odf,"min"),getNumber(ndf,"val")),getNumber(odf,"max"));
}

inline int JsonConfig::intFieldValue(cJSON* ndf, cJSON* odf){
	return (int)doubleFieldValue(ndf,odf);
}

bool JsonConfig::update(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val){
	double tmp = *val;
	bool ret;
	ret = update(jsonNew, jsonOld, id, &tmp);
	*val = (int)tmp;
	return ret;
}

bool JsonConfig::update(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val){
	cJSON* ntmp = getArrayEntry(jsonNew,id);
	cJSON* otmp;
	bool ret(false);
	//VPRINT("update of %s:",id);				
	double nval=0.0, oval=*val;
	if( jsonOld != NULL && NULL != (otmp=getArrayEntry(jsonOld,id)) ){
		oval = getNumber(otmp,"val");//probably redundant.
		nval = doubleFieldValue(ntmp,otmp);
		if(oval!=nval){
			/* Attention. The case 'oval!=*val' indicates changes of this property
			 * by an other thread/operation. nval OVERWRITE *val. This could
			 * be problematic in some special cases. */
			*val = nval;
			ret=true;
		}
	}else if( ntmp != NULL){
		nval = doubleFieldValue(ntmp,ntmp);
		if(*val != nval){
			*val = nval;
			ret = true;
		}
	}
	//VPRINT(" %f\n",nval);				
	
	return ret;
}

bool JsonConfig::updateCheckbox(cJSON* jsonNew, cJSON* jsonOld,const char* id, bool* val){
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
	*val = nval!=0.0;
	return ret;
}
