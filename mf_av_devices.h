#ifndef MF_AV_DEVICES_H
#define MF_AV_DEVICES_H

#include <qsystemdetection.h>

#if defined(Q_OS_WIN)
#include <vector>
#include <Windows.h>
#include <mfidl.h>

#ifndef MACRO_GROUP_DEVICENAME
#define MACRO_GROUP_DEVICENAME

#define MAX_FRIENDLY_NAME_LENGTH    128
#define MAX_MONIKER_NAME_LENGTH     256

struct DeviceCtx
{
    WCHAR FriendlyName[MAX_FRIENDLY_NAME_LENGTH];   // 设备友好名
    WCHAR MonikerName[MAX_MONIKER_NAME_LENGTH];     // 设备Moniker名
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
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID：获取音频输入设备列表
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID：获取视频输入设备列表
返回值：
    错误代码，=0表示成功，否则失败
说明：
    基于Media Foundation
    列表中的第一个设备为系统缺省设备
    capGetDriverDescription只能获得设备驱动名
    操作系统要求Windows 7及以上版本（关键）
*/
int MFGetAVDevices(std::vector<DeviceCtx>& device_ctx_vec, REFGUID guid_value);

#ifdef __cplusplus
}
#endif

#endif
#endif // MF_AV_DEVICES_H
