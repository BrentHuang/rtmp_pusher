#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <QMutex>

class Config
{
public:
    Config();
    ~Config() {}

    void SetVideoCaptureDevice(const std::string& device_name);
    std::string GetVideoCaptureDevice();

    void SetAudioCaptureDevice(const std::string& device_name);
    std::string GetAudioCaptureDevice();

    void SetStarted(bool flag);
    bool Started();

private:
    QMutex mutex_;
    std::string video_device_;
    std::string audio_device_;
    bool started_;
};

#endif // CONFIG_H
