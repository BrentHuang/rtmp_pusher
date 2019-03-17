#include "av_input_stream.h"
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif

#include "global.h"

AVInputStream::AVInputStream() : video_capture_(), microphone_(), speaker_()
{
}

AVInputStream::~AVInputStream()
{
}

void AVInputStream::SetVideoDeviceName(const std::string& fmt_name, const std::string& device_name, bool prefix)
{
    video_capture_.SetDeviceName(fmt_name, device_name, prefix);
}

void AVInputStream::SetMicrophoneName(const std::string& fmt_name, const std::string& device_name, bool prefix)
{
    microphone_.SetDeviceName(fmt_name, device_name, prefix);
}

void AVInputStream::SetSpeakerName(const std::string& fmt_name, const std::string& device_name, bool prefix)
{
    speaker_.SetDeviceName(fmt_name, device_name, prefix);
}

void AVInputStream::SetVideoDeviceOpts()
{
    // 采集的时候，frame_rate选择指定分辨率下最大的那个，pix_fmt选择支持的一种，sample_rate选择44100，sample_fmt选择支持的一种，channels选择2
}

void AVInputStream::SetMicrophoneOpts()
{

}

void AVInputStream::SetSpeakerOpts()
{

}

void AVInputStream::SetVideoCaptureCB(VideoCaptureCB cb)
{
    video_capture_.SetCaptureCB(cb);
}

void AVInputStream::SetMicrophoneCaptureCB(AudioCaptureCB cb)
{
    microphone_.SetCaptureCB(cb);
}

void AVInputStream::SetSpeakerCaptureCB(AudioCaptureCB cb)
{
    speaker_.SetCaptureCB(cb);
}

int AVInputStream::Open()
{
    video_capture_.Open(true);
    microphone_.Open(false);
    speaker_.Open(false);

    return 0;
}

void AVInputStream::Close()
{
    GLOBAL->thread_exit = true;

    video_capture_.Close();
    microphone_.Close();
    speaker_.Close();
}

int AVInputStream::StartCapture()
{
    const int64_t start_time = av_gettime();

    video_capture_.StartCapture(start_time);
    microphone_.StartCapture(start_time);
    speaker_.StartCapture(start_time);

    GLOBAL->thread_exit = false;
    return 0;
}

int AVInputStream::GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt)
{
    return video_capture_.GetVideoOpts(width, height, frame_rate, pix_fmt);
}

int AVInputStream::GetMicrophoneOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels)
{
    return microphone_.GetAudioOpts(sample_rate, sample_fmt, channels);
}

int AVInputStream::GetSpeakerOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels)
{
    return speaker_.GetAudioOpts(sample_rate, sample_fmt, channels);
}
