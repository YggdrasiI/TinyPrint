#ifndef JOBFILEB9J_H
#define JOBFILEB9J_H

class SimpleSupport;
class QDataStream;
class QBitArray;
class CrushedBitMap;
#include "JobFile.h"



/*
 * B9J File. Requires QT to 
 * load the datastream.
 * */
class JobFileB9J : public JobFile {
	private:
		std::vector<SimpleSupport> m_supports;
		std::vector<CrushedBitMap> m_packedSlices;
		std::string m_version;
		std::string m_xyPixel;
		std::string m_zLayer;
		int32_t m_base;
		int32_t m_filled;
		std::string m_reserved1;
		std::string m_reserved2;
		std::string m_reserved3;

		/* 'm_modelPosition' is the conterpart to B9Creators 'mJobExtends'
		 * The saved imagesdata
		 * of b9j files is always JobFile.m_size = (1024,768).
		 * and the inital value of JobFile.m_position is (0,0) (top left image corner).
		 * Thus, this data can not be used as infomation about the model dimensions.
		 * Use JobFileB9J.m_modelPosition to get the model location+dimension for B9J files.
		 * * Attention, I convert the data to opencv syntax:
		 *   cv::Rect stores (left,top,width,height),
		 *   QRect stores (left,top,right,bottom).
		 * */
		cv::Rect m_modelPosition;
	public:
		/*
		 * Filename: Path to list of images.
		 * Pathprefix: Parent dir for relative paths in list 'filename'
		 * */
		JobFileB9J(const char* filename);
		~JobFileB9J();
		void setScale(double scale);

	protected:
		/* Load raw image data. */
		cv::Mat loadSlice(int layer);
	private:
		void loadStream(QDataStream* pIn);

};

#endif
