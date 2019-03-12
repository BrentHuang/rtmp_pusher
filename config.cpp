#include "config.h"
#include <QMutexLocker>

Config::Config() : mutex_(), video_device_(), audio_device_()
{
    has_video_ = true;
    has_audio_ = true;
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

void Config::SetHasVideo(bool flag)
{
    QMutexLocker lock(&mutex_);
    has_video_ = flag;
}

bool Config::HasVideo()
{
    QMutexLocker lock(&mutex_);
    return has_video_;
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

void Config::SetHasAudio(bool flag)
{
    QMutexLocker lock(&mutex_);
    has_audio_ = flag;
}

bool Config::HasAudio()
{
    QMutexLocker lock(&mutex_);
    return has_audio_;
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
