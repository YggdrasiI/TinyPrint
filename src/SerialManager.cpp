#include "SerialManager.h"

using namespace std ;
using namespace boost;

void SerialManager::run(){

	while( !m_die  ){
		try{
			writeLineFromQueue(); //send single command. 
			readLineToQueue();
		} catch(boost::system::system_error& e)
		{
			cout<<"Error: "<<e.what()<<endl;
			m_die = true;
		}

		usleep(100000);//wait 100ms
	}


}

void SerialManager::readLineToQueue(){
	string str = readLine();

	m_messageMutex.lock();
	//m_messageQueue.push(str);
	m_messageMutex.unlock();

#ifdef VERBOSE
	std::cout << str << std::endl;
#endif
}

void SerialManager::writeLineFromQueue(){
	if( m_commandQueue.size() < 1 ) return;

	m_commandMutex.lock();
	string cmd = m_commandQueue.front();
	m_commandQueue.pop();
	m_commandMutex.unlock();

#ifdef VERBOSE
	std::cout << "Send " << cmd << std::endl;
#endif
	
	writeString(cmd);
}
//auslesen mit .front() und .pop()
