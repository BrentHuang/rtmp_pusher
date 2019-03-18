#include "audio_capture.h"
#include <QDebug>

AudioCapture::AudioCapture()
{
    sample_rate_ = 0;
    channels_ = 0;
}

AudioCapture::~AudioCapture()
{
}

void AudioCapture::SetDeviceOpts(int sample_rate, int channels)
{
    sample_rate_ = sample_rate;
    channels_ = channels;
}

int AudioCapture::GetAudioOpts(int& sample_rate, AVSampleFormat& sample_fmt, int& channels)
{
    if (nullptr == fmt_ctx_ || -1 == stream_idx_)
    {
        return -1;
    }

    AVStream* stream = fmt_ctx_->streams[stream_idx_];
    AVCodecContext* codec_ctx = stream->codec;

    sample_fmt = codec_ctx->sample_fmt;
    sample_rate = codec_ctx->sample_rate;
    channels = codec_ctx->channels;

    QString sample_fmt_str;
    switch (sample_fmt)
    {
        case AV_SAMPLE_FMT_S16:
        {
            sample_fmt_str = "AV_SAMPLE_FMT_S16";
        }
        break;

        default:
        {
            sample_fmt_str = QString("%1").arg((int) sample_fmt);
        }
        break;
    }

    qDebug() << sample_rate << sample_fmt_str << channels << codec_ctx->bit_rate;
    return 0;
}
