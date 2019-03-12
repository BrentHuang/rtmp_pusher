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

    void SetHasVideo(bool flag);
    bool HasVideo();

    void SetAudioCaptureDevice(const std::string& device_name);
    std::string GetAudioCaptureDevice();

    void SetHasAudio(bool flag);
    bool HasAudio();

    void SetFilePath(const std::string& file_path);
    std::string GetFilePath();

    void SetStarted(bool flag);
    bool Started();

private:
    QMutex mutex_;
    std::string video_device_;
    std::string audio_device_;
    bool has_video_;
    bool has_audio_;
    std::string file_path_;
    bool started_;
};

#endif // CONFIG_H
