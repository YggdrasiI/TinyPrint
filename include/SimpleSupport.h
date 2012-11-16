#ifndef SIMPLESUPPORT_H
#define SIMPLESUPPORT_H

enum SupportType {st_CIRCLE, st_SQUARE, st_TRIANGLE, st_DIAMOND};

#include <opencv2/core/core.hpp>
class QDataStream;

class SimpleSupport {
	public:
		uint32_t m_type;
		uint32_t m_size;
		uint32_t m_start;
		uint32_t m_end;
		cv::Point m_point;//or QPoint	
	public:
		SimpleSupport():
			m_type(0),
			m_size(0),
			m_start(0),
			m_end(0),
			m_point(-1,-1){

			}

		void streamInSupport(QDataStream* pIn);
		void draw(cv::Mat &img ); 
};


#endif
