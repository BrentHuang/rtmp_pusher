#include "av_output_stream.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif

AVOutputStream::AVOutputStream()
{
    video_codec_id_ = AV_CODEC_ID_NONE;
    width_ = 320;
    height_ = 240;
    frame_rate_ = 25;
    video_bit_rate_ = 500000;
    gop_size_ = 100;

    audio_codec_id_ = AV_CODEC_ID_NONE;
    sample_rate_ = 44100;
    channels_ = 2;
    audio_bit_rate_ = 32000;

    fmt_ctx_ = nullptr;
    video_codec_ctx_ = nullptr;
    video_stream_ = nullptr;
    yuv_frame_ = nullptr;
    out_buffer_ = nullptr;

    audio_codec_ctx_ = nullptr;
    audio_stream_ = nullptr;
    audio_fifo_ = nullptr;
    converted_input_samples_ = nullptr;

    video_frame_count_ = 0;
    audio_frame_count_ = 0;
    nb_samples_ = 0;
    last_audio_pts_ = 0;
    next_video_ts_ = 0;
    next_audio_ts_ = 0;
    first_video_ts1_ = first_video_ts2_ = -1;
    first_audio_ts_ = -1;

    img_convert_ctx_ = nullptr;
    aud_convert_ctx_ = nullptr;
}

AVOutputStream::~AVOutputStream()
{
}

void AVOutputStream::SetVideoCodecProp(AVCodecID codec_id, int frame_rate, int bit_rate, int gop_size, int width, int height)
{
    video_codec_id_ = codec_id;
    width_ = width;
    height_ = height;
    frame_rate_ = ((frame_rate == 0) ? 10 : frame_rate);
    video_bit_rate_ = bit_rate;
    gop_size_ = gop_size;
}

void AVOutputStream::SetAudioCodecProp(AVCodecID codec_id, int sample_rate, int channels, int bit_rate)
{
    audio_codec_id_ = codec_id;
    sample_rate_ = sample_rate;
    channels_ = channels;
    audio_bit_rate_ = bit_rate;
}

int AVOutputStream::Open(const std::string& file_path)
{
    // output initialize
    int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, file_path.c_str());
    if (ret < 0)
    {
        return -1;
    }

    if (video_codec_id_ != AV_CODEC_ID_NONE)
    {
        // output video encoder initialize
        AVCodec* codec = avcodec_find_encoder(video_codec_id_);
        if (nullptr == codec)
        {
//            ATLTRACE("Can not find output video encoder! (没有找到合适的编码器！)\n");
            return -1;
        }

        video_codec_ctx_ = avcodec_alloc_context3(codec);
        if (nullptr == video_codec_ctx_)
        {
            return -1;
        }

        video_codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
        video_codec_ctx_->width = width_;
        video_codec_ctx_->height = height_;
        video_codec_ctx_->time_base.num = 1;
        video_codec_ctx_->time_base.den = frame_rate_;
        video_codec_ctx_->bit_rate = video_bit_rate_;
        video_codec_ctx_->gop_size = gop_size_;

        /* Some formats want stream headers to be separate. */
        if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
        {
            video_codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        AVDictionary* opts = 0;

        // set H264 codec param
        if (AV_CODEC_ID_H264 == video_codec_id_)
        {
            // TODO
            //pCodecCtx->me_range = 16;
            //pCodecCtx->max_qdiff = 4;
            //pCodecCtx->qcompress = 0.6;
            video_codec_ctx_->qmin = 10;
            video_codec_ctx_->qmax = 51;
            //Optional Param
            video_codec_ctx_->max_b_frames = 0; // 不要B帧？

#if 1
            //下面设置两个参数影响编码延时，如果不设置，编码器默认会缓冲很多帧
            // Set H264 preset and tune
            av_dict_set(&opts, "preset", "fast", 0);
            av_dict_set(&opts, "tune", "zerolatency", 0);
#else
            /**
             * ultrafast,superfast, veryfast, faster, fast, medium
             * slow, slower, veryslow, placebo.
             注意：这是x264编码速度的选项， 设置该参数可以降低编码延时
             */
            av_opt_set(video_codec_ctx_->priv_data, "preset", "superfast", 0);
#endif
        }

        ret = avcodec_open2(video_codec_ctx_, codec, &opts);
        if (ret < 0)
        {
//            ATLTRACE("Failed to open output video encoder! (编码器打开失败！)\n");
            return -1;
        }

        // Add a new stream to output,should be called by the user before avformat_write_header() for muxing
        video_stream_ = avformat_new_stream(fmt_ctx_, codec);
        if (nullptr == video_stream_)
        {
            return -1;
        }

        video_stream_->time_base.num = 1;
        video_stream_->time_base.den = frame_rate_;
//        video_stream_->codec = video_codec_ctx_;

        ret = avcodec_parameters_from_context(video_stream_->codecpar, video_codec_ctx_);
        if (ret < 0)
        {
            return -1;
        }

        // Initialize the buffer to store YUV frames to be encoded.
        yuv_frame_ = av_frame_alloc();
        if (nullptr == yuv_frame_)
        {
            return -1;
        }

        out_buffer_ = (uint8_t*) av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video_codec_ctx_->width, video_codec_ctx_->height, 1));
        if (nullptr == out_buffer_)
        {
            return -1;
        }

        ret = av_image_fill_arrays(yuv_frame_->data, yuv_frame_->linesize, out_buffer_, AV_PIX_FMT_YUV420P,
                                   video_codec_ctx_->width, video_codec_ctx_->height, 1);
        if (ret < 0)
        {
            return -1;
        }
    }

    if (audio_codec_id_ != AV_CODEC_ID_NONE)
    {
        //output audio encoder initialize
        AVCodec* codec = avcodec_find_encoder(audio_codec_id_);
        if (nullptr == codec)
        {
//            ATLTRACE("Can not find output audio encoder! (没有找到合适的编码器！)\n");
            return -1;
        }

        audio_codec_ctx_ = avcodec_alloc_context3(codec);
        if (nullptr == audio_codec_ctx_)
        {
            return -1;
        }

        audio_codec_ctx_->channels = channels_;
        audio_codec_ctx_->channel_layout = av_get_default_channel_layout(channels_);
        audio_codec_ctx_->sample_rate = sample_rate_;
        audio_codec_ctx_->sample_fmt = codec->sample_fmts[0];
        audio_codec_ctx_->bit_rate = audio_bit_rate_;
        audio_codec_ctx_->time_base.num = 1;
        audio_codec_ctx_->time_base.den = audio_codec_ctx_->sample_rate;

        if (AV_CODEC_ID_AAC == audio_codec_id_)
        {
            /** Allow the use of the experimental AAC encoder */
            audio_codec_ctx_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
        }

        /* Some formats want stream headers to be separate. */
        if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
        {
            audio_codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret = avcodec_open2(audio_codec_ctx_, codec, nullptr);
        if (ret < 0)
        {
//            ATLTRACE("Failed to open ouput audio encoder! (编码器打开失败！)\n");
            return -1;
        }

        // Add a new stream to output,should be called by the user before avformat_write_header() for muxing
        audio_stream_ = avformat_new_stream(fmt_ctx_, codec);
        if (nullptr == audio_stream_)
        {
            return -1;
        }

        audio_stream_->time_base.num = 1;
        audio_stream_->time_base.den = audio_codec_ctx_->sample_rate;
//        audio_stream_->codec = audio_codec_ctx_;

        ret = avcodec_parameters_from_context(audio_stream_->codecpar, audio_codec_ctx_);
        if (ret < 0)
        {
            return -1;
        }

        // Initialize the FIFO buffer to store audio samples to be encoded.
        audio_fifo_ = av_audio_fifo_alloc(audio_codec_ctx_->sample_fmt, audio_codec_ctx_->channels, 1);
        if (nullptr == audio_fifo_)
        {
            return -1;
        }

        // Initialize the buffer to store converted samples to be encoded.

        /**
        * Allocate as many pointers as there are audio channels.
        * Each pointer will later point to the audio samples of the corresponding
        * channels (although it may be nullptr for interleaved formats).
        */
        converted_input_samples_ = (uint8_t**) calloc(audio_codec_ctx_->channels, sizeof(**converted_input_samples_));
        if (nullptr == converted_input_samples_)
        {
//            ATLTRACE("Could not allocate converted input sample pointers\n");
            return -1;
        }

        converted_input_samples_[0] = nullptr;
    }

    // Open output URL,set before avformat_write_header() for muxing
    if (avio_open(&fmt_ctx_->pb, file_path.c_str(), AVIO_FLAG_READ_WRITE) < 0)
    {
//        ATLTRACE("Failed to open output file! (输出文件打开失败！)\n");
        return -1;
    }

    // Show some Information
    av_dump_format(fmt_ctx_, 0, file_path.c_str(), 1);

    // Write File Header
    avformat_write_header(fmt_ctx_, nullptr);

    video_frame_count_ = 0;
    audio_frame_count_ = 0;
    nb_samples_ = 0;
    last_audio_pts_ = 0;
    next_video_ts_ = 0;
    next_audio_ts_ = 0;
    first_video_ts1_ = first_video_ts2_ = -1;
    first_audio_ts_ = -1;

    return 0;
}

void  AVOutputStream::Close()
{
    if (fmt_ctx_ != nullptr)
    {
        if (video_stream_ != nullptr || audio_stream_ != nullptr)
        {
            // Write file trailer
            av_write_trailer(fmt_ctx_);
        }
    }

    if (out_buffer_)
    {
        av_free(out_buffer_);
        out_buffer_ = nullptr;
    }

    if (yuv_frame_ != nullptr)
    {
        av_frame_free(&yuv_frame_);
        yuv_frame_ = nullptr;
    }

    if (converted_input_samples_ != nullptr)
    {
        av_freep(&converted_input_samples_[0]);
        //free(converted_input_samples);
        converted_input_samples_ = nullptr;
    }

    if (audio_fifo_ != nullptr)
    {
        av_audio_fifo_free(audio_fifo_);
        audio_fifo_ = nullptr;
    }

//    if (video_codec_ctx_ != nullptr)
//    {
//        avcodec_free_context(&video_codec_ctx_);
//        video_codec_ctx_ = nullptr;
//    }

//    if (audio_codec_ctx_ != nullptr)
//    {
//        avcodec_free_context(&audio_codec_ctx_);
//        audio_codec_ctx_ = nullptr;
//    }

    if (fmt_ctx_ != nullptr)
    {
        avio_close(fmt_ctx_->pb);
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = nullptr;
    }

    video_codec_id_ = AV_CODEC_ID_NONE;
    video_codec_ctx_ = nullptr;
    video_stream_ = nullptr;

    audio_codec_id_ = AV_CODEC_ID_NONE;
    audio_codec_ctx_ = nullptr;
    audio_stream_ = nullptr;
}

//input_st -- 输入流的信息
//input_frame -- 输入视频帧的信息
//lTimeStamp -- 时间戳，时间单位为1/1000000
//
int AVOutputStream::WriteVideoFrame(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp)
{
    (void) input_stream;

//    对传入的图像帧进行编码（H264），并且写到指定的封装文件
    if (nullptr == video_stream_)
    {
        return -1;
    }

    //ATLTRACE("Video timestamp: %ld \n", lTimeStamp);

    if (-1 == first_video_ts1_)
    {
//        TRACE("First Video timestamp: %ld \n", lTimeStamp);
        first_video_ts1_ = timestamp;
    }

    AVRational time_base_q = { 1, AV_TIME_BASE };

    int ret = avcodec_parameters_to_context(video_codec_ctx_, video_stream_->codecpar);
    if (ret < 0)
    {
        return -1;
    }

    if (nullptr == img_convert_ctx_)
    {
        //camera data may has a pix fmt of RGB or sth else,convert it to YUV420
        img_convert_ctx_ = sws_getContext(width_, height_, input_pix_fmt,
                                          video_codec_ctx_->width, video_codec_ctx_->height, AV_PIX_FMT_YUV420P,
                                          SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (nullptr == img_convert_ctx_)
        {
            return -1;
        }
    }

    sws_scale(img_convert_ctx_, (const uint8_t* const*) input_frame->data, input_frame->linesize, 0, video_codec_ctx_->height,
              yuv_frame_->data, yuv_frame_->linesize);
    yuv_frame_->width = input_frame->width;
    yuv_frame_->height = input_frame->height;
    yuv_frame_->format = AV_PIX_FMT_YUV420P;

    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    enc_pkt.data = nullptr;
    enc_pkt.size = 0;

    int enc_got_frame = 0;

    ret = avcodec_encode_video2(video_codec_ctx_, &enc_pkt, yuv_frame_, &enc_got_frame);

    if (enc_got_frame == 1)
    {
        //printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, enc_pkt.size);

        if (first_video_ts2_ == -1)
        {
            first_video_ts2_ = timestamp;
        }

        enc_pkt.stream_index = video_stream_->index;

#if 0
        //Write PTS
        AVRational time_base = video_st->time_base;//{ 1, 1000 };
        AVRational r_framerate1 = input_st->r_frame_rate;//{ 50, 2 };
        //Duration between 2 frames (us)
        // int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));    //内部时间戳
        int64_t calc_pts = (double)m_vid_framecnt * (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));

        //Parameters
        enc_pkt.pts = av_rescale_q(calc_pts, time_base_q, time_base);  //enc_pkt.pts = (double)(framecnt*calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));
        enc_pkt.dts = enc_pkt.pts;
        //enc_pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base); //(double)(calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));
        //enc_pkt.pos = -1;
#else
        //enc_pkt.pts= av_rescale_q(lTimeStamp, time_base_q, video_st->time_base);
        enc_pkt.pts = (int64_t)video_stream_->time_base.den * timestamp / AV_TIME_BASE;
#endif

        video_frame_count_++;

        ////Delay
        //int64_t pts_time = av_rescale_q(enc_pkt.pts, time_base, time_base_q);
        //int64_t now_time = av_gettime() - start_time;
        //if ((pts_time > now_time) && ((vid_next_pts + pts_time - now_time)<aud_next_pts))
        //  av_usleep(pts_time - now_time);

        ret = av_interleaved_write_frame(fmt_ctx_, &enc_pkt);
        if (ret < 0)
        {
//            char tmpErrString[128] = {0};
//            ATLTRACE("Could not write video frame, error: %s\n", av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, ret));
            return ret;
        }
    }
    else if (ret == 0)
    {
//        ATLTRACE("Buffer video frame, timestamp: %I64d.\n", lTimeStamp); //编码器缓冲帧
    }

    return 0;
}

//input_st -- 输入流的信息
//input_frame -- 输入音频帧的信息
//lTimeStamp -- 时间戳，时间单位为1/1000000
//
int  AVOutputStream::WriteAudioFrame(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp)
{
//    对音频编码（AAC），然后输出到指定的封装文件
    if (nullptr == audio_stream_)
    {
        return -1;
    }

    if (-1 == first_audio_ts_)
    {
//        TRACE("First Audio timestamp: %ld \n", lTimeStamp);
        first_audio_ts_ = timestamp;
    }

    int ret = avcodec_parameters_to_context(audio_codec_ctx_, audio_stream_->codecpar);
    if (ret < 0)
    {
        return -1;
    }

    const int output_frame_size = audio_codec_ctx_->frame_size;
    AVRational time_base_q = { 1, AV_TIME_BASE };

    //if((int64_t)(av_audio_fifo_size(m_fifo) + input_frame->nb_samples) * AV_TIME_BASE /(int64_t)(input_st->codec->sample_rate) - lTimeStamp > AV_TIME_BASE/10)
    //{
    //  TRACE("audio data is overflow \n");
    //  return 0;
    //}

    const int fifo_samples = av_audio_fifo_size(audio_fifo_);
    int64_t timeshift = (int64_t)fifo_samples * AV_TIME_BASE / (int64_t)(input_stream->codec->sample_rate); // 因为Fifo里有之前未读完的数据，所以从Fifo队列里面取出的第一个音频包的时间戳等于当前时间减掉缓冲部分的时长

//    TRACE("audio time diff: %I64d \n", lTimeStamp - timeshift - m_nLastAudioPresentationTime); //理论上该差值稳定在一个水平，如果差值一直变大（在某些采集设备上发现有此现象），则会有视音频不同步的问题，具体产生的原因不清楚
    audio_frame_count_ += input_frame->nb_samples;

    if (nullptr == aud_convert_ctx_)
    {
        // Initialize the resampler to be able to convert audio sample formats
        aud_convert_ctx_ = swr_alloc_set_opts(nullptr,
                                              av_get_default_channel_layout(audio_codec_ctx_->channels),
                                              audio_codec_ctx_->sample_fmt,
                                              audio_codec_ctx_->sample_rate,
                                              av_get_default_channel_layout(input_stream->codec->channels),
                                              input_stream->codec->sample_fmt,
                                              input_stream->codec->sample_rate,
                                              0, nullptr);
        if (nullptr == aud_convert_ctx_)
        {
            return -1;
        }

        /**
        * Perform a sanity check so that the number of converted samples is
        * not greater than the number of samples to be converted.
        * If the sample rates differ, this case has to be handled differently
        */
//        ATLASSERT(pCodecCtx_a->sample_rate == input_st->codec->sample_rate);

        swr_init(aud_convert_ctx_);
    }

    /**
    * Allocate memory for the samples of all channels in one consecutive
    * block for convenience.
    */
    // TODO 内存泄露
    ret = av_samples_alloc(converted_input_samples_, nullptr,
                           audio_codec_ctx_->channels, input_frame->nb_samples, audio_codec_ctx_->sample_fmt, 0);
    if (ret < 0)
    {
//        ATLTRACE("Could not allocate converted input samples\n");
        av_freep(&(*converted_input_samples_)[0]);
        free(*converted_input_samples_);
        return ret;
    }

    /**
    * Convert the input samples to the desired output sample format.
    * This requires a temporary storage provided by converted_input_samples.
    */
    /** Convert the samples using the resampler. */
    ret = swr_convert(aud_convert_ctx_,
                      converted_input_samples_, swr_get_out_samples(aud_convert_ctx_, input_frame->nb_samples),
                      (const uint8_t**) input_frame->extended_data, input_frame->nb_samples);
    if (ret < 0)
    {
//        ATLTRACE("Could not convert input samples\n");
        return ret;
    }

    /** Add the converted input samples to the FIFO buffer for later processing. */
    /**
    * Make the FIFO as large as it needs to be to hold both,
    * the old and the new samples.
    */
    // TODO 内存泄露
    ret = av_audio_fifo_realloc(audio_fifo_, av_audio_fifo_size(audio_fifo_) + input_frame->nb_samples);
    if (ret < 0)
    {
//        ATLTRACE("Could not reallocate FIFO\n");
        return ret;
    }

    /** Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(audio_fifo_, (void**) converted_input_samples_, input_frame->nb_samples) < input_frame->nb_samples)
    {
//        ATLTRACE("Could not write data to FIFO\n");
        return -1;
    }

    const int64_t timeinc = (int64_t)audio_codec_ctx_->frame_size * AV_TIME_BASE / (int64_t)(input_stream->codec->sample_rate);

    // 当前帧的时间戳不能小于上一帧的值
    if (timestamp - timeshift > last_audio_pts_)
    {
        last_audio_pts_ = timestamp - timeshift;
    }

    while (av_audio_fifo_size(audio_fifo_) >= output_frame_size)
        /**
        * Take one frame worth of audio samples from the FIFO buffer,
        * encode it and write it to the output file.
        */
    {
        /** Temporary storage of the output samples of the frame written to the file. */
        AVFrame* output_frame = av_frame_alloc();
        if (nullptr == output_frame)
        {
            return -1;
        }

        /**
        * Use the maximum number of possible samples per frame.
        * If there is less than the maximum possible frame size in the FIFO
        * buffer use this number. Otherwise, use the maximum possible frame size
        */
        const int frame_size = FFMIN(av_audio_fifo_size(audio_fifo_), audio_codec_ctx_->frame_size);


        /** Initialize temporary storage for one output frame. */
        /**
        * Set the frame's parameters, especially its size and format.
        * av_frame_get_buffer needs this to allocate memory for the
        * audio samples of the frame.
        * Default channel layouts based on the number of channels
        * are assumed for simplicity.
        */
        output_frame->nb_samples = frame_size;
        output_frame->channel_layout = audio_codec_ctx_->channel_layout;
        output_frame->format = audio_codec_ctx_->sample_fmt;
        output_frame->sample_rate = audio_codec_ctx_->sample_rate;

        /**
        * Allocate the samples of the created frame. This call will make
        * sure that the audio frame can hold as many samples as specified.
        */
        ret = av_frame_get_buffer(output_frame, 0);
        if (ret < 0)
        {
//            ATLTRACE("Could not allocate output frame samples\n");
            av_frame_free(&output_frame);
            return ret;
        }

        /**
        * Read as many samples from the FIFO buffer as required to fill the frame.
        * The samples are stored in the frame temporarily.
        */
        if (av_audio_fifo_read(audio_fifo_, (void**) output_frame->data, frame_size) < frame_size)
        {
//            ATLTRACE("Could not read data from FIFO\n");
            av_frame_free(&output_frame);
            return -1;
        }

        /** Encode one frame worth of audio samples. */
        /** Packet used for temporary storage. */
        AVPacket enc_pkt;
        av_init_packet(&enc_pkt);
        enc_pkt.data = nullptr;
        enc_pkt.size = 0;

        int enc_got_frame_a = 0;

        /**
        * Encode the audio frame and store it in the temporary packet.
        * The output audio stream encoder is used to do this.
        */
        if ((ret = avcodec_encode_audio2(audio_codec_ctx_, &enc_pkt, output_frame, &enc_got_frame_a)) < 0)
        {
//            ATLTRACE("Could not encode frame\n");
            av_frame_free(&output_frame);
            return ret;
        }


        /** Write one audio frame from the temporary packet to the output file. */
        if (enc_got_frame_a)
        {
            //output_packet.flags |= AV_PKT_FLAG_KEY;
            enc_pkt.stream_index = audio_stream_->index;

#if 0
            AVRational r_framerate1 = { input_st->codec->sample_rate, 1 };// { 44100, 1};
            //int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));  //内部时间戳
            int64_t calc_pts = (double)m_nb_samples * (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));

            output_packet.pts = av_rescale_q(calc_pts, time_base_q, audio_st->time_base);
            //output_packet.dts = output_packet.pts;
            //output_packet.duration = output_frame->nb_samples;
#else
            enc_pkt.pts = av_rescale_q(last_audio_pts_, time_base_q, audio_stream_->time_base);

#endif

            //ATLTRACE("audio pts : %ld\n", output_packet.pts);

            //int64_t pts_time = av_rescale_q(output_packet.pts, time_base, time_base_q);
            //int64_t now_time = av_gettime() - start_time;
            //if ((pts_time > now_time) && ((aud_next_pts + pts_time - now_time)<vid_next_pts))
            //  av_usleep(pts_time - now_time);

            ret = av_interleaved_write_frame(fmt_ctx_, &enc_pkt);
            if (ret < 0)
            {
//                char tmpErrString[128] = {0};
//                ATLTRACE("Could not write audio frame, error: %s\n", av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, ret));
                av_frame_free(&output_frame);
                return ret;
            }
        }//if (enc_got_frame_a)

        nb_samples_ += output_frame->nb_samples;
        last_audio_pts_ += timeinc;

        av_frame_free(&output_frame);
    }//while

    return 0;
}
