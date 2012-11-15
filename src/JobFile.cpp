#include <iostream>
#include <string>
#include <cmath>

#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "boost/filesystem.hpp" 
namespace fs = boost::filesystem; 

#include "constants.h"
#include "JobFile.h"

JobFile::JobFile(const char* filename, double scale):
	m_cache(),
	m_minLayer(0),
	m_maxLayer(-1),
	m_nmbrOfLayers(0),
	m_zResolution(-1),
	m_xyResolution(-1),
	m_scale(scale),
	m_position(cv::Point(0,0)),
	m_size(cv::Size(0,0)),
	m_description("No description."),
	m_filename(filename)
	{

	
	}

JobFile::~JobFile(){

}

/* Return reference to cv::Mat 
 * with the image data.
 * Attention, there is no check
 * included with test the m_minLayer
 * or m_maxLayer. Moreover, the returned
 * value is independed from m_minLayer.
 * */
cv::Mat JobFile::getSlice(int layer, SliceType type){

	// check cache.
	cv::Mat cache = m_cache.getImage(layer, type);
	if( !cache.empty() ) return cache;

	cv::Mat ret;

	switch( type ){
		case OVERCURE1:
			{
				/* This algorithm blur the slice and take the intensity of the pixel
				 * as border indicator.
				 * Pixel without any color change will set to black and only pixels
				 * at the border will left.
				 *
				 */
				int blurwidth = 9;
				int bwbw = blurwidth*blurwidth;
				uchar mins = 0.7*255;
				uchar maxs = 1*255;

				uchar colmap[256];
				for( int a=0; a<256; ++a ){
					if( a < mins ) colmap[a] = 255;
					else if( a > maxs ) colmap[a] = 0;
					else{
						//map linear from [maxs,mins] on [0,255].
						colmap[a] = 255* sqrt(sqrt( (float)((maxs-a))/(maxs-mins)));
					}
				}

				//Assume CV_8UC4
				const cv::Mat raw = getSlice(layer, RAW);
				cv::Mat tmp(raw.size(),raw.type());
				//cv::Mat tmp(raw.size(),CV_8UC4);
				cv::Size ksize(blurwidth, blurwidth);
				cv::blur(raw, tmp, ksize, cv::Point(-1,-1), cv::BORDER_CONSTANT);

				//Assume CV_8UC4 data
				typedef cv::Vec<uchar, 4> VT;
				cv::MatConstIterator_<VT> itRaw = raw.begin<VT>(),
					itRaw_end = raw.end<VT>();
				cv::MatIterator_<VT> itTmp = tmp.begin<VT>();
				for( ; itRaw != itRaw_end; ++itRaw, ++itTmp ) { 
					VT pixRaw = *itRaw;
					VT pixTmp = *itTmp;
					if( pixRaw[0] == 0 ){
						*itTmp = VT(0, 0, 0, 0 ); //transparent-black, rgba
					}else{
						*itTmp = VT(
								colmap[ pixTmp[0]],
								colmap[ pixTmp[1]],
								colmap[ pixTmp[2]],
								colmap[ pixTmp[0]]?255:0 //red=0=>full transparence.
								);
					}
				}  

				ret = tmp;
			}
			break;
		case RAW:
		default:
			{
				ret = loadSlice(layer);
			}
			break;
	}

	m_cache.putImage(layer, type, ret);
	return ret;
}




//+++++++++++++++++++++++++++++ JobFileSvg

JobFileSvg::JobFileSvg(const char* filename, double scale):
	JobFile(filename,scale),
	m_pCairo(NULL),
	m_pSurface(NULL),
	m_pRsvgHandle(NULL) {

		float dpi=254.0;
		GError *error = NULL;
		RsvgDimensionData dimensions;
		g_type_init();
		rsvg_set_default_dpi_x_y (dpi,dpi);//no reaction?
		m_pRsvgHandle = rsvg_handle_new_from_file (filename, &error);
		if( m_pRsvgHandle != NULL ){
			rsvg_handle_get_dimensions (m_pRsvgHandle, &dimensions);
			//m_position.x = (1024-dimensions.width)/2;
			//m_position.y = (768-dimensions.height)/2;

			m_pSurface = (cairo_surface_t *)cairo_image_surface_create(
					CAIRO_FORMAT_ARGB32,
					m_scale*dimensions.width, m_scale*dimensions.height
					//1024, 768
					);
			m_pCairo = cairo_create(m_pSurface);
			cairo_scale( m_pCairo, m_scale, m_scale);

			m_size.width = m_scale*dimensions.width;
			m_size.height = m_scale*dimensions.height;
			m_position.x = (1024- m_size.width)/2;
			m_position.y = (768- m_size.height)/2;

			/* Check existens of layers "#layer0","#layer1",...
			 * to get number of layers and the guarantee that
			 * no inner layer is missed.
			 * */
			m_nmbrOfLayers = 0;
			while( true ){
				std::ostringstream layerid;
				layerid << "#layer" << m_nmbrOfLayers;
				if( !rsvg_handle_has_sub( m_pRsvgHandle, layerid.str().c_str() )){
					break;
				}
				m_nmbrOfLayers++;
				//std::cout << "Found layer " << layerid.str() << std::endl;
			}
			std::cout << "Found layers: " << m_nmbrOfLayers << std::endl;
			m_maxLayer = m_nmbrOfLayers-1;

		}else{
			std::cout << "Error while loading file '"
				<< filename << "'." << std::endl;
			throw JOB_LOAD_EXCEPTION;
		}


	}

JobFileSvg::~JobFileSvg(){
	cairo_destroy (m_pCairo);
	cairo_surface_destroy (m_pSurface);
	g_object_unref (G_OBJECT (m_pRsvgHandle));
}


cv::Mat JobFileSvg::loadSlice(int layer){
	cv::Mat ret;

	//clear cairo context
	cairo_set_source_rgb (m_pCairo, 0, 0, 0);
	cairo_paint (m_pCairo);

	std::ostringstream id;
	id << "#layer" << layer;//filter with group name
	printf("(JobFile) Extract layer %s from svg\n",id.str().c_str() );
	rsvg_handle_render_cairo_sub(m_pRsvgHandle, m_pCairo, id.str().c_str() ); 

	ret = cv::Mat(
			cairo_image_surface_get_height(m_pSurface),
			cairo_image_surface_get_width(m_pSurface),
			CV_8UC4,
			(void*) cairo_image_surface_get_data(m_pSurface)
			);

	//convert to grayscale image
	//ret.convertTo( ret, CV_8UC1 );
	//convert to grayscale image with alpha channel
	//ret.convertTo( ret, CV_8UC2 );
	
	/* Remove interpolation B9Creator beta software approach to
	 * get black-white-image (see crushbitmap.cpp )
	 * if(c.red()<32 && c.blue()<32 && c.green()<32) return false;
	 */
	cv::threshold(ret, ret, 31, 255, cv::THRESH_BINARY);
	
	//use red values as mask (assume white/black image)
#ifdef FLIP_COLORS
	int from_to[] = { 0,0,  1,1,  2,2,  2,3 };
#else
	int from_to[] = { 0,0,  1,1,  2,2,  0,3 };
#endif
	cv::mixChannels( &ret, 1, &ret, 1, from_to, 4 );

	return ret;
}

void JobFileSvg::setScale(double scale){
	if( scale == m_scale ) return;
	m_scale = scale;

	//Flush cache of images
	m_cache.clear();
	
	//save current midpoint
	cv::Point mid(
			m_position.x + m_size.width/2,
			m_position.y + m_size.height/2 );

	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions (m_pRsvgHandle, &dimensions);

	//destroy and recreate cairo objects
	cairo_destroy (m_pCairo);
	cairo_surface_destroy (m_pSurface);

	m_pSurface = (cairo_surface_t *)cairo_image_surface_create(
					CAIRO_FORMAT_ARGB32,
					m_scale*dimensions.width, m_scale*dimensions.height
					);
	m_pCairo = cairo_create(m_pSurface);

	cairo_scale( m_pCairo, m_scale, m_scale);

	m_size.width = m_scale*dimensions.width;
	m_size.height = m_scale*dimensions.height;

	m_position.x = mid.x - m_size.width/2;
	m_position.y = mid.y - m_size.height/2;
}


//+++++++++++++++++++++++++++++ JobFileList

JobFileList::JobFileList(const char* filename, const char* pathPrefix):
	JobFile(filename,1.0),
	m_filelist() {

		// load list and check if each file exists.
		std::ifstream file(filename);
		std::string line;
		bool findAllFiles(true);
		int foundFiles=0;
		int notFoundFiles=0;

		if( file.is_open()){
			try{
				while (std::getline(file, line)) { 
					std::string imagepath(pathPrefix);
					imagepath.append("/");
					imagepath.append(line);
					if ( fs::exists(imagepath) ){
						m_filelist.push_back( imagepath );
						++foundFiles;
					}else{
						std::cerr << "Error while reading " << imagepath << std::endl;
						findAllFiles = false;
						++notFoundFiles;
					}
				}
			}
			catch ( const std::exception & ex ){
				std::cerr << "Error while reading " << filename << std::endl;
				throw JOB_LOAD_EXCEPTION;
			}
		}else{
			std::cerr << "Can not read " << filename << std::endl;
			throw JOB_LOAD_EXCEPTION;
		}

		std::cout << "Load " << filename << "." << std::endl;
		std::cout << "Number of found images: " << foundFiles << std::endl;
		std::cout << "Number of missing images: " << notFoundFiles << std::endl;

		if( notFoundFiles > 0 ){
				throw JOB_LOAD_EXCEPTION;
		}

		m_nmbrOfLayers = foundFiles;
		m_maxLayer = m_nmbrOfLayers-1;
		m_size = cv::Size(1024,768);

		//Load first slice to get correct size.
		cv::Mat tmp = getSlice(0, RAW);	
		m_size = tmp.size();
		m_position.x = (1024- m_size.width)/2;
		m_position.y = (768- m_size.height)/2;

	}

JobFileList::~JobFileList(){
}


cv::Mat JobFileList::loadSlice(int layer){
	cv::Mat raw;

	//raw = cv::imread( m_filelist[layer], CV_LOAD_IMAGE_GRAYSCALE ); 
	raw = cv::imread( m_filelist[layer], 1 /*RGB*/ );//transparency will be ignored. 
	//raw = cv::imread( m_filelist[layer], -1 /*RGBA, RBA or GRAYSCALE*/ ); 
	//
	/* Remove interpolation B9Creator beta software approach to
	 * get black-white-image (see crushbitmap.cpp )
	 * if(c.red()<32 && c.blue()<32 && c.green()<32) return false;
	 */
	cv::threshold(raw, raw, 31, 255, cv::THRESH_BINARY);

	//raw.convertTo( raw, CV_8UC4 );//hm, this add no alpha channel to rgb images...
	//use red channel as alpha channel.
	cv::Mat ret( raw.rows, raw.cols, CV_8UC4 );
#ifdef FLIP_COLORS
	int from_to[] = { 0,0,  1,1,  2,2,  2,3 };
#else
	int from_to[] = { 0,0,  1,1,  2,2,  0,3 };
#endif
	cv::mixChannels( &raw, 1, &ret, 1, from_to, 4 );

	return ret;
}


void JobFileList::setScale(double scale){
	//No scale of pixel data
	return;
}

