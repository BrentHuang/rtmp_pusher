#ifndef AV_INPUT_STREAM_H
#define AV_INPUT_STREAM_H

#include <QMutex>
#include <string>
#include <thread>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

typedef int (*VideoCaptureCB)(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp);
typedef int (*AudioCaptureCB)(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp);

class AVInputStream
{
public:
    AVInputStream();
    ~AVInputStream();

public:
    void SetVideoCaptureDevice(const std::string& device_name);
    void SetAudioCaptureDevice(const std::string& device_name);

    void SetVideoCaptureCB(VideoCaptureCB cb);
    void SetAudioCaptureCB(AudioCaptureCB cb);

    int Open();
    void Close();

    int StartCapture();

    int GetVideoInfo(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt);
    int GetAudioInfo(AVSampleFormat& sample_fmt, int& sample_rate, int& channels);

protected:
    static int CaptureVideoThreadFunc(void* args);
    static int CaptureAudioThreadFunc(void* args);

    int ReadVideoPackets();
    int ReadAudioPackets();

protected:
    std::string video_device_;
    std::string audio_device_;

    VideoCaptureCB video_cb_;
    AudioCaptureCB audio_cb_;

    AVInputFormat* video_input_fmt_;
    AVFormatContext* video_fmt_ctx_;
    int video_index_;

    AVInputFormat* audio_input_fmt_;
    AVFormatContext* audio_fmt_ctx_;
    int audio_index_;

    int64_t start_time_; // 采集的起点时间，单位：毫秒

    std::thread* capture_video_thread_;
    std::thread* capture_audio_thread_;
    std::atomic_bool exit_thread_;

    QMutex write_file_mutex_;
};

#endif // AV_INPUT_STREAM_H
