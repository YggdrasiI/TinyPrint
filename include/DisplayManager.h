/*
 * Mange diplay of fullscreen images.
 * Based on Directfb framework.
 * */

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <cstdio>
#include <iostream>
#include <vector>

#include <pthread.h>

#include <directfb.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "constants.h"
#include "B9CreatorSettings.h"
#include "Mutex.h"

static void* displayThread(void* arg);


//	An error checking macro for a call to DirectFB.  It is suitable for very simple applications or tutorials.  In more sophisticated applications this general error checking should not be used.
#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( #x, err );                         \
      }                                                        \
  }

class DisplayManager {
	public: 
		bool m_gridShow;
		DisplayManager(B9CreatorSettings &b9CreatorSettings ) :
			m_b9CreatorSettings( b9CreatorSettings ),
			m_gridShow(false),
			m_die(false),
			m_pause(true),
			m_redraw(false),
			m_screenWidth(0),
			m_screenHeight(0),
			m_pDfb(NULL),
			m_grid(NULL),
			m_pPrimary(NULL)
	{
		if( pthread_create( &m_pthread, NULL, &displayThread, this) ){
			std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
				<< "Error: Could not create thread for frame buffer display."
				<< std::endl ;
			exit(1) ;
		}
	}

		~DisplayManager(){

			// kill loop in other thread
			m_die = true;
			//wait on other thread
	    pthread_join( m_pthread, NULL);
			//now, freeFB was already
			//called in the other thread
			//do not release this data again.

			/*Release the images.
			 * Attention: The vector contains pointers and
			 * not the objects itself.
			 * */
			
			std::vector<IDirectFBSurface*>::iterator it = m_imageSurfaces.begin();
			std::vector<IDirectFBSurface*>::iterator it_end = m_imageSurfaces.end();
			m_img_mutex.lock();
			for ( ; it < it_end ; it++ )
			{
				if( *it != NULL ) (*it)->Release( *it );
			}

			m_imageSurfaces.clear();
			m_img_mutex.unlock();
		}

		// Init & Start framebuffer loop
		void start();

		// Stop framebuffer loop
		void stop();

		/*Display (unscaled) image img. 
		 * img should contain a rgb, rgba or
		 * greyscaled image.
		 */
		void show(cv::Mat &img, cv::Point topLeftCorner=cv::Point(0,0) );
		void show( );

		/* Hide all displayed images. */
		void blank(bool black=true);

		/* Should only called by displayThread() */
		void run();

	private:
		//This is the super interface, it's the entry point to all functionality.
		IDirectFB *m_pDfb;
		//	The primary surface, i.e. the "screen".  In cooperative level DFSCL_FULLSCREEN it's the surface of the primary layer.
		IDirectFBSurface *m_pPrimary;

		//	Store the width and height of the primary surface here to support all resolutions.
		int m_screenWidth;
		int m_screenHeight;


		B9CreatorSettings &m_b9CreatorSettings;
		bool m_die; // quit frame buffer thread loop.
		bool m_pause; // pause frame buffer loop.
		bool m_redraw; //mark if redraw is ness.
		bool m_black; //mark if slices are not shown.
		pthread_t m_pthread;
		Mutex m_img_mutex;
		cv::Mat m_img;
		std::vector<IDirectFBSurface*> m_imageSurfaces;
		IDirectFBSurface* m_grid; 
		void createGrid();//init m_grid
		/* Create directfb objects */
		void initFB();
		/* Destroy directfb objects */
		void freeFB();
		/* Redraw whole primary surface */
		void redraw();
};


/* wrapper function for display thread.*/
static void* displayThread(void* arg){
	VPRINT("Start display thread\n");
	((DisplayManager*)arg)->run();
	VPRINT("Quit display thread\n");
}

#endif
