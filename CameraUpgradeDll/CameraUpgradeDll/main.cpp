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
extern "C" _declspec(dllexport)int openDevice()//0: 成功   1：失败  ，要再重新打开
{
	if (cu.InitDeviceList())
		return 0;
	return 1;
}
extern "C" _declspec(dllexport) int openUpdateFile(char *filePath)//0:正确 (.bin后缀)  1:文件格式错误     宽字符
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
extern "C" _declspec(dllexport)int getUpdatingProgress()//  返回进度：0~100
{
	return updatingProgress;
}
extern "C" _declspec(dllexport)  int getUpdateStatus()//  //升级状态查询  0：null    1:打开文件失败   2：升级指令校验失败  3：写数据失败 4：成功
{
	return updateStatus;
}