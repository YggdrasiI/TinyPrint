#ifndef JOBFILE_H
#define JOBFILE_H

#include <librsvg/rsvg.h>
#include <cairo/cairo-svg.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


extern "C"{
gboolean    rsvg_handle_render_cairo     (RsvgHandle * handle, cairo_t * cr);
gboolean    rsvg_handle_render_cairo_sub (RsvgHandle * handle, cairo_t * cr, const char *id);
}

class JobFile{
	public:
		int m_zResolution; //unit: 10μm.
		int m_xyResolution; //unit: 10μm.
		cv::Point m_position;
		int m_minLayer; //cut of low layers
		int m_maxLayer; //cut of high layers
		int m_nmbrOfLayers;
	private:
		cairo_t *m_pCairo;
		cairo_surface_t *m_pSurface;
		RsvgHandle *m_pRsvgHandle;
		RsvgDimensionData m_dimensions;
		int m_layer;
		cv::Mat m_slice;
	public:
		JobFile(const char* filename);
		~JobFile();

		/* Return reference to cv::Mat 
		 * with the image data.
		 * Attention, there is no check
		 * included with test the m_minLayer
		 * or m_maxLayer. Moreover, the returned
		 * value is independed from m_minLayer.
		 * */
		cv::Mat &getSlice(int layer);

};

#endif
