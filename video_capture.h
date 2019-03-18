#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "capture_device.h"

class VideoCapture : public CaptureDevice
{
public:
    VideoCapture();
    ~VideoCapture();

    void SetDeviceOpts(int width, int height, int frame_rate);
    int GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt);

private:
    int width_;
    int height_;
    int frame_rate_;
};

#endif // VIDEO_CAPTURE_H
