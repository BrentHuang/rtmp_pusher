#include "main_window.h"
#include <QDebug>
#include "ui_main_window.h"

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include <chrono>
#include <QCameraInfo>
#include <QCamera>

MainWindow*           gpMainFrame = NULL;

static int64_t timeGetTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

//采集到的视频图像回调
static int VideoCaptureCallback(AVStream* input_st, AVPixelFormat pix_fmt, AVFrame* pframe, int64_t lTimeStamp)
{
//    if (gpMainFrame->IsPreview())
//    {
//        gpMainFrame->m_Painter.Play(input_st, pframe);
//    }

    gpMainFrame->m_OutputStream.write_video_frame(input_st, pix_fmt, pframe, lTimeStamp);
    return 0;
}

//采集到的音频数据回调
static int AudioCaptureCallback(AVStream* input_st, AVFrame* pframe, int64_t lTimeStamp)
{
    gpMainFrame->m_OutputStream.write_audio_frame(input_st, pframe, lTimeStamp);
    return 0;
}

int64_t  StartTime = 0;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    gpMainFrame = this;

    avdevice_register_all();
}

MainWindow::~MainWindow()
{
    OnStopStream();
    delete ui;
}

void MainWindow::on_actionDevices_triggered()
{
    AVFormatContext* pFmtCtx = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat* iformat = av_find_input_format("dshow");
    printf("Device Info=============\n");
    avformat_open_input(&pFmtCtx, "video=dummy", iformat, &options);
    printf("========================\n");

// 开始采集

//    m_InputStream.SetVideoCaptureDevice(T2A((LPTSTR)(LPCTSTR)dlg.m_strVideoDevice));
//    m_InputStream.SetAudioCaptureDevice(T2A((LPTSTR)(LPCTSTR)dlg.m_strAudioDevice));

//    m_InputStream.SetVideoCaptureDevice(QCameraInfo::defaultCamera().description().toStdString());

//    QCamera* camera = new QCamera(QCameraInfo::defaultCamera());
//    camera->start();

    OnStartStream();

// 停止采集
//    OnStopStream();


}

void MainWindow::OnStartStream()
{
    m_InputStream.SetVideoCaptureCB(VideoCaptureCallback);
    m_InputStream.SetAudioCaptureCB(AudioCaptureCallback);

    bool bRet;
    bRet = m_InputStream.OpenInputStream(); //初始化采集设备
    if (!bRet)
    {
//        MessageBox(_T("打开采集设备失败"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

    int cx, cy, fps;
    AVPixelFormat pixel_fmt;
    if (m_InputStream.GetVideoInputInfo(cx, cy, fps, pixel_fmt)) //获取视频采集源的信息
    {
        m_OutputStream.SetVideoCodecProp(AV_CODEC_ID_H264, fps, 500000, 100, cx, cy); //设置视频编码器属性
    }

    int sample_rate = 0, channels = 0;
    AVSampleFormat  sample_fmt;
    if (m_InputStream.GetAudioInputInfo(sample_fmt, sample_rate, channels)) //获取音频采集源的信息
    {
        m_OutputStream.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, 32000); //设置音频编码器属性
    }

    //从Config.INI文件中读取录制文件路径
//    P_GetProfileString(_T("Client"), "file_path", m_szFilePath, sizeof(m_szFilePath));

    bRet  = m_OutputStream.OpenOutputStream(m_szFilePath.c_str()); //设置输出路径
    if (!bRet)
    {
//        MessageBox(_T("初始化输出失败"), _T("提示"), MB_OK | MB_ICONWARNING);
        return;
    }

//    m_Painter.SetVideoWindow(m_wndView.GetSafeHwnd()); //设置视频预览窗口
//    m_Painter.SetSrcFormat(cx, cy, 24, TRUE); //第4个参数表示是否翻转图像
//    m_Painter.SetStretch(TRUE); //是否缩放显示图像
//    m_Painter.Open();

    bRet = m_InputStream.StartCapture(); //开始采集

    StartTime = timeGetTime();

    m_frmCount = 0;
    m_nFPS = 0;
}

void MainWindow::OnStopStream()
{
    m_InputStream.CloseInputStream();
    m_OutputStream.CloseOutput();
//    m_Painter.Close();

//    TRACE("采集用时：%d 秒\n", (timeGetTime() - StartTime) / 1000);

    StartTime = 0;
}

int MainWindow::ShowDevices()
{
    //    ::CoInitialize(NULL); //调用DirectShow SDK的API需要用到COM库


    //    int iVideoCapDevNum = 0;
    //    int iAudioCapDevNum = 0;

    //    char* DevicesArray[20];
    //    for (int i = 0; i < 20; i++)
    //    {
    //        DevicesArray[i] = new char[256];
    //        memset(DevicesArray[i], 0, 256);
    //    }

    //    HRESULT hr;
    //    hr = EnumDevice(DSHOW_VIDEO_DEVICE, DevicesArray, sizeof(DevicesArray) / sizeof(DevicesArray[0]), iVideoCapDevNum);
    //    if (hr == S_OK)
    //    {
    //        for (int i = 0; i < iVideoCapDevNum; i++)
    //        {
    //            CString strDevName = DevicesArray[i];
    //            ((CComboBox*)GetDlgItem(IDC_COMBO_VIDEO_DEVICES))->AddString(strDevName);
    //        }
    //    }

    //    hr = EnumDevice(DSHOW_AUDIO_DEVICE, DevicesArray, sizeof(DevicesArray) / sizeof(DevicesArray[0]), iAudioCapDevNum);
    //    if (hr == S_OK)
    //    {
    //        for (int i = 0; i < iAudioCapDevNum; i++)
    //        {
    //            CString strDevName = DevicesArray[i];
    //            ((CComboBox*)GetDlgItem(IDC_COMBO_AUDIO_DEVICES))->AddString(strDevName);
    //        }
    //    }

    //    for (int i = 0; i < 20; i++)
    //    {
    //        delete DevicesArray[i];
    //        DevicesArray[i] = NULL;
    //    }

    //    if (iVideoCapDevNum > 0)
    //    {
    //        ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_VIDEODEV))->SetCheck(TRUE);
    //    }

    //    if (iAudioCapDevNum > 0)
    //    {
    //        ((CButton*)GetDlgItem(IDC_CHECK_ENABLE_AUDIODEV))->SetCheck(TRUE);
    //    }

    //    CoUninitialize();

    return 0;
}
