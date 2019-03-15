#include "video_capture.h"
#include <QDebug>

VideoCapture::VideoCapture()
{
    width_ = 0;
    height_ = 0;
    frame_rate_ = 0;
}

VideoCapture::~VideoCapture()
{
}

void VideoCapture::SetDeviceOpts(int width, int height, int frame_rate)
{
    width_ = width;
    height_ = height;
    frame_rate_ = frame_rate;
}

void VideoCapture::SetCaptureCB(VideoCaptureCB cb)
{
    capture_cb_ = cb;
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

void VideoCapture::OnFrameReady(AVStream* stream, AVFrame* frame, int64_t timestamp)
{
    if (capture_cb_ != nullptr)
    {
        capture_cb_(stream, stream->codec->pix_fmt, frame, timestamp);
    }
}
