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

AVInputStream::AVInputStream() : video_device_(), audio_device_(), write_file_mutex_()
{
    video_cb_ = nullptr;
    audio_cb_ = nullptr;
    input_fmt_ = nullptr;
    video_fmt_ctx_ = nullptr;
    video_index_ = -1;
    audio_fmt_ctx_ = nullptr;
    audio_index_ = -1;
    start_time_ = 0;
    capture_video_thread_ = nullptr;
    capture_audio_thread_ = nullptr;
    exit_thread_ = false;
}

AVInputStream::~AVInputStream()
{
    Close();
}

void AVInputStream::SetVideoCaptureDevice(const std::string& device_name)
{
    video_device_ = device_name;
    qDebug() << QString::fromStdString(video_device_);
}

void AVInputStream::SetAudioCaptureDevice(const std::string& device_name)
{
    audio_device_ = device_name;
    qDebug() << QString::fromStdString(audio_device_);
}

void AVInputStream::SetVideoCaptureCB(VideoCaptureCB cb)
{
    video_cb_ = cb;
}

void AVInputStream::SetAudioCaptureCB(AudioCaptureCB cb)
{
    audio_cb_ = cb;
}

int AVInputStream::Open()
{
    if (video_device_.empty() && audio_device_.empty())
    {
//        ATLTRACE("you have not set any capture device \n");
        return -1;
    }

    // 打开Directshow设备前需要调用FFmpeg的avdevice_register_all函数，否则下面返回失败
#if defined(Q_OS_WIN)
    input_fmt_ = av_find_input_format("dshow");
#elif defined(Q_OS_LINUX)
    m_pInputFormat = av_find_input_format("video4linux2");
#endif

    if (nullptr == input_fmt_)
    {
        return -1;
    }

    // Set device params
    AVDictionary* device_param = nullptr;

    // if not setting rtbufsize, error messages will be shown in cmd, but you can still watch or record the stream correctly in most time
    // setting rtbufsize will erase those error messages, however, larger rtbufsize will bring latency
    av_dict_set(&device_param, "rtbufsize", "10M", 0); // TODO

    if (!video_device_.empty())
    {
        const std::string device_name = "video=" + video_device_;

        // Set own video device's name 打开设备，将设备名称作为参数传进去，注意这个设备名称需要转成UTF-8编码
        int ret = avformat_open_input(&video_fmt_ctx_, device_name.c_str(), input_fmt_, &device_param);
        if (ret != 0)
        {
//            ATLTRACE("Couldn't open input video stream.（无法打开输入流）\n");
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

    if (!audio_device_.empty())
    {
        const std::string device_name = "audio=" + audio_device_;

        // Set own audio device's name
        int ret = avformat_open_input(&audio_fmt_ctx_, device_name.c_str(), input_fmt_, &device_param);
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
    input_fmt_ = nullptr;
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

    if (!video_device_.empty())
    {
        capture_video_thread_ = new std::thread(CaptureVideoThreadFunc, this);
    }

    if (!audio_device_.empty())
    {
        capture_audio_thread_ = new std::thread(CaptureAudioThreadFunc, this);
    }

    return 0;
}

int AVInputStream::GetVideoInputInfo(int& width, int& height, int& frame_rate, AVPixelFormat& pix_fmt)
{
    if (-1 == video_index_)
    {
        return -1;
    }

    width = video_fmt_ctx_->streams[video_index_]->codec->width;
    height = video_fmt_ctx_->streams[video_index_]->codec->height;

    AVStream* stream = video_fmt_ctx_->streams[video_index_];
    pix_fmt = stream->codec->pix_fmt;

    //frame_rate = stream->avg_frame_rate.num/stream->avg_frame_rate.den;//每秒多少帧

    if (stream->r_frame_rate.den > 0)
    {
        frame_rate = stream->r_frame_rate.num / stream->r_frame_rate.den;
    }
    else if (stream->codec->framerate.den > 0)
    {
        frame_rate = stream->codec->framerate.num / stream->codec->framerate.den;
    }

    qDebug() << width << height << frame_rate << pix_fmt;
    return 0;
}

int AVInputStream::GetAudioInputInfo(AVSampleFormat& sample_fmt, int& sample_rate, int& channels)
{
    if (-1 == audio_index_)
    {
        return -1;
    }

    sample_fmt = audio_fmt_ctx_->streams[audio_index_]->codec->sample_fmt;
    sample_rate = audio_fmt_ctx_->streams[audio_index_]->codec->sample_rate;
    channels = audio_fmt_ctx_->streams[audio_index_]->codec->channels;

    qDebug() << sample_fmt << sample_rate << channels;
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
