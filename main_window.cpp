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

static int VideoCaptureCallback(AVStream* input_stream, AVPixelFormat input_pix_fmt, AVFrame* input_frame, int64_t timestamp)
{
    g_main_frame->output_stream_.WriteVideoFrame(input_stream, input_pix_fmt, input_frame, timestamp);
    return 0;
}

static int AudioCaptureCallback(AVStream* input_stream, AVFrame* input_frame, int64_t timestamp)
{
    g_main_frame->output_stream_.WriteAudioFrame(input_stream, input_frame, timestamp);
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
        input_stream_.SetVideoCaptureDevice("dshow", GLOBAL->config.GetVideoCaptureDevice(), true);
    }
    else if (GLOBAL->config.TestCompos(COMPOS_BIT_DESKTOP))
    {
        input_stream_.SetVideoCaptureDevice("gdigrab", "desktop", false);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_MICROPHONE))
    {
        input_stream_.SetAudioCaptureDevice("dshow", GLOBAL->config.GetAudioCaptureDevice(), true);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_SYSTEM_VOICE))
    {
        input_stream_.SetAudio2CaptureDevice("dshow", GLOBAL->config.GetAudio2CaptureDevice(), true);
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
        input_stream_.SetAudioCaptureDevice("alsa", GLOBAL->config.GetAudioCaptureDevice(), false);
    }

    if (GLOBAL->config.TestCompos(COMPOS_BIT_SYSTEM_VOICE))
    {
        input_stream_.SetAudio2CaptureDevice("alsa", GLOBAL->config.GetAudio2CaptureDevice(), false);
    }
#endif

    input_stream_.SetVideoCaptureCB(VideoCaptureCallback);
    input_stream_.SetAudioCaptureCB(AudioCaptureCallback);

    int ret = -1;

    do
    {
        if (input_stream_.Open() != 0)
        {
            break;
        }

        int cx, cy, fps;
        AVPixelFormat pixel_fmt;

        if (0 == input_stream_.GetVideoInfo(cx, cy, fps, pixel_fmt))
        {
            output_stream_.SetVideoCodecProp(AV_CODEC_ID_H264, fps, 500000, 100, cx, cy); // gop_size=2*fps，视频比特率：TODO
        }

        AVSampleFormat sample_fmt;
        int sample_rate, channels;

        if (0 == input_stream_.GetAudioInfo(sample_fmt, sample_rate, channels))
        {
            output_stream_.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, 32000); // 音频比特率：TODO
        }

        if (output_stream_.Open(GLOBAL->config.GetFilePath()) != 0)
        {
            break;
        }

        if (input_stream_.StartCapture() != 0)
        {
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
