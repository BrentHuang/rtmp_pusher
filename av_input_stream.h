#ifndef AV_INPUT_STREAM_H
#define AV_INPUT_STREAM_H

#include "video_capture.h"
#include "audio_capture.h"

class AVInputStream
{
public:
    AVInputStream();
    ~AVInputStream();

public:
    void SetVideoDeviceName(const std::string& fmt_name, const std::string& device_name, bool prefix);
    void SetMicrophoneName(const std::string& fmt_name, const std::string& device_name, bool prefix);
    void SetSpeakerName(const std::string& fmt_name, const std::string& device_name, bool prefix);

    void SetVideoDeviceOpts();
    void SetMicrophoneOpts();
    void SetSpeakerOpts();

    void SetVideoCaptureCB(CaptureCB cb);
    void SetMicrophoneCaptureCB(CaptureCB cb);
    void SetSpeakerCaptureCB(CaptureCB cb);

    int Open();
    void Close();

    int Start();
    void Stop();

    int GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt);
    int GetMicrophoneOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels);
    int GetSpeakerOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels);

private:
    VideoCapture video_capture_;
    AudioCapture microphone_;
    AudioCapture speaker_;
};

#endif // AV_INPUT_STREAM_H
