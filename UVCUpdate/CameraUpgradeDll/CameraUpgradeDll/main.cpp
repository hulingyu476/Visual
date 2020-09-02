#include "stdafx.h"
#include "CameraUpgradeDll.h"

CCameraUpgradeDll cu;

extern UINT updatingProgress;
extern int updateStatus;

extern "C" __declspec(dllexport) ULONG CGetPid()
{
	return cu.GetDevPid();
}

extern "C" __declspec(dllexport) ULONG CGetVid()
{
	return cu.GetDevVid();
}

extern "C" __declspec(dllexport) UINT32 CGetDevVersion(char* dev_version)
{
	UINT32 version;
	cu.GetDevVersion(&version);
	sprintf_s(dev_version, 100,"V%d.%d.%d", version >> 8, (version >> 4) & 0x0f, version & 0x0f);

	return version;
}
extern "C" __declspec(dllexport) ULONG CGetDevFlatform(char* dev_platform)
{
	cu.GetDevPlatform(dev_platform);
	//CString ComName = CString(dev_platform);
	//_ftprintf_s(stdout, _T("dev_platform %s \r\n"), ComName);
	return 0;
}
extern "C" __declspec(dllexport) ULONG CGetDevManufacturer(char* dev_manufacturer)
{
	cu.GetDevManufacturer(dev_manufacturer);
	//CString ComName = CString(dev_manufacturer);
	//_ftprintf_s(stdout, _T("dev_manufacturer %s \r\n"), ComName);
	return 0;
}
extern "C" __declspec(dllexport) ULONG CGetDevProduct(char* dev_product)
{
	cu.GetDevProduct(dev_product);
	//CString ComName = CString(dev_product);
	//_ftprintf_s(stdout, _T("dev_product %s \r\n"), ComName);
	return 0;
}

extern "C" __declspec(dllexport) UINT32 CGetFwVersion(char *fw_version)
{
	UINT32 version;
	cu.GetFwVersion(&version);
	sprintf_s(fw_version, 100, "V%d.%d.%d", version >> 8, (version >> 4) & 0x0f, version & 0x0f);

	return version;
}
extern "C" __declspec(dllexport) UINT32 CGetFwPlatform(char* fw_flatform)
{
	cu.GetFwPlatform(fw_flatform);
	//CString ComName = CString(fw_flatform);
	//_ftprintf_s(stdout, _T("fw_flatform %s \r\n"), ComName);
	return 0;
}

extern "C" __declspec(dllexport) HRESULT AutoUpdate()
{
	cu.AutoUpdate();
	return 0;
}
extern "C" _declspec(dllexport)int openDevice()//0: �ɹ�   1��ʧ��  ��Ҫ�����´�
{
	if (cu.InitDeviceList())
		return 0;
	return 1;
}
extern "C" _declspec(dllexport) int openUpdateFile(char *filePath)//0:��ȷ (.bin��׺)  1:�ļ���ʽ����     ���ַ�
{
	CString strTepm = CString(filePath);
	if (strTepm.Right(4).MakeLower() != _T(".bin"))
	{
		return 1;
	}
	cu.SetFwPath(filePath);
	//CString strTepm = CString(cu.FwPath);
	//_ftprintf_s(stdout, _T("SetFwPath = %s \r\n"), strTepm);
	return 0;
}
extern "C" _declspec(dllexport)  int CDevReboot()//  DevReboot
{
	return cu.DevReboot();
}
extern "C" _declspec(dllexport)int getUpdatingProgress()//  ���ؽ��ȣ�0~100
{
	return updatingProgress;
}
extern "C" _declspec(dllexport)  int getUpdateStatus()//  //����״̬��ѯ  0��null    1:���ļ�ʧ��   2������ָ��У��ʧ��  3��д����ʧ�� 4���ɹ�
{
	return updateStatus;
}