/*
 * Class provides the Loading and Saving of json files.
 * If the json contains some keywords postprocessing extract
 * and convert the content in a struct. 
 */
#ifndef JSONCONFIG_H
#define JSONCONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "cJSON.h"
#include "Mutex.h"
#include "constants.h"

/*
 * Functioal for second parameter of loadConfigFile method.
 * Use a function of this type to create default values if file is not found.
 */
typedef cJSON* DefaultsType(void);

class JsonConfig{
	private:
		int clearConfig();
	protected:
		cJSON* m_pjson_root;
		//Mutex m_json_mutex;
		Mutex m_block_all;

	public:
		void lock();
		void unlock();
		JsonConfig():
			m_pjson_root(NULL),
			m_block_all()
		{
		};

		~JsonConfig(){
			clearConfig();
		};
	
		int setConfig(const char* json_str, int update);
		char* getConfig();//const;
		int loadConfigFile(const char* filename);
		int saveConfigFile(const char* filename);	
		/* 
		 * Create minimal json element.
		 */
		virtual cJSON* loadDefaults();
		virtual int update(cJSON* new_json, cJSON* old_json, int changes);
		cJSON* getJSON() {return m_pjson_root;};

		int init(const char* filename="")
		{
			return loadConfigFile(filename);
		};

	protected:
		 bool update(cJSON* jsonNew, cJSON* jsonOld,const char* id, int* val);
		 bool update(cJSON* jsonNew, cJSON* jsonOld,const char* id, double* val);
		 bool updateCheckbox(cJSON* jsonNew, cJSON* jsonOld,const char* id, bool* val);

	public:
		 /* Static helper functions */
		 /*static*/ const char* getString(cJSON* r, const char* string) const;
		 /*static*/ double getNumber(cJSON* r, const char* string) const;
		 /*static*/ cJSON* getArrayEntry(cJSON* arr, const char* string) const;
		 /*static*/ void setString(cJSON* r,const char* string, const char* value);

		 cJSON* jsonDoubleField(const char* id,
				 double val, double min,
				 double max, double diff,
				 bool readonly=0, const char* format="" , const char* parse="" );
		 cJSON* jsonIntField(const char* id,
				 int val, int min,
				 int max, int diff,
				 bool readonly=0, const char* format="" , const char* parse="" );
		 cJSON* jsonCheckbox(const char* id,
				 bool checked,
				 bool readonly=0, const char* format="" , const char* parse="" );
		 cJSON* jsonStateField(const char* id,
				 double val,
				 const char* format="" , const char* parse="" );

		/*
		 * Member based variants of helper functions.
		 */

		/* Access to string child nodes of root node.*/
		const char* getString(const char* string) const;

		/* Access to number child nodes of root node.*/
		double getNumber(const char* string) const;

		void setString(const char* string, const char* value);

		/* Do not trust range limitation of http request.
		 * Instead, use range limitation of old config.
		 */
		double doubleFieldValue(cJSON* ndf, cJSON* odf);
		inline int intFieldValue(cJSON* ndf, cJSON* odf);

};
/* End JsonConfig class */

//--------------------------------------------------------
//--------------------------------------------------------
//--------------------------------------------------------


#endif
