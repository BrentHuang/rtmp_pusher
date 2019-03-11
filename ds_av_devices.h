#ifndef DS_AV_DEVICES_H
#define DS_AV_DEVICES_H

#include <qsystemdetection.h>

#if defined(Q_OS_WIN)
#include <vector>
#include <Windows.h>
#include <dshow.h>

#ifndef MACRO_GROUP_DEVICENAME
#define MACRO_GROUP_DEVICENAME

#define MAX_FRIENDLY_NAME_LENGTH    128
#define MAX_MONIKER_NAME_LENGTH     256

struct DeviceCtx
{
    WCHAR FriendlyName[MAX_FRIENDLY_NAME_LENGTH];   // 设备友好名
    WCHAR MonikerName[MAX_MONIKER_NAME_LENGTH];     // 设备Moniker名
    // TODO 摄像头支持的分辨率列表，帧率，pixel format；麦克风的采样率，采用精度，声道数
};
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*
功能：获取音频视频输入设备列表
参数说明：
    vectorDevices：用于存储返回的设备友好名及Moniker名
    guidValue：
        CLSID_AudioInputDeviceCategory：获取音频输入设备列表
        CLSID_VideoInputDeviceCategory：获取视频输入设备列表
返回值：
    错误代码 =0表示成功，否则失败
说明：
    基于DirectShow
    列表中的第一个设备为系统缺省设备
    capGetDriverDescription只能获得设备驱动名
*/
int DSGetAVInputDevices(std::vector<DeviceCtx>& device_ctx_vec, REFGUID guid_value);

#ifdef __cplusplus
}
#endif

#endif
#endif // DS_AV_DEVICES_H
