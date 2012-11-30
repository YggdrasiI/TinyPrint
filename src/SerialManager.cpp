#include "SerialManager.h"

using namespace std ;
using namespace boost;

void SerialManager::run(){

	while( !m_die  ){

		if( m_b9CreatorSettings.m_connected /*m_open*/ ){
			try{
				writeLineFromQueue(); //send single command. 
				//readLineToQueue();
				string str = readLine();
				update(str);
			} catch(boost::system::system_error& e)
			{
				cout<<"Error: "<<e.what()<<endl;
				//m_die = true;
				m_b9CreatorSettings.lock();
				m_b9CreatorSettings.m_connected = false;
				m_b9CreatorSettings.unlock();
				usleep(5000000);
			}
			usleep(10000);//wait 100ms
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
				int xres(0);
				sscanf(str.c_str(),"%*c %d ",&xres);
				if( 0 <= xres ) m_b9CreatorSettings.m_projectorXResolution = xres;
			}
			break;
		case 'E':
			{
				int yres(0);
				sscanf(str.c_str(),"%*c %d ",&yres);
				if( 0 <= yres ) m_b9CreatorSettings.m_projectorYResolution = yres;
			}
			break;
		case 'F': // Layer finished
			{
				m_b9CreatorSettings.m_readyForNextCycle = true;
			}
			break;
		case 'G':
			{
			}
			break;
		case 'H':
			{
				int pixelSize(0);
				sscanf(str.c_str(),"%*c %d ",&pixelSize);
				if( 0 <= pixelSize ) m_b9CreatorSettings.m_projectorPixelSize = pixelSize;
			}
			break;
		case 'I':	// PU info. Unit: 1/100 mm
			{
				int pu(0);
				sscanf(str.c_str(),"%*c %d ",&pu);
				/* Should be the same value as m_PU and
				 * will ignored
				 */
#ifdef VERBOSE
				std::cout << "PU check: " << m_b9CreatorSettings.m_PU << " " << pu << std::endl;
#endif
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
				std::cout << "Printer send abort message due to lost comm with host." <<
					std::endl << "This error will be ignored by TinyPrint" << std::endl;
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
				m_b9CreatorSettings.m_shutterOpen = s;
			}
			break;
		case 'T':
			{
			}
			break;
		case 'U':
			{
				int errorNum(-1);
				sscanf(str.c_str(),"%*c %d ",&errorNum);
				std::cout << "Printer send hardware fault number " << errorNum << "." <<
					std::endl << "This error will be ignored by TinyPrint" << std::endl;
			}
			break;
		case 'V': //Version
			{
				m_b9CreatorSettings.m_firmwareVersion = str.substr(1);
			}
			break;
		case 'W':
			{
				m_b9CreatorSettings.m_printerModel = str.substr(1);
			}
			break;
		case 'X':
			{
				int zOffset(0);
				sscanf(str.c_str(),"%*c %d ",&zOffset);
				VPRINT("zAxis Offset: %i \n", zOffset);
				m_b9CreatorSettings.m_zOffset = zOffset;
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
