// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 CAMERAUPGRADEDLL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// CAMERAUPGRADEDLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#pragma once

#ifdef CAMERAUPGRADEDLL_EXPORTS
#define CAMERAUPGRADEDLL_API __declspec(dllexport)
#else
#define CAMERAUPGRADEDLL_API __declspec(dllimport)
#endif

#include <atlstr.h>
#include <dshow.h>
#include <Vidcap.h>
#include <KsProxy.h>
#define DATA_PACKET_UNIT		1024

#include <atlbase.h>

typedef struct
{
	UINT8 *buf;
	int len;
	//Device HANDER
	IBaseFilter	 *pVCap;
	ULONG NodeId;
	//CProgressCtrl *pPrgCtrl;
}load_info;

//#define MAX_ADDR_FIELD      10
//#define WM_MSG_UPDATE_END	WM_USER+1


// 此类是从 CameraUpgradeDll.dll 导出的
class CCameraUpgradeDll {
public:
	CCameraUpgradeDll(void);

public:
	/************************************************************************
	Usb device handle
	************************************************************************/
	IMoniker *m_pVideoMoniker[10];
	IBaseFilter	 *m_pVCapMult[10];
	ULONG	m_pNodeId[10];
    int index;

	//char *FwPath = "C:\\upgrade.bin";
	char FwPath[128];

	BOOL InitDeviceList();
	
	int RefreshUvcDevices(IMoniker ** ppMoniker, IBaseFilter ** ppVCapMult, ULONG * pNodeId);

	int RefreshDeviceList();

	BOOL SetFwPath(char * path);

	void GetFwVersion(UINT32 * fw_ver);

	void GetFwPlatform(char * fw_flatform);

	void GetDevProduct(char * product);

	void GetDevPlatform(char * platform);

	void GetDevVersion(UINT32 * dev_version);

	void GetDevManufacturer(char * manufacturer);

	ULONG GetDevPid();

	ULONG GetDevVid();

	HRESULT AutoUpdate();
	
};

//extern CAMERAUPGRADEDLL_API int nCameraUpgradeDll;

//CAMERAUPGRADEDLL_API int fnCameraUpgradeDll(void);


