#include "main_window.h"
#include <QDebug>
#include <qsystemdetection.h>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include "ui_main_window.h"
#include "devices_dialog.h"
#include "signal_center.h"
#include "global.h"

MainWindow* g_main_frame = nullptr;

static int VideoCaptureCallback(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp)
{
    g_main_frame->output_stream_.WriteVideoFrame(input_stream, input_frame, timestamp);
    return 0;
}

static int MicrophoneCaptureCallback(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp)
{
    g_main_frame->output_stream_.WriteMicrophoneFrame(input_stream, input_frame, timestamp);
    return 0;
}

static int SpeakerCaptureCallback(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp)
{
    g_main_frame->output_stream_.WriteSpeakerFrame(input_stream, input_frame, timestamp);
    return 0;
}

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    g_main_frame = this;

    connect(SIGNAL_CENTER, &SignalCenter::StartStream, this, &MainWindow::OnStartStream);
    connect(SIGNAL_CENTER, &SignalCenter::StopStream, this, &MainWindow::OnStopStream);
}

MainWindow::~MainWindow()
{
    disconnect(SIGNAL_CENTER, &SignalCenter::StartStream, this, &MainWindow::OnStartStream);
    disconnect(SIGNAL_CENTER, &SignalCenter::StopStream, this, &MainWindow::OnStopStream);

    OnStopStream();
    delete ui;
}

void MainWindow::on_actionDevices_triggered()
{
    DevicesDialog devices_dialog;
    devices_dialog.exec(); // show(): 非模态  open(): 半模态  exec(): 模态
}

void MainWindow::OnStartStream()
{
#if defined(Q_OS_WIN)
    if (GLOBAL->config.TestCompos(COMPOS_BIT_CAMERA))
    {
        input_stream_.SetVideoDeviceName("dshow", GLOBAL->config.GetVideoCaptureDevice(), true);
    }
    else if (GLOBAL->config.TestCompos(COMPOS_BIT_DESKTOP))
    {
        input_stream_.SetVideoDeviceName("gdigrab", "desktop", false);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_MICROPHONE))
    {
        input_stream_.SetMicrophoneName("dshow", GLOBAL->config.GetMicrophone(), true);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_SPEAKER))
    {
        input_stream_.SetSpeakerName("dshow", GLOBAL->config.GetSpeaker(), true);
    }
#elif defined(Q_OS_LINUX)
    if (GLOBAL->config.TestCompos(COMPOS_BIT_CAMERA))
    {
        input_stream_.SetVideoCaptureDevice("video4linux2", GLOBAL->config.GetVideoCaptureDevice(), false);
    }
    else if (GLOBAL->config.TestCompos(COMPOS_BIT_DESKTOP))
    {
        input_stream_.SetVideoCaptureDevice("x11grab", "desktop", false);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_MICROPHONE))
    {
        input_stream_.SetAudioCaptureDevice("alsa", GLOBAL->config.GetMicrophone(), false);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_SYSTEM_VOICE))
    {
        input_stream_.SetAudio2CaptureDevice("alsa", GLOBAL->config.GetSpeaker(), false);
    }
#endif

    input_stream_.SetVideoCaptureCB(VideoCaptureCallback);
    input_stream_.SetMicrophoneCaptureCB(MicrophoneCaptureCallback);
    input_stream_.SetSpeakerCaptureCB(SpeakerCaptureCallback);

    int ret = -1;

    do
    {
        if (input_stream_.Open())
        {
            qDebug() << "failed to open input stream";
            break;
        }

        if (GLOBAL->config.TestCompos(COMPOS_BIT_CAMERA) || GLOBAL->config.TestCompos(COMPOS_BIT_DESKTOP))
        {
            int cx, cy, fps;
            AVPixelFormat pix_fmt;

            input_stream_.GetVideoOpts(cx, cy, fps, pix_fmt);

            const int bit_rate = (int) (1000000.0 * cy / 1080); // 基本所有摄像头都支持的分辨率："320*240", "640*480", "1280*720"
            qDebug() << "output video bit rate:" << bit_rate;

            // 15 20 25 30 从这四种里面选择一个最接近的
            if (0 == fps)
            {
                fps = 30;
            }
            else
            {
                const int elem_count = 4;
                int fps_array[elem_count] = { 15, 20, 25, 30 };
                int fps_delta_array[elem_count] = { abs(fps - 15), abs(fps - 20), abs(fps - 25), abs(fps - 30) };

                int min_delta = fps_delta_array[0];
                int min_idx = 0;

                for (int i = 1; i < sizeof(fps_delta_array) / sizeof(fps_delta_array[0]); ++i)
                {
                    if (fps_delta_array[i] < min_delta)
                    {
                        min_delta = fps_delta_array[i];
                        min_idx = i;
                    }
                }

                fps = fps_array[min_idx];
            }

            output_stream_.SetVideoCodecProp(AV_CODEC_ID_H264, fps, bit_rate, fps * 2, cx, cy); // gop_size=2*fps，视频比特率按比例调
        }

        int microphone_sample_rate = 192000, microphone_channels = 18;
        AVSampleFormat microphone_sample_fmt;

        int speaker_sample_rate = 192000, speaker_channels = 18;
        AVSampleFormat speaker_sample_fmt;

        if (GLOBAL->config.TestCompos(COMPOS_BIT_MICROPHONE))
        {
            input_stream_.GetMicrophoneOpts(microphone_sample_rate, microphone_sample_fmt, microphone_channels);
        }

        if (GLOBAL->config.TestCompos(COMPOS_BIT_SPEAKER))
        {
            input_stream_.GetSpeakerOpts(speaker_sample_rate, speaker_sample_fmt, speaker_channels);
        }

        const int sample_rate = qMin(microphone_sample_rate, speaker_sample_rate);
        const int channels = qMin(microphone_channels, speaker_channels);

        const int bit_rate = 32000 * channels;
        output_stream_.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, bit_rate); // 音频比特率：单声道32k，双声道64k，音乐128k

        if (output_stream_.Open(GLOBAL->config.GetFilePath()) != 0)
        {
            qDebug() << "failed to open output stream";
            break;
        }

        if (input_stream_.StartCapture() != 0)
        {
            qDebug() << "failed to start capture";
            break;
        }

        ret = 0;
    } while (0);

    if (ret != 0)
    {
        output_stream_.Close();
        input_stream_.Close();
        return;
    }

    GLOBAL->config.SetStarted(true);
}

void MainWindow::OnStopStream()
{
    input_stream_.Close();
    output_stream_.Close();

    GLOBAL->config.SetStarted(false);
}
