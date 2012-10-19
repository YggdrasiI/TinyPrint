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

}

//get printer properties.
std::string cmd_info("A"); //'A' messages includes 'I'
m_b9CreatorSettings.m_queues.add_command(cmd_info);	
//usleep(100000);//0.1s, not ness.

return 0;	
}

int JobManager::startJob(){
	
	if( m_state != IDLE ){
		std::string msg("Runtime error in __FILE__, __LINE__. Can not start job.");
		std::cerr << msg << std::endl;
		m_b9CreatorSettings.m_queues.add_message(msg);
		return -1;
	}

	b9CreatorSettings.lock();
	b9CreatorSettings.m_printProp.m_lockTimes = false;
	b9CreatorSettings.unlock();

	m_state = FIRST_LAYER;
}

int JobManager::pauseJob(){
	Messages &q = m_b9CreatorSettings.m_queues;

	m_tPause.begin = time(NULL);
	m_pauseInState = m_state;

	/** Try to reach hazard-free state. **/
	/* Blank image. */
	m_displayManager.blank();

	/* Close VAT if printer is equipped with slider */
	if( m_b9CreatorSettings.m_shutterEquipped ){
		std::string cmd_close("V0"); 
		q.add_command(cmd);	
		m_b9CreatorSettings.m_queues.add_command(cmd_info);	
	}

	m_state = PAUSE;
}

int JobManager::resumeJob(){
	// Analyse m_pauseInState to fix some timing
	switch( m_pauseInState ){
		case IDLE: 
			std::string msg("Can not resume job. It was not paused");
			std::cerr << msg << std::endl;
			m_b9CreatorSettings.m_queues.add_message(msg);
			return -1;
		case BREATH:
			// Open shutter 
			if( m_b9CreatorSettings.m_shutterEquipped ){
				std::string cmd_close("V0"); 
				q.add_command(cmd);	
				m_b9CreatorSettings.m_queues.add_command(cmd_info);	
			}
			m_tBreath.begin = time(NULL);
			break;
		case CURING:
			// Substract elapsed time before job was stoped.
			m_tCuring.diff = m_tCuring.diff
				- Timer.difference(NULL,m_tCuring.begin,m_tCuring.begin);
			m_tCuring.begin = time(NULL);
			m_displayManager.show(NULL,NULL); //TODO
			break;
		case WAIT_ON_R_MESS:
			m_pauseInState = RESET;
			break;
		case WAIT_ON_F_MESS:
				m_b9CreatorSettings.lock();
				m_b9CreatorSettings.m_readyForNextCycle = true;
				m_b9CreatorSettings.unlock();
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

	while( !m_b9CreatorSettings.m_die ){
		m_job_mutex.lock();
		switch( m_state ){
			case RESET:
				{
					m_tRWait.begin = time(NULL);
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
					m_tFWait.begin = time(NULL);
					m_tFWait.diff = MaxWaitF; 
					m_state = WAIT_ON_F_MESS;

				}
				break;
			case NEXT_LAYER:
				{
					std::string cmd_next;
					cmd_next << "N" << 1000; //has to reseach
					VPRINT("Send N%i for next layer.\n",1000 );
					q.add_command(cmd_next);	

					//vat open?!
					VPRINT("Wait on 'F' message\n");
					m_tFWait.begin = time(NULL);
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

						m_tBreath.begin = time(NULL);
						m_tBreath.diff = m_b9CreatorSettings.m_breathTime;

						VPRINT("Start breath for layer %i.\n",l);
						m_state = BREATH;
					}
				}
				break;
			case BREATH:
				{
					if( m_tBreath.timePassed() ){
						int l = m_b9CreatorSettings.m_currentLayer;
						m_tCuring.begin = time(NULL);
						if( l <= m_b9CreatorSettings.m_nmbrOfAttachedLayers ){
							m_tCuring.diff = m_b9CreatorSettings.m_exposureTimeAL;
						}else{
							m_tCuring.diff = m_b9CreatorSettings.m_exposureTime;
						}

						m_displayManager.show(NULL,NULL); //TODO
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

						int &l = m_b9CreatorSettings.m_currentLayer;
						l++;
						if( l <= m_b9CreatorSettings.m_maxLayer ){
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
					cmd_finished << "F" << 9000; 
					q.add_command(cmd_finished);

					b9CreatorSettings.lock();
					b9CreatorSettings.m_printProp.m_lockTimes = false;
					b9CreatorSettings.unlock();

					m_state = IDLE;
				}
				break;
			default: 
				std::cout << "State " << m_state << " unknown" << std::endl;
		}

		m_job_mutex.unlock();
		usleep(50000); //.05m
	}
}
