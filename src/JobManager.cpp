#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/bind.hpp>

#include "JobManager.h"
#include "JobFile.h"
#include "B9CreatorSettings.h"
#include "DisplayManager.h"

//#define RUNPRINT VPRINT
static JobState runprint_state=IDLE;
#define RUNPRINT(...) if(runprint_state!=m_state){ \
	runprint_state = m_state; \
	VPRINT( __VA_ARGS__ ); \
}

using namespace std;
using namespace cv;

JobManager::JobManager(B9CreatorSettings &b9CreatorSettings, DisplayManager &displayManager ) :
	m_pthread(),
	m_die(false),
	m_b9CreatorSettings(b9CreatorSettings),
	m_displayManager(displayManager),
	//m_showedLayer(b9CreatorSettings.m_printProp.m_currentLayer),
	m_state(START_STATE),
	m_pauseInState(IDLE),
	m_timingState(FINISH), //startvalue just some value != START_STATE
	m_job_mutex(),
	m_tTimer(),
	m_tPause(),
	m_tCuring(m_tTimer),
	m_tCloseSlider(m_tTimer),
	m_tProjectImage(m_tTimer),
	m_tBreath(m_tTimer),
	m_tFWait(m_tTimer),
	m_tRWait(m_tTimer),
	m_force_preload(false)
{

	if( pthread_create( &m_pthread, NULL, &jobThread, this) ){
		std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
			<< "Error: Could not create thread for job manager."
			<< std::endl ;
		exit(1) ;
	}

	//init timer value
	m_tTimer.diff = 0;
	gettimeofday( &m_tTimer.begin, NULL );

	//connect B9CreatorSettings update signal
	m_b9CreatorSettings.updateSettings.connect(
					boost::bind(&JobManager::updateSignalHandler,this, _1 )
					);

}

JobManager::~JobManager(){
	// kill loop in other thread
	m_die = true;
	//wait on other thread
	pthread_join( m_pthread, NULL);


}

int JobManager::loadJob(const std::string filename){

	m_job_mutex.lock();
	/* Most lines moved to B9CreatorSettings */
	if( m_b9CreatorSettings.loadJob(filename) == 0)
		show( m_b9CreatorSettings.m_printProp.m_currentLayer, RAW );
	m_job_mutex.unlock();
	return 0;
}

int JobManager::unloadJob(int index){

	m_job_mutex.lock();
	/* Most lines moved to B9CreatorSettings */
	if( m_b9CreatorSettings.unloadJob(index) == 0)
		show( m_b9CreatorSettings.m_printProp.m_currentLayer, RAW );
	m_job_mutex.unlock();
	return 0;
}


int JobManager::initJob(bool withReset){
	m_job_mutex.lock();
	if( m_state != IDLE && m_state != START_STATE ){
		std::string msg("(Job) Job already running. Abort initialisation of job.");
		std::cerr << msg << std::endl;
		m_b9CreatorSettings.m_queues.add_message(msg);
		return -1;
	}

	// reset at least one time
	if( withReset ||
			( (m_b9CreatorSettings.m_resetStatus != 0) && m_state == START_STATE )
		){
		m_state = RESET;
	}else{
		m_state = INIT;
	}
	m_job_mutex.unlock();

return 0;
}

int JobManager::startJob(){
	if( m_state == START_STATE ){
	}

	if( m_state != IDLE ){
		std::string msg("(Job) Can not start job.");
		std::cerr << msg << std::endl;
		m_b9CreatorSettings.m_queues.add_message(msg);
		return -1;
	}

	m_b9CreatorSettings.lock();
	m_b9CreatorSettings.m_printProp.m_lockTimes = true;
	m_b9CreatorSettings.unlock();

	m_state = FIRST_LAYER;
	return 0;
}

int JobManager::pauseJob(){
	if( m_state == IDLE ) return -1;

	Messages &q = m_b9CreatorSettings.m_queues;

	gettimeofday( &m_tPause.begin, NULL );
	m_pauseInState = m_state;

	/** Try to reach hazard-free state. **/
	/* Blank image. */
	m_displayManager.blank();

	/* Close VAT if printer is equipped with slider */
	if( m_b9CreatorSettings.m_shutterEquipped ){
		std::string cmd_close("V0");
		q.add_command(cmd_close);
		m_b9CreatorSettings.m_queues.add_command(cmd_close);
	}

	m_state = PAUSE;
	return 0;
}

int JobManager::resumeJob(){
	if( m_state != PAUSE ) return -1;

	Messages &q = m_b9CreatorSettings.m_queues;
	// Analyse m_pauseInState to fix some timing
	switch( m_pauseInState ){
		case IDLE:
			{
				std::string msg("Can not resume job. It was not paused");
				std::cerr << msg << std::endl;
				m_b9CreatorSettings.m_queues.add_message(msg);
				return -1;
			}
			break;
		case BREATH:
			{
				// Open shutter
				if( m_b9CreatorSettings.m_shutterEquipped ){
					std::string cmd_open("V100");
					q.add_command(cmd_open);
					m_b9CreatorSettings.m_queues.add_command(cmd_open);
				}
				VPRINT("Repeat breath for layer %i.\n",
						m_b9CreatorSettings.m_printProp.m_currentLayer );
				gettimeofday( &m_tBreath.begin, NULL );
			}
			break;
		case CURING:
		case OVERCURING:
			{
				// Substract elapsed time before job was stoped.
				m_tCuring.diff = m_tCuring.diff
					- Timer::timeval_diff( &m_tPause.begin ,&m_tCuring.begin );
				gettimeofday( &m_tCuring.begin, NULL );
				show( m_b9CreatorSettings.m_printProp.m_currentLayer,
						(m_state==OVERCURING?OVERCURE1:RAW)  );
			}
			break;
		case WAIT_ON_R_MESS:
			{
				m_pauseInState = RESET;
			}
			break;
		case WAIT_ON_F_MESS:
			{
				m_force_preload = true;

				m_b9CreatorSettings.lock();
				m_b9CreatorSettings.m_readyForNextCycle = true;
				m_b9CreatorSettings.unlock();
			}
			break;
		default:
			break;
	}

	m_state = m_pauseInState;
	m_pauseInState = IDLE;
	return 0;
}

int JobManager::stopJob(){
	//can not stop job, if in idle state
	if( m_state == IDLE) return -1;

	// reset pause state to default value
	if( m_state == PAUSE ) m_pauseInState = IDLE;

	m_state = FINISH;
	return 0;
}

/*
int JobManager::nextStep(){
	m_state = NEXT_LAYER;
	return 0;
}*/

void JobManager::run(){
	Messages &q = m_b9CreatorSettings.m_queues;

	while( !m_b9CreatorSettings.m_die && !m_die ){
		m_job_mutex.lock();
		switch( m_state ){
			case RESET:
				{
					gettimeofday( &m_tRWait.begin, NULL );
					m_tRWait.diff = MaxWaitR;

					/* Send reset command to printer */
					std::string cmd_reset("R");
					q.add_command(cmd_reset);

					RUNPRINT("Reset requested. Now wait on 'R0' Message.\n");
					m_state = WAIT_ON_R_MESS;
				}
				break;
			case WAIT_ON_R_MESS:
				{
					RUNPRINT("Wait on 'R' message...\n");
					if( m_b9CreatorSettings.m_resetStatus == 0  ){
						m_state = INIT;
					}
					if( m_tRWait.timePassed() ){
						std::string msg("(Job) Wait to long on ready signal of printer. Printer connection lost?");
						std::cerr << msg << std::endl;
						m_b9CreatorSettings.m_queues.add_message(msg);
						m_state = IDLE;
					}
					if( !m_b9CreatorSettings.m_connected ){
						std::string msg("(Job) No printer connection. Skip wait on 'R0' Message.");
						std::cerr << msg << std::endl;
						//m_b9CreatorSettings.m_queues.add_message(msg);
						m_b9CreatorSettings.m_resetStatus = 0;
					}
					if( m_b9CreatorSettings.m_die ){
						m_state = IDLE;
					}
				}
				break;
			case INIT:
				{
					/* Set release cycle time.
					 * This is called 'Breath Time' in DPL3DAPI. 
					 * */
					std::ostringstream cmd_cycle;
					cmd_cycle << "D" << (int)(1000*m_b9CreatorSettings.m_printProp.m_releaseCycleTime);
					std::string cmd_cycleStr(cmd_cycle.str());
					q.add_command(cmd_cycleStr);

					/* Set settle cycle time. Default is 0.
					 * Require DLP3DAPI version >= 1.1. 
					 * */
					std::ostringstream cmd_settle;
					cmd_settle << "E" << (int)(1000*m_b9CreatorSettings.m_printProp.m_settleTime);
					std::string cmd_settleStr(cmd_settle.str());
					q.add_command(cmd_settleStr);

					/* Set raise and lower speed of z-axis
					 * Values in Percent. 0% = Lowest Speed.
					 * Require DLP3DAPI version >= 1.1. 
					 * */
					std::ostringstream cmd_lower,cmd_raise;
					cmd_raise << "K" << m_b9CreatorSettings.m_zAxisRaiseSpeed;
					cmd_lower << "L" << m_b9CreatorSettings.m_zAxisLowerSpeed;
					q.add_command(cmd_raise.str());
					q.add_command(cmd_lower.str());

					/* Set opening and cloing speed of shutter
					 * Values in Percent. 0% = Lowest Speed.
					 * Require DLP3DAPI version >= 1.1. 
					 * */
					std::ostringstream cmd_open,cmd_close;
					cmd_open << "W" << m_b9CreatorSettings.m_shutterOpenSpeed;
					cmd_close << "X" << m_b9CreatorSettings.m_shutterCloseSpeed;
					q.add_command(cmd_open.str());
					q.add_command(cmd_close.str());

					/* Update machine data ('A' covers 'I' command.) */
					std::string cmd_info("A");
					q.add_command(cmd_info);

					// Set layer number to base layer index.
					m_b9CreatorSettings.m_printProp.m_currentLayer = 0;
					// Set display framebuffer on
					m_b9CreatorSettings.m_display = true;


					// reset pause state to default value
					m_pauseInState = IDLE;

					RUNPRINT("Init done. Idle in JobManager.\n");

					//m_state = FIRST_LAYER;
					m_state = IDLE;
				}
				break;
			case FIRST_LAYER:
				{
					std::string cmd_base("B0");
					q.add_command(cmd_base);

					//wait on shutter opening.
					/* Thats not possible. 'F' message is send on shutter opening and not
					 * after table movement.
					RUNPRINT("First layer state. Wait on 'F' message\n");
					gettimeofday( &(m_tFWait.begin), NULL );
					m_tFWait.diff = MaxWaitFfrist;
					m_state = WAIT_ON_F_MESS;
					*/

					/* Wait on zHeight = 0 */
					gettimeofday( &(m_tFWait.begin), NULL );
					m_tFWait.diff = MaxWaitF;
					RUNPRINT("Move Table to base position.\n");

					m_state = WAIT_ON_ZERO_HEIGHT;

				}
				break;
			case WAIT_ON_ZERO_HEIGHT:
				{
					if( !m_b9CreatorSettings.m_connected ){
						RUNPRINT("(Job) Printer not connected. Set height manually to 0.\n");
						m_b9CreatorSettings.m_zHeight = 0;
					}
					if( m_b9CreatorSettings.m_zHeight == 0 ){
						m_force_preload = true;
						m_state = WAIT_ON_F_MESS; // or BREATH
					}else{
						RUNPRINT("Wait till base position reached...\n");
					}
				}
				break;
			case NEXT_LAYER:
				{
					int &zHeight = m_b9CreatorSettings.m_zHeight; //Reference!
					int zHeightLimit = m_b9CreatorSettings.m_zHeightLimit;
					int zRes = m_b9CreatorSettings.m_printProp.m_zResolution;
					int pu = m_b9CreatorSettings.m_PU;
					int l = m_b9CreatorSettings.m_printProp.m_currentLayer;

					//int zHeight2 = 100*( zRes )/pu + zHeight; //sum up rounding errors
					int zHeight2 = 100*( l*zRes )/pu; //require zHeight(layer 0)=0
					if( zHeight2 > zHeightLimit ){
						std::ostringstream zError;
						zError << "(Job) Height of next layer higher as maximal height. Abort job."
						<< std::endl << "Current height: " << zHeight << " Next height: " << zHeight2
						<< std::endl << "Height limit: " << zHeightLimit
						<< std::endl << "Next layer: " << l;
						std::string zErrorStr = zError.str();
						RUNPRINT("%s", zErrorStr.c_str() );
						q.add_message( zErrorStr );
						m_state = ERROR;
						break;
					}
					if( zHeight2 <= zHeight ){
						std::ostringstream zError;
						zError << "(Job) Height of next layer lower as current height. Abort job."
						<< std::endl << "Current height: " << zHeight << " Next height: " << zHeight2
						<< std::endl << "Next layer: " << l;
						std::string zErrorStr = zError.str();
						RUNPRINT("%s", zErrorStr.c_str() );
						q.add_message( zErrorStr );
						m_state = ERROR;
						break;
					}

					// Update height.
					// Do not update value manually, if printer is connected.
					// Wait on message on serial channel.
					if( !m_b9CreatorSettings.m_connected ){
						zHeight = zHeight2;
					}

					std::ostringstream cmd_next;
					cmd_next << "N" << zHeight2 ;
					std::string cmd_nextStr = cmd_next.str();
					q.add_command( cmd_nextStr );
					RUNPRINT("Next layer state. Send N%i for next layer.\n", zHeight2 );

					//hm, should I wait on release cycle here?!

					//wait on shutter opening.
					gettimeofday( &m_tFWait.begin, NULL );
					m_tFWait.diff = MaxWaitF;

					m_force_preload = true;
					m_state = WAIT_ON_F_MESS;
				}
				break;
				/*		case LAST_LAYER:
							{
							}
							break;*/
			case WAIT_ON_F_MESS:
				{
					RUNPRINT("Wait on 'F' message...\n");

					if( m_force_preload ){
						VPRINT("Preload of next slice images.\n");
						preload(m_b9CreatorSettings.m_printProp.m_currentLayer, RAW);
						m_force_preload = false;
					}

					bool & r = m_b9CreatorSettings.m_readyForNextCycle;
					if( r || m_tFWait.timePassed() ){
						// unset the ready flag
						r = false;

						gettimeofday( &m_tBreath.begin, NULL );
						m_tBreath.diff = m_b9CreatorSettings.m_printProp.m_breathTime*1000000;

						RUNPRINT("Begin breath for layer %i.\n",
								m_b9CreatorSettings.m_printProp.m_currentLayer);
						m_force_preload = true;
						m_state = BREATH;
					}else
					if( m_tFWait.timePassed() ){
						std::string msg("(Job) Wait to long on 'F' signal of printer. Printer connection lost?");
						std::cerr << msg << std::endl;
						m_b9CreatorSettings.m_queues.add_message(msg);
						m_state = IDLE;
					}

					if( !m_b9CreatorSettings.m_connected ){
						std::string msg("(Job) Printer not connected. Skip waiting on 'F' message.");
						std::cerr << msg << std::endl;
						//m_b9CreatorSettings.m_queues.add_message(msg);
						r = true;
					}
				}
				break;
			case BREATH:
				{
					RUNPRINT("Breathing...\n");
					if( m_tBreath.timePassed() ){
						int l = m_b9CreatorSettings.m_printProp.m_currentLayer;

						gettimeofday( &m_tCuring.begin, NULL );
						if( l < m_b9CreatorSettings.m_printProp.m_nmbrOfAttachedLayers ){
							m_tCuring.diff = m_b9CreatorSettings.m_printProp.m_exposureTimeAL*1000000;
						}else{
							m_tCuring.diff = m_b9CreatorSettings.m_printProp.m_exposureTime*1000000;
						}

						//update and display slice
						//m_job_mutex.unlock();
						show(l, RAW);
						//m_job_mutex.lock();

						m_state = CURING;
						//RUNPRINT("Start curing of layer %i with %is.\n",l, (int) (m_tCuring.diff/1000000) );
					}
				}
				break;
			case CURING:
				{
					RUNPRINT("Curing...\n");

					if( m_force_preload ){
						VPRINT("Preload next overcure images\n");
						preload(m_b9CreatorSettings.m_printProp.m_currentLayer, OVERCURE1);
						m_force_preload = false;
					}

					if( m_tCuring.timePassed() ){

						int &l = m_b9CreatorSettings.m_printProp.m_currentLayer;
						if( l < m_b9CreatorSettings.m_printProp.m_nmbrOfAttachedLayers ||
								m_b9CreatorSettings.m_printProp.m_overcureTime <= 0 ){
							l++;
							m_displayManager.blank();

							if( l < m_b9CreatorSettings.m_printProp.m_nmbrOfLayers ){
							m_state = NEXT_LAYER;
							}else {
								m_state = FINISH;
							}

						}else {
							// Init Overcuring
							gettimeofday( &m_tCuring.begin, NULL );
							m_tCuring.diff = m_b9CreatorSettings.m_printProp.m_overcureTime*1000000;

							show(l, OVERCURE1);
							m_state = OVERCURING;
						}
					}

				}
				break;
			case OVERCURING:
				{
					RUNPRINT("Overcuring...\n");
					if( m_tCuring.timePassed() ){
						//hide slice on projector image.
						m_displayManager.blank();

						int &l = m_b9CreatorSettings.m_printProp.m_currentLayer;
						l++;
						if( l < m_b9CreatorSettings.m_printProp.m_nmbrOfLayers ){
							m_state = NEXT_LAYER;
						}else {
							m_state = FINISH;

							/* Power of the Projector.
							 * This is not done by the printer.
							 * I placed it here and not in FINISH to
							 * avoid the projector shutoff if
							 * the job canceld by user (over stopJob())
							 * */
							std::string P0 = "PO" ;
							q.add_command(P0);

						}

					}
				}
				break;
			case PAUSE:
				{
					RUNPRINT("Pause...\n");
				}
				break;
			case FINISH:
				{
					std::string cmd_finished;
					RUNPRINT("Send F%i. Job finished\n",9000 );
					cmd_finished = "F9000" ;
					q.add_command(cmd_finished);

					m_b9CreatorSettings.lock();
					//m_b9CreatorSettings.m_printProp.m_currentLayer = 0;
					m_b9CreatorSettings.unlock();
					//fall through...
				}
			case ERROR:
				{
					m_displayManager.blank();

					m_b9CreatorSettings.lock();
					m_b9CreatorSettings.m_printProp.m_lockTimes = false;
					m_b9CreatorSettings.unlock();

					m_state = IDLE;
				}
				break;
		case IDLE:
				{
					RUNPRINT("Idle...\n");
				}
				break;
		case START_STATE:
				{

				}
				break;
			default:
				std::cout << "State " << m_state << " unknown" << std::endl;
		}

		/*m_b9CreatorSettings.lock();
		m_b9CreatorSettings.m_jobState = m_state;
		m_b9CreatorSettings.unlock();*/

		//check change external change of current layer
		/*
		if( m_b9CreatorSettings.m_display &&
				(m_b9CreatorSettings.m_printProp.m_currentLayer != m_showedLayer)
			){
			RUNPRINT("(Job) Change of layer detected. Show new layer.\n");
			show( m_b9CreatorSettings.m_printProp.m_currentLayer );
		}
		*/


		m_job_mutex.unlock();
		usleep(50000); //.05s
		//usleep(2000000); //2s
	}

}

bool JobManager::webserverSetState(Onion::Request *preq, int actionid, Onion::Response *pres){

	switch( actionid ){
		case 8:
			{	/* remove Job */
				std::string reply("failed");
				//int index = atoi( onion_request_get_queryd(req,"job_file_index","0") );
				int index = atoi( onion_request_get_post(preq->c_handler(), "job_file_index") );
				if( 0 == m_b9CreatorSettings.unloadJob(index) ){
					cJSON *root = cJSON_CreateObject();
					cJSON *html = cJSON_CreateArray();
					cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonFilesField("files") );
					cJSON_AddItemToObject(root, "html", html);
					char *files = cJSON_Print( root );
					cJSON_Delete( root );

					pres->write( files, strlen(files) );
					free( files );

					//reset this state to send more data
					//on next getJobTimings call.
					m_timingState = FINISH;

				}else{
					pres->write( reply.c_str(), reply.size() );
				}
				return true;

			}
			break;
		case 7:
			{  /* load Job */
				std::string reply("failed");

				std::string job_file ( onion_request_get_post(preq->c_handler(),"job_file") );
#ifdef VERBOSE
				std::cout << "Load '"<< job_file << "'" << std::endl;
#endif
				if( loadJob( job_file.c_str() ) == 0){
					//reply = "ok";
					/* Create json struct with list of open files.
					 * this is an subset of all nodes of B9CreatorSettings.genJson()
					 * */
					cJSON *root = cJSON_CreateObject();
					cJSON *html = cJSON_CreateArray();
					cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonFilesField("files") );
					cJSON_AddItemToObject(root, "html", html);
					char *files = cJSON_Print( root );
					cJSON_Delete( root );

					pres->write( files, strlen(files) );
					free( files );

					//reset this state to send more data
					//on next getJobTimings call.
					m_timingState = FINISH;

				}else{
					pres->write( reply.c_str(), reply.size() );
				}
				return true;
			}
			break;
		case 6: /* control JobManager */
			{
				std::string reply("error");
				std::string print_cmd ( onion_request_get_post(preq->c_handler(),"print") );
#ifdef VERBOSE
				std::cout << "'"<< print_cmd << "'" << std::endl;
#endif
				//convert toggle into start or pause
				if( 0 == print_cmd.compare("toggle") ){
					if( m_state == START_STATE || m_state == IDLE )
						print_cmd = "start";
					else if( m_state == PAUSE )
						print_cmd = "resume";
					else
						print_cmd = "pause";
				}


				if( 0 == print_cmd.compare("init") ){
					if( 0 == initJob( (m_b9CreatorSettings.m_resetStatus != 0)  ) )
						reply = "idle";

				}else if( 0 == print_cmd.compare("start") ){
					if( m_state == START_STATE){
						//we can not start job. Init at first.
						if( 0 == initJob( (m_b9CreatorSettings.m_resetStatus != 0)  ) )
							reply = "idle";

					}else if( m_state == IDLE ){
						if( 0 == startJob() )
							reply = "print";

					}else{
						//can not start. Reply error message or just send idle.
						reply = "idle";
					}

				}else if( 0 == print_cmd.compare("pause") ){
						if( 0 == pauseJob() )
							reply = "pause";

				}else if( 0 == print_cmd.compare("resume") ){
					if( 0 == resumeJob() )
						reply = "print";

				}else if( 0 == print_cmd.compare("abort") ){
					if( 0 == stopJob() ){
						reply = "idle";
					}else{
						if( m_state == IDLE ){
							//stop failed, but printer mode is idle. Thus,
							//stopping produce no error.
							reply = "idle";
						}
					}
				}

				//now, write "error","idle","print" or "pause" to response struct.
				pres->write( reply.c_str(), reply.size() );
				return true;
			}
			break;
		case 5: /* Toggle Display */
			{
				const char* disp = onion_request_get_post(preq->c_handler(),"display");
				/* Do noting, if printing is running and display is on
				 * to avoid problems.
				 * */
				/*
				if( m_b9CreatorSettings.m_display && 
						//!( m_state & ( IDLE|START_STATE|PAUSE|ERROR ))
						m_b9CreatorSettings.m_printProp.m_lockTimes
					){
					std::string msg("(Job) Job running. You can not disable display.");
					m_b9CreatorSettings.m_queues.add_message(msg);
					return true;
				}*/

				if( disp != NULL ){

					m_b9CreatorSettings.lock();
					if( disp[0] == '2' )
						m_b9CreatorSettings.m_display = !m_b9CreatorSettings.m_display;
					else
						m_b9CreatorSettings.m_display = (disp[0] == '1');
					m_b9CreatorSettings.unlock();

					if( m_b9CreatorSettings.m_display ){
						//wait second on creation of display
						usleep(1000000);
						VPRINT("Show!\n");
						int &l = m_b9CreatorSettings.m_printProp.m_currentLayer;
						show(l, RAW);
					}

					std::string reply = m_b9CreatorSettings.m_display?"1":"0";
					pres->write( reply.c_str(), reply.size() );
					return true;
				}
			}
			break;
		case 2: /* Save config */
			{
				std::string reply;
				const char* configFilename = onion_request_get_post(preq->c_handler(),"configFilename");

				if( configFilename == NULL ) break;
				if( check_configFilename(configFilename ) == 1){

					printf("Save config: %s\n",configFilename);

					//Saveconfig
					m_b9CreatorSettings.saveConfigFile(configFilename);

					reply = "ok";
				}else{
					reply = "error";
					printf("Filename not allowed\n");
				}

				pres->write( reply.c_str(), reply.size() );
				return true;
			}
			break;
		case 1: /* Load config */
			{
				std::string reply;
				const char* configFilename = onion_request_get_post(preq->c_handler(),"configFilename");

				if( configFilename == NULL ) break;
				if( check_configFilename(configFilename ) == 1){

					printf("Load config: %s\n",configFilename);

					//Free references to m_files elements from display.
					m_displayManager.clear();

					//Release m_files
					m_b9CreatorSettings.clearJobs();

					//Load config
					m_b9CreatorSettings.loadConfigFile(configFilename);

					reply = "ok";
				}else{
					reply = "error";
					printf("Filename not allowed\n");
				}

				pres->write( reply.c_str(), reply.size() );
				return true;
			}
		default:
			{
			}
			break;
	}

	return false;
}


void JobManager::preload(int slice, SliceType type){
	vector<JobFile*>::iterator it = m_b9CreatorSettings.m_files.begin();
	const vector<JobFile*>::const_iterator it_end = m_b9CreatorSettings.m_files.end();
	for( ; it<it_end ; ++it ){
		// shift slice by local m_minLayer
		int local_slice = slice + (*it)->m_minLayer;
		if( local_slice <= (*it)->m_maxLayer &&
				local_slice >=  (*it)->m_minLayer ){
			cv::Mat tmp = (*it)->getSlice(local_slice, type);
		}
	}
}


void JobManager::show(int slice, SliceType type){

	//remove old displayed images
	VPRINT("Clear \n");
	m_displayManager.clear();

	//add new slices
	vector<JobFile*>::iterator it = m_b9CreatorSettings.m_files.begin();
	const vector<JobFile*>::const_iterator it_end = m_b9CreatorSettings.m_files.end();
	for( ; it<it_end ; ++it ){
		// shift slice by local m_minLayer
		int local_slice = slice + (*it)->m_minLayer;
		if( local_slice <= (*it)->m_maxLayer &&
				local_slice >=  (*it)->m_minLayer ){
			cv::Mat s = (*it)->getSlice(local_slice, type);
			cv::Point &p = (*it)->m_position;
			m_displayManager.add( s, p );
		}
	}

	//force redraw of screen
#ifdef VERBOSE
	std::cout << "(Job) Show sprites of slice " << slice << "."  << std::endl;
#endif
	m_displayManager.show();
}


void JobManager::updateSignalHandler(int changes){

	if( changes & LAYER ){
		if( m_b9CreatorSettings.m_display )
			show( m_b9CreatorSettings.m_printProp.m_currentLayer, RAW );
	}

	if( changes ){
		//reset this state to send more data
		//on next getJobTimings call.
		m_timingState = FINISH;
	}

}


bool JobManager::getJobTimings(Onion::Request *preq, int actionid, Onion::Response *pres){

	switch( actionid ){
		case 12:
			{
				m_timingState = (JobState)-1;
			}
			//fall through
		case 11:
			{
				/* check if state was changed during last update
				 * If its the same state, just a subset of
				 * information will be created and sended.
				 * */
				bool newstate( m_timingState != m_state );
				PrintProperties &p = m_b9CreatorSettings.m_printProp;

				int runTime = (p.m_nmbrOfLayers - p.m_currentLayer) * (
						p.m_exposureTime +
						p.m_releaseCycleTime +
						p.m_breathTime +
						p.m_overcureTime );
				// Conside exposure time of attached layers
				if( p.m_currentLayer < p.m_nmbrOfAttachedLayers ){
					runTime +=  (p.m_nmbrOfAttachedLayers-p.m_currentLayer) * (
							p.m_exposureTimeAL -
							p.m_exposureTime -
							p.m_overcureTime );
				}

				int stateTime, stateCountdown;
				//if( m_state == PAUSE || m_state == START_STATE || m_state == IDLE )
				if( m_state & (PAUSE|START_STATE|IDLE|ERROR) )
				{
					stateTime = 1;
					stateCountdown = 0;
				}else{
					stateTime = m_tTimer.diff / 1000000;
					timeval_t now;
					gettimeofday( &now, NULL );
					stateCountdown = (m_tTimer.diff - Timer::timeval_diff(&now, &(m_tTimer.begin) )) / 1000000;
				}

				cJSON *root = cJSON_CreateObject();
				cJSON *html = cJSON_CreateArray();

				if( newstate ){
					cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonStateField("jobState",m_state,"state","token") );
					cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonStateField("runTime",(double)runTime,"hhmmss","") );
					cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonStateField("stateTime",(double)stateTime,"","") );
					m_timingState = m_state;
				}
				cJSON_AddItemToArray(html, m_b9CreatorSettings.jsonStateField("stateCountdown",(double)stateCountdown,"","") );

				cJSON_AddItemToObject(root, "html", html);

				char* reply = cJSON_Print(root);
				cJSON_Delete(root);
				pres->write( reply, strlen(reply) );
				free( reply );

				return true;
			}
			break;
		default:
			break;
	}
	return false;
}

