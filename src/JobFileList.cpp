#include <iostream>
#include <string>
#include <cmath>

#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "boost/filesystem.hpp" 
namespace fs = boost::filesystem; 

#include "constants.h"
#include "JobFileList.h"


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

