#include <iostream>
#include <string>
#include <cmath>

#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "boost/filesystem.hpp" 
namespace fs = boost::filesystem; 

#include "constants.h"
#include "JobFileSvg.h"

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


