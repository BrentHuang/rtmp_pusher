#include "av_input_stream.h"
#include <QDebug>
#include <qsystemdetection.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

AVInputStream::AVInputStream() : video_device_name_(), audio_device_name_(), write_file_mutex_()
{
    video_cb_ = nullptr;
    audio_cb_ = nullptr;

    video_input_fmt_ = nullptr;
    video_fmt_ctx_ = nullptr;
    video_index_ = -1;

    audio_input_fmt_ = nullptr;
    audio_fmt_ctx_ = nullptr;
    audio_index_ = -1;

    start_time_ = 0;

    capture_video_thread_ = nullptr;
    capture_audio_thread_ = nullptr;
    exit_thread_ = false;
}

AVInputStream::~AVInputStream()
{
}

void AVInputStream::SetVideoCaptureDevice(const std::string& fmt_name, const std::string& device_name, bool video_prefix)
{
    video_fmt_name_ = fmt_name;
    video_device_name_ = device_name;
    video_prefix_ = video_prefix;
    qDebug() << QString::fromStdString(video_fmt_name_) << QString::fromStdString(video_device_name_);
}

void AVInputStream::SetAudioCaptureDevice(const std::string& fmt_name, const std::string& device_name, bool audio_prefix)
{
    audio_fmt_name_ = fmt_name;
    audio_device_name_ = device_name;
    audio_prefix_ = audio_prefix;
    qDebug() << QString::fromStdString(audio_fmt_name_) << QString::fromStdString(audio_device_name_);
}

void AVInputStream::SetAudio2CaptureDevice(const std::string& fmt_name, const std::string& device_name, bool audio_prefix)
{
    audio2_fmt_name_ = fmt_name;
    audio2_device_name_ = device_name;
    audio2_prefix_ = audio_prefix;
    qDebug() << QString::fromStdString(audio2_fmt_name_) << QString::fromStdString(audio2_device_name_);
}

void AVInputStream::SetVideoCaptureCB(VideoCaptureCB cb)
{
    video_cb_ = cb;
}

void AVInputStream::SetAudioCaptureCB(AudioCaptureCB cb)
{
    audio_cb_ = cb;
}

int AVInputStream::Open(int width, int height, int frame_rate, AVPixelFormat pix_fmt, int sample_rate, AVSampleFormat sample_fmt, int channels)
{
    if (!video_fmt_name_.empty() && !video_device_name_.empty())
    {
        std::string device_name = video_device_name_;

        if (video_prefix_)
        {
            device_name = "video=" + video_device_name_;
        }

        video_input_fmt_ = av_find_input_format(video_fmt_name_.c_str());
        if (nullptr == video_input_fmt_)
        {
            qDebug() << "you may missed call 'avdevice_register_all'";
            return -1;
        }

        // TODO 在这里通过opts可以设置采集的分辨率、比特率、帧率、pixel format等参数
        AVDictionary* video_device_opts = nullptr;
        // if not setting rtbufsize, error messages will be shown in cmd, but you can still watch or record the stream
        // correctly in most time. setting rtbufsize will erase those error messages, however, larger rtbufsize will bring latency
        av_dict_set(&video_device_opts, "rtbufsize", "10M", 0); // TODO 在分辨率、帧率比较高的时候，这个buf size要设大一点？
//        av_dict_set(&video_device_opts, "video_size", "640x480", 0);
//        av_dict_set(&video_device_opts, "framerate", "15", 0);

        // 打开设备
        int ret = avformat_open_input(&video_fmt_ctx_, device_name.c_str(), video_input_fmt_, &video_device_opts);
        if (ret != 0)
        {
            qDebug() << "can not open input video stream";
            return -1;
        }

        // input video initialize 获取流的信息，得到视频流或音频流的索引号，之后会频繁用到这个索引号来定位视频和音频的Stream信息
        ret = avformat_find_stream_info(video_fmt_ctx_, nullptr);
        if (ret < 0) // TODO
        {
//            ATLTRACE("Couldn't find video stream information.（无法获取流信息）\n");
            return -1;
        }

        video_index_ = -1;

        for (int i = 0; i < (int) video_fmt_ctx_->nb_streams; ++i)
        {
            if (AVMEDIA_TYPE_VIDEO == video_fmt_ctx_->streams[i]->codec->codec_type)
            {
                video_index_ = i;
                break;
            }
        }

        if (-1 == video_index_)
        {
//            ATLTRACE("Couldn't find a video stream.（没有找到视频流）\n");
            return -1;
        }

        // 打开视频解码器或音频解码器，实际上，我们可以把设备也看成是一般的文件源，
        // 而文件一般采用某种封装格式，要播放出来需要进行解复用，分离成裸流，然后对单独的视频流、音频流进行解码。
        // 虽然采集出来的图像或音频都是未编码的，但是按照FFmpeg的常规处理流程，我们需要加上“解码”这个步骤。
        ret = avcodec_open2(video_fmt_ctx_->streams[video_index_]->codec, avcodec_find_decoder(video_fmt_ctx_->streams[video_index_]->codec->codec_id), nullptr);
        if (ret != 0)
        {
//            ATLTRACE("Could not open video codec.（无法打开解码器）\n");
            return -1;
        }
    }

    if (!audio_fmt_name_.empty() && !audio_device_name_.empty())
    {
        std::string device_name = audio_device_name_;

        if (audio_prefix_)
        {
            device_name = "audio=" + audio_device_name_;
        }

        audio_input_fmt_ = av_find_input_format(audio_fmt_name_.c_str());
        if (nullptr == audio_input_fmt_)
        {
            qDebug() << "you may missed call 'avdevice_register_all'";
            return -1;
        }

        AVDictionary* audio_device_opts = nullptr;

        int ret = avformat_open_input(&audio_fmt_ctx_, device_name.c_str(), audio_input_fmt_, &audio_device_opts);
        if (ret != 0)
        {
//            ATLTRACE("Couldn't open input audio stream.（无法打开输入流）\n");
            return -1;
        }

        // input audio initialize
        ret = avformat_find_stream_info(audio_fmt_ctx_, nullptr);
        if (ret < 0)
        {
//            ATLTRACE("Couldn't find audio stream information.（无法获取流信息）\n");
            return -1;
        }

        audio_index_ = -1;
        for (int i = 0; i < (int) audio_fmt_ctx_->nb_streams; ++i)
        {
            if (AVMEDIA_TYPE_AUDIO == audio_fmt_ctx_->streams[i]->codec->codec_type)
            {
                audio_index_ = i;
                break;
            }
        }

        if (-1 == audio_index_)
        {
//            ATLTRACE("Couldn't find a audio stream.（没有找到音频流）\n");
            return -1;
        }

        ret = avcodec_open2(audio_fmt_ctx_->streams[audio_index_]->codec, avcodec_find_decoder(audio_fmt_ctx_->streams[audio_index_]->codec->codec_id), nullptr);
        if (ret != 0)
        {
//            ATLTRACE("Could not open audio codec.（无法打开解码器）\n");
            return -1;
        }
    }

    return 0;
}

void AVInputStream::Close()
{
    exit_thread_ = true;

    if (capture_video_thread_ != nullptr)
    {
        capture_video_thread_->join();
    }

    if (capture_audio_thread_ != nullptr)
    {
        capture_audio_thread_->join();
    }

    if (capture_video_thread_ != nullptr)
    {
        delete capture_video_thread_;
        capture_video_thread_ = nullptr;
    }

    if (capture_audio_thread_ != nullptr)
    {
        delete capture_audio_thread_;
        capture_audio_thread_ = nullptr;
    }

    // 关闭输入流
    if (video_fmt_ctx_ != nullptr)
    {
        avformat_close_input(&video_fmt_ctx_);
    }

    if (audio_fmt_ctx_ != nullptr)
    {
        avformat_close_input(&audio_fmt_ctx_);
    }

    if (video_fmt_ctx_ != nullptr)
    {
        avformat_free_context(video_fmt_ctx_);
    }

    if (audio_fmt_ctx_ != nullptr)
    {
        avformat_free_context(audio_fmt_ctx_);
    }

    video_fmt_ctx_ = nullptr;
    video_index_ = -1;

    audio_fmt_ctx_ = nullptr;
    audio_index_ = -1;

    video_input_fmt_ = nullptr;
    audio_input_fmt_ = nullptr;
    start_time_ = 0;
}

int AVInputStream::StartCapture()
{
    // StartCapture函数分别建立了一个读取视频包和读取音频包的线程，两个线程各自独立工作，分别从视频采集设备，音频采集设备获取到数据，然后进行后续的处理。
    if (-1 == video_index_ && -1 == audio_index_)
    {
//        ATLTRACE("错误：你没有打开设备 \n");
        return -1;
    }

    start_time_ = av_gettime();
    exit_thread_ = false;

    if (!video_device_name_.empty())
    {
        capture_video_thread_ = new std::thread(CaptureVideoThreadFunc, this);
    }

    if (!audio_device_name_.empty())
    {
        capture_audio_thread_ = new std::thread(CaptureAudioThreadFunc, this);
    }

    return 0;
}

int AVInputStream::GetVideoInfo(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt)
{
    if (nullptr == video_fmt_ctx_ || -1 == video_index_)
    {
        return -1;
    }

    AVStream* stream = video_fmt_ctx_->streams[video_index_];
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

int AVInputStream::GetAudioInfo(int& sample_rate, AVSampleFormat& sample_fmt, int& channels)
{
    if (nullptr == audio_fmt_ctx_ || -1 == audio_index_)
    {
        return -1;
    }

    AVStream* stream = audio_fmt_ctx_->streams[audio_index_];
    sample_fmt = stream->codec->sample_fmt;
    sample_rate = stream->codec->sample_rate;
    channels = stream->codec->channels;

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

    qDebug() << sample_rate << sample_fmt_str << channels;
    return 0;
}

int AVInputStream::ReadVideoPackets()
{
    // 不停地调用 av_read_frame读取采集到的图像帧，接着调用avcodec_decode_video2进行“解码”，这样获得了原始的图像，图像可能是RGB或YUV格式。
    // 解码后的图像通过m_pVideoCBFunc指向的回调函数回调给上层处理，回调函数里可进行后续的一些操作，比如对视频帧编码或直接显示。
    while (true)
    {
        // TODO 读完所有的帧然后再退出
        if (exit_thread_)
        {
            break;
        }

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr;
        pkt.size = 0;

        int ret = av_read_frame(video_fmt_ctx_, &pkt);
        if (ret != 0)
        {
            qDebug() << "av_read_frame failed";
            break;
        }

        ret = avcodec_send_packet(video_fmt_ctx_->streams[pkt.stream_index]->codec, &pkt);
        if (ret != 0)
        {
            qDebug() << "avcodec_send_packet failed";
            break;
        }

        AVFrame* frame = av_frame_alloc();
        if (nullptr == frame)
        {
            qDebug() << "av_frame_alloc failed";
            break;
        }

        ret = avcodec_receive_frame(video_fmt_ctx_->streams[pkt.stream_index]->codec, frame);
        if (0 == ret)
        {
            // TODO 视频转成yuv420p

            if (video_cb_ != nullptr)
            {
                QMutexLocker lock(&write_file_mutex_);
                video_cb_(video_fmt_ctx_->streams[pkt.stream_index],
                          video_fmt_ctx_->streams[video_index_]->codec->pix_fmt, frame,
                          av_gettime() - start_time_);
            }
        }
        else
        {
            qDebug() << "avcodec_receive_frame failed";
        }

        av_frame_free(&frame);
    }

    return 0;
}

int AVInputStream::ReadAudioPackets()
{
    while (true)
    {
        // TODO 读完所有的帧然后再退出
        if (exit_thread_)
        {
            break;
        }

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = nullptr;
        pkt.size = 0;

        /** Read one audio frame from the input file into a temporary packet. */
        int ret = av_read_frame(audio_fmt_ctx_, &pkt);
        if (ret != 0)
        {
            qDebug() << "av_read_frame failed";
            break;
        }

        ret = avcodec_send_packet(audio_fmt_ctx_->streams[pkt.stream_index]->codec, &pkt);
        if (ret != 0)
        {
            qDebug() << "avcodec_send_packet failed";
            break;
        }

        AVFrame* frame = av_frame_alloc();
        if (nullptr == frame)
        {
            qDebug() << "av_frame_alloc failed";
            break;
        }

        ret = avcodec_receive_frame(audio_fmt_ctx_->streams[pkt.stream_index]->codec, frame);
        if (0 == ret)
        {
            // TODO 音频转成pcm

            if (audio_cb_ != nullptr)
            {
                QMutexLocker lock(&write_file_mutex_);
                audio_cb_(audio_fmt_ctx_->streams[pkt.stream_index], frame, av_gettime() - start_time_);
            }
        }
        else
        {
            qDebug() << "avcodec_receive_frame failed";
        }

        av_frame_free(&frame);
    }

    return 0;
}

int AVInputStream::CaptureVideoThreadFunc(void* args)
{
    AVInputStream* input_stream = static_cast<AVInputStream*>(args);
    input_stream->ReadVideoPackets();
    return 0;
}

int AVInputStream::CaptureAudioThreadFunc(void* args)
{
    AVInputStream* input_stream = static_cast<AVInputStream*>(args);
    input_stream->ReadAudioPackets();
    return 0;
}
