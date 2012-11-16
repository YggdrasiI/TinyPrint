#ifndef JOBFILESVG_H
#define JOBFILESVG_H

#include "Mutex.h"
#include "JobFile.h"

class JobFileSvg : public JobFile {
	private:
		cairo_t *m_pCairo;
		cairo_surface_t *m_pSurface;
		RsvgHandle *m_pRsvgHandle;
		Mutex m_cairoMutex;
	public:
		JobFileSvg(const char* filename, double scale=1.0f);
		~JobFileSvg();
		void setScale(double scale);

	protected:
		/* Load raw image data. */
		cv::Mat loadSlice(int layer);


};

#endif
