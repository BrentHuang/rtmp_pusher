#include "MF_AudioVideoDevices.h"
#include <mfapi.h>

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mf.lib")

HRESULT MF_GetAudioVideoInputDevices( std::vector<TDeviceName>& vectorDevices, REFGUID guidValue )
{
    TDeviceName name;
    HRESULT hr;

    // 初始化
    vectorDevices.clear();

    // 初始化Media Foundation
    hr = MFStartup( MF_VERSION, MFSTARTUP_LITE );
    if (SUCCEEDED(hr))
    {
        // 创建属性搜索页
        IMFAttributes* pAttributes = NULL;
        hr = MFCreateAttributes( &pAttributes, 1 ); // 要求Windows Vista
        if (SUCCEEDED(hr))
        {
            // 设置搜索关键字-枚举音频视频设备
            hr = pAttributes->SetGUID( MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, guidValue );
            if (SUCCEEDED(hr))
            {
                // 获取搜索结果
                IMFActivate** ppDevices = NULL;
                UINT32 dwCount = 0;

                hr = MFEnumDeviceSources( pAttributes, &ppDevices, &dwCount );  // 要求Windows 7
                if (SUCCEEDED(hr))
                {
                    if (dwCount == 0)
                    {
                        // 没有找到
                        hr = E_FAIL;
                    }

                    for (DWORD i = 0; i < dwCount; i++)
                    {
                        // 获取设备友好名
                        ppDevices[i]->GetString( MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name.FriendlyName, MAX_FRIENDLY_NAME_LENGTH, NULL );

                        // 获取设备Moniker名
                        ppDevices[i]->GetString( MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, name.MonikerName, MAX_MONIKER_NAME_LENGTH, NULL );

                        // 加入列表
                        vectorDevices.push_back( name );

                        // 释放资源
                        ppDevices[i]->Release();
                    }

                    // 释放内存
                    CoTaskMemFree( ppDevices );
                }
            }

            pAttributes->Release();
        }

        // 关闭Media Foundation
        MFShutdown();
    }

    return hr;
}
