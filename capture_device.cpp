﻿#include "capture_device.h"
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

#include "global.h"

CaptureDevice::CaptureDevice() : fmt_name_(), device_name_()
{
    prefix_ = false;
    input_fmt_ = nullptr;
    fmt_ctx_ = nullptr;
    stream_idx_ = -1;
    capture_thread_ = nullptr;
    start_time_ = 0;
}

CaptureDevice::~CaptureDevice()
{
}

void CaptureDevice::SetDeviceName(const std::string& fmt_name, const std::string& device_name, bool prefix)
{
    fmt_name_ = fmt_name;
    device_name_ = device_name;
    prefix_ = prefix;
}

void CaptureDevice::SetCaptureCB(CaptureCB cb)
{
    capture_cb_ = cb;
}

int CaptureDevice::Open(bool video)
{
    input_fmt_ = av_find_input_format(fmt_name_.c_str());
    if (nullptr == input_fmt_)
    {
        qDebug() << "failed to find input format: " << QString::fromStdString(fmt_name_);
        return -1;
    }

    // TODO 在这里通过opts可以设置采集的分辨率、比特率、帧率、pixel format等参数
    AVDictionary* opts = nullptr;
    // if not setting rtbufsize, error messages will be shown in cmd, but you can still watch or record the stream
    // correctly in most time. setting rtbufsize will erase those error messages, however, larger rtbufsize will bring latency
    av_dict_set(&opts, "rtbufsize", "10M", 0); // TODO 在分辨率、帧率比较高的时候，这个buf size要设大一点？
//        av_dict_set(&video_device_opts, "video_size", "640x480", 0);
//        av_dict_set(&video_device_opts, "framerate", "15", 0);

    // 打开设备
    const std::string prefix = video ? "video=" : "audio=";
    const std::string device_name = prefix_ ? (prefix + device_name_) : device_name_;

    int ret = avformat_open_input(&fmt_ctx_, device_name.c_str(), input_fmt_, &opts);
    if (ret != 0)
    {
        qDebug() << "failed to open input: " << strerror(AVERROR(ret));
        return -1;
    }

#if defined(Q_OS_LINUX)
    AVDeviceCapabilitiesQuery* caps = nullptr;
    AVOptionRanges* ranges;

    if (avdevice_capabilities_create(&caps, fmt_ctx_, nullptr) >= 0)
    {
        av_opt_query_ranges(&ranges, caps, "codec", AV_OPT_MULTI_COMPONENT_RANGE);
        // pick codec here and set it
//        av_opt_set(caps, "codec", AV_CODEC_ID_RAWVIDEO, 0);
        av_opt_query_ranges(&ranges, caps, "pixel_format", AV_OPT_MULTI_COMPONENT_RANGE);
        // pick format here and set it
//        av_opt_set(caps, "pixel_format", AV_PIX_FMT_YUV420P, 0);

        avdevice_capabilities_free(&caps, fmt_ctx_);
    }
#endif

    // 获取流的信息，得到视频流或音频流的索引号，之后会频繁用到这个索引号来定位视频和音频的Stream信息
    ret = avformat_find_stream_info(fmt_ctx_, nullptr);
    if (ret < 0)
    {
        qDebug() << "failed to find stream info: " << strerror(AVERROR(ret));
        return -1;
    }

    const AVMediaType media_type = video ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
    stream_idx_ = -1;

    for (int i = 0; i < (int) fmt_ctx_->nb_streams; ++i)
    {
        if (media_type == fmt_ctx_->streams[i]->codec->codec_type)
        {
            stream_idx_ = i;
            break;
        }
    }

    if (-1 == stream_idx_)
    {
        qDebug() << "failed to find video stream";
        return -1;
    }

    qDebug() << "stream idx: " << stream_idx_;

    // 打开视频解码器或音频解码器，实际上，我们可以把设备也看成是一般的文件源，
    // 而文件一般采用某种封装格式，要播放出来需要进行解复用，分离成裸流，然后对单独的视频流、音频流进行解码。
    // 虽然采集出来的图像或音频都是未编码的，但是按照FFmpeg的常规处理流程，我们需要加上“解码”这个步骤。
    AVStream* stream = fmt_ctx_->streams[stream_idx_];
    ret = avcodec_open2(stream->codec, avcodec_find_decoder(stream->codec->codec_id), nullptr);
    if (ret != 0)
    {
        qDebug() << "failed to open codec";
        return -1;
    }

    return 0;
}

void CaptureDevice::Close()
{
    // 关闭输入流
    if (fmt_ctx_ != nullptr)
    {
        avformat_close_input(&fmt_ctx_);
    }

    stream_idx_ = -1;
    input_fmt_ = nullptr;
}

int CaptureDevice::Start(int64_t timestamp)
{
    if (-1 == stream_idx_)
    {
        qDebug() << "device not open, name: " << QString::fromStdString(device_name_);
        return -1;
    }

    start_time_ = timestamp;

    capture_thread_ = new std::thread(CaptureThreadFunc, this);
    if (nullptr == capture_thread_)
    {
        qDebug() << "failed to start capture thread";
        return -1;
    }

    return 0;
}

void CaptureDevice::Stop()
{
    if (capture_thread_ != nullptr)
    {
        capture_thread_->join();
        delete capture_thread_;
        capture_thread_ = nullptr;
    }

    start_time_ = 0;
}

int CaptureDevice::CaptureThreadFunc(void* args)
{
    CaptureDevice* capture_device = static_cast<CaptureDevice*>(args);

    while (true)
    {
        capture_device->ReadPackets();

        if (GLOBAL->thread_exit)
        {
            break;
        }
    }

    return 0;
}

int CaptureDevice::ReadPackets()
{
    AVPacket pkt;

    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int ret = av_read_frame(fmt_ctx_, &pkt);
    if (ret != 0)
    {
        qDebug() << "av_read_frame failed";
        return -1;
    }

    AVStream* stream = fmt_ctx_->streams[pkt.stream_index];
    AVCodecContext* codec_ctx = stream->codec;

    ret = avcodec_send_packet(codec_ctx, &pkt);
    if (ret != 0)
    {
        qDebug() << "avcodec_send_packet failed";
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    if (nullptr == frame)
    {
        qDebug() << "av_frame_alloc failed";
        return -1;
    }

    ret = avcodec_receive_frame(codec_ctx, frame);
    if (0 == ret)
    {
        if (capture_cb_ != nullptr)
        {
            capture_cb_(stream, frame, av_gettime() - start_time_);
        }
    }
    else
    {
        qDebug() << "avcodec_receive_frame failed";
    }

    av_frame_free(&frame);
    return 0;
}
