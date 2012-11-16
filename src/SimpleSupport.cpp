#include <QDataStream>
#include <QPoint>

//#include <QPixmap>
//#include <QBitArray>
//#include <QFile>
//#include <QtCore/qmath.h>


#include "SimpleSupport.h"

void SimpleSupport::streamInSupport(QDataStream* pIn){
  QPoint tempPoint;
  *pIn >> tempPoint >> m_type >> m_size >> m_start >> m_end;
	m_point.x = tempPoint.x();
	m_point.y = tempPoint.y();
}

// Render the support to the image
void SimpleSupport::draw(cv::Mat &img ) {
	const cv::Scalar col(255);
	int ihalfSize = m_size/2;
	float ihalfBase = m_size/1.73205081;
	cv::Point points[4];

	switch( (SupportType) m_type ) {
		case st_CIRCLE:	
			{
				cv::circle( img, m_point, ihalfSize, col, -1);
			}
			break;
		case st_SQUARE:
			{
				cv::Rect r(m_point.x-ihalfSize,m_point.y-ihalfSize,m_size,m_size);
				img(r) = col;
			}
			break;
		case st_TRIANGLE:
			{
				points[0].x = m_point.x;
				points[0].y = m_point.y - ihalfSize; 

				points[1].x = m_point.x + ihalfBase;
				points[1].y = m_point.y + ihalfSize;

				points[2].x = m_point.x - ihalfBase;
				points[2].y = m_point.y + ihalfSize;

				const cv::Point *entries[1]; entries[0] = points;
				int entryLengths[1]; entryLengths[0] = 3;
				cv::fillPoly(img, entries, entryLengths, 1, col);
			}
			break;
		case st_DIAMOND:
			{
				points[0].x = m_point.x;
				points[0].y = m_point.y - ihalfSize; 

				points[1].x = m_point.x + ihalfBase;
				points[1].y = m_point.y;

				points[2].x = m_point.x;
				points[2].y = m_point.y + ihalfSize;

				points[3].x = m_point.x - ihalfBase;
				points[3].y = m_point.y;

				const cv::Point *entries[1]; entries[0] = points;
				int entryLengths[1]; entryLengths[0] = 4;
				cv::fillPoly(img, entries, entryLengths, 1, col);
			}
			break;
		default:
			break;
	}

}

