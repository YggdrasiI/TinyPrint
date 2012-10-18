#include <unistd.h>
#include "DisplayManager.h"

using namespace cv;
using namespace std;

void DisplayManager::start(){
	m_pause = false;
}
void DisplayManager::stop(){
	m_pause = true;
}

void DisplayManager::createGrid(){

	/*DFBSurfaceDescription dsc;
	dsc.flags = DSDESC_CAPS;
	dsc2.caps  = (DFBSurfaceCapabilities)( DSCAPS_PRIMARY | DSCAPS_FLIPPING);*/
	DFBSurfaceDescription dsc2;
	dsc2.caps  = (DFBSurfaceCapabilities)( DSDESC_HEIGHT | DSDESC_WIDTH | DSDESC_PIXELFORMAT );
	//dsc2.caps = DSCAPS_NONE;
	dsc2.flags = (DFBSurfaceDescriptionFlags) (DSDESC_HEIGHT | DSDESC_WIDTH | DSDESC_PIXELFORMAT);
	dsc2.width = screen_width;
	dsc2.height = screen_height;
	dsc2.pixelformat = DSPF_ARGB;

	DFBCHECK (dfb->CreateSurface( dfb, &dsc2, &m_grid ));

  //DFBCHECK (primary->SetColor (primary, 0x00, 0xF0, 0xF0, 0xFF));//
  //DFBCHECK (primary->FillRectangle (primary, 0, 0, screen_width, screen_height ));

  //DFBCHECK (m_grid->SetColor (m_grid, 0x00, 0xFF, 0x00, 0x00));//transparent
  //DFBCHECK (m_grid->FillRectangle (m_grid, 0, 0, dsc2.width, dsc2.height ));

	// Set color of grid lines */
  DFBCHECK (m_grid->SetColor (m_grid, 
				m_b9CreatorSettings.m_gridColor[0],
				m_b9CreatorSettings.m_gridColor[1],
				m_b9CreatorSettings.m_gridColor[2],
				0xFF));
	/* Create 100px X 100px grid */
	int i;
	for( i=0; i<dsc2.width; i+=100){
		m_grid->DrawLine (m_grid, i, 0, i, dsc2.height - 1);
	}
	for( i=0; i<dsc2.height; i+=100){
		m_grid->DrawLine (m_grid, 0, i, dsc2.width - 1, i);
	}
}

void DisplayManager::show(cv::Mat img, cv::Point topLeftCorner ){


}

/* Hide all displayed images. */
void DisplayManager::blank(bool black){
	m_black = black;
	m_redraw = true; 
}

void DisplayManager::redraw(){

	m_img_mutex.lock();
	//Clear the screen.
  DFBCHECK (primary->SetColor (primary, 0xFF, 0x00, 0x00, 0x00));//black
	DFBCHECK (primary->FillRectangle (primary, 0, 0, screen_width, screen_height));

	if( m_gridShow ){
		DFBCHECK (primary->Blit(primary, m_grid, NULL, 0, 0));
	}

	if( !m_black ){
		//[...]
	}

	//Flip the front and back buffer, but wait for the vertical retrace to avoid tearing.
	DFBCHECK (primary->Flip (primary, NULL, DSFLIP_WAITFORSYNC));

	m_redraw = false;
	m_img_mutex.unlock();
}
/* Start thread with framebuffer loop */
void DisplayManager::initFB(){

	//create arg to disable cursor
	char foo[] = { "arg1" };
	char no_cur[] = { "--dfb:no-cursor" };
	int argc2 = /*argc+*/2;
	char** argv2 = (char**) malloc( (argc2)*sizeof(char*));
	//memcpy( argv2, argv, argc*sizeof(char*) );
	argv2[0] = foo;
	argv2[1] = no_cur;
	DFBCHECK (DirectFBInit (&argc2, &argv2));

	//A surface description is needed to create a surface.
	DFBSurfaceDescription dsc;

	DFBCHECK (DirectFBCreate (&dfb));

	if( false ){
		//this made problems on my laptop?! I assume you can set
		//the value on your system.
		DFBCHECK (dfb->SetCooperativeLevel (dfb, DFSCL_FULLSCREEN));
	}

	dsc.flags = DSDESC_CAPS;
	dsc.caps  = (DFBSurfaceCapabilities)( DSCAPS_PRIMARY | DSCAPS_FLIPPING);
	DFBCHECK (dfb->CreateSurface( dfb, &dsc, &primary ));
	DFBCHECK (primary->GetSize (primary, &screen_width, &screen_height));

	createGrid();
	m_redraw = true;

}

/* Inverse operation of initFB */
void DisplayManager::freeFB(){
	m_img_mutex.lock();
	if( primary != NULL ) primary->Release (primary);
	if( m_grid != NULL ) m_grid->Release (m_grid);
	if( dfb != NULL ) dfb->Release (dfb);
	m_img_mutex.unlock();
}

/* Should only called by displayThread() */
void DisplayManager::run(){


	while( !m_die ){

		if( !m_pause ){
			initFB();//start fb
			//init red grid surface
			m_redraw = true;

			while( !m_pause && !m_die ){

				if( m_b9CreatorSettings.m_gridShow != m_gridShow ){
					m_gridShow = m_b9CreatorSettings.m_gridShow;
					m_redraw = true;
				}

				if( m_redraw ){
					redraw();
					m_redraw = false;
				}

				usleep(10000);
			}

			freeFB();//quit fb
		}else{
			//wait
			usleep(1000000);
		}
	}

	//quit of thread loop function
}
