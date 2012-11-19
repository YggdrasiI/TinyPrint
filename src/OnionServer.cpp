/*
 * This class use the onion library from David Moreno.
 * See https://github.com/davidmoreno/onion .
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
onion_connection_status index_html_template(void *p, onion_request *req, onion_response *res);
onion_connection_status b9creator_settings_js_template(void *p, onion_request *req, onion_response *res);
}

extern "C" {
	/* Replace default onion_log function to 
	 * to filter out O_INFO messages */
	void log_wrapper(onion_log_level level, const char *filename,
			int lineno, const char *fmt, ...){

		if( level == O_INFO) return;

		char tmp[256];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(tmp,sizeof(tmp),fmt, ap);
		va_end(ap);
		onion_log_stderr(level, filename, lineno, tmp);
	};

//	void (*onion_log)(onion_log_level level, const char *filename,
//			int lineno, const char *fmt, ...)=log_wrapper;
}



/*
 * Parse data from client. Use actionid-arg to distinct different
 * cases.
 */
onion_connection_status OnionServer::updateData(
		Onion::Request &req, Onion::Response &res) {
	/* Default reply is 'reload' which force reload
	 * of complete website. In mosty cases this string will replaced
	 * by one of the signal handlers.
	 */
	int actionid = atoi( onion_request_get_queryd(req.c_handler(), "actionid","0") );

	if( ! updateSignal( &req, actionid, &res) ){
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
onion_connection_status OnionServer::getB9CreatorSettings(
		Onion::Request &req, Onion::Response &res ){
	const char* b9Creator = m_b9CreatorSettings.getConfig(true);
	size_t len = strlen( b9Creator );
	res.write(b9Creator, (int) len );
	return OCS_PROCESSED;
}

/*
 Returns json struct of filenames in job files folder.
*/
onion_connection_status OnionServer::getJobFolder(
		Onion::Request &req, Onion::Response &res ){

	std::string &folder = m_b9CreatorSettings.m_b9jDir;
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
onion_connection_status OnionServer::getJobFolderWrapped(
		Onion::Request &req, Onion::Response &res ){
	res.write("json_job_files = ", 17);
	onion_connection_status ret = getJobFolder(req, res);
	res.write(";", 1);
	return ret;
}


/*
 * Convert all enties of message queue into json code and send this file
 * to the client.
 */
onion_connection_status OnionServer::getPrinterMessages(
		Onion::Request &req, Onion::Response &res ){

		Messages &q = m_b9CreatorSettings.m_queues;
		cJSON* tmp = jsonMessages("serialMessages", q.m_messageQueue);
		if( tmp != NULL ){
			char* json_serialMessages = cJSON_Print( tmp );
			size_t len = strlen( json_serialMessages );
			res.write(json_serialMessages, (int) len);

			cJSON_Delete(tmp);
			tmp = NULL;
			free(json_serialMessages);
			json_serialMessages = NULL;
		}else{
			const char* json_serialMessages = "(OnionServer) Serial Messages Error";
			size_t len = strlen( json_serialMessages );
			res.write(json_serialMessages, (int) len);
			//here no free of json_serialMessages, because it's point to static string
		}

	return OCS_PROCESSED;
}


/* SendSignal with actionid=10 to get png image from
 * DisplayManager.
 */
onion_connection_status OnionServer::preview(
		Onion::Request &req, Onion::Response &res ){
	int actionid = 10;
	if( ! updateSignal(&req, actionid, &res) ){
		//signals did not write into response. Write default reply.
		std::string reply("Could not generate Image.");
		res.write( reply.c_str(), reply.size() );
	}
	return OCS_PROCESSED;
}

/*
 Return raw file if found. Security risk?! Check of filename/path required?!
*/
onion_connection_status OnionServer::search_file(
		Onion::Request &req, Onion::Response &res ){
	//const char* path = onion_request_get_path(req);//empty?!
	const char* path = onion_request_get_fullpath( req.c_handler() );
#ifdef VERBOSE
	printf("Request of %s.\n",path);
#endif
	std::string filename("./html/");
	filename.append(path);

	std::ifstream file(filename.c_str());
	std::string line;

	if( file.is_open()){

		/* Create header with mime type and charset information for several file extensions.
		 * This is just a workaround. There should be an automatic mechanicm
		 * in libonion. */
		int periodPos = filename.find_last_of('.');
		std::string extension = filename.substr(periodPos+1);
		std::string key("Content-Type");
		std::string defaultType("text/html; charset: utf-8");

		std::string mime = m_mimedict.get( extension , defaultType ) ;
		res.setHeader(key,mime);
		onion_response_write_headers(res.c_handler());// missing in cpp bindings?
		//res.writeHeaders();//this was added by me...

		try{
			while (std::getline(file, line)) {
				res.write( line.c_str(), line.size() );
				res.write("\n", 1 );
			}
		}//catch ( const boost::iobase::failure &ex )
		catch ( const std::exception & ex ){
			std::cerr << "Can not read " << filename << std::endl;
			res.write( "<h1>Error while reading File.</h1>", 34);
		}
	}else{
		res.write( "<h1>File not found.</h1>", 34);
	}

	return OCS_PROCESSED;
}

/*
 Replace some template variables (filename of last config) call index_html_template
*/
onion_connection_status OnionServer::index_html( Onion::Request &req, Onion::Response &res)
{
	/* Problem: This cause double free of mem because
	 * onion_dict_free will called twice: in index_html_template and deconstructor.
	 * Onion::Dict d;
	 std::string key("LAST_SETTING_FILENAME");
	 d.add(key,m_b9CreatorSettings.m_configFilename,0);
	 return index_html_template(d.c_handler(), req.c_handler(), res.c_handler() );
	 */
	onion_dict *d=onion_dict_new();//will free'd in template call
	onion_dict_add( d, "LAST_SETTING_FILENAME",
			m_b9CreatorSettings.m_configFilename.c_str(), 0);

	return index_html_template(d, req.c_handler(), res.c_handler() );
}


/*
 Replace some template variables and send b9creator_settings.js
*/
onion_connection_status OnionServer::getB9CreatorSettingsWrapped(
		Onion::Request &req, Onion::Response &res)
{
	/*
		 std::string key("ONION_JSON");
		 std::string conf(m_b9CreatorSettings.getConfig());
		 Onion::Dict d;
		 d.add(key, conf, 0);
		 return b9creator_settings_js_template(d.c_handler(), req.c_handler(), res.c_handler());
		 */
	onion_dict *d=onion_dict_new();//will free'd in template call
	onion_dict_add( d, "ONION_JSON",
			m_b9CreatorSettings.getConfig(), 0);
	return b9creator_settings_js_template(d, req.c_handler(), res.c_handler());
}


/*+++++++++++++ OnionServer-Class ++++++++++++++++++ */
OnionServer::OnionServer(B9CreatorSettings &b9CreatorSettings ):
	m_onion( O_ONE_LOOP),
	//m_onion( O_THREADED),//never shutdown server
	//m_onion( O_THREADED|O_DETACH_LISTEN ),
	//m_onion( O_ONE_LOOP|O_DETACH_LISTEN ),
	m_url(m_onion),
	m_mimedict(),
	m_mimes(),
	m_urls(),
	m_b9CreatorSettings(b9CreatorSettings) {
		//m_onion.setTimeout(5000);

		// Store used urls
		m_urls.push_back( "" );
		m_urls.push_back("index.html");
		m_urls.push_back("b9creator_settings.js");
		m_urls.push_back("settings");
		m_urls.push_back("files.js");
		m_urls.push_back("files");
		m_urls.push_back("messages");
		m_urls.push_back("preview.png");
		m_urls.push_back("update");
		m_urls.push_back("^.*$");

		// Store used mime types
		m_mimes.push_back("html");
		m_mimes.push_back("text/html; charset: utf-8");
		m_mimes.push_back("css");
		m_mimes.push_back("text/css; charset: utf-8"); 
		m_mimes.push_back("js");
		m_mimes.push_back("application/javascript; charset: utf-8");
		m_mimes.push_back("png");
		m_mimes.push_back("image/png");

		//Set mime types dict
		m_mimedict.add( m_mimes[0], m_mimes[1], 0);
		m_mimedict.add( m_mimes[2], m_mimes[3], 0);
		m_mimedict.add( m_mimes[4], m_mimes[5], 0);
		m_mimedict.add( m_mimes[6], m_mimes[7], 0);


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

	m_url.add<OnionServer>(m_urls[0], this, &OnionServer::index_html );
	m_url.add<OnionServer>(m_urls[1], this, &OnionServer::index_html );

	/** Dynamic content **/
	/* Send data */
	m_url.add<OnionServer>(m_urls[2], this, &OnionServer::getB9CreatorSettingsWrapped );
	m_url.add<OnionServer>(m_urls[3], this, &OnionServer::getB9CreatorSettings );
	m_url.add<OnionServer>(m_urls[4], this, &OnionServer::getJobFolderWrapped );
	m_url.add<OnionServer>(m_urls[5], this, &OnionServer::getJobFolder );
	m_url.add<OnionServer>(m_urls[6], this, &OnionServer::getPrinterMessages );
	m_url.add<OnionServer>(m_urls[7], this, &OnionServer::preview );
	
	/* Recive data */
	m_url.add<OnionServer>(m_urls[8], this, &OnionServer::updateData );

	/** Static content **/
	/* Send data */
	m_url.add<OnionServer>(m_urls[9], this, &OnionServer::search_file );

	//start loop as thread  (O_DETACH_LISTEN flag is set.)
	//m_onion.listen();//loop
	//return 0;
	return pthread_create( &m_pthread, NULL, &start_myonion_server, &m_onion);
}

int OnionServer::stop_server()
{
	m_onion.listenStop();
	//onion_listen_stop(m_ponion);//stop loop
	int i = pthread_join( m_pthread, NULL);//wait till loop ends
	//onion_free(m_ponion);
	m_onion.~Onion();
	return i;
}

/* return value marks, if reply string contains data which should
 * return to the web client:
 * -2: No data written into reply. Input generate error. Currently, it's not handled.
 * -1: No data written into reply. Input generate state which require reloading of web page.
 *  0: data written into reply
 *  1: No data written into reply, but input processed successful.*/
bool OnionServer::updateWebserver(
		Onion::Request *preq, int actionid, Onion::Response *pres ){
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



