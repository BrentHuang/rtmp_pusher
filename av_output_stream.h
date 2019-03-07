﻿#ifndef AV_OUTPUT_STREAM_H
#define AV_OUTPUT_STREAM_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libavutil/audio_fifo.h>
#ifdef __cplusplus
}
#endif

class AVOutputStream
{
public:
    AVOutputStream(void);
    ~AVOutputStream(void);

public:

    //初始化视频编码器
    void SetVideoCodecProp(AVCodecID codec_id, int framerate, int bitrate, int gopsize, int width, int height);

    //初始化音频编码器
    void SetAudioCodecProp(AVCodecID codec_id, int samplerate, int channels, int bitrate);

    //创建编码器和混合器，打开输出
    bool  OpenOutputStream(const char* out_path);

    //写入一帧图像
    int   write_video_frame(AVStream* st, AVPixelFormat pix_fmt, AVFrame* pframe, int64_t lTimeStamp);

    //写入一帧音频
    int   write_audio_frame(AVStream* st, AVFrame* pframe, int64_t lTimeStamp);

    //关闭输出
    void  CloseOutput();

protected:
    //AVFormatContext *m_pVidFmtCtx;
    // AVFormatContext *m_pAudFmtCtx;
    //AVInputFormat* m_pInputFormat;

    AVStream* video_st;
    AVStream* audio_st;
    AVFormatContext* ofmt_ctx;

    AVCodecContext* pCodecCtx;
    AVCodecContext* pCodecCtx_a;
    AVCodec* pCodec;
    AVCodec* pCodec_a;
    AVPacket enc_pkt;
    AVPacket enc_pkt_a;
    AVFrame* pFrameYUV;
    struct SwsContext* img_convert_ctx;
    struct SwrContext* aud_convert_ctx;

    AVAudioFifo* m_fifo;

    int  m_vid_framecnt;
    int  m_aud_framecnt;

    int  m_nb_samples;

    int64_t m_first_vid_time1, m_first_vid_time2; //前者是采集视频的第一帧的时间，后者是编码器输出的第一帧的时间
    int64_t m_first_aud_time; //第一个音频帧的时间

    int64_t m_next_vid_time;
    int64_t m_next_aud_time;

    int64_t  m_nLastAudioPresentationTime; //记录上一帧的音频时间戳

    uint8_t** m_converted_input_samples;
    uint8_t* m_out_buffer;

public:
    std::string     m_output_path; //输出路径

    AVCodecID  m_video_codec_id;
    AVCodecID  m_audio_codec_id;

    int m_width, m_height;
    int m_framerate;
    int m_video_bitrate;
    int m_gopsize;

    int m_samplerate;
    int m_channels;
    int m_audio_bitrate;

};

#endif // AV_OUTPUT_STREAM_H
