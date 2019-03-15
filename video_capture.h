#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "capture_device.h"

typedef int (*VideoCaptureCB)(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp);

class VideoCapture : public CaptureDevice
{
public:
    VideoCapture();
    ~VideoCapture();

    void SetVideoOpts(int width, int height, int frame_rate);
    int GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt);

    int Open();
    void Close();
    int Start();

private:
    int width_;
    int height_;
    int frame_rate_;
    VideoCaptureCB capture_cb_;
};

#endif // VIDEO_CAPTURE_H
