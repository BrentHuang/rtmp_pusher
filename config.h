#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <bitset>
#include <vector>
#include <QMutex>

enum
{
    COMPOS_BIT_MIN = 0,
    COMPOS_BIT_SYSTEM_VOICE = 0,
    COMPOS_BIT_DESKTOP = 1,
    COMPOS_BIT_CAMERA = 2,
    COMPOS_BIT_MICROPHONE = 3,
    COMPOS_BIT_MAX
};

class Config
{
public:
    Config();
    ~Config() {}

    void SetVideoCaptureDevice(const std::string& device_name);
    std::string GetVideoCaptureDevice();

    void SetAudioCaptureDevice(const std::string& device_name);
    std::string GetAudioCaptureDevice();

    void SetAudio2CaptureDevice(const std::string& device_name);
    std::string GetAudio2CaptureDevice();

    bool IsValidComposSet();
    void SetCompos(int bit, bool on);
    bool TestCompos(int bit);

    void SetFilePath(const std::string& file_path);
    std::string GetFilePath();

    void SetStarted(bool flag);
    bool Started();

private:
    QMutex mutex_;

    std::string video_device_;
    std::string audio_device_;
    std::string audio2_device_;

    // 麦克风、摄像头、系统桌面，系统声音的合法组合，用4个bit位表示
    typedef std::vector<std::bitset<COMPOS_BIT_MAX>> ComposVec;
    ComposVec compos_vec_;

    std::bitset<COMPOS_BIT_MAX> compos_;

    std::string file_path_;
    bool started_;
};

#endif // CONFIG_H
