#include "devices_dialog.h"
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QFileDialog>
#include <QMessageBox>
#include <QCameraInfo>
#include <QCamera>
#include <QCameraViewfinder>
#include <QAudioDeviceInfo>
#include <QButtonGroup>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

//#include <dshowcapture.hpp>

#include "ui_devices_dialog.h"
//#include "ds_av_devices.h"
//#include "mf_av_devices.h"
#include "global.h"
#include "signal_center.h"

#if defined(Q_OS_LINUX)
#include <alsa/asoundlib.h>

#define error printf

static void AlsaDeviceList()
{
    int i;
    printf("<----- 1="" -----="">ALSA library version=%s\n", SND_LIB_VERSION_STR);

    printf("\n<----- 2="" -----="">PCM stream types:\n");
    for (i = 0; i <= SND_PCM_STREAM_LAST; i++)
    {
        printf("%s\n", snd_pcm_stream_name((snd_pcm_stream_t) i));
    }

    printf("\n<----- 3="" -----="">PCM access type:\n");
    for (i = 0; i <= SND_PCM_ACCESS_LAST; i++)
    {
        printf("%s\n", snd_pcm_access_name((snd_pcm_access_t) i));
    }

    printf("\n<----- 4="" -----="">PCM formats:\n");
    for (i = 0; i <= SND_PCM_FORMAT_LAST; i++)
    {
        if (NULL != snd_pcm_format_name((snd_pcm_format_t) i))
        {
            printf("%s:%s\n", snd_pcm_format_name((snd_pcm_format_t) i),
                   snd_pcm_format_description((snd_pcm_format_t) i));
        }
    }

    printf("\n<----- 5="" -----="">PCM subformats:\n");
    for (i = 0; i <= SND_PCM_SUBFORMAT_LAST; i++)
    {
        printf("%s:%s\n", snd_pcm_subformat_name((snd_pcm_subformat_t) i),
               snd_pcm_subformat_description((snd_pcm_subformat_t) i));
    }

    printf("\n<----- 6="" -----="">PCM state:\n");
    for (i = 0; i <= SND_PCM_STATE_LAST; i++)
    {
        printf("%s\n", snd_pcm_state_name((snd_pcm_state_t) i));
    }

    snd_ctl_t* handle;
    int card, err, dev, idx;
    snd_ctl_card_info_t* info;
    snd_pcm_info_t* pcminfo;
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);

    card = -1;
    if (snd_card_next(&card) < 0 || card < 0)
    {
        error("no soundcards found...");
        return;
    }

    snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

    printf("\n<----- 4="" -----="">List of %s Hardware Devices:\n", snd_pcm_stream_name(stream));
    while (card >= 0)
    {
        char name[32];
        sprintf(name, "hw:%d", card);
        if ((err = snd_ctl_open(&handle, name, 0)) < 0)
        {
            error("control open (%i): %s", card, snd_strerror(err));
            goto next_card;
        }
        if ((err = snd_ctl_card_info(handle, info)) < 0)
        {
            error("control hardware info (%i): %s", card, snd_strerror(err));
            snd_ctl_close(handle);
            goto next_card;
        }
        dev = -1;
        while (1)
        {
            unsigned int count;
            if (snd_ctl_pcm_next_device(handle, &dev) < 0)
            {
                error("snd_ctl_pcm_next_device");
            }
            if (dev < 0)
            {
                break;
            }
            snd_pcm_info_set_device(pcminfo, dev);
            snd_pcm_info_set_subdevice(pcminfo, 0);
            snd_pcm_info_set_stream(pcminfo, stream);
            if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
            {
                if (err != -ENOENT)
                {
                    error("control digital audio info (%i): %s", card, snd_strerror(err));
                }
                continue;
            }
            printf("card %i: %s [%s], device %i: %s [%s]\n",
                   card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info),
                   dev,
                   snd_pcm_info_get_id(pcminfo),
                   snd_pcm_info_get_name(pcminfo));
            count = snd_pcm_info_get_subdevices_count(pcminfo);
            printf("  Subdevices: %i/%i\n",
                   snd_pcm_info_get_subdevices_avail(pcminfo), count);
            for (idx = 0; idx < (int)count; idx++)
            {
                snd_pcm_info_set_subdevice(pcminfo, idx);
                if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
                {
                    error("control digital audio playback info (%i): %s", card, snd_strerror(err));
                }
                else
                {
                    printf("  Subdevice #%i: %s\n",
                           idx, snd_pcm_info_get_subdevice_name(pcminfo));
                }
            }
        }
        snd_ctl_close(handle);
next_card:
        if (snd_card_next(&card) < 0)
        {
            error("snd_card_next");
            break;
        }
    }
}
#endif

DevicesDialog::DevicesDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::DevicesDialog)
{
    ui->setupUi(this);

    QButtonGroup* button_group = new QButtonGroup(this);
    button_group->addButton(ui->checkBox_Camera, 1);
    button_group->addButton(ui->checkBox_Desktop, 2);

//#if defined(Q_OS_WIN)
//    std::vector<DShow::VideoDevice> video_devices;
//    std::vector<DShow::AudioDevice> audio_devices;
//    DShow::Device::EnumVideoDevices(video_devices);
//    DShow::Device::EnumAudioDevices(audio_devices);

//    for (auto video_device : video_devices)
//    {
//        qDebug() << QString::fromStdWString(video_device.name);

//        for (auto x : video_device.caps)
//        {
//            qDebug() << x.minCX << x.minCY << x.maxCX << x.maxCY << x.minInterval << x.maxInterval
//                     << x.granularityCX << x.granularityCY << (int) x.format;
//        }
//    }

//    std::vector<DeviceCtx> video_device_vec;
//    std::vector<DeviceCtx> audio_device_vec;
//    std::vector<DeviceCtx> audio2_device_vec;

//    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
//    {
//        if (MFGetAVDevices(video_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID) != 0)
//        {
//            qDebug() << "no video devices";
//            return;
//        }

//        if (MFGetAVDevices(audio_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID) != 0)
//        {
//            qDebug() << "no audio devices";
//            return;
//        }
//    }
//    else
//    {
//        if (DSGetAVDevices(video_device_vec, CLSID_VideoInputDeviceCategory) != 0)
//        {
//            qDebug() << "no video devices";
//            return;
//        }

//        if (DSGetAVDevices(audio_device_vec, CLSID_AudioInputDeviceCategory) != 0)
//        {
//            qDebug() << "no audio devices";
//            return;
//        }

//        if (DSGetAVDevices(audio2_device_vec, CLSID_AudioRendererCategory) != 0)
//        {
//            qDebug() << "no audio2(speaker) devices";
//            return;
//        }
//    }

//    qDebug() << "video input devices:";
//    for (int i = 0; i < (int) video_device_vec.size(); ++i)
//    {
//        qDebug() << i << QString::fromWCharArray(video_device_vec[i].FriendlyName)
//                 << QString::fromWCharArray(video_device_vec[i].MonikerName);
//        ui->comboBox_Video->addItem(QString::fromWCharArray(video_device_vec[i].FriendlyName));
//    }

//    qDebug() << "audio input devices:";
//    for (int i = 0; i < (int) audio_device_vec.size(); ++i)
//    {
//        qDebug() << i << QString::fromWCharArray(audio_device_vec[i].FriendlyName)
//                 << QString::fromWCharArray(audio_device_vec[i].MonikerName);
//        ui->comboBox_Audio->addItem(QString::fromWCharArray(audio_device_vec[i].FriendlyName));
//    }

//    qDebug() << "audio2(speaker) input devices:";
//    for (int i = 0; i < (int) audio2_device_vec.size(); ++i)
//    {
//        qDebug() << i << QString::fromWCharArray(audio2_device_vec[i].FriendlyName)
//                 << QString::fromWCharArray(audio2_device_vec[i].MonikerName);
//        ui->comboBox_AudioSpeaker->addItem(QString::fromWCharArray(audio2_device_vec[i].FriendlyName));
//    }
//#elif defined(Q_OS_LINUX)
//    AlsaDeviceList();
//    ui->comboBox_Audio->addItem(QString::fromStdString("default"));
//    ui->comboBox_Audio->addItem(QString::fromStdString("4")); // cat /proc/asound/devices
//    ui->comboBox_Audio->addItem(QString::fromStdString("/dev/snd/pcmC0D0c"));
//    ui->comboBox_Video->addItem(QString::fromStdString("default")); // v4l2-ctl --list-devices
//    ui->comboBox_Video->addItem(QString::fromStdString("/dev/video0"));
//#endif
    auto cis = QCameraInfo::availableCameras();
    for (auto ci : cis)
    {
        qDebug() << ci.description() << ci.deviceName() << ci.orientation() << ci.position();
        ui->comboBox_Video->addItem(ci.description(), QVariant(ci.deviceName()));

        QCamera* camera = new QCamera(ci);
//        QCameraViewfinder* viewfinder = new QCameraViewfinder();
//        camera->setViewfinder(viewfinder);

        camera->load();

        // 返回支持的取景器分辨率列表
//        for (auto resolution : camera->supportedViewfinderResolutions())
//        {
//            //dosomething about the resolution
//            qDebug() << resolution;
//        }

//        for (auto frame_rate_range : camera->supportedViewfinderFrameRateRanges())
//        {
//            qDebug() << frame_rate_range.minimumFrameRate << frame_rate_range.maximumFrameRate;
//        }

//        for (auto pixel_format : camera->supportedViewfinderPixelFormats())
//        {
//            qDebug() << pixel_format;
//        }

        for (auto vf_settings : camera->supportedViewfinderSettings())
        {
            qDebug() << vf_settings.minimumFrameRate() << vf_settings.maximumFrameRate()
                     << vf_settings.pixelAspectRatio() << vf_settings.pixelFormat()
                     << vf_settings.resolution();
        }

        camera->unload();
        delete camera;
    }

    qDebug() << "audio input devices:";
    for (auto adi : QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioInput))
    {
        qDebug() << adi.deviceName() << adi.supportedSampleRates() << adi.supportedSampleSizes()
                 << adi.supportedSampleTypes() << adi.supportedByteOrders()
                 << adi.supportedChannelCounts();
        ui->comboBox_Microphone->addItem(adi.deviceName());
    }

    qDebug() << "audio output devices:";
    for (auto adi : QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput))
    {
        qDebug() << adi.deviceName() << adi.supportedSampleRates() << adi.supportedSampleSizes()
                 << adi.supportedSampleTypes() << adi.supportedByteOrders()
                 << adi.supportedChannelCounts();
        ui->comboBox_Speaker->addItem(adi.deviceName());
    }

    if (!last_video_device_.isEmpty())
    {
        ui->comboBox_Video->setCurrentText(last_video_device_);
    }

    if (!last_microphone_.isEmpty())
    {
        ui->comboBox_Microphone->setCurrentText(last_microphone_);
    }

    if (!last_speaker_.isEmpty())
    {
        ui->comboBox_Speaker->setCurrentText(last_speaker_);
    }

    ui->checkBox_Camera->setChecked(GLOBAL->config.TestCompos(COMPOS_BIT_CAMERA));
    ui->checkBox_Microphone->setChecked(GLOBAL->config.TestCompos(COMPOS_BIT_MICROPHONE));
    ui->checkBox_Desktop->setChecked(GLOBAL->config.TestCompos(COMPOS_BIT_DESKTOP));
    ui->checkBox_Speaker->setChecked(GLOBAL->config.TestCompos(COMPOS_BIT_SPEAKER));

    if (!GLOBAL->config.GetFilePath().empty())
    {
        ui->textBrowser_FilePath->setText(QString::fromStdString(GLOBAL->config.GetFilePath()));
    }

    if (GLOBAL->config.Started())
    {
        ui->pushButton_Start->setEnabled(false);
        ui->pushButton_Stop->setEnabled(true);
    }
    else
    {
        ui->pushButton_Start->setEnabled(true);
        ui->pushButton_Stop->setEnabled(false);
    }
}

DevicesDialog::~DevicesDialog()
{
    delete ui;
}

void DevicesDialog::on_comboBox_Video_currentIndexChanged(const QString& arg1)
{
    last_video_device_ = QString::fromStdString(GLOBAL->config.GetVideoCaptureDevice());
    ui->comboBox_VideoProp->clear();

    for (auto ci : QCameraInfo::availableCameras())
    {
        if (ci.description() == arg1)
        {
            QCamera* camera = new QCamera(ci);
//            QCameraViewfinder* viewfinder = new QCameraViewfinder();
//            camera->setViewfinder(viewfinder);

            camera->load();

            for (auto vf_settings : camera->supportedViewfinderSettings())
            {
                QVideoFrame::PixelFormat pix_fmt = vf_settings.pixelFormat();
                QString pix_fmt_str;

                switch (pix_fmt)
                {
                    case QVideoFrame::Format_YUYV:
                    {
                        pix_fmt_str = "Format_YUYV";
                    }
                    break;

                    case QVideoFrame::Format_Jpeg:
                    {
                        pix_fmt_str = "Format_Jpeg";
                    }
                    break;

                    default:
                    {
                        pix_fmt_str = QString("%1").arg((int) pix_fmt);
                    }
                    break;
                }

                QString item = QString("%1x%2 %3 %4 %5").arg(vf_settings.resolution().width())
                               .arg(vf_settings.resolution().height())
                               .arg(vf_settings.minimumFrameRate()) // TODO 帧率只显示15 20 25 30这几种
                               .arg(vf_settings.maximumFrameRate())
                               .arg(pix_fmt_str);
                ui->comboBox_VideoProp->addItem(item);
            }

            camera->unload();
//            delete viewfinder;
            delete camera;
        }
    }

#if defined(Q_OS_WIN)
    GLOBAL->config.SetVideoCaptureDevice(ui->comboBox_Video->currentText().toStdString());
#elif defined(Q_OS_LINUX)
    GLOBAL->config.SetVideoCaptureDevice(ui->comboBox_Video->currentData().toString().toStdString());
#endif
}

void DevicesDialog::on_comboBox_Microphone_currentIndexChanged(const QString& arg1)
{
    last_microphone_ = QString::fromStdString(GLOBAL->config.GetMicrophone());
    GLOBAL->config.SetMicrophone(arg1.toStdString());
}

void DevicesDialog::on_comboBox_Speaker_currentIndexChanged(const QString& arg1)
{
    last_speaker_ = QString::fromStdString(GLOBAL->config.GetSpeaker());
    GLOBAL->config.SetSpeaker(arg1.toStdString());
}

void DevicesDialog::on_pushButton_Start_clicked()
{
    if (!GLOBAL->config.IsValidComposSet())
    {
        QMessageBox::warning(this, "warning", QStringLiteral("请重新选择音频&视频采集控制组合"));
        return;
    }

    if (GLOBAL->config.GetFilePath().empty())
    {
        QMessageBox::warning(this, "warning", QStringLiteral("录制路径未设置"));
        return;
    }

    close();
    emit SIGNAL_CENTER->StartStream();
}

void DevicesDialog::on_pushButton_Stop_clicked()
{
    close();
    emit SIGNAL_CENTER->StopStream();
}

void DevicesDialog::on_pushButton_Dir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("请选择文件保存路径"));
    if (!dir.isEmpty())
    {
        GLOBAL->config.SetFilePath(dir.toStdString() + "/my.flv");
        ui->textBrowser_FilePath->setText(QString::fromStdString(GLOBAL->config.GetFilePath()));
    }
}

void DevicesDialog::on_checkBox_Microphone_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetCompos(COMPOS_BIT_MICROPHONE, ui->checkBox_Microphone->isChecked());
}

void DevicesDialog::on_checkBox_Camera_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetCompos(COMPOS_BIT_CAMERA, ui->checkBox_Camera->isChecked());
}

void DevicesDialog::on_checkBox_Speaker_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetCompos(COMPOS_BIT_SPEAKER, ui->checkBox_Speaker->isChecked());
}

void DevicesDialog::on_checkBox_Desktop_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetCompos(COMPOS_BIT_DESKTOP, ui->checkBox_Desktop->isChecked());
}
