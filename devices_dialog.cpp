#include "devices_dialog.h"
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QFileDialog>
#include <QMessageBox>

#ifdef __cplusplus
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#endif
#if __cplusplus
}
#endif

#include "ui_devices_dialog.h"
#include "ds_av_devices.h"
#include "mf_av_devices.h"
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

#if defined(Q_OS_WIN)
    std::vector<DeviceCtx> video_device_vec;
    std::vector<DeviceCtx> audio_device_vec;

    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
    {
        if (MFGetAVInputDevices(video_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID) != 0)
        {
            qDebug() << "no video devices";
            return;
        }

        if (MFGetAVInputDevices(audio_device_vec, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID) != 0)
        {
            qDebug() << "no audio devices";
            return;
        }
    }
    else
    {
        if (DSGetAVInputDevices(video_device_vec, CLSID_VideoInputDeviceCategory) != 0)
        {
            qDebug() << "no video devices";
            return;
        }

        if (DSGetAVInputDevices(audio_device_vec, CLSID_AudioInputDeviceCategory) != 0)
        {
            qDebug() << "no audio devices";
            return;
        }
    }

    qDebug() << "video input devices:";
    for (int i = 0; i < (int) video_device_vec.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(video_device_vec[i].FriendlyName)
                 << QString::fromWCharArray(video_device_vec[i].MonikerName);
        ui->comboBox_Video->addItem(QString::fromWCharArray(video_device_vec[i].FriendlyName));
    }

    qDebug() << "audio input devices:";
    for (int i = 0; i < (int) audio_device_vec.size(); ++i)
    {
        qDebug() << i << QString::fromWCharArray(audio_device_vec[i].FriendlyName)
                 << QString::fromWCharArray(audio_device_vec[i].MonikerName);
        ui->comboBox_Audio->addItem(QString::fromWCharArray(audio_device_vec[i].FriendlyName));
    }
#elif defined(Q_OS_LINUX)
    AlsaDeviceList();
    ui->comboBox_Audio->addItem(QString::fromStdString("default"));
    ui->comboBox_Audio->addItem(QString::fromStdString("4")); // cat /proc/asound/devices
    ui->comboBox_Audio->addItem(QString::fromStdString("/dev/snd/pcmC0D0c"));
    ui->comboBox_Video->addItem(QString::fromStdString("default")); // v4l2-ctl --list-devices
    ui->comboBox_Video->addItem(QString::fromStdString("/dev/video0"));
#endif

    if (!last_video_device_.isEmpty())
    {
        ui->comboBox_Video->setCurrentText(last_video_device_);
    }

    if (!last_audio_device_.isEmpty())
    {
        ui->comboBox_Audio->setCurrentText(last_audio_device_);
    }

    ui->checkBox_Video->setChecked(GLOBAL->config.HasVideo());
    ui->checkBox_Audio->setChecked(GLOBAL->config.HasAudio());

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
    GLOBAL->config.SetVideoCaptureDevice(arg1.toStdString());
}

void DevicesDialog::on_comboBox_Audio_currentIndexChanged(const QString& arg1)
{
    last_audio_device_ = QString::fromStdString(GLOBAL->config.GetAudioCaptureDevice());
    GLOBAL->config.SetAudioCaptureDevice(arg1.toStdString());
}

void DevicesDialog::on_pushButton_Start_clicked()
{
    if (!ui->checkBox_Audio->isChecked() && !ui->checkBox_Video->isChecked())
    {
        QMessageBox::warning(this, "warning", QStringLiteral("音频和视频至少要有一个"));
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

void DevicesDialog::on_checkBox_Audio_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetHasAudio(ui->checkBox_Audio->isChecked());
}

void DevicesDialog::on_checkBox_Video_stateChanged(int arg1)
{
    (void) arg1;
    GLOBAL->config.SetHasVideo(ui->checkBox_Video->isChecked());
}
