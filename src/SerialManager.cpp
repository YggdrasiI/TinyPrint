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
				usleep(5000000);
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

#ifdef VERBOSE
	std::cout << "Send " << cmd << std::endl;
#endif
	
	cmd.append("\n");
	writeString(cmd);

}


void SerialManager::update(string str){
	//[...] analyse
	if( str.size() < 1 ) return;
	m_b9CreatorSettings.lock();
	switch( str.at(0) ){
		case 'A':
			{
				m_b9CreatorSettings.m_projectorEquipped = (str.at(1) == '1' );
			}
			break;
		case 'B':
			{
			}
			break;
		case 'C': //Comment. 
			{
				//str.erase(0,1);
			}
			break;
		case 'D':
			{
			}
			break;
		case 'E':
			{
			}
			break;
		case 'F':
			{
			}
			break;
		case 'G':
			{
			}
			break;
		case 'H':
			{
			}
			break;
		case 'I':	// PU info. Unit: 1/100 mm
			{
				int pu(0);
				sscanf(str.c_str(),"%*c %d ",&pu);
				//hm, should be the same value as m_PU....
				std::cout << "PU check: " << m_b9CreatorSettings.m_PU << " " << pu << std::endl;
			}
			break;
		case 'J':	// Shutter equipped info.
			{
				m_b9CreatorSettings.m_shutterEquipped = (str.at(1) == '1' );
			}
			break;
		case 'K':
			{
			}
			break;
		case 'L':	// Lamp hours
			{
				int l(0);
				sscanf(str.c_str(),"%*c %d ",&l);
				m_b9CreatorSettings.m_lampHours = l;
			}
			break;
		case 'M':
			{
				int mzh(0);
				sscanf(str.c_str(),"%*c %d ",&mzh);
				m_b9CreatorSettings.m_zHeightLimit = mzh;
			}
			break;
		case 'N':
			{
			}
			break;
		case 'O':
			{
			}
			break;
		case 'P':
			{
				m_b9CreatorSettings.m_projectorStatus = (str.at(1) == '1' );
			}
			break;
		case 'Q':
			{
			}
			break;
		case 'R':
			{
				m_b9CreatorSettings.m_resetStatus = (str.at(1) == '1' );
			}
			break;
		case 'S':	// Slide in %. >0% and >100% are possible
			{
				std::cout << "Set silde val" << std::endl;
				int s(0);
				sscanf(str.c_str(),"%*c %d ",&s);
				m_b9CreatorSettings.m_vatOpen = s;
			}
			break;
		case 'T':
			{
			}
			break;
		case 'U':
			{
			}
			break;
		case 'V': //Version
			{
			}
			break;
		case 'W':
			{
			}
			break;
		case 'X':
			{
			}
			break;
		case 'Y': // current home position
			{
				int home(0);
				sscanf(str.c_str(),"%*c %d ",&home);
				m_b9CreatorSettings.m_zHome = home;
			}
			break;
		case 'Z':	// Height of build table in PU
			{
				int zh(0);
				sscanf(str.c_str(),"%*c %d ",&zh);
				m_b9CreatorSettings.m_zHeight = zh;
			}
			break;
		default:
			std::cout << "Can not handle" << str << std::endl;
			break;
	}

	/* Call of regenerateConfig() is not ness.
	 * Regeneration of string will forced
	 * on web page reload.
	 */
	//m_b9CreatorSettings.regenerateConfig();
	m_b9CreatorSettings.unlock();

	Messages &q = m_b9CreatorSettings.m_queues;	
	q.add_message(str);

}
