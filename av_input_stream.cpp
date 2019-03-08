#include "av_input_stream.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

#include <qsystemdetection.h>
#include <QDebug>

//static std::string AnsiToUTF8(const char* _ansi, int _ansi_len)
//{
//    std::string str_utf8("");
//    wchar_t* pUnicode = NULL;
//    uint8_t* pUtfData = NULL;
//    do
//    {
//        int unicodeNeed = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, NULL, 0);
//        pUnicode = new wchar_t[unicodeNeed + 1];
//        memset(pUnicode, 0, (unicodeNeed + 1)*sizeof(wchar_t));
//        int unicodeDone = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, (LPWSTR)pUnicode, unicodeNeed);

//        if (unicodeDone != unicodeNeed)
//        {
//            break;
//        }

//        int utfNeed = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char*)pUtfData, 0, NULL, NULL);
//        pUtfData = new BYTE[utfNeed + 1];
//        memset(pUtfData, 0, utfNeed + 1);
//        int utfDone = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char*)pUtfData, utfNeed, NULL, NULL);

//        if (utfNeed != utfDone)
//        {
//            break;
//        }
//        str_utf8.assign((char*)pUtfData);
//    } while (false);

//    if (pUnicode)
//    {
//        delete[] pUnicode;
//    }
//    if (pUtfData)
//    {
//        delete[] pUtfData;
//    }

//    return str_utf8;
//}

AVInputStream::AVInputStream(void)
{
    m_hCapVideoThread = NULL;
    m_hCapAudioThread = NULL;
    m_exit_thread = false;

    m_pVidFmtCtx = NULL;
    m_pAudFmtCtx = NULL;
    m_pInputFormat = NULL;

    dec_pkt = NULL;

    m_pVideoCBFunc = NULL;
    m_pAudioCBFunc = NULL;

    m_videoindex = -1;
    m_audioindex = -1;

    m_start_time = 0;

    //avcodec_register_all();
//   av_register_all();
    //avdevice_register_all();
}

AVInputStream::~AVInputStream(void)
{
    CloseInputStream();
}


void  AVInputStream::SetVideoCaptureCB(VideoCaptureCB pFuncCB)
{
    m_pVideoCBFunc = pFuncCB;
}

void  AVInputStream::SetAudioCaptureCB(AudioCaptureCB pFuncCB)
{
    m_pAudioCBFunc = pFuncCB;
}

void  AVInputStream::SetVideoCaptureDevice(std::string device_name)
{
    m_video_device = device_name;
    qDebug() << QString::fromStdString(m_video_device);
}

void  AVInputStream::SetAudioCaptureDevice(std::string device_name)
{
    m_audio_device = device_name;
    qDebug() << QString::fromStdString(m_audio_device);
}


bool  AVInputStream::OpenInputStream()
{
    if (m_video_device.empty() && m_audio_device.empty())
    {
//        ATLTRACE("you have not set any capture device \n");
        return false;
    }


    int i;

    //打开Directshow设备前需要调用FFmpeg的avdevice_register_all函数，否则下面返回失败
#ifdef Q_OS_LINUX
    m_pInputFormat = av_find_input_format("video4linux2");
#else Q_OS_WIN
    m_pInputFormat = av_find_input_format("dshow");
#endif

    if (nullptr == m_pInputFormat)
    {
        return false;
    }

//    ASSERT(m_pInputFormat != NULL);

    // Set device params
    AVDictionary* device_param = 0;
    //if not setting rtbufsize, error messages will be shown in cmd, but you can still watch or record the stream correctly in most time
    //setting rtbufsize will erase those error messages, however, larger rtbufsize will bring latency
    //av_dict_set(&device_param, "rtbufsize", "10M", 0);

    if (!m_video_device.empty())
    {
        int res = 0;

        std::string device_name = "video=" + m_video_device;

        //Set own video device's name 打开设备，将设备名称作为参数传进去，注意这个设备名称需要转成UTF-8编码
        if ((res = avformat_open_input(&m_pVidFmtCtx, device_name.c_str(), m_pInputFormat, &device_param)) != 0)
        {
//            ATLTRACE("Couldn't open input video stream.（无法打开输入流）\n");
            qDebug() << strerror(AVERROR(res));
            return false;
        }
        //input video initialize 获取流的信息，得到视频流或音频流的索引号，之后会频繁用到这个索引号来定位视频和音频的Stream信息
        if (avformat_find_stream_info(m_pVidFmtCtx, NULL) < 0)
        {
//            ATLTRACE("Couldn't find video stream information.（无法获取流信息）\n");
            return false;
        }
        m_videoindex = -1;
        for (i = 0; i < m_pVidFmtCtx->nb_streams; i++)
        {
            if (m_pVidFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                m_videoindex = i;
                break;
            }
        }

        if (m_videoindex == -1)
        {
//            ATLTRACE("Couldn't find a video stream.（没有找到视频流）\n");
            return false;
        }

        // 打开视频解码器或音频解码器，实际上，我们可以把设备也看成是一般的文件源，而文件一般采用某种封装格式，要播放出来需要进行解复用，分离成裸流，然后对单独的视频流、音频流进行解码。虽然采集出来的图像或音频都是未编码的，但是按照FFmpeg的常规处理流程，我们需要加上“解码”这个步骤。
        if (avcodec_open2(m_pVidFmtCtx->streams[m_videoindex]->codec, avcodec_find_decoder(m_pVidFmtCtx->streams[m_videoindex]->codec->codec_id), NULL) < 0)
        {
//            ATLTRACE("Could not open video codec.（无法打开解码器）\n");
            return false;
        }
    }

    //////////////////////////////////////////////////////////

    if (!m_audio_device.empty())
    {
        std::string device_name = "audio=" + m_audio_device;

        //Set own audio device's name
        if (avformat_open_input(&m_pAudFmtCtx, device_name.c_str(), m_pInputFormat, &device_param) != 0)
        {

//            ATLTRACE("Couldn't open input audio stream.（无法打开输入流）\n");
            return false;
        }

        //input audio initialize
        if (avformat_find_stream_info(m_pAudFmtCtx, NULL) < 0)
        {
//            ATLTRACE("Couldn't find audio stream information.（无法获取流信息）\n");
            return false;
        }
        m_audioindex = -1;
        for (i = 0; i < m_pAudFmtCtx->nb_streams; i++)
        {
            if (m_pAudFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                m_audioindex = i;
                break;
            }
        }
        if (m_audioindex == -1)
        {
//            ATLTRACE("Couldn't find a audio stream.（没有找到音频流）\n");
            return false;
        }
        if (avcodec_open2(m_pAudFmtCtx->streams[m_audioindex]->codec, avcodec_find_decoder(m_pAudFmtCtx->streams[m_audioindex]->codec->codec_id), NULL) < 0)
        {
//            ATLTRACE("Could not open audio codec.（无法打开解码器）\n");
            return false;
        }
    }

    return true;
}

bool  AVInputStream::StartCapture()
{
    // StartCapture函数分别建立了一个读取视频包和读取音频包的线程，两个线程各自独立工作，分别从视频采集设备，音频采集设备获取到数据，然后进行后续的处理。
    if (m_videoindex == -1 && m_audioindex == -1)
    {
//        ATLTRACE("错误：你没有打开设备 \n");
        return false;
    }

    m_start_time = av_gettime();

    m_exit_thread = false;

    if (!m_video_device.empty())
    {
        m_hCapVideoThread = new std::thread(CaptureVideoThreadFunc, this);
    }

    if (!m_audio_device.empty())
    {
        m_hCapAudioThread = new std::thread(CaptureAudioThreadFunc, this);
    }

    return true;
}

void  AVInputStream::CloseInputStream()
{
    m_exit_thread = true;

    // 通知线程退出
//    if (m_hCapVideoThread)
//    {
//        if ( WAIT_TIMEOUT == WaitForSingleObject(m_hCapVideoThread, 3000) )
//        {
//            OutputDebugString("WaitForSingleObject timeout.\n");
//            //::TerminateThread(m_hCapVideoThread, 0);
//        }
//        CloseHandle(m_hCapVideoThread);
//        m_hCapVideoThread = NULL;
//    }

//    if (m_hCapAudioThread)
//    {
//        if ( WAIT_TIMEOUT == WaitForSingleObject(m_hCapAudioThread, 3000) )
//        {
//            OutputDebugString("WaitForSingleObject timeout.\n");
//            //::TerminateThread(m_hCapAudioThread, 0);
//        }
//        CloseHandle(m_hCapAudioThread);
//        m_hCapAudioThread = NULL;
//    }

    //关闭输入流
    if (m_pVidFmtCtx != NULL)
    {
        avformat_close_input(&m_pVidFmtCtx);
        //m_pVidFmtCtx = NULL;
    }
    if (m_pAudFmtCtx != NULL)
    {
        avformat_close_input(&m_pAudFmtCtx);
        //m_pAudFmtCtx = NULL;
    }

    if (m_pVidFmtCtx)
    {
        avformat_free_context(m_pVidFmtCtx);
    }
    if (m_pAudFmtCtx)
    {
        avformat_free_context(m_pAudFmtCtx);
    }

    m_pVidFmtCtx = NULL;
    m_pAudFmtCtx = NULL;
    m_pInputFormat = NULL;

    m_videoindex = -1;
    m_audioindex = -1;
}

int  AVInputStream::ReadVideoPackets()
{
    if (dec_pkt == NULL)
    {
        ////prepare before decode and encode
        dec_pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
    }

    int encode_video = 1;
    int ret;

//    不停地调用 av_read_frame读取采集到的图像帧，接着调用avcodec_decode_video2进行“解码”，这样获得了原始的图像，图像可能是RGB或YUV格式。解码后的图像通过m_pVideoCBFunc指向的回调函数回调给上层处理，回调函数里可进行后续的一些操作，比如对视频帧编码或直接显示。
    //start decode and encode

    while (encode_video)
    {
        if (m_exit_thread)
        {
            break;
        }

        AVFrame* pframe = NULL;
        if ((ret = av_read_frame(m_pVidFmtCtx, dec_pkt)) >= 0)
        {
            pframe = av_frame_alloc();
            if (!pframe)
            {
                ret = AVERROR(ENOMEM);
                return ret;
            }
            int dec_got_frame = 0;
            ret = avcodec_decode_video2(m_pVidFmtCtx->streams[dec_pkt->stream_index]->codec, pframe, &dec_got_frame, dec_pkt);
            if (ret < 0)
            {
                av_frame_free(&pframe);
                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                break;
            }
            if (dec_got_frame)
            {
                if (m_pVideoCBFunc)
                {
                    QMutexLocker lock(&m_WriteLock);

                    m_pVideoCBFunc(m_pVidFmtCtx->streams[dec_pkt->stream_index], m_pVidFmtCtx->streams[m_videoindex]->codec->pix_fmt, pframe, av_gettime() - m_start_time);
                }

                av_frame_free(&pframe);
            }
            else
            {
                av_frame_free(&pframe);
            }

            av_free_packet(dec_pkt);
        }
        else
        {
            if (ret == AVERROR_EOF)
            {
                encode_video = 0;
            }
            else
            {
//                ATLTRACE("Could not read video frame\n");
                break;
            }
        }
    }

    return 0;
}

int AVInputStream::ReadAudioPackets()
{
    //audio trancoding here
    int ret;

    int encode_audio = 1;
    int dec_got_frame_a = 0;

    //start decode and encode
    while (encode_audio)
    {
        if (m_exit_thread)
        {
            break;
        }

        /**
        * Decode one frame worth of audio samples, convert it to the
        * output sample format and put it into the FIFO buffer.
        */
        AVFrame* input_frame = av_frame_alloc();
        if (!input_frame)
        {
            ret = AVERROR(ENOMEM);
            return ret;
        }

        /** Decode one frame worth of audio samples. */
        /** Packet used for temporary storage. */
        AVPacket input_packet;
        av_init_packet(&input_packet);
        input_packet.data = NULL;
        input_packet.size = 0;

        /** Read one audio frame from the input file into a temporary packet. */
        if ((ret = av_read_frame(m_pAudFmtCtx, &input_packet)) < 0)
        {
            /** If we are at the end of the file, flush the decoder below. */
            if (ret == AVERROR_EOF)
            {
                encode_audio = 0;
            }
            else
            {
//                ATLTRACE("Could not read audio frame\n");
                return ret;
            }
        }

        /**
        * Decode the audio frame stored in the temporary packet.
        * The input audio stream decoder is used to do this.
        * If we are at the end of the file, pass an empty packet to the decoder
        * to flush it.
        */
        if ((ret = avcodec_decode_audio4(m_pAudFmtCtx->streams[m_audioindex]->codec, input_frame, &dec_got_frame_a, &input_packet)) < 0)
        {
//            ATLTRACE("Could not decode audio frame\n");
            return ret;
        }
        av_packet_unref(&input_packet);
        /** If there is decoded data, convert and store it */
        if (dec_got_frame_a)
        {
            if (m_pAudioCBFunc)
            {
                QMutexLocker lock(&m_WriteLock);

                m_pAudioCBFunc(m_pAudFmtCtx->streams[m_audioindex], input_frame, av_gettime() - m_start_time);
            }
        }

        av_frame_free(&input_frame);


    }//while

    return 0;
}


bool AVInputStream::GetVideoInputInfo(int& width, int& height, int& frame_rate, AVPixelFormat& pixFmt)
{
    if (m_videoindex != -1)
    {
        width  =  m_pVidFmtCtx->streams[m_videoindex]->codec->width;
        height =  m_pVidFmtCtx->streams[m_videoindex]->codec->height;

        AVStream* stream = m_pVidFmtCtx->streams[m_videoindex];

        pixFmt = stream->codec->pix_fmt;

        //frame_rate = stream->avg_frame_rate.num/stream->avg_frame_rate.den;//每秒多少帧

        if (stream->r_frame_rate.den > 0)
        {
            frame_rate = stream->r_frame_rate.num / stream->r_frame_rate.den;
        }
        else if (stream->codec->framerate.den > 0)
        {
            frame_rate = stream->codec->framerate.num / stream->codec->framerate.den;
        }

        return true;
    }
    return false;
}

bool  AVInputStream::GetAudioInputInfo(AVSampleFormat& sample_fmt, int& sample_rate, int& channels)
{
    if (m_audioindex != -1)
    {
        sample_fmt = m_pAudFmtCtx->streams[m_audioindex]->codec->sample_fmt;
        sample_rate = m_pAudFmtCtx->streams[m_audioindex]->codec->sample_rate;
        channels = m_pAudFmtCtx->streams[m_audioindex]->codec->channels;

        return true;
    }
    return false;
}

int AVInputStream::CaptureVideoThreadFunc(void* args)
{
    AVInputStream* pThis = (AVInputStream*)args;

    pThis->ReadVideoPackets();

    return 0;
}

int AVInputStream::CaptureAudioThreadFunc(void* args)
{
    AVInputStream* pThis = (AVInputStream*)args;

    pThis->ReadAudioPackets();

    return 0;
}
