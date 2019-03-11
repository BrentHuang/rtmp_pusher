#include "config.h"
#include <QMutexLocker>

Config::Config() : video_device_(), audio_device_()
{
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

void Config::SetAudioCaptureDevice(const std::string& device_name)
{
    QMutexLocker lock(&mutex_);
    audio_device_ = device_name;
}

std::string Config::GetAudioCaptureDevice()
{
    QMutexLocker lock(&mutex_);
    return audio_device_;
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
