/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <assert.h>
#include <sys/types.h>
#include <regex.h>
#include "JsonMessage.h"
#include "B9CreatorSettings.h"
#include "OnionServer.h"


int check_filename(const char *filename){
	regex_t regex;
	int reti, ret;
	char msgbuf[100];

	/* Compile regular expression */
	//reti = regcomp(&regex, "^[[:alnum:]]*\\.json$", 0);
	reti = regcomp(&regex, "^[[:alnum:]]*\\.b9j$", 0);
	if( reti ){ fprintf(stderr, "Could not compile regex\n"); return -1; }

	/* Execute regular expression */
	reti = regexec(&regex, filename, 0, NULL, 0);
	if( !reti ){
		ret= 1;
	}else if( reti == REG_NOMATCH ){
		ret= 0;
	}else{
		regerror(reti, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		ret= 0;
	}

	/* Free compiled regular expression if you want to use the regex_t again */
	regfree(&regex);

	return ret;
}

// This has to be extern, as we are compiling C++
extern "C"{
int index_html_template(void *p, onion_request *req, onion_response *res);
int b9creator_script_js_template(void *p, onion_request *req, onion_response *res);
int b9creator_settings_js_template(void *p, onion_request *req, onion_response *res);
}


/*
 * Parse data from client. Use actionid-arg to distinct different
 * cases.
 */
int update_data(void *p, onion_request *req, onion_response *res){
	/* Default reply is 'reload' which force reload
	 * of complete website. In mosty cases this string will replaced 
	 * by one of the signal handlers.
	 */
	std::string reply("reload");
	int actionid = atoi( onion_request_get_queryd(req,"actionid","0") );

	// Attention each signal handler wrote into the same string 'post_reply'
	((OnionServer*)p)->updateSignal(req, actionid, reply);

	onion_response_set_length(res, reply.size() );
	onion_response_write(res, reply.c_str(), reply.size() ); 
	return OCS_PROCESSED;
}

/*
 Returns json struct of current settings. If
 argument
 Check post values and then return template of index.html.
 (Or use *p for other callbacks (not implemented))
*/
int getB9CreatorSettings(void *p, onion_request *req, onion_response *res){
	const char* b9Creator = ((OnionServer*)p)->m_b9CreatorSettings.getConfig(true);
	size_t len = strlen( b9Creator );
	onion_response_set_length(res, (int) len);
	onion_response_write(res, b9Creator, (int) len); 
	return OCS_PROCESSED;
}

/*
 * Convert all enties of message queue into json code and send this file
 * to the client.
 */
int getPrinterMessages(void *p, onion_request *req, onion_response *res){

		Messages &q = ((OnionServer*)p)->m_b9CreatorSettings.m_queues;
		//VPRINT("Messages: %i\n", q.m_messageQueue.size() );
		cJSON* tmp = jsonMessages("serialMessages", q.m_messageQueue);
		const char* json_serialMessages = cJSON_Print( tmp );
		if( tmp != NULL ) cJSON_Delete(tmp);

		size_t len = strlen( json_serialMessages );
		onion_response_set_length(res, (int) len);
		onion_response_write(res, json_serialMessages, (int) len); 

	return OCS_PROCESSED;
}

/*
 Return raw file if found. Security risk?! Check of filename/path required?!
*/
int search_file(onion_dict *context, onion_request *req, onion_response *res){
	//const char* path = onion_request_get_path(req);//empty?!
	const char* path = onion_request_get_fullpath(req);
	printf("Request of %s %i.\n",path, strlen(path));
	char filename[strlen(path)+8];
	//sprintf(filename,"./%s",path);
	sprintf(filename,"./html/%s",path);

		//read file 
	if( FILE *f=fopen(filename,"rb") ){
		fseek(f,0,SEEK_END);
		long len=ftell(f);
		fseek(f,0,SEEK_SET);
		char *data=(char*)malloc(len+1);
		fread(data,1,len,f);
		fclose(f);

		if (context) onion_dict_add(context, "LANG", onion_request_get_language_code(req), OD_FREE_VALUE);
		onion_response_set_length(res, len);
		onion_response_write(res, data, len); 
		if (context) onion_dict_free(context);

		free(data);
	}else{
		onion_response_set_length(res, 24);
		onion_response_write(res, "<h1>File not found</h1>", 24); 
	}
	return OCS_PROCESSED;
}

/*
 Replace some template variables and send b9creator_settings.js
*/
int insert_json(void *data, onion_request *req, onion_response *res, void* foo, void* datafree)
{
 //	printf("Pointer in callback: %p %p %p)\n",data,p,datafree);
onion_dict *d=onion_dict_new();
if( data != NULL){
	//onion_dict_add(d, "ONION_JSON",((JsonConfig*)data)->getConfig(),0);
	onion_dict_add(d, "ONION_JSON",((OnionServer*)data)->m_b9CreatorSettings.getConfig(),0);
}
//onion_dict_add(d, "user", user, OD_DICT|OD_FREE_VALUE);

return b9creator_settings_js_template(d, req, res);
}

/*
 Replace some template variables (filename of last config) call index_html_template
*/
int index_html(void *data, onion_request *req, onion_response *res, void* foo, void* datafree)
{
 //	printf("Pointer in callback: %p %p %p)\n",data,p,datafree);
onion_dict *d=onion_dict_new();
if( data != NULL){
	onion_dict_add(d, "LAST_SETTING_FILENAME",((JsonConfig*)data)->getString("lastSetting"),0);
}

return index_html_template(d, req, res);
}

/*+++++++++++++ OnionServer-Class ++++++++++++++++++ */
OnionServer::OnionServer(B9CreatorSettings &b9CreatorSettings ):
			//m_ponion( onion_new(O_THREADED|O_DETACH_LISTEN) ),
			m_ponion( onion_new(O_THREADED) ),
			m_pthread(),
			m_b9CreatorSettings(b9CreatorSettings)
		{
			//add default signal handler.
			updateSignal.connect(
					boost::bind(&OnionServer::updateSetting,this, _1, _2, _3)
					);
			//add signal handler of b9CreatorSettings.
			updateSignal.connect(
					boost::bind(&B9CreatorSettings::webserverUpdateConfig,&b9CreatorSettings, _1, _2, _3)
					);
			//start_server();
		}

int OnionServer::start_server()
{
	onion_url *url=onion_root_url(m_ponion);

	const char *host, *port;
	//if( &m_b9CreatorSettings != NULL){
		host = m_b9CreatorSettings.getString("host");
		port = m_b9CreatorSettings.getString("port");
	/*}else{
		host = "0.0.0.0";
		port = "8080";
	}*/

	onion_set_hostname(m_ponion, host); // Force ipv4.
	onion_set_port(m_ponion, port);
	//onion_url_add_with_data(url, "b9creator_settings.js", (void*)insert_json, m_pprintSetting, NULL);
	onion_url_add_with_data(url, "b9creator_settings.js", (void*)insert_json, this, NULL);
	onion_url_add_with_data(url, "index.html", (void*)index_html, &m_b9CreatorSettings, NULL);
	onion_url_add_with_data(url, "", (void*)index_html, &m_b9CreatorSettings, NULL);
	//onion_url_add_with_data(url, "index.html", (void*)getB9CreatorSettings, this, NULL);
	//onion_url_add_with_data(url, "", (void*)getB9CreatorSettings, this, NULL);
	onion_url_add_with_data(url, "update", (void*)update_data, this, NULL); /* <-- Recive data */
	onion_url_add_with_data(url, "json", (void*)getB9CreatorSettings, this, NULL); /* <-- Send data */
	onion_url_add_with_data(url, "messages", (void*)getPrinterMessages, this, NULL); /* <-- Send data */
	onion_url_add(url, "^.*$", (void*)search_file);

	/* Now, m_ponion get the O_DETACH_LISTEN flag on creation and
	   the Extra thread is omitable. */
	//start loop as thread
	return pthread_create( &m_pthread, NULL, &start_myonion_server, m_ponion);	
	//onion_listen(m_ponion);//loop

	return 0;
}

int OnionServer::stop_server()
{
	onion_listen_stop(m_ponion);//stop loop
	int i = pthread_join( m_pthread, NULL);//wait till loop ends
	onion_free(m_ponion);
	return i;
}

/* return value marks, if reply string contains data which should
 * return to the web client:
 * -2: No data written into reply. Input generate error. Currently, it's not handled.
 * -1: No data written into reply. Input generate state which require reloading of web page.
 *  0: data written into reply
 *  1: No data written into reply, but input processed successful.*/
//TODO: Shift several cases of the switch into own signal handler.
void OnionServer::updateSetting(onion_request *req, int actionid, std::string &reply){
	VPRINT("Actionid: %i \n", actionid);
	switch(actionid){
		case 5:
			{ /* Toggle Display */
				const char* disp = onion_request_get_post(req,"display");
				m_b9CreatorSettings.lock();
				if( disp != NULL ){
					if( disp[0] == '2' )
						m_b9CreatorSettings.m_display = !m_b9CreatorSettings.m_display;
					else 
						m_b9CreatorSettings.m_display = (disp[0] == '1');
				}

				reply = m_b9CreatorSettings.m_display?"1":"0";
				m_b9CreatorSettings.unlock();
				//return 0; 
			}
			break;

		case 4:
			{ /* Command Message */
				const char* json_str = onion_request_get_post(req,"cmd");
				if( json_str != NULL){
					Messages &q = m_b9CreatorSettings.m_queues;
					std::string cmd(json_str); 
					q.add_command(cmd);	
					reply = "ok";
				}else{
					reply = "missing post variable 'cmd'";
				}
			}
			break;
		case 3:
			{ /* Quit */
				printf("Quitting...\n");
				m_b9CreatorSettings.lock();
				m_b9CreatorSettings.m_die = true;
				m_b9CreatorSettings.unlock();
				reply = "quit";
			}
			break;
		case 0:
		default:
			{
			}
			break;
	}

	//return 1; 
}





