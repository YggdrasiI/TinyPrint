#include "SerialManager.h"

void SerialManager::run(){
	std::string file( m_b9CreatorSettings.getString("comPort") ); 

	// Open the serial port for communication.
	m_serialStream.Open( file );

	if ( ! m_serialStream.IsOpen() )
	{
		std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
			<< "Error: Could not open serial port "
			<< "'" << file << "'."
			<< std::endl ;
		exit(1) ;
	}

	m_serialStream.SetBaudRate( SerialStreamBuf::BAUD_115200 );
	if ( ! m_serialStream.IsOpen() ) std::cerr << "SetBaudRate failed\n" ;

	// Use 8 bit wide chars
	m_serialStream.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 ) ;
	if ( ! m_serialStream.IsOpen() ) std::cerr << "SetCharSize failed\n" ;
	// Use odd parity during serial communication. 
	m_serialStream.SetParity( SerialStreamBuf::PARITY_NONE ) ;
	if ( ! m_serialStream.IsOpen() ) std::cerr << "SetParity failed\n" ;
	// Use one stop bit. 
	m_serialStream.SetNumOfStopBits(1) ;
	if ( ! m_serialStream.IsOpen() ) std::cerr << "SetNumOfStopBits failed\n" ;
	// Use hardware flow-control. 
	m_serialStream.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_HARD ) ;
	if ( ! m_serialStream.IsOpen() ) std::cerr << "SetFlowControl failed\n" ;

	if ( ! m_serialStream.IsOpen() )
	{
		std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
			<< "Error: Could not set serial port properties."
			<< std::endl ;
		exit(1) ;
	}

	/* Do not ignore white space chars */
    // Do not skip whitespace characters while reading from the
    // serial port.
    //
    // serial_port.unsetf( std::ios_base::skipws ) ;
	m_serialStream >> std::noskipws;

	while( !m_die  ){
		//sendLines()
		readLine();
		usleep(1000000);
	}

	m_serialStream.Close();

}

void SerialManager::readLine(){
	//string str;
	m_serialStream << 'I';
	usleep(1000);
	//m_serialStream >> str;

	char str[160];
	for(int i=0;i<159;i++) str[i] = ' ';
	str[0] = 'H'; str[1]='e'; str[2]='l'; str[3]='l'; str[4]='o';
	str[159] = '\0';

	m_serialStream >> str;
 //   sscanf(str,"%d",&res);
	

	m_messageMutex.lock();
	//m_messageQueue.push(str);
	m_messageMutex.unlock();

	std::cout << str << std::endl;
}
//auslesen mit .front() und .pop()
