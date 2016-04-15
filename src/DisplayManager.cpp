#include <unistd.h>
#include <stdint.h>

#include <directfb.h>
#include <boost/bind.hpp>

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
DisplayManager::DisplayManager(B9CreatorSettings &b9CreatorSettings, vector<std::string> &dfbOptions ) :
	m_b9CreatorSettings( b9CreatorSettings ),
	m_dfbOptions(dfbOptions),
	m_gridShow(false),
	m_die(false),
	m_pause(true),
	m_redraw(false),
	m_png_redraw(false),
	m_png_scale(-1),
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

	//connect B9CreatorSettings update signal
	m_b9CreatorSettings.updateSettings.connect(
					boost::bind(&DisplayManager::updateSignalHandler,this, _1 )
					);
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
#ifdef FLIP_COLORS
	//brga
  DFBCHECK (m_grid->SetColor (m_grid, 
				m_b9CreatorSettings.m_gridColor[2],
				m_b9CreatorSettings.m_gridColor[1],
				m_b9CreatorSettings.m_gridColor[0],
				0xFF));
#else
	//rgba
  DFBCHECK (m_grid->SetColor (m_grid, 
				m_b9CreatorSettings.m_gridColor[0],
				m_b9CreatorSettings.m_gridColor[1],
				m_b9CreatorSettings.m_gridColor[2],
				0xFF));
#endif
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
	if( m_pDfb == NULL || m_pPrimary == NULL ){
		VPRINT("(DisplayManager) m_pDfb is null. Abort add\n");
		return;
		initFB();
	}


	m_img_mutex.lock();
	Sprite *sprite = new Sprite(topLeftCorner);
	sprite->cvmat = cv::Mat(cvimg.size().height,4*cvimg.size().width,CV_8UC1);

	/*Die Farben in opencv sind in slices organisiert,
	 * bbbb,gggg,rrrr,aaaa,… , aber in directfb punktweise,
	 * argb, argb,… . Daher müssen die Daten
	 * umorganisiert werden. Da es den Typ in Opencv nicht gibt, werden
	 * alle Kanäle in ein Graustufenbild mit 4*8bit gepackt und dann
	 * in directfb als "argb" interpretiert.
	 * Auf einem 'Big Endian' System ist die Bytereihenfolge allerdings
	 * bgra.
	 * */
	typedef Vec<uchar, 4> VT;

	if( m_b9CreatorSettings.m_flipSprites ){
		//flip image vertical

		MatIterator_<uchar> dst_it = sprite->cvmat.begin<uchar>();
		for( int r = cvimg.rows-1 ; r>=0; --r ){ //<== TODO: Remove redundant loop?! 
			cv::Mat row = cvimg.row(r);
			MatConstIterator_<VT> it1 = row.begin<VT>(),
				it1_end = row.end<VT>();
			for( ; it1 != it1_end; ++it1, ++dst_it ) {
				VT pix = *it1;
#ifdef FLIP_COLORS
				//bgra	
				 *dst_it = pix[2];
				 ++dst_it;
				 *dst_it = pix[1];
				 ++dst_it;
				 *dst_it = pix[0];
				 ++dst_it;
				 *dst_it = pix[3];
#else
				//argb
				//*dst_it = (pix[3] << 24) | (pix[2] << 16) | (pix[1] << 8) | pix[0];
				 *dst_it = pix[3];
				 ++dst_it;
				 *dst_it = pix[0];
				 ++dst_it;
				 *dst_it = pix[1];
				 ++dst_it;
				 *dst_it = pix[2];
#endif

			}		
		}
	}else{
		MatConstIterator_<VT> it1 = cvimg.begin<VT>(),
				it1_end = cvimg.end<VT>();
		MatIterator_<uchar> dst_it = sprite->cvmat.begin<uchar>();
		for( ; it1 != it1_end; ++it1, ++dst_it ) {
			VT pix = *it1;
#ifdef FLIP_COLORS
				//bgra	
				 *dst_it = pix[2];
				 ++dst_it;
				 *dst_it = pix[1];
				 ++dst_it;
				 *dst_it = pix[0];
				 ++dst_it;
				 *dst_it = pix[3];
#else
				//argb
				//*dst_it = (pix[3] << 24) | (pix[2] << 16) | (pix[1] << 8) | pix[0];
				 *dst_it = pix[3];
				 ++dst_it;
				 *dst_it = pix[0];
				 ++dst_it;
				 *dst_it = pix[1];
				 ++dst_it;
				 *dst_it = pix[2];
#endif
		}
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

	//m_img_mutex.lock();
	m_sprites.push_back( sprite );
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
	m_png_redraw = true;
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


	m_img_mutex.lock();
	
	/* Create options for directfb window. This depends
	 * on your system.
	 * */
	//char foo[] = { "ignored first arg" };
	//char no_cur[] = { "--dfb:no-cursor" };
	//char mode[] = { "--dfb:mode=1024x768" };
	//char pixelformat[] = { "--dfb:pixelformat=RGB16" };
	//char pixelformat[] = { "--dfb:pixelformat=RGB32" };

	int argc2 = m_dfbOptions.size()/*+2*/;
	char** argv2 = (char**) malloc( (argc2)*sizeof(char*));

	for( int i=0; i<argc2; ++i ){
		argv2[i] = (char*) m_dfbOptions[i].c_str();//points into char array in std::string obj.
	}
	DFBCHECK (DirectFBInit (&argc2, &argv2));
	free(argv2);

	//search for 'nofullscreen' flag
	bool fullscreen(true);
	for( int i=0; i<argc2; ++i ){
		if( m_dfbOptions[i].compare("--nofullscreen") == 0 ){
			fullscreen = false;
			break;
		}
	}


	//A surface description is needed to create a surface.
	DFBSurfaceDescription dsc;

	DFBCHECK (DirectFBCreate (&m_pDfb));

	if( fullscreen ){
		// This was problematic on one of my systems.
		// Do not know why and add an argument to disable
		// line.
		DFBCHECK (m_pDfb->SetCooperativeLevel (m_pDfb, DFSCL_FULLSCREEN));
	}

	//dsc.flags = (DFBSurfaceDescriptionFlags) (DSDESC_CAPS );//16bit as default on raspbian
	dsc.pixelformat = DSPF_ARGB;
	dsc.flags = (DFBSurfaceDescriptionFlags) (DSDESC_CAPS | DSDESC_PIXELFORMAT);
	dsc.caps  = (DFBSurfaceCapabilities)( DSCAPS_PRIMARY | DSCAPS_FLIPPING);
	DFBCHECK (m_pDfb->CreateSurface( m_pDfb, &dsc, &m_pPrimary ));
	DFBCHECK (m_pPrimary->GetSize (m_pPrimary, &m_screenWidth, &m_screenHeight));

	//Set blitting flags to DSBLIT_BLEND_ALPHACHANNEL that enables alpha blending using the alpha channel of the source.
	DFBCHECK (m_pPrimary->SetBlittingFlags (m_pPrimary, DSBLIT_BLEND_ALPHACHANNEL));

	createGrid();

	//set flipping flag
	//setFlipping(true);

	m_redraw = true;
	m_img_mutex.unlock();

}

/* Inverse operation of initFB */
void DisplayManager::freeFB(){
	clear();
	m_img_mutex.lock();
	m_redraw = false;
	if( m_grid != NULL ) m_grid->Release (m_grid);
	m_grid = NULL;

	if( m_pPrimary != NULL ) m_pPrimary->Release (m_pPrimary);
	m_pPrimary = NULL;

	if( m_pDfb != NULL ) m_pDfb->Release (m_pDfb);
	m_pDfb  = NULL;

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
					std::cout << "Redraw\n";
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


void DisplayManager::updateSignalHandler(int changes){
	if( changes & REDRAW )
		m_redraw = true;
}



/* Use Sprite data to generate png of current screen
 *
 * */
bool DisplayManager::getDisplayedImage(Onion::Request *preq, int actionid, Onion::Response *pres){

	switch(actionid){
		case 10:
			{ /* Generate png image */
				int scale = atoi( onion_request_get_queryd(preq->c_handler(),"scale","100") );
				const char *force = onion_request_get_queryd(preq->c_handler(),"force","0");

				//check if png generation is forced
				if( *force == '1' ) m_png_redraw = true;

				VPRINT("redraw: %i, scale: %i, newscale: %i", m_png_redraw?1:0, m_png_scale, scale);

				if( m_redraw ){
					/* An redraw action is pending. Do not 
					 * create a png with the old image data
					 * and wait on new image.
					 * This can cause by fast layer switches on the
					 * webinterface.
					 * */
					unsigned char image[4];
					image[0] = 0; image[1] = 0; image[2] = 0; image[3] = 0;
					onion_png_response( image, 4, 1, 1, pres->c_handler() );
					return true;
				}

				if( !m_png_redraw && scale==m_png_scale ){
					//There was no change between the last sended image.
					//std::string reply = "noNewImage";
					//onion_response_write(res, reply.c_str(), reply.size() ); 
					unsigned char image[4];
					image[0] = 0; image[1] = 0; image[2] = 0; image[3] = 0;
					onion_png_response( image, 4, 1, 1, pres->c_handler() );
					return true;
				}


				if( m_pDfb == NULL || m_pPrimary == NULL ){
					//Display is not active
					//generate 1x1 pixel
					unsigned char image[4];
					image[0] = 0; image[1] = 0; image[2] = 0; image[3] = 0;
					onion_png_response( image, 4, 1, 1, pres->c_handler() );
					m_png_redraw = false;
					return true;
				}

				DFBSurfacePixelFormat format; // Should be DSPF_ARGB
				int channels=1;//4=RGBA, -4=ABRG 

				m_pPrimary->GetPixelFormat(m_pPrimary, &format);
				//printf("Pixelformat: %i", (int)format);
				if( format == DSPF_ARGB ){
#ifdef FLIP_COLORS
					channels=4;
#else
					channels=-4;
#endif
				}else{
					//generate 1x1 pixel
					unsigned char image[4];
					image[0] = 0; image[1] = 0; image[2] = 0; image[3] = 0;
					onion_png_response( image, 4, 1, 1, pres->c_handler() );
					m_png_redraw = false;
					m_png_scale  = scale;
					return true;
				}

				int width;
				int height;
				m_pPrimary->GetSize(m_pPrimary,&width,&height);

				void *data_ptr;
				int pitch;

				m_img_mutex.lock();
				m_pPrimary->Lock(m_pPrimary, DSLF_READ, &data_ptr, &pitch);

				if( scale == 100 ){
					onion_png_response( (unsigned char*) data_ptr , channels, width, height, pres->c_handler() );
				}else{
					//rescale to 50% or 25%				
					int w2 = width*scale/100;
					int h2 = height*scale/100;
					int incW = width/w2;
					int incH = incW;//height/h2;

					unsigned char image[4*w2*h2];

					uint32_t *pdata = (uint32_t*) data_ptr;//4*char, ARGB
					uint32_t *pimage = (uint32_t*) image;

					uint32_t *nextRowImage = pimage + w2;
					uint32_t *nextRowData = pdata +  incH*width;
					for( int i=0; i<h2; ++i ){
						for( ; pimage<nextRowImage; ++pimage, pdata+=incW ){
							*pimage = *pdata;
						}
						nextRowImage += w2;
						pdata = nextRowData;
					  nextRowData = pdata +  incH*width;
					}

					onion_png_response( image , channels, w2, h2, pres->c_handler() );
				}


				m_pPrimary->Unlock(m_pPrimary);
				m_img_mutex.unlock();

				m_png_redraw = false;
				m_png_scale  = scale;

				return true;
			}
			break;
		default:
			break;
	}

	return false; 
}



void DisplayManager::setFlipping(bool flip){
	if( m_pPrimary == NULL ) return;
	//Require > DirectFB 1.4.x
	/*DFBSurfaceBlittingFlags	flags = DSBLIT_FLIP_HORIZONTAL; 
	m_pPrimary->SetBlittingFlags ( m_pPrimary, flags	);
	*/
	VPRINT("setFlipping require DirectFB 1.4.x\n");
}










