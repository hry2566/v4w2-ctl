#ifndef CLSDIRECTSHOW_H
#define CLSDIRECTSHOW_H

#include <iostream>
#include <dshow.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>

struct video_formats
{
    int width;
    int height;
    double fps;
    std::string type;
    bool flag = false;
};

class ClsDirectShow
{
public:
    ClsDirectShow();
    virtual ~ClsDirectShow();
    IEnumMoniker *video_init(ICreateDevEnum *pDevEnum);
    void get_devices_list(IEnumMoniker *pClassEnum);
    void get_videoformats_list(int device_num, IEnumMoniker *pClassEnum);
    void get_camera_settings(int device_num, IEnumMoniker *pClassEnum);

protected:
private:
    void get_config(IBaseFilter *pbf);
    video_formats get_format_type(VIDEOINFOHEADER *video);
    void get_user_controls(IBaseFilter *pbf);
    void get_camera_controls(IBaseFilter *pbf);
};

#endif // CLSDIRECTSHOW_H
