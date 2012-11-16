#include <iostream>
#include <string>
#include <cmath>

#include "constants.h"
#include "SimpleSupport.h"
#include "JobFileB9J.h"

#include <QString>
#include <QDataStream>
#include <QFile>
#include <QPixmap>
#include <QBitArray>



//class for a 'packed slice' (Minimal subset of data struture from B9Creator)
class CrushedBitMap{
	private:
		bool m_bIsBaseLayer;
	public:
		uint32_t uiWhitePixels;
		int m_iWidth, m_iHeight;
		QRect mExtents;
		QBitArray mBitarray;

		CrushedBitMap():
			uiWhitePixels(0),
			m_iWidth(-1),
			m_iHeight(-1),
			m_bIsBaseLayer(false),
			mExtents(),
			mBitarray() {

			}

		void streamInCMB(QDataStream* pIn) {
			*pIn >> uiWhitePixels >> mExtents >> mBitarray;
			m_iWidth  = getBits(0,16);
			m_iHeight = getBits(16,16);
		}

		/* Return Number
		 *  'map[begin] map[begin+1] .... map[begin+n-1]'
		 * First bit is highest one.
		 * */
		int getBits( int begin, int n){
			if( n>32 || n<1 ) return 0;
			int ret = mBitarray.testBit(begin);
			int end = begin+n;
			for( ++begin; begin<end ; ++begin ){
				ret = (ret << 1) + mBitarray.testBit(begin) ;
			}
			return ret;
		}

		cv::Mat getImage(){
			//Mike read width and height values from bitfield, but
			//they should be already set.
			cv::Mat raw(cv::Size(m_iWidth, m_iHeight), CV_8UC1);
			cv::MatIterator_<uchar> pixel = raw.begin<uchar>();//iterator loop through image
			cv::MatConstIterator_<uchar> colChange;//iterator will mark end of same color
			const cv::MatConstIterator_<uchar> pixel_end = raw.end<uchar>();//end of image

			uchar curBrushCol = getBits(32,1)?255:0;
			int index = 33;
			const int index_end = mBitarray.size();
			int iKey = getBits(index,5);
			int iSameCol = getBits(index+5,iKey+1);
			index += 6+iKey;
			while( (pixel < pixel_end) && iSameCol > 0 ){
				colChange = pixel + iSameCol;
				//colChange = min(pixel + iSameCol, pixel_end);
				for( ; pixel < colChange ; ++pixel ){
					*pixel = curBrushCol;
				}
				if( index >= index_end ){
#ifdef VERBOSE
					std::cout << "Abort access behind end of bit array." << std::endl;
#endif
					break;
				}
				iKey = getBits(index,5);
				iSameCol = getBits(index+5,iKey+1);
				index += 6+iKey;
				//Toggle brush color
				curBrushCol = 255-curBrushCol;
			}

			//use data for all four channels.
			/*
				 int from_to[] = { 0,0,  0,1,  0,2,  0,3 };
				 cv::Mat ret( raw.rows, raw.cols, CV_8UC4 );
				 cv::mixChannels( &raw, 1, &ret, 1, from_to, 4 );
				 */

			return raw;
		}
};


//convert QString to std::string
std::string qToS( QString q ){
	std::string ret;
	QString::const_iterator begin = q.begin(),
		end = q.end();
	for( ; begin != end; ++begin ){
		ret.append(1, (*begin).toLatin1() );
	}

	return ret;
}



JobFileB9J::JobFileB9J(const char* filename):
	JobFile(filename,1.0),
	m_packedSlices(),
	m_supports(),
	m_modelPosition(0,0,0,0){

		//Read file
		QString infile(filename);
		QFile file(infile);

		if(!file.open(QIODevice::ReadOnly)){
			std::cout << "Error while loading file '"
				<< filename << "'." << std::endl;
			throw JOB_LOAD_EXCEPTION;
		}

		QDataStream in(&file);
		loadStream(&in);
		file.close();

	}

JobFileB9J::~JobFileB9J(){
}


cv::Mat JobFileB9J::loadSlice(int layer){
	if( layer > m_maxLayer || layer < 0 ){
		cv::Mat empty( cv::Size(1,1), CV_8UC4, cv::Scalar(0,0,0,0));
		return empty;
	}

	cv::Mat retC1;//only one color channel is required.

	if( layer < m_base ){
		/*This images will filled with support structs
		 * and/or fill layer */
		retC1 = cv::Mat(m_size, CV_8UC1, cv::Scalar(0) );
	}else{
		//getImage return 8UC1 image
		retC1 = m_packedSlices[layer-m_base].getImage();
	}

	if( layer < m_filled ) {
		//fill first layers with a white
		//square on m_modelPosition.

		//std::cout << m_modelPosition.x << ", " << m_modelPosition.y << ", "
		//	<< m_modelPosition.width << ", " << m_modelPosition.height << std::endl;
		retC1(m_modelPosition) = cv::Scalar(255);

	}else{
		// Render Supports
		//Loop through supports list, if current slice has support, draw it
		for(int i=0; i<m_supports.size(); i++){
			if( ( layer<m_base && m_supports[i].m_start==0 ) ||
					( layer >= m_supports[i].m_start + m_base &&
						layer <= m_supports[i].m_end + m_base )
				){
				m_supports[i].draw(retC1);
			}
		}
	}

	//use retC1 data for all four channels.
	int from_to[] = { 0,0,  0,1,  0,2,  0,3 };
	cv::Mat ret( retC1.size(), CV_8UC4 );
	cv::mixChannels( &retC1, 1, &ret, 1, from_to, 4 );

	return ret;
}


void JobFileB9J::setScale(double scale){
	//No scale of pixel data
	return;
}

void JobFileB9J::loadStream(QDataStream* pIn){
	//0. Clear data
	m_packedSlices.clear();
	m_supports.clear();

	//1. Read First part (Header)
	QString qVersion, qName, qDescription,
					qr1, qr2, qr3;
	double xy, zl;

	*pIn >> qVersion >> qName >> qDescription
		>> xy >> zl
		>> m_base >> m_filled
		>> qr3 >> qr2 >> qr1;

	std::cout << "xy: " << xy << std::endl;
	//convert qstring to std::string
	m_version = qToS(qVersion);
	m_filename = qToS(qName);
	m_description = qToS(qDescription);
	/*
	std::ostringstream xystr; xystr << xy*1000; 
	std::ostringstream zlstr; zlstr << zl*1000; 
	m_xyPixel = xystr.str();
	m_zLayer = zlstr.str();
	*/
	m_xyPixel = 1000*xy;
	m_zLayer = 1000*zl;
	m_reserved3 = qToS(qr3);
	m_reserved2 = qToS(qr2);
	m_reserved1 = qToS(qr1);

	//check if version is supported
	int version;
	sscanf( m_version.c_str() , "%d ", &version );
	if( version > 1 ){
		throw JOB_LOAD_EXCEPTION;
	}

	//2. Read slices. This is a list of BitArray objects
	int sliceSize = -1;
	int width = 0, height = 0;
	int r=0, l=1000000, b=0, t=1000000;
	*pIn >> sliceSize;
	for( int i=0; i<sliceSize; ++i){
		//Read slice
#ifdef VERBOSE
		std::cout << "Read Slice " << i << std::endl;
#endif
		CrushedBitMap cbm;
		m_packedSlices.push_back( cbm );
		CrushedBitMap &s = m_packedSlices[i];
		s.streamInCMB(pIn);

		if( r < s.mExtents.right() ) r = s.mExtents.right();
		if( l > s.mExtents.left() ) l = s.mExtents.left();
		if( b < s.mExtents.bottom() ) b = s.mExtents.bottom();
		if( t > s.mExtents.top() ) t = s.mExtents.top();

		width = max(width, s.m_iWidth);
		height = max(height, s.m_iHeight);
	}

	m_nmbrOfLayers = sliceSize+ m_base;
	m_minLayer = 0;
	m_maxLayer = m_nmbrOfLayers-1;

	m_size = cv::Size(width,height);
	m_position.x = (1024- m_size.width)/2;
	m_position.y = (768- m_size.height)/2;

	m_modelPosition.x = l;
	m_modelPosition.y = t;
	m_modelPosition.width = r-l+1;
	m_modelPosition.height = b-t+1;


	//3. Read support objects
	int supportSize = -1;
	*pIn >> supportSize;
	for( int i=0; i<supportSize; ++i){
		//Read support
		SimpleSupport s;
		m_supports.push_back( s );
		m_supports[i].streamInSupport(pIn);
	}


}


