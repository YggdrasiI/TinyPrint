/*
 * Mange diplay of fullscreen images.
 * Based on Directfb framework.
 * */

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <pthread.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

//#include <directfb.h>
/* To remove the directfb dependency to directfb
 * in this header a few Types are predefined here */
#define DECLARE_INTERFACE( IFACE )                \
	typedef struct _##IFACE IFACE;
DECLARE_INTERFACE( IDirectFB )
DECLARE_INTERFACE( IDirectFBSurface )


#include "constants.h"
#include "Mutex.h"
class B9CreatorSettings;

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

//#define DFBCHECK(x...)                                         

class Sprite;
static void* displayThread(void* arg);

class DisplayManager {
	public: 
		bool m_gridShow;
		DisplayManager(B9CreatorSettings &b9CreatorSettings, std::vector<std::string> &dfbOptions ); 
		~DisplayManager();

		// Init & Start framebuffer loop
		void start();

		// Stop framebuffer loop
		void stop();

		/*Add (unscaled) image img. 
		 * img should contain a rgb, rgba or
		 * greyscaled image.
		 */
		void add(cv::Mat &img, cv::Point &topLeftCorner );
		/* Remove added images */
		void clear();
		/* Redraw images */
		void show( );

		/* Hide all displayed images. */
		void blank(bool black=true);

		/* Should only called by displayThread() */
		void run();

		/* Will called if m_b9CreatorSettings propagate settings change. */
		void updateSignalHandler(int changes);

	private:
		//This is the super interface, it's the entry point to all functionality.
		IDirectFB *m_pDfb;
		//	The primary surface, i.e. the "screen".  In cooperative level DFSCL_FULLSCREEN it's the surface of the primary layer.
		IDirectFBSurface *m_pPrimary;
		std::vector<std::string> &m_dfbOptions;

		//	Store the width and height of the primary surface here to support all resolutions.
		int m_screenWidth;
		int m_screenHeight;


		B9CreatorSettings &m_b9CreatorSettings;
		bool m_die; // quit frame buffer thread loop.
		bool m_pause; // pause frame buffer loop.
		bool m_redraw; //mark if redraw is ness.
		bool m_blank; //mark if slices are not shown.
		pthread_t m_pthread;
		Mutex m_img_mutex;
		//std::vector<IDirectFBSurface*> m_imageSurfaces;
		std::vector<Sprite*> m_sprites;
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
