// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� CAMERAUPGRADEDLL_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// CAMERAUPGRADEDLL_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�

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


// �����Ǵ� CameraUpgradeDll.dll ������
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


