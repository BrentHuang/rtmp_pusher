#include "video_capture.h"
#include <QDebug>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif

VideoCapture::VideoCapture()
{
    width_ = 0;
    height_ = 0;
    frame_rate_ = 0;
}

VideoCapture::~VideoCapture()
{
}

void VideoCapture::SetVideoOpts(int width, int height, int frame_rate)
{
    width_ = width;
    height_ = height;
    frame_rate_ = frame_rate;
}

int VideoCapture::GetVideoOpts(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt)
{
    if (nullptr == fmt_ctx_ || -1 == stream_idx_)
    {
        qDebug() << "not init ok";
        return -1;
    }

    AVStream* stream = fmt_ctx_->streams[stream_idx_];
    width = stream->codec->width;
    height = stream->codec->height;

    if (stream->codec->framerate.den > 0)
    {
        frame_rate = stream->codec->framerate.num / stream->codec->framerate.den;
    }
    else if (stream->avg_frame_rate.den > 0)
    {
        frame_rate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
    }
    else if (stream->r_frame_rate.den > 0)
    {
        frame_rate = stream->r_frame_rate.num / stream->r_frame_rate.den;
    }
    else
    {
        frame_rate = 0;
    }

    pix_fmt = stream->codec->pix_fmt;

    QString pix_fmt_str;
    switch (pix_fmt)
    {
        case AV_PIX_FMT_BGRA:
        {
            pix_fmt_str = "AV_PIX_FMT_BGRA";
        }
        break;

        case AV_PIX_FMT_YUYV422:
        {
            pix_fmt_str = "AV_PIX_FMT_YUYV422";
        }
        break;

        case AV_PIX_FMT_YUVJ422P:
        {
            pix_fmt_str = "AV_PIX_FMT_YUVJ422P";
        }
        break;

        default:
        {
            pix_fmt_str =  QString("%1").arg((int) pix_fmt);
        }
        break;
    }

    qDebug() << width << "x" << height << frame_rate << pix_fmt_str;
    return 0;
}

int VideoCapture::Open()
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
    const std::string device_name = prefix_ ? ("video=" + device_name_) : device_name_;

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

    stream_idx_ = -1;

    for (int i = 0; i < (int) fmt_ctx_->nb_streams; ++i)
    {
        if (AVMEDIA_TYPE_VIDEO == fmt_ctx_->streams[i]->codec->codec_type)
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
