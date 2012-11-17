#ifndef ONIONSERVER_H
#define ONIONSERVER_H

#include <string>
#include <iostream>
#include <pthread.h>
//#include <png.h>

//#include <onion/onion.h>
//#include <onion/dict.h>
//#include <onion/log.h>
#include <onion/onion.hpp>
#include <onion/response.hpp>
#include <onion/request.hpp>
#include <onion/dict.hpp>
#include <onion/url.hpp>

//#include <onion/mime.h>
//#include <onion/extras/png.h>

#include <boost/signal.hpp>
//#include <boost/bind.hpp>

#include "JsonConfig.h"
//#include "B9CreatorSettings.h"
class B9CreatorSettings;


//thread function
static void* start_myonion_server(void* arg){
	printf("Onion server: Start listening.\n");
	onion_listen((onion*)arg);//loop
	printf("Onion server: Stop listening.\n");
}

// Signal return value combiner. 
// See http://www.boost.org/doc/libs/1_52_0/doc/html/signals/tutorial.html#id3284993
template<typename T>
struct maximum
{
	typedef T result_type;

	template<typename InputIterator>
		T operator()(InputIterator first, InputIterator last) const
		{
			// If there are no slots to call, just return the
			// default-constructed value
			if (first == last)
				return T();

			T max_value = *first++;
			while (first != last) {
				if (max_value < *first)
					max_value = *first;
				++first;
			}

			return max_value;
		}
};

class OnionServer{
	public:	
		Onion::Onion m_onion;
		Onion::Url m_url;
		/* Store header with mime type and charset information for several file extensions.
		 * This is just a workaround. There should be an automatic mechanicm
		 * in libonion. */
		Onion::Dict m_mimedict;

		B9CreatorSettings &m_b9CreatorSettings;
	public:
		OnionServer(B9CreatorSettings &b9CreatorSettings );
		
		~OnionServer()
		{
			stop_server();
		}

		int start_server();
		int stop_server();

		/* Update signal. Called by sending of data by js script.
		 *	Every signal handler gain access to
		 *	- the raw request req,
		 *	- the get param 'actionid' and,
		 *	- rhe response data res
		 * Signal returns false, if no signall handler wrote into res.
		 * For each actionid should only one signal handler wrote into the response struture res.
		 *	*/
		//boost::signal<void (onion_request *req,int actionid, std::string &reply)> updateSignal;
		boost::signal<bool (Onion::Request *preq, int actionid, Onion::Response *pres ), maximum<bool> > updateSignal;

		/* Update signal handler of this class.*/
		bool updateWebserver(Onion::Request *preq, int actionid, Onion::Response *pres);

		int index_html( Onion::Request &req, Onion::Response &res);
};

#endif
