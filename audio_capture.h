#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "capture_device.h"

typedef int (*AudioCaptureCB)(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp);

class AudioCapture : public CaptureDevice
{
public:
    AudioCapture();
    ~AudioCapture();

    void SetCaptureOpts(int sample_rate, int channels);

private:
    int sample_rate_;
    int channels_;
    AudioCaptureCB capture_cb_;
};

#endif // AUDIO_CAPTURE_H
