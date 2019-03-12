#ifndef AV_OUTPUT_STREAM_H
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
    AVOutputStream();
    ~AVOutputStream();

public:
    // 初始化视频编码器
    void SetVideoCodecProp(AVCodecID codec_id, int frame_rate, int bit_rate, int gop_size, int width, int height);

    // 初始化音频编码器
    void SetAudioCodecProp(AVCodecID codec_id, int sample_rate, int channels, int bit_rate);

    // 创建编码器和混合器，打开输出
    int Open(const std::string& file_path);

    // 关闭输出
    void Close();

//       Write_video_frame和write_audio_frame是CAVOutputStream的两个很重要的函数，其中对音频包的处理略为复杂一些，
//    主要是因为输入的音频和编码后的音频的frame_size不一样，中间需要一个Fifo作缓冲队列。
//    另外时间戳PTS的计算也是很关键的，弄得不好保存的文件播放视音频就不同步
    // 写入一帧图像
    int WriteVideoFrame(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp);

    // 写入一帧音频
    int WriteAudioFrame(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp);

private:
    AVCodecID video_codec_id_;
    int width_;
    int height_;
    int frame_rate_;
    int video_bit_rate_;
    int gop_size_;

    AVCodecID  audio_codec_id_;
    int sample_rate_;
    int channels_;
    int audio_bit_rate_;

    AVFormatContext* fmt_ctx_;
    AVCodecContext* video_codec_ctx_;
    AVStream* video_stream_; // 里面含有video_codec_ctx
    AVFrame* yuv_frame_;
    uint8_t* out_buffer_;

    AVCodecContext* audio_codec_ctx_;
    AVStream* audio_stream_; // 里面含有audio_codec_ctx
    AVAudioFifo* audio_fifo_;
    uint8_t** converted_input_samples_;

    int video_frame_count_;
    int audio_frame_count_;
    int nb_samples_;
    int64_t last_audio_pts_; // 记录上一帧的音频时间戳
    int64_t next_video_ts_;
    int64_t next_audio_ts_;
    int64_t first_video_ts1_, first_video_ts2_; // 前者是采集视频的第一帧的时间，后者是编码器输出的第一帧的时间
    int64_t first_audio_ts_; // 第一个音频帧的时间

    struct SwsContext* img_convert_ctx_;
    struct SwrContext* aud_convert_ctx_;
};

#endif // AV_OUTPUT_STREAM_H
