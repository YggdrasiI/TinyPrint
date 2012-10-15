#include "SerialManager.h"

using namespace std ;
using namespace boost;

void SerialManager::run(){

	while( !m_die  ){

		if( m_open){
			try{
				writeLineFromQueue(); //send single command. 
				//readLineToQueue();
				string str = readLine();
				update(str);
			} catch(boost::system::system_error& e)
			{
				cout<<"Error: "<<e.what()<<endl;
				//m_die = true;
				usleep(1000000);
			}
			usleep(100000);//wait 100ms
		}else{
			//com port not open-> Wait. ToDo: Try reconnect to maschine.
			usleep(1000000);
			//No reconnection provided -> Quit this thread
			m_die = true;
		}
	}


}

void SerialManager::readLineToQueue(){
	string str = readLine();

	Messages &q = m_b9CreatorSettings.m_queues;	
	q.add_message(str);

#ifdef VERBOSE
	std::cout << str << std::endl;
#endif
}

void SerialManager::writeLineFromQueue(){
	Messages &q = m_b9CreatorSettings.m_queues;	

	if( q.m_commandQueue.size() < 1 ) return;
	q.m_commandMutex.lock();
	string cmd = q.m_commandQueue.front();
	q.m_commandQueue.pop();
	q.m_commandMutex.unlock();

	writeString(cmd);

#ifdef VERBOSE
	std::cout << "Send " << cmd << std::endl;
#endif
	
	writeString(cmd);
}


void SerialManager::update(string str){
 //[...] analyse
 if( str.size() < 1 ) return;
	m_b9CreatorSettings.lock();
	switch( str.at(0) ){
		case 'R':
			{
				m_b9CreatorSettings.m_resetStatus = (str.at(1) == '1' );
			}
			break;
		case 'P':
			{
				m_b9CreatorSettings.m_projectorStatus = (str.at(1) == '1' );
			}
			break;
		case 'V': //Version
		case 'C': //Comment. Remove first char.
			{
				str.erase(0,1);
			}
			break;
		case 'Z':	// Height of build table
			{
				int zh(0);
				sscanf(str.c_str(),"%*c %d ",&zh);
				m_b9CreatorSettings.m_zHeight = zh;
			}
			break;
		case 'S':	// Slide in %. >0% and >100% are possible
			{
				int s(0);
				sscanf(str.c_str(),"%*c %d ",&s);
				m_b9CreatorSettings.m_vatOpen = s;
			}
			break;
		case 'I':	// PU info. Unit: 1/100 mm
			{
				int pu(0);
				sscanf(str.c_str(),"%*c %d ",&pu);
				//hm, should be the same value as m_PU....
			}
			break;
		default:
			std::cout << "Can not handle" << str << std::endl;
			break;

	}
	//regenerate json string here?!
	m_b9CreatorSettings.unlock();

	Messages &q = m_b9CreatorSettings.m_queues;	
	q.add_message(str);

}
