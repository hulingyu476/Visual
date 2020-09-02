// ConsoleTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

int main()
{
	static int count;

	HMODULE module = LoadLibrary(_T("CameraUpgradeDll.dll"));//����CameraUpgradeDll.dll�ļ�
	if (module == NULL)
	{
		printf("load CameraUpgradeDll.dll fail\n");

		while (1);
	}
	printf("load CameraUpgradeDll.dll ok\n");
	

	int updateProgress = 0;
	int updateProgress_bak = 0;
	int updateStatus = 0;
	
	//�����豸
	typedef int(*openDevice)(void); // ���庯��ָ������   �򿪴���
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

	//��ȡVID PID
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
	

	//��ȡ�豸�ϵ�����汾��
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



	//���������ļ�
	char * FwPath = "C:\\newline.bin";
	typedef int(*openFilePath)(char*); // ���庯��ָ������   ���ļ�
	openFilePath openUpdateFile;
	openUpdateFile = (openFilePath)GetProcAddress(module, "openUpdateFile");
	openUpdateFile(FwPath);

	//�鿴��������
	typedef int(*getUpdateProgress)(void); // ���庯��ָ������  �������Ȳ鿴
	getUpdateProgress getUpdatingProgress;
	getUpdatingProgress = (getUpdateProgress)GetProcAddress(module, "getUpdatingProgress");

	typedef int(*getUpdateSta)(void); // ���庯��ָ������       �鿴����״̬
	getUpdateSta getUpdateStatus;
	getUpdateStatus = (getUpdateSta)GetProcAddress(module, "getUpdateStatus");



	//��ȡ����汾��
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

	//��ʱʹ�ã���Ҫ����̼߳�������
	/*
	while ((updateStatus = getUpdateStatus()) == 0)//����״̬��ѯ  0��null    1:���ļ�ʧ��   2������ָ��У��ʧ��  3��д����ʧ�� 4���ɹ� 5���Ҳ����豸
	{
		updateProgress = getUpdatingProgress();//�������� 0~100
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

