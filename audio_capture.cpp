#include "audio_capture.h"

AudioCapture::AudioCapture()
{
    sample_rate_ = 0;
    channels_ = 0;
}

AudioCapture::~AudioCapture()
{
}

void AudioCapture::SetCaptureOpts(int sample_rate, int channels)
{
    sample_rate_ = sample_rate;
    channels_ = channels;
}
