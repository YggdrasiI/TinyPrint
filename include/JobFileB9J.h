#ifndef JOBFILEB9J_H
#define JOBFILEB9J_H

#include "JobFile.h"
class SimpleSupport;

/*
 * B9J File. Requires QT to 
 * load the datastream.
 * */
class JobFileB9J : public JobFile {
	private:
		std::vector<SimpleSupport> m_support;
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

};

#endif
