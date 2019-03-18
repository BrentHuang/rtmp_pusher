#include "config.h"
#include <QDebug>
#include <QMutexLocker>
#include <qsystemdetection.h>

Config::Config() : mutex_(), video_device_(), microphone_(),
    speaker_(), compos_vec_(), compos_(), file_path_()
{
    std::bitset<COMPOS_BIT_MAX> compos;
    compos.set();
    compos.reset(COMPOS_BIT_SPEAKER);
    compos.reset(COMPOS_BIT_DESKTOP);
//    qDebug() << QString::fromStdString(compos.to_string()); // 1100
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_CAMERA);
//    qDebug() << QString::fromStdString(compos.to_string()); // 1011
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_SPEAKER);
    compos.reset(COMPOS_BIT_DESKTOP);
    compos.reset(COMPOS_BIT_CAMERA);
//    qDebug() << QString::fromStdString(compos.to_string()); // 1000
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_SPEAKER);
    compos.reset(COMPOS_BIT_DESKTOP);
    compos.reset(COMPOS_BIT_MICROPHONE);
//    qDebug() << QString::fromStdString(compos.to_string()); // 0100
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_CAMERA);
    compos.reset(COMPOS_BIT_MICROPHONE);
//    qDebug() << QString::fromStdString(compos.to_string()); // 0011
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_CAMERA);
    compos.reset(COMPOS_BIT_MICROPHONE);
    compos.reset(COMPOS_BIT_SPEAKER);
//    qDebug() << QString::fromStdString(compos.to_string()); // 0010
    compos_vec_.push_back(compos);

    compos.set();
    compos.reset(COMPOS_BIT_CAMERA);
    compos.reset(COMPOS_BIT_SPEAKER);
//    qDebug() << QString::fromStdString(compos.to_string()); // 1010
    compos_vec_.push_back(compos);

    compos_.reset();
    compos_.set(COMPOS_BIT_CAMERA, true);
    compos_.set(COMPOS_BIT_MICROPHONE, true);

#if defined(Q_OS_WIN)
    file_path_ = "D:/my.flv";
#elif defined(Q_OS_LINUX)
    file_path_ = "~/my.flv";
#endif

    started_ = false;
}

void Config::SetVideoCaptureDevice(const std::string& device_name)
{
    QMutexLocker lock(&mutex_);
    video_device_ = device_name;
}

std::string Config::GetVideoCaptureDevice()
{
    QMutexLocker lock(&mutex_);
    return video_device_;
}

void Config::SetMicrophone(const std::string& device_name)
{
    QMutexLocker lock(&mutex_);
    microphone_ = device_name;
}

std::string Config::GetMicrophone()
{
    QMutexLocker lock(&mutex_);
    return microphone_;
}

void Config::SetSpeaker(const std::string& device_name)
{
    QMutexLocker lock(&mutex_);
    speaker_ = device_name;
}

std::string Config::GetSpeaker()
{
    QMutexLocker lock(&mutex_);
    return speaker_;
}

bool Config::IsValidComposSet()
{
    mutex_.lock();
    const std::bitset<COMPOS_BIT_MAX> compos = compos_;
    mutex_.unlock();

    bool ok = false;

    for (auto c : compos_vec_)
    {
        if (compos == c)
        {
            ok = true;
            break;
        }
    }

    return ok;
}

void Config::SetCompos(int bit, bool on)
{
    if (bit < COMPOS_BIT_MIN || bit >= COMPOS_BIT_MAX)
    {
        return;
    }

    QMutexLocker lock(&mutex_);
    compos_.set(bit, on);
}

bool Config::TestCompos(int bit)
{
    if (bit < COMPOS_BIT_MIN || bit >= COMPOS_BIT_MAX)
    {
        return false;
    }

    QMutexLocker lock(&mutex_);
    return compos_.test(bit);
}

void Config::SetFilePath(const std::string& file_path)
{
    QMutexLocker lock(&mutex_);
    file_path_ = file_path;
}

std::string Config::GetFilePath()
{
    QMutexLocker lock(&mutex_);
    return file_path_;
}

void Config::SetStarted(bool flag)
{
    QMutexLocker lock(&mutex_);
    started_ = flag;
}

bool Config::Started()
{
    QMutexLocker lock(&mutex_);
    return started_;
}
