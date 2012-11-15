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

