#include <unistd.h>

#include "JobManager.h"

int JobManager::loadJob(const std::string filename){
}

int JobManager::loadImg(const std::string filename){
}

int JobManager::initJob(bool withReset){
	m_job_mutex.lock();
	if( m_state != IDLE ){
		std::string msg("Runtime error in __FILE__, __LINE__. Job already running. Abort initialisation of job.");
		std::cerr << msg << std::endl;
		m_b9CreatorSettings.m_queues.add_message(msg);
		return -1;
	}
	if( withReset ){
		m_state = RESET;
	}else{
		m_state = INIT;
	}
	m_job_mutex.unlock();

return 0;	
}

int JobManager::startJob(){
	
	if( m_state != IDLE ){
		std::string msg("Runtime error in __FILE__, __LINE__. Can not start job.");
		std::cerr << msg << std::endl;
		m_b9CreatorSettings.m_queues.add_message(msg);
		return -1;
	}

	m_b9CreatorSettings.lock();
	m_b9CreatorSettings.m_printProp.m_lockTimes = false;
	m_b9CreatorSettings.unlock();

	m_state = FIRST_LAYER;
}

int JobManager::pauseJob(){
	if( m_state == IDLE ) return -1;

	Messages &q = m_b9CreatorSettings.m_queues;

	gettimeofday( &m_tTimer.begin, NULL );
	//m_tPause.begin = (timeval_t*) time(NULL);
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
					std::string cmd_close("V0"); 
					q.add_command(cmd_close);	
					m_b9CreatorSettings.m_queues.add_command(cmd_close);	
				}
				gettimeofday( &m_tBreath.begin, NULL );
			}
			break;
		case CURING:
			{
				// Substract elapsed time before job was stoped.
				m_tCuring.diff = m_tCuring.diff
					- Timer::timeval_diff( &m_tPause.begin ,&m_tCuring.begin );
				gettimeofday( &m_tCuring.begin, NULL );
				m_displayManager.show(); //TODO
			}
			break;
		case WAIT_ON_R_MESS:
			{
				m_pauseInState = RESET;
			}
			break;
		case WAIT_ON_F_MESS:
			{
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
	m_displayManager.blank();
	m_state = FINISHED;
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

					m_state = WAIT_ON_R_MESS;
				}
				break;
			case WAIT_ON_R_MESS:
				{
					if( m_b9CreatorSettings.m_resetStatus != 0  ){
						m_state = INIT;
					}
					if( m_tRWait.timePassed() ){
						std::string msg("Runtime error in __FILE__, __LINE__. Wait to long on ready signal of printer. Printer connection lost?");
						std::cerr << msg << std::endl;
						m_b9CreatorSettings.m_queues.add_message(msg);
						m_state = IDLE;
					}
					if( m_b9CreatorSettings.m_die ){
						m_state = IDLE;
					}
				}
				break;
			case INIT:
				{
					/* Update machine data ('A' covers 'I' command.) */
					std::string cmd_info("A"); 
					q.add_command(cmd_info);	

					//VPRINT("Start first layer\n");
					//m_state = FIRST_LAYER;
					m_state = IDLE;
				}
				break;
			case FIRST_LAYER:
				{
					std::string cmd_base("B550"); //has to reseachr original value...
					q.add_command(cmd_base);	

					//vat open?!
					VPRINT("Wait on 'F' message\n");
					gettimeofday( &m_tFWait.begin, NULL );
					m_tFWait.diff = MaxWaitF; 
					m_state = WAIT_ON_F_MESS;

				}
				break;
			case NEXT_LAYER:
				{
					std::string cmd_next;
					cmd_next = "N1000"; //has to reseach
					VPRINT("Send N%i for next layer.\n",1000 );
					q.add_command(cmd_next);	

					//vat open?!
					VPRINT("Wait on 'F' message\n");
					gettimeofday( &m_tFWait.begin, NULL );
					m_tFWait.diff = MaxWaitF; 
					m_state = WAIT_ON_F_MESS;
				}
				break;
				/*		case LAST_LAYER:
							{
							}
							break;*/
			case WAIT_ON_F_MESS:
				{	
					bool & r = m_b9CreatorSettings.m_readyForNextCycle;
					if( r ){
						// unset the ready flag
						r = false;

						gettimeofday( &m_tBreath.begin, NULL );
						m_tBreath.diff = m_b9CreatorSettings.m_printProp.m_breathTime;

						VPRINT("Repeat breath for layer %i.\n",
								m_b9CreatorSettings.m_printProp.m_currentLayer);
						m_state = BREATH;
					}
				}
				break;
			case BREATH:
				{
					if( m_tBreath.timePassed() ){
						int l = m_b9CreatorSettings.m_printProp.m_currentLayer;
						gettimeofday( &m_tCuring.begin, NULL );
						if( l <= m_b9CreatorSettings.m_printProp.m_nmbrOfAttachedLayers ){
							m_tCuring.diff = m_b9CreatorSettings.m_printProp.m_exposureTimeAL;
						}else{
							m_tCuring.diff = m_b9CreatorSettings.m_printProp.m_exposureTime;
						}

						m_displayManager.show(); //TODO
						m_state = CURING;
						VPRINT("Start curing of layer %i.\n",l);
					}
				}
				break;
			case CURING:
				{
					if( m_tCuring.timePassed() ){
						//hide slice on projector image.
						m_displayManager.blank();

						int &l = m_b9CreatorSettings.m_printProp.m_currentLayer;
						l++;
						if( l <= m_b9CreatorSettings.m_printProp.m_maxLayer ){
							m_state = NEXT_LAYER;
						}else {
							m_state = FINISHED;
						}

					}
				}
				break;
			case PAUSE:
				{

				}
				break;
			case FINISHED:
				{
					std::string cmd_finished;
					VPRINT("Send F%i. Job finished\n",9000 );
					cmd_finished = "F9000" ; 
					q.add_command(cmd_finished);

					m_b9CreatorSettings.lock();
					m_b9CreatorSettings.m_printProp.m_lockTimes = false;
					m_b9CreatorSettings.unlock();

					m_state = IDLE;
				}
				break;
		case IDLE:
				{
				}
				break;
			default: 
				std::cout << "State " << m_state << " unknown" << std::endl;
		}

		m_job_mutex.unlock();
		usleep(50000); //.05m
	}
}
