#include <unistd.h>
#include <directfb.h>
#include "DisplayManager.h"
#include "B9CreatorSettings.h"

using namespace cv;
using namespace std;

/* Definiton of elements with dependenies to directfb */
class Sprite{
	public:
		IDirectFBSurface* m_pSurface; 
		cv::Point &m_position;
		cv::Mat cvmat; //cv struture with same data array as pSurface.
		Sprite(cv::Point &p):
			m_position(p),
			m_pSurface(NULL){
			}
};


/* ++++++++++ DisplayManager class functions +++++++++++++*/
DisplayManager::DisplayManager(B9CreatorSettings &b9CreatorSettings ) :
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

DisplayManager::~DisplayManager(){

			// kill loop in other thread
			m_die = true;
			//wait on other thread
	    pthread_join( m_pthread, NULL);
			//now, freeFB was already
			//called in the other thread
			//do not release this data again.
			freeFB();
		}

void DisplayManager::start(){
	m_pause = false;
}
void DisplayManager::stop(){
	m_pause = true;
}

void DisplayManager::createGrid(){

	DFBSurfaceDescription dsc2;
	dsc2.caps  = DSCAPS_NONE ;
	dsc2.flags = (DFBSurfaceDescriptionFlags) (DSDESC_HEIGHT | DSDESC_WIDTH | DSDESC_PIXELFORMAT);
	dsc2.width = m_screenWidth;
	dsc2.height = m_screenHeight;
	dsc2.pixelformat = DSPF_ARGB;

	DFBCHECK (m_pDfb->CreateSurface( m_pDfb, &dsc2, &m_grid ));

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

void DisplayManager::show(){
	m_blank = false;
	m_redraw = true; 
}

/*Release the images.
 * Attention: The vector contains pointers and
 * not the objects itself.
 * */
void DisplayManager::clear(){
			m_img_mutex.lock();
			std::vector<Sprite*>::iterator it = m_sprites.begin();
			const std::vector<Sprite*>::const_iterator it_end = m_sprites.end();
			for ( ; it < it_end ; ++it )
			{
				if( (*it)->m_pSurface != NULL ){
					VPRINT("(DisplayManager) Remove sprite\n");
					(*it)->m_pSurface->Release( (*it)->m_pSurface );
				}else{
					VPRINT("(DisplayManager) Sprite is null?!\n");
				}
				delete (*it);
			}
			m_sprites.clear();

			m_img_mutex.unlock();
}

void DisplayManager::add(cv::Mat &cvimg, cv::Point &topLeftCorner ){
	// Omit call m_pDfb->CreateSurface(...) for null pointer.
	if( m_pDfb == NULL ){
		VPRINT("(DisplayManager) m_pDfb is null. Abort add\n");
		return;
		initFB();
	}

	m_img_mutex.lock();

	Sprite *sprite = new Sprite(topLeftCorner);
	sprite->cvmat = cv::Mat(cvimg.size().height,4*cvimg.size().width,CV_8UC1);
	m_sprites.push_back( sprite );


	/*Die Farben in opencv sind in slices organisiert,
	 * bbbb,gggg,rrrr,aaaa,… , aber in directfb punktweise,
	 * argb, argb,… . Daher müssen die Daten
	 * umorganisiert werden. Da es den Typ in Opencv nicht gibt werden
	 * alle Kanäle in ein Graustufenbild mit 4*8bit gepackt und dann
	 * in directfb als "argb" interpretiert.
	 * Kann die Umwandlung vermieden/automatisiert werden?
	 * */
	typedef Vec<uchar, 4> VT;

	MatConstIterator_<VT> it1 = cvimg.begin<VT>(),
		it1_end = cvimg.end<VT>();
	MatIterator_<uchar> dst_it = sprite->cvmat.begin<uchar>();
  for( ; it1 != it1_end; ++it1, ++dst_it ) {
		VT pix1 = *it1;
		/**dst_it = (*it1.val[3] << 24) 
			+ (*it1.val[4] << 16)
			+ (*it1.val[4] << 8)
			+ (*it1.val[0]); */
		*dst_it = pix1[3];//saturate_cast<uchar>( pix1[3]);
		++dst_it;
		*dst_it = pix1[1];//saturate_cast<uchar>( pix1[3]);
		++dst_it;
		*dst_it = pix1[2];//saturate_cast<uchar>( pix1[3]);
		++dst_it;
		*dst_it = pix1[0];//saturate_cast<uchar>( pix1[3]);
	}

	DFBSurfaceDescription dsc;
	dsc.width = cvimg.size().width;
	dsc.height = cvimg.size().height;
	dsc.caps = DSCAPS_NONE;
	dsc.flags = (DFBSurfaceDescriptionFlags) 
		( DSDESC_HEIGHT | DSDESC_WIDTH | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT );
	dsc.pixelformat = DSPF_ARGB;
	dsc.preallocated[0].data = sprite->cvmat.data;      
	dsc.preallocated[0].pitch = dsc.width*4;
	dsc.preallocated[1].data = NULL;
	dsc.preallocated[1].pitch = 0;

	//DFBCHECK (m_pDfb->CreateSurface( m_pDfb, &dsc, &(sprite.m_pSurface) ));
	
	//printf("Test c. Pointer: %p\n", sprite->m_pSurface) ;
	DFBCHECK (m_pDfb->CreateSurface( m_pDfb, &dsc, &(sprite->m_pSurface)));
	m_img_mutex.unlock();
	VPRINT("(DisplayManager) Sprite added\n") ;
}

/* Hide all displayed images. */
void DisplayManager::blank(bool black){
	m_blank = black;
	m_redraw = true; 
}

void DisplayManager::redraw(){

	m_img_mutex.lock();
	//Clear the screen.
  DFBCHECK (m_pPrimary->SetColor (m_pPrimary, 0x00, 0x00, 0x00, 0xFF));//black
	DFBCHECK (m_pPrimary->FillRectangle (m_pPrimary, 0, 0, m_screenWidth, m_screenHeight));

	if( m_gridShow ){
		DFBCHECK (m_pPrimary->Blit(m_pPrimary, m_grid, NULL, 0, 0));
	}

	if( !m_blank ){
		std::vector<Sprite*>::iterator it = m_sprites.begin();
		const std::vector<Sprite*>::const_iterator it_end = m_sprites.end();
		for ( ; it < it_end ; ++it )
		{
			VPRINT("(DiplayManager) Draw sprite\n");
			if( (*it)->m_pSurface != NULL ){
				DFBCHECK (m_pPrimary->Blit(m_pPrimary,
							(*it)->m_pSurface, NULL, (*it)->m_position.x , (*it)->m_position.y ));
			}
		}

	}

	//Flip the front and back buffer, but wait for the vertical retrace to avoid tearing.
	DFBCHECK (m_pPrimary->Flip (m_pPrimary, NULL, DSFLIP_WAITFORSYNC));

	m_redraw = false;
	m_img_mutex.unlock();
}

/* Start thread with framebuffer loop */
void DisplayManager::initFB(){
	if( m_pDfb != NULL ){
		VPRINT("(DisplayManager) initFB() called twice.\n");
		return;
	}else{
		VPRINT("(DisplayManager) initFB() called.\n");
	}


	//create arg to disable cursor
	char foo[] = { "arg1" };
	char no_cur[] = { "--dfb:no-cursor" };
	//char no_cur[] = { "--dfb:debug" };
	int argc2 = /*argc+*/2;
	char** argv2 = (char**) malloc( (argc2)*sizeof(char*));
	//memcpy( argv2, argv, argc*sizeof(char*) );
	argv2[0] = foo;
	argv2[1] = no_cur;
	DFBCHECK (DirectFBInit (&argc2, &argv2));

	//A surface description is needed to create a surface.
	DFBSurfaceDescription dsc;

	DFBCHECK (DirectFBCreate (&m_pDfb));

	if( false ){
		//this made problems on my laptop?! I assume you can set
		//the value on your system.
		DFBCHECK (m_pDfb->SetCooperativeLevel (m_pDfb, DFSCL_FULLSCREEN));
	}

	dsc.flags = DSDESC_CAPS;
	dsc.caps  = (DFBSurfaceCapabilities)( DSCAPS_PRIMARY | DSCAPS_FLIPPING);
	DFBCHECK (m_pDfb->CreateSurface( m_pDfb, &dsc, &m_pPrimary ));
	DFBCHECK (m_pPrimary->GetSize (m_pPrimary, &m_screenWidth, &m_screenHeight));

	//Set blitting flags to DSBLIT_BLEND_ALPHACHANNEL that enables alpha blending using the alpha channel of the source.
	DFBCHECK (m_pPrimary->SetBlittingFlags (m_pPrimary, DSBLIT_BLEND_ALPHACHANNEL));

	createGrid();
	m_redraw = true;

}

/* Inverse operation of initFB */
void DisplayManager::freeFB(){
	m_img_mutex.lock();
	m_redraw = false;
	if( m_grid != NULL ) m_grid->Release (m_grid);
	m_grid = NULL;

	if( m_pPrimary != NULL ) m_pPrimary->Release (m_pPrimary);
	m_pPrimary = NULL;

	if( m_pDfb != NULL ) m_pDfb->Release (m_pDfb);
	m_pDfb  = NULL;

	m_img_mutex.unlock();
	clear();
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
				//check if display flag was unset 
				if(! m_b9CreatorSettings.m_display )
					m_pause = true;
			}

			freeFB();//quit fb
		}else{
			//wait
			usleep(1000000);
		}

		//check if display flag was set 
		if( m_b9CreatorSettings.m_display )
			m_pause = false;
	}

	freeFB();

	//quit of thread loop function
}























