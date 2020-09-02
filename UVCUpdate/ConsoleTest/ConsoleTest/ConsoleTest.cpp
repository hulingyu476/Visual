// ConsoleTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

int main()
{
	static int count;

	HMODULE module = LoadLibrary(_T("CameraUpgradeDll.dll"));//加载CameraUpgradeDll.dll文件
	if (module == NULL)
	{
		printf("load CameraUpgradeDll.dll fail\n");

		while (1);
	}
	printf("load CameraUpgradeDll.dll ok\n");
	

	int updateProgress = 0;
	int updateProgress_bak = 0;
	int updateStatus = 0;
	
	//查找设备
	typedef int(*openDevice)(void); // 定义函数指针类型   打开串口
	openDevice openDevicePort;
	openDevicePort = (openDevice)GetProcAddress(module, "openDevice");
	if (openDevicePort())
	{
		printf("cannot find devices\n");
	}
	else
	{
		printf(" find devices ok\n");
	}

	//获取VID PID
	typedef ULONG(*GetDevPid)(void);
	GetDevPid GetDevPidInfo;
	GetDevPidInfo = (GetDevPid)GetProcAddress(module, "CGetPid");
		
	typedef ULONG(*GetDevVid)(void);
	GetDevVid GetDevVidInfo;
	GetDevVidInfo = (GetDevVid)GetProcAddress(module, "CGetVid");
		
	ULONG pid =0;
	ULONG vid =0;
	pid = GetDevPidInfo();
	vid = GetDevVidInfo();
	printf("getDevVid = 0x%lx   getDevPid : 0x%lx\n", vid, pid);
	

	//获取设备上的软件版本号
	typedef UINT32(*GetDevVersion)(void*);
	GetDevVersion GetDevVersionInfo;
	GetDevVersionInfo = (GetDevVersion)GetProcAddress(module, "CGetDevVersion");

	UINT32 devversion = 0;
	char DevVersionInfo[100];
	devversion = GetDevVersionInfo(DevVersionInfo);

	//printf("devversion = %lx \n", devversion);
	printf("devversion = %d.%d.%d \n", devversion >> 8, (devversion >> 4) & 0x0f, devversion & 0x0f);
	printf("DevVersionInfo = %s \n", DevVersionInfo);



	//get Platform
	typedef ULONG(*GetDevPlatform)(char*);
	GetDevPlatform GetDevPlatformInfo;
	GetDevPlatformInfo = (GetDevPlatform)GetProcAddress(module, "CGetDevFlatform");

	char DevPlatformInfo[100] ;
	GetDevPlatformInfo(DevPlatformInfo);
	printf("DevPlatform = %s \n", DevPlatformInfo);



	//get Product
	typedef ULONG(*GetDevProduct)(char*);
	GetDevProduct GetDevProductInfo;
	GetDevProductInfo = (GetDevProduct)GetProcAddress(module, "CGetDevProduct");

	char DevProductInfo[100];
	GetDevProductInfo(DevProductInfo);
	printf("DevProduct = %s \n", DevProductInfo);


	//get Manufacturer
	typedef ULONG(*GetDevManufacturer)(char*);
	GetDevManufacturer GetDevManufacturerInfo;
	GetDevManufacturerInfo = (GetDevManufacturer)GetProcAddress(module, "CGetDevManufacturer");

	char DevManufacturerInfo[100];
	GetDevManufacturerInfo(DevManufacturerInfo);
	printf("DevManufacturer = %s \n", DevManufacturerInfo);



	//设置升级文件
	char * FwPath = "C:\\newline.bin";
	typedef int(*openFilePath)(char*); // 定义函数指针类型   打开文件
	openFilePath openUpdateFile;
	openUpdateFile = (openFilePath)GetProcAddress(module, "openUpdateFile");
	openUpdateFile(FwPath);

	//查看升级进度
	typedef int(*getUpdateProgress)(void); // 定义函数指针类型  升级进度查看
	getUpdateProgress getUpdatingProgress;
	getUpdatingProgress = (getUpdateProgress)GetProcAddress(module, "getUpdatingProgress");

	typedef int(*getUpdateSta)(void); // 定义函数指针类型       查看升级状态
	getUpdateSta getUpdateStatus;
	getUpdateStatus = (getUpdateSta)GetProcAddress(module, "getUpdateStatus");



	//获取软件版本号
	typedef UINT32(*GetFwVersion)(void*);
	GetFwVersion GetFwVersionInfo;
	GetFwVersionInfo = (GetFwVersion)GetProcAddress(module, "CGetFwVersion");

	char FwVersionInfo[100];
	UINT32 fwversion = 0;
	fwversion = GetFwVersionInfo(FwVersionInfo);
	printf("fwversion = %d.%d.%d \n", fwversion >> 8, (fwversion >> 4) & 0x0f, fwversion & 0x0f);
	printf("FwVersionInfo = %s \n", FwVersionInfo);



	//get Fw Platform
	typedef ULONG(*GetFwPlatform)(char*);
	GetFwPlatform GetFwPlatformInfo;
	GetFwPlatformInfo = (GetFwPlatform)GetProcAddress(module, "CGetFwPlatform");

	char FwPlatformInfo[100];
	GetFwPlatformInfo(FwPlatformInfo);
	printf("FwPlatform = %s \n", FwPlatformInfo);


	typedef HRESULT(*AutoUpdate)(void);
	AutoUpdate AutoUpdateThread;
	AutoUpdateThread = (AutoUpdate)GetProcAddress(module, "AutoUpdate");
	AutoUpdateThread();

	//临时使用，需要添加线程检测进度条
	/*
	while ((updateStatus = getUpdateStatus()) == 0)//升级状态查询  0：null    1:打开文件失败   2：升级指令校验失败  3：写数据失败 4：成功 5：找不到设备
	{
		updateProgress = getUpdatingProgress();//升级进度 0~100
		updateStatus = getUpdateStatus();//temp
		if (updateProgress != updateProgress_bak)
		{
			updateProgress_bak = updateProgress;
			printf("updating!!!!!!>>>>>updateProgress = %d  updateStatus = %d\n", updateProgress, updateStatus);

		}

	}
	if (updateStatus == 4)
	{
		printf("finishi>>>>>>>>>success!!!\n");
	}
	else
	{
		printf("finishi>>>>>>>>fail!!! updateStatus = %d    Progress = %d    \n", updateStatus, updateProgress);
	}
	*/
	//while ((updateStatus = getUpdateStatus()) == 0) 

   /*while (0)
	{
		updateStatus = getUpdateStatus();//temp
		if (updateStatus == 4) {
			if (openDevicePort())
			{
				printf("cannot find devices\n");
			}
			else
			{
				devversion = GetDevVersionInfo(DevVersionInfo);
				printf("DevVersionInfo = %s \n", DevVersionInfo);

				printf(" find devices ok then exit app\n");
				
			}
		}
	}*/

	printf("flow last\n");

	while(1) {
		count+=1;
		//printf("last count=%d\n", count);
		Sleep(100);
	}
    return 0;
}

