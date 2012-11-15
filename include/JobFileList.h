#ifndef JOBFILELIST_H
#define JOBFILELIST_H

#include "JobFile.h"

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
