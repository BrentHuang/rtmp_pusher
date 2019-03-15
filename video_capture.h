#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "capture_device.h"

typedef int (*VideoCaptureCB)(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp);

class VideoCapture : public CaptureDevice
{
public:
    VideoCapture();
    ~VideoCapture();

    void SetDeviceOpts(int width, int height, int frame_rate);
    void SetCaptureCB(VideoCaptureCB cb);
    int GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt);

private:
    void OnFrameReady(AVStream* stream, AVFrame* frame, int64_t timestamp) override;

private:
    int width_;
    int height_;
    int frame_rate_;
    VideoCaptureCB capture_cb_;
};

#endif // VIDEO_CAPTURE_H
