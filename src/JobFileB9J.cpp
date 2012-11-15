#include <iostream>
#include <string>
#include <cmath>

#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include "boost/filesystem.hpp" 
namespace fs = boost::filesystem; 

#include "constants.h"
#include "SimpleSupport.h"
#include "JobFileB9J.h"


JobFileB9J::JobFileB9J(const char* filename):
	JobFile(filename,1.0),
	m_support() {

	}

JobFileB9J::~JobFileB9J(){
}


cv::Mat JobFileB9J::loadSlice(int layer){
	cv::Mat ret;

	return ret;
}


void JobFileB9J::setScale(double scale){
	//No scale of pixel data
	m_scale = scale;
	return;
}

