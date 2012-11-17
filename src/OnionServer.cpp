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
#include <iostream>
#include <fstream>
//#include <sys/types.h>

#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "boost/filesystem.hpp" 
//#include <istream>
namespace fs = boost::filesystem; 

#include "JsonMessage.h"
#include "B9CreatorSettings.h"
#include "OnionServer.h"


// This has to be extern, as we are compiling C++
extern "C"{
int index_html_template(void *p, onion_request *req, onion_response *res);
int b9creator_settings_js_template(void *p, onion_request *req, onion_response *res);
}


/*
 * Parse data from client. Use actionid-arg to distinct different
 * cases.
 */
int update_data(void *p, Onion::Request &req, Onion::Response &res){
	/* Default reply is 'reload' which force reload
	 * of complete website. In mosty cases this string will replaced 
	 * by one of the signal handlers.
	 */
	int actionid = atoi( onion_request_get_queryd(req.c_handler(), "actionid","0") );

	if( ! ((OnionServer*)p)->updateSignal( &req, actionid, &res) ){
		// Signal returns true if at least one handler writes into res.
		// Write default reply, if nothing was written.
		std::string reply("reload");
		res.write(reply.c_str(), reply.size() ); 
	}

	return OCS_PROCESSED;
}

/*
 * Returns json struct of current settings. 
 */
int getB9CreatorSettings(void *p, Onion::Request &req, Onion::Response &res){
	const char* b9Creator = ((OnionServer*)p)->m_b9CreatorSettings.getConfig(true);
	size_t len = strlen( b9Creator );
	res.write(b9Creator, (int) len ); 
	return OCS_PROCESSED;
}

/*
 Returns json struct of filenames in job files folder.
*/
int getJobFolder(void *p, Onion::Request &req, Onion::Response &res){

	std::string &folder = ((OnionServer*)p)->m_b9CreatorSettings.m_b9jDir;
	std::ostringstream json_reply;

	fs::path full_path( fs::initial_path<fs::path>() );
	full_path = fs::system_complete( fs::path( folder ) );

	unsigned long file_count = 0;

	json_reply << "{ \"name\" : \"" << folder << "\", \"content\" : [" ;

	if( !fs::exists( full_path ) ){
		std::cout << "Not found: " << full_path.filename() << std::endl;
		json_reply << "\"none\"";
	}else if ( !fs::is_directory( full_path ) ){
		std::cout << "Path is no directory: " << full_path.filename() << std::endl;
		json_reply << "\"none\"";
	}else{
		fs::directory_iterator end_iter;
		for ( fs::directory_iterator dir_itr( full_path );
				dir_itr != end_iter;
				++dir_itr )
		{
			try
			{
				if (! fs::is_directory( dir_itr->status() ) )
				{ //regluar file or symbolic link
					if( file_count ) json_reply << ", " << std::endl;
					json_reply << "{ \"" << file_count << "\":	" \
						<< dir_itr->path().filename() << " }";
					++file_count;
				}
				/*Remark: The index numbers are sourounded by "'s 
				 * to avoid problems on the javascript side.
				 * I.E. {"0" : "filename" }.
				 * */

			}
			catch ( const std::exception & ex )
			{
				std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
			}
		}
	}

	json_reply << "] }" ;

	std::string json_replyStr = json_reply.str();
	size_t len = json_replyStr.size();
	res.write(json_replyStr.c_str(), (int) len ); 
	return OCS_PROCESSED;
}


/* Like getJobFolder but with some prefix and suffix text
 * to get an *.js file.
 * */
int getJobFolderWrapped(void *p, Onion::Request &req, Onion::Response &res){
	res.write("json_job_files = ", 17); 
	int ret = getJobFolder(p, req, res);
	res.write(";", 1); 
	return ret;
}


/*
 * Convert all enties of message queue into json code and send this file
 * to the client.
 */
int getPrinterMessages(void *p, Onion::Request &req, Onion::Response &res){

		Messages &q = ((OnionServer*)p)->m_b9CreatorSettings.m_queues;
		//VPRINT("Messages: %i\n", q.m_messageQueue.size() );
		cJSON* tmp = jsonMessages("serialMessages", q.m_messageQueue);
		if( tmp != NULL ){
			const char* json_serialMessages = cJSON_Print( tmp );
			size_t len = strlen( json_serialMessages );
			res.write(json_serialMessages, (int) len); 

			cJSON_Delete(tmp);
			tmp = NULL;
		}else{
			const char* json_serialMessages = "(OnionServer) Serial Messages Error";
			size_t len = strlen( json_serialMessages );
			res.write(json_serialMessages, (int) len); 
		}

	return OCS_PROCESSED;
}


int preview(void *p, Onion::Request &req, Onion::Response &res){

//sendSignal with actionid=10 to get png image from
//DisplayManager.
	if( ! ((OnionServer*)p)->updateSignal(&req, 10, &res) ){
		//signals did not write into response. Write default reply.
		std::string reply("Could not generate Image.");
		res.write( reply.c_str(), reply.size() ); 
	}
	return OCS_PROCESSED;
}

/*
 Return raw file if found. Security risk?! Check of filename/path required?!
*/
int search_file(void *p, Onion::Request &req, Onion::Response &res){
	//const char* path = onion_request_get_path(req);//empty?!
	const char* path = onion_request_get_fullpath( req.c_handler() );
#ifdef VERBOSE
	printf("Request of %s.\n",path);
#endif
	std::string filename("./html/");
	filename.append(path);
	OnionServer *pOnionServer = (OnionServer*)p;

	//boost::iostreams::stream<boost::iostreams::file_source> file(filename.c_str());//.is_open() does not react like ifstream variant.
	std::ifstream file(filename.c_str());
	std::string line;

	if( file.is_open()){
		
		/* Create header with mime type and charset information for several file extensions.
		 * This is just a workaround. There should be an automatic mechanicm
		 * in libonion. */
		int periodPos = filename.find_last_of('.');
		std::string key("Content-Type");
		std::string mime = 
			pOnionServer->m_mimedict.get( filename.substr(periodPos+1), "text/html; charset: utf-8" ) ;
		res.setHeader(key,mime);
		//onion_response_write_headers(res);//??

		try{
			while (std::getline(file, line)) { 
				res.write( line.c_str(), line.size() ); 
				res.write("\n", 1 ); 
			}
		}//catch ( const boost::iobase::failure &ex ){
		catch ( const std::exception & ex ){
			std::cerr << "Can not read " << filename << std::endl;
			res.write( "<h1>Error while reading File.</h1>", 25); 
		}
	}else{
		res.write( "<h1>File not found.</h1>", 25); 
	}

	return OCS_PROCESSED;
	}

/*
 Replace some template variables and send b9creator_settings.js
*/
	/*
int getB9CreatorSettingsWrapped(void *data, onion_request *req, onion_response *res, void* foo, void* datafree)
{
 //	printf("Pointer in callback: %p %p %p)\n",data,p,datafree);
onion_dict *d=onion_dict_new();
if( data != NULL){
	//onion_dict_add(d, "ONION_JSON",((JsonConfig*)data)->getConfig(),0);
	onion_dict_add(d, "ONION_JSON",((OnionServer*)data)->m_b9CreatorSettings.getConfig(),0);
}
//onion_dict_add(d, "user", user, OD_DICT|OD_FREE_VALUE);

return b9creator_settings_js_template(d, req, res);
}*/

/*
 Replace some template variables (filename of last config) call index_html_template
*/
int OnionServer::index_html( Onion::Request &req, Onion::Response &res)
{
	Onion::Dict d;
	d.add("LAST_SETTING_FILENAME",m_b9CreatorSettings.m_configFilename,0);

return index_html_template(d.c_handler() , req.c_handler(), res.c_handler() );
}

/*+++++++++++++ OnionServer-Class ++++++++++++++++++ */
OnionServer::OnionServer(B9CreatorSettings &b9CreatorSettings ):
	m_onion( O_THREADED|O_DETACH_LISTEN ),
	m_url(m_onion),
	m_mimedict(),
	m_b9CreatorSettings(b9CreatorSettings) {
		m_onion.setTimeout(5000);

		//Set mime types dict
		m_mimedict.add( "html","text/html; charset: utf-8",0);
		m_mimedict.add( "css","text/css; charset: utf-8",0);
		m_mimedict.add( "js","application/javascript; charset: utf-8",0);
		m_mimedict.add( "png","image/png",0);


		//add default signal handler.
		updateSignal.connect(
				boost::bind(&OnionServer::updateWebserver,this, _1, _2, _3)
				);
		//add signal handler of b9CreatorSettings.
		updateSignal.connect(
				boost::bind(&B9CreatorSettings::webserverUpdateConfig,&b9CreatorSettings, _1, _2, _3)
				);
		//start_server();

	}

int OnionServer::start_server() {

	std::string host(m_b9CreatorSettings.getString("host"));
	std::string port(m_b9CreatorSettings.getString("port"));

	m_onion.setHostname(host);
	m_onion.setPort(port);

	std::string a = "index.html";
	//m_url.add(a, this, &OnionServer::index_html );

	//onion_url_add_with_data(url, "", (void*)index_html, &m_b9CreatorSettings, NULL);

	//dynamic content
	//onion_url_add_with_data(url, "b9creator_settings.js", (void*)getB9CreatorSettingsWrapped, this, NULL);
	//onion_url_add_with_data(url, "settings", (void*)getB9CreatorSettings, this, NULL); /* <-- Send data */
	//onion_url_add_with_data(url, "files.js", (void*)getJobFolderWrapped, this, NULL); /* <-- Send data */
	//onion_url_add_with_data(url, "files", (void*)getJobFolder, this, NULL); /* <-- Send data */
	//onion_url_add_with_data(url, "messages", (void*)getPrinterMessages, this, NULL); /* <-- Send data */
	//onion_url_add_with_data(url, "update", (void*)update_data, this, NULL); /* <-- Recive data */

	//onion_url_add_with_data(url, "preview.png", (void*)preview, this, NULL);//preview

	//static content
	//onion_url_add(url, "^.*$", (void*)search_file);
	//onion_url_add_with_data(url, "^.*$", (void*)search_file, this, NULL);

	//start loop as thread  (O_DETACH_LISTEN flag is set.)
	m_onion.listen();//loop

	return 0;
}

int OnionServer::stop_server()
{
	m_onion.listenStop();
	return 0;
}

/* return value marks, if reply string contains data which should
 * return to the web client:
 * -2: No data written into reply. Input generate error. Currently, it's not handled.
 * -1: No data written into reply. Input generate state which require reloading of web page.
 *  0: data written into reply
 *  1: No data written into reply, but input processed successful.*/
bool OnionServer::updateWebserver(Onion::Request *preq, int actionid, Onion::Response *pres){
	VPRINT("Actionid: %i \n", actionid);
	switch(actionid){
		case 4:
			{ /* Command Message */
				const char* json_str = onion_request_get_post(preq->c_handler(), "cmd");
				std::string reply;

				if( json_str != NULL){
					Messages &q = m_b9CreatorSettings.m_queues;
					std::string cmd(json_str); 
					q.add_command(cmd);	
					reply = "ok";
				}else{
					reply = "missing post variable 'cmd'";
				}

				pres->write(reply.c_str(), reply.size() ); 
				return true;
			}
			break;
		case 3:
			{ /* Quit */
				std::string reply("quit");
				pres->write(reply.c_str(), reply.size() ); 

				printf("Quitting...\n");
				m_b9CreatorSettings.lock();
				m_b9CreatorSettings.m_die = true;
				m_b9CreatorSettings.unlock();

				return true;
			}
			break;
		default:
			break;
	}

	return false; 
}



