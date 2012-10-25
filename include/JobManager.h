/*
 * Mange start, flow and end of 
 * printing jobs.
 * Works like an finite state machine. (See run())
 * See http://b9creator.com/downloads/DLP3DPAPI_1.0.htm
 * for a description of the printing loop.
 *
 * */
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <sys/time.h>
#include <onion/onion.h>

#include <librsvg/rsvg.h>
#include <cairo/cairo-svg.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "constants.h"
#include "Mutex.h"
//#include "B9CreatorSettings.h"
//#include "DisplayManager.h"
class B9CreatorSettings;
class DisplayManager;

#ifndef JOBMANAGER_H
#define JOBMANAGER_H

extern "C"{
gboolean    rsvg_handle_render_cairo     (RsvgHandle * handle, cairo_t * cr);
gboolean    rsvg_handle_render_cairo_sub (RsvgHandle * handle, cairo_t * cr, const char *id);
}

// invoke thread loop.
static void* jobThread(void* arg);

typedef struct timeval timeval_t;

struct Timer{
		timeval_t begin;
		/* m_tDiff will used for if-statements of the type
		 * 'if( m_tDiff < difference(NULL,now, m_t[...]) )'
		 * My intention: I want omit thread blocking by usleep calls
		 * and also avoid an event based approach.
		 */
		long long diff;

		bool timePassed( timeval_t *now = NULL ){
			if( now == NULL ){
				timeval_t tmp;
				now = &tmp;
				gettimeofday( now, NULL );
			}
			//VPRINT("Diff1: %lli",diff);
			return (diff < timeval_diff(now, &begin));
		}

		/* Return time diff in ns */
		static long long timeval_diff(
				timeval_t *pEndTime,
				timeval_t *pStartTime) 
		{   
			timeval_t difference;	

			difference.tv_sec =pEndTime->tv_sec -pStartTime->tv_sec ;
			difference.tv_usec=pEndTime->tv_usec-pStartTime->tv_usec;


			//VPRINT("Start: %li End: %li Diff: %lli\n",pStartTime->tv_sec,pEndTime->tv_sec,1000000LL*difference.tv_sec);
			/* Using while instead of if below makes the code slightly more robust. */

			while(difference.tv_usec<0)
			{   
				difference.tv_usec+=1000000;
				difference.tv_sec -=1;
			}   

			return 1000000LL*difference.tv_sec+
				difference.tv_usec;

		}
};

/* Should be filled by loadJob */
class JobFile{
	public:
		//std::vector<cv::Mat> slices;
		int m_zResolution; //unit: 10μm.
		int m_xyResolution; //unit: 10μm.
		cv::Point m_position;
	private:
		cairo_t *m_pCairo;
		cairo_surface_t *m_pSurface;
		RsvgHandle *m_pRsvgHandle;
		RsvgDimensionData m_dimensions;
		int m_layer;
		cv::Mat m_slice;
	public:
		JobFile(const char* filename):
			m_layer(-1),
			m_zResolution(-1),
			m_xyResolution(-1),
			m_pCairo(NULL),
			m_pSurface(NULL),
			m_position(cv::Point()),
			m_pRsvgHandle(NULL) {

				float dpi=254.0;
				GError *error = NULL;
				g_type_init();
				rsvg_set_default_dpi_x_y (dpi,dpi);//no reaction?
				m_pRsvgHandle = rsvg_handle_new_from_file (filename, &error);
				if( m_pRsvgHandle != NULL ){
					rsvg_handle_get_dimensions (m_pRsvgHandle, &m_dimensions);
					//m_position.x = (1024-m_dimensions.width)/2;
					//m_position.y = (768-m_dimensions.height)/2;
				}else{
					std::cout << "Error while loading file '"
						<< filename << "'." << std::endl;
				}

				int scale = 10; //why 10?

				m_pSurface = (cairo_surface_t *)cairo_image_surface_create(
						CAIRO_FORMAT_ARGB32,
						scale*m_dimensions.width, scale*m_dimensions.height
						//10244, 768
						);
				m_pCairo = cairo_create(m_pSurface);
				cairo_scale( m_pCairo, scale, scale);

				m_position.x = (1024- scale*m_dimensions.width)/2;
				m_position.y = (768- scale*m_dimensions.height)/2;
			}


		~JobFile(){

			cairo_destroy (m_pCairo);
			cairo_surface_destroy (m_pSurface);
			g_object_unref (G_OBJECT (m_pRsvgHandle));
		}
		
		/* Return reference to cv::Mat 
		 * with the image data. */
		/*const*/ cv::Mat &getSlice(int layer){
			if( layer == m_layer ) return m_slice;

			//clear cairo context
			cairo_set_source_rgb (m_pCairo, 0, 0, 0);
			cairo_paint (m_pCairo);
	
			std::ostringstream id;
			id << "#layer" << layer;//filter with group name
			printf("(JobManager.h) Extract layer %s from svg\n",id.str().c_str() );
			rsvg_handle_render_cairo_sub(m_pRsvgHandle, m_pCairo, id.str().c_str() ); 

			//m_slice.release();
			m_slice = cv::Mat(
					cairo_image_surface_get_height(m_pSurface),
					cairo_image_surface_get_width(m_pSurface),
					CV_8UC4,
					(void*) cairo_image_surface_get_data(m_pSurface)
					);

			/*
			std::string test("job_files/");
			test.append( id.str() );
			test.append(".png");
			imwrite(test.c_str(), m_slice);
			*/

			m_layer = layer;
			return m_slice;
		}

};


class JobManager {
		static const long long MaxWaitR = 12E7; //120s. Maximal waiting time on 'Ri' in ns.
		//static const long long MaxWaitF = 5E6; //5s. Maximal waiting time on 'F' in ns.
		static const long long MaxWaitF = 20E6; //20s. 5s was to low. It limit maximal release cycle timeout! 
		//static const long long MaxWaitFfrist = MaxWaitR; // Maximal waiting time on 'F' for base layer.
	private:
		pthread_t m_pthread;
		bool m_die;
		B9CreatorSettings &m_b9CreatorSettings;
		DisplayManager &m_displayManager;
		JobState m_state;
		JobState m_pauseInState; //marks state which got paused.
		Mutex m_job_mutex;
		//save some timings
		Timer m_tTimer;
		Timer m_tPause;
		//until we need two timer paralell, all other
		//timer are just references to m_tTimer;
		Timer &m_tCuring;
		Timer &m_tCloseSlider;
		Timer &m_tProjectImage;
		Timer &m_tBreath;
		Timer &m_tFWait;
		Timer &m_tRWait;
		//std::vector<JobFile> m_files;
		std::vector<JobFile*> m_files;
		int m_showedLayer; //save last displayed layer number.

	public:
		JobManager(B9CreatorSettings &b9CreatorSettings, DisplayManager &displayManager );	 
		~JobManager();

		JobState getState() { return m_state; };

		/* Load b9j file */
		int loadJob(const std::string filename);
		/* Load image. (For testing) */
		int loadImg(const std::string filename);

		/* Init printer (read printer properties and set z-Table) */
		int initJob(bool withReset);
		/* Start job if none running */
		int startJob();
		int pauseJob();
		int resumeJob();
		int stopJob();

		void run();

		//int nextStep();

		/* Will called if website send data */
		void webserverSetState(onion_request *req, int actionid, std::string &reply);

	private:
		void show(int slice);
};

/* wrapper function for job thread.*/
static void* jobThread(void* arg){
	VPRINT("Start job thread\n");
	((JobManager*)arg)->run();
	VPRINT("Quit job thread\n");
}

#endif
