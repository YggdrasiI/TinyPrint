#ifndef JOBFILE_H
#define JOBFILE_H

#include <queue>
#include <map>

#include <librsvg/rsvg.h>
#include <cairo/cairo-svg.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
//#include <opencv2/features2d/features2d.hpp>


extern "C"{
gboolean    rsvg_handle_render_cairo     (RsvgHandle * handle, cairo_t * cr);
gboolean    rsvg_handle_render_cairo_sub (RsvgHandle * handle, cairo_t * cr, const char *id);
}

/* Simple cache for image data. Every call
 * of JobFile.getSlice() should look into this
 * cache.
 * */
class ImageCache{
	private:
		std::map<int, cv::Mat> m_map;
		std::queue<int> m_queue;
		long m_bytes;

		/*inline*/ int hash(int i, SliceType t){
			return (i<<4) + t;
		}

	public:
		ImageCache(): m_map(), m_queue(){

		}

		/* Returns empty matrix if no image is found.
		 * cv:Mat objects are cheap. Thus it is
		 * not ness. to operate with pointers or
		 * references.
		 * */
		cv::Mat getImage(int slice, SliceType type){
			cv::Mat empty;

			const std::map<int, cv::Mat>::iterator it =
				m_map.find( hash(slice, type) );
			const std::map<int, cv::Mat>::iterator it_end =
				m_map.end();


			if( it == it_end ) return empty;
			VPRINT("(JobFile) Return cached image\n");
			return it->second;
		}

		/* Add image to cache. */
		bool putImage(const int slice, const SliceType type, const cv::Mat mat){
			int index = hash(slice,type);
			m_map[ index  ] = mat;
			m_queue.push( index );
			m_bytes += mat.elemSize() * mat.size().area();

			while( m_queue.size() > JOBFILE_CACHE_MAX_NMBR_OF_IMAGES ){
				const std::map<int, cv::Mat>::iterator it = m_map.find( m_queue.front() );

				m_map.erase( m_queue.front() );
				m_queue.pop();
			}

		}

		void clear(){
			m_map.clear();
			while(! m_queue.empty() )	m_queue.pop();
			m_bytes = 0;
		}

};

/* Abstract class for JobFiles
 * There should be children for the possible cases
 * 1. Loading svg files
 * 2. Loading raw layers from folders.
 * 3. Loading b9j files
 * */
class JobFile{
	public:
		int m_zResolution; //unit: 10μm.
		int m_xyResolution; //unit: 10μm.
		cv::Point m_position;
		cv::Size m_size;
		int m_minLayer; //cut of low layers
		int m_maxLayer; //cut of high layers
		int m_nmbrOfLayers;
		std::string m_description;
		std::string m_filename;
	protected:
		double m_scale;
		ImageCache m_cache;
	public:
		JobFile(const char* filename, double scale=1.0f);
		~JobFile();

		/* Return reference to cv::Mat 
		 * with the image data.
		 * Attention, there is no check
		 * included with test the m_minLayer
		 * or m_maxLayer. Moreover, the returned
		 * value is independed from m_minLayer.
		 * */
		cv::Mat getSlice(int layer, SliceType=RAW);
		virtual void setScale(double scale) = 0;
		double getScale() const{  
			return m_scale;
		}

	protected:
		/* Load raw image data. */
		virtual cv::Mat loadSlice(int layer) = 0;


};

class JobFileSvg : public JobFile {
	private:
		cairo_t *m_pCairo;
		cairo_surface_t *m_pSurface;
		RsvgHandle *m_pRsvgHandle;
	public:
		JobFileSvg(const char* filename, double scale=1.0f);
		~JobFileSvg();
		void setScale(double scale);

	protected:
		/* Load raw image data. */
		cv::Mat loadSlice(int layer);


};

/*
 * Use list of filenames to load
 * image for each layer.
 * Filenames are relative to job_files folder.
 * This JobFile type allows no scaling.
 * */
class JobFileList : public JobFile {
	private:
		std::vector<std::string> m_filelist;
	public:
		/*
		 * Filename: Path to list of images.
		 * Pathprefix: Parent dir for relative paths in list 'filename'
		 * */
		JobFileList(const char* filename, const char* pathPrefix);
		~JobFileList();
		void setScale(double scale);

	protected:
		/* Load raw image data. */
		cv::Mat loadSlice(int layer);

};

#endif
