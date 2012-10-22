/*
 * Mange the serial communication with the arduino board.
 * Code adapted from http://www.webalice.it/fede.tft/serial_port/serial_port.html
 *
 * */
#include <cstdlib>
//#include <cstdio>
#include <string>
#include <iostream>
#include <queue>
#include <pthread.h>
#include <boost/asio.hpp>
#include "Mutex.h"
#include "B9CreatorSettings.h"
#include "JsonMessage.h"

#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

// invoke thread loop.
static void* serialThread(void* arg);

class SerialManager {
	public: 
		bool m_die;
		//bool m_open;
	private:
		B9CreatorSettings &m_b9CreatorSettings;
    boost::asio::io_service m_io;
    boost::asio::serial_port m_serialStream;
		pthread_t m_pthread;
	public:
		SerialManager(B9CreatorSettings &b9CreatorSettings ) :
			m_b9CreatorSettings(b9CreatorSettings),
			m_die(false),
			//m_open(false),
			m_io(),
			m_serialStream(m_io)
	{
		std::string file( m_b9CreatorSettings.getString("comPort") );
		int baudrate( (int) m_b9CreatorSettings.getNumber("comBaudrate") ); 
		// Open the serial port for communication.
		try {
			m_serialStream.open( file );
			m_serialStream.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
			//m_open = true;
			m_b9CreatorSettings.lock();
			m_b9CreatorSettings.m_connected = true;
			m_b9CreatorSettings.unlock();
		} catch(boost::system::system_error& e)
		{
			std::cout<<"Error: "<<e.what()<<std::endl;
			//m_open = false;
			m_b9CreatorSettings.lock();
			m_b9CreatorSettings.m_connected = false;
			m_b9CreatorSettings.unlock();
			std::ostringstream message;
			message << "File '" << file << "' not found.\n Error: " <<e.what() << std::endl;
			std::string mess = message.str();
			m_b9CreatorSettings.m_queues.add_message( mess );
			//exit(1) ;
		}

		/* Do not ignore white space chars */
		// Do not skip whitespace characters while reading from the
		// serial port.
		//
		// serial_port.unsetf( std::ios_base::skipws ) ;
		//m_serialStream >> std::noskipws;

		if( pthread_create( &m_pthread, NULL, &serialThread, this) ){
			std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
				<< "Error: Could not create thread for serial communication."
				<< std::endl ;
			exit(1) ;
		}

	}

		~SerialManager(){
			// kill loop in other thread
			m_die = true;
			//wait on other thread
	    pthread_join( m_pthread, NULL);

		}

		void run();

/* Update state =>
 *	 Analyse serial message and alter m_b9CreatorSettings.
 */
		void update(std::string);

	private:
		/* Buffer serial input */
		void readLineToQueue();
		/* Write command from queue to serial input */
		void writeLineFromQueue();

    /**
     * Blocks until a line is received from the serial device.
     * Eventual '\n' or '\r\n' characters at the end of the string are removed.
     * \return a string containing the received line
     * \throws boost::system::system_error on failure
     */
    std::string readLine()
    {
        //Reading data char by char, code is optimized for simplicity, not speed
        char c;
        std::string result;
        for(;;)
				{
					boost::asio::read(m_serialStream,boost::asio::buffer(&c,1));
					switch(c)
					{
						case '\r':
							break;
						case '\n':
							return result;
						default:
							result+=c;
					}
				}
				return "Error in __FILE__ in __LINE__ \n";
    }
    /**
     * Write a string to the serial device.
     * \param s string to write
     * \throws boost::system::system_error on failure
     */
    void writeString(std::string s)
    {
        boost::asio::write(m_serialStream,boost::asio::buffer(s.c_str(),s.size()));
    }

};

/* function for serial communication thread */
static void* serialThread(void* arg){
	VPRINT("Start serial thread\n");
	((SerialManager*)arg)->run();
	VPRINT("Quit serial thread\n");
}


#endif
