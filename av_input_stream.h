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

typedef int (*VideoCaptureCB)(AVStream* input_st, AVPixelFormat pix_fmt, AVFrame* pframe, int64_t lTimeStamp);
typedef int (*AudioCaptureCB)(AVStream* input_st, AVFrame* pframe, int64_t lTimeStamp);

class AVInputStream
{
public:
    AVInputStream(void);
    ~AVInputStream(void);

public:
    void  SetVideoCaptureDevice(std::string device_name);
    void  SetAudioCaptureDevice(std::string device_name);

    bool  OpenInputStream();
    void  CloseInputStream();

    bool  StartCapture();

    void  SetVideoCaptureCB(VideoCaptureCB pFuncCB);
    void  SetAudioCaptureCB(AudioCaptureCB pFuncCB);

    bool  GetVideoInputInfo(int& width, int& height, int& framerate, AVPixelFormat& pixFmt);
    bool  GetAudioInputInfo(AVSampleFormat& sample_fmt, int& sample_rate, int& channels);

protected:
    static int CaptureVideoThreadFunc(void* args);
    static int CaptureAudioThreadFunc(void* args);

    int  ReadVideoPackets();
    int  ReadAudioPackets();

protected:
    std::string  m_video_device;
    std::string  m_audio_device;

    int     m_videoindex;
    int     m_audioindex;

    AVFormatContext* m_pVidFmtCtx;
    AVFormatContext* m_pAudFmtCtx;
    AVInputFormat*  m_pInputFormat;

    AVPacket* dec_pkt;

    std::thread* m_hCapVideoThread;
    std::thread* m_hCapAudioThread; //线程句柄
    bool   m_exit_thread; //退出线程的标志变量

    VideoCaptureCB  m_pVideoCBFunc; //视频数据回调函数指针
    AudioCaptureCB  m_pAudioCBFunc; //音频数据回调函数指针

    QMutex     m_WriteLock;

    int64_t     m_start_time; //采集的起点时间
};

#endif // AV_INPUT_STREAM_H
