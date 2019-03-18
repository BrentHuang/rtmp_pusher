#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "capture_device.h"

class AudioCapture : public CaptureDevice
{
public:
    AudioCapture();
    ~AudioCapture();

    void SetDeviceOpts(int sample_rate, int channels);
    int GetAudioOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels);

private:
    int sample_rate_;
    int channels_;
};

#endif // AUDIO_CAPTURE_H
