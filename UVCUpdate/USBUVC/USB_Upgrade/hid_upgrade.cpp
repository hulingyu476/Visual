#include "stdafx.h"
#include "config.h"
#include "hid_upgrade.h"
#include "Hid.h"
#include "USB_UpgradeDlg.h"
#include <stdlib.h>
#include <windows.h>
#include <process.h> 
#include <afxwin.h>
#include<regex>

using namespace std;

LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
LARGE_INTEGER Frequency;

#define QUERY_EVENT_COUNT			(2)

HANDLE	QueryEvent[QUERY_EVENT_COUNT];
CWinThread* pQueryThread;

extern USHORT vid1, pid1;

int CHid_Upgrade::Hid_UpgradeInit(void *pParam)
{
	int i;

	for (i = 0; i < QUERY_EVENT_COUNT; i++)
	{
		QueryEvent[i] = CreateEvent(
			NULL,   // default security attributes
			FALSE,  // auto-reset event object
			FALSE,  // initial state is nonsignaled
			NULL);  // unnamed object

		if (QueryEvent[i] == NULL)
		{
			//printf("CreateEvent error: %d\n", GetLastError() ); 
			ExitProcess(0);
		}
	}

	pQueryThread = AfxBeginThread(QueryThread, pParam);

	CameraInfoInit();
	return 0;
}

//初始化参数
void CHid_Upgrade::CameraInfoInit(void)
{
	CameraInfo.MyDevFound = 0;
	CameraInfo.HIDUpgradeFirmwareID = 0;
	CameraInfo.UpgradeFlag = 0;
	CameraInfo.AutoUpdateFlag = 0;
	CameraInfo.FPGAVersion = { 0 };
	CameraInfo.SOCVersion = { 0 };
	CameraInfo.AFVersion = { 0 };
	CameraInfo.HWVersion = { 0 };
	CameraInfo.SocHidVersion = { 0 };
	CameraInfo.USBVersion.Mode = 0;
	CameraInfo.USBVersion.BigVersion = 0;
	CameraInfo.USBVersion.LittleVersion = 0;
	CameraInfo.USBVersion.CustomCode = ' ';
	CameraInfo.StringDeviceName = "";
	CameraInfo.StringUSBVersion = "";
	CameraInfo.StringFPGAVersion = "";
	CameraInfo.StringSOCVersion = "";
	CameraInfo.StringPackageVersion = "";
	CameraInfo.StringAFVersion = "";
	CameraInfo.StringHWVersion = "";

	Language = LANGUAGE_SIMPLIFIED_CHINESE;
		
}

//搜索文件
CString CHid_Upgrade::BrowPathFiles(CString path_ext_name)
{
	CString files_name = "";
	//CString path_ext_name = "";// [MAX_PATH];
	//path_ext_name = path_name + filterExt;

	//先拼接得到带扩展名的路径名path_ext_name字符串
	//strcpy(path_ext_name, path_name);
	//strcat(path_ext_name, filterExt);

	WIN32_FIND_DATA FileData;
	HANDLE    hSearch;
	int        nCount = 0;
	BOOL    fFinished = FALSE;

	// Start searching for path_ext_name files in the current directory.
	hSearch = FindFirstFile(path_ext_name, &FileData); //先搜索本路径下的第一个文件   
	if (hSearch != INVALID_HANDLE_VALUE)
	{
		//strcpy(files[nCount], FileData.cFileName);
		nCount++;                                    //添加第一个文件

		while (FindNextFile(hSearch, &FileData))
		{
			//strcpy(files[nCount], FileData.cFileName);
			nCount++;
		}
		if (GetLastError() != ERROR_NO_MORE_FILES)
		{
			//printf("\nCouldn't find next file!\n");
			nCount = 0;

		}
		// Close the search handle.
		if (!FindClose(hSearch))
		{
			//printf("\nCouldn't close search handle!\n");
			nCount = 0;
		}
	}
	else//否则没找到本路径下的第一个文件,即没找到任何文件
	{
		//printf("\nNo file found!\n");
		nCount = 0;
	}

	//文件夹下只允许有一个升级文件
	if (nCount == 1)
	{
		files_name = FileData.cFileName;
		//printf("%s\n",files_name);
	}

	return files_name;
}



BOOL CHid_Upgrade::DataComparison(unsigned char *buff1, unsigned char *buff2, unsigned long int  size)
{
	unsigned long int  i;

	if (size > m_MyHidDevice.HidOutputReportByteLength)
		size = m_MyHidDevice.HidOutputReportByteLength;

	for (i = 0; i < size; i++)
	{
		if (buff1[i] != buff2[i])
		{
			return	FALSE;
		}

	}
	return	TRUE;
}

BOOL CHid_Upgrade::HidSend(unsigned char *TxBuffer, unsigned char *RxBuffer, unsigned char PackSize)
{
	unsigned char error_cnt = 0;
	unsigned char i = 0;

	for (i = 0; i < 10; i++)
	{
		//写操作    
		if (m_MyHidDevice.WriteHid(TxBuffer, PackSize) == FALSE)
		{
			error_cnt++;
			Sleep(100);
		}

		//读操作
		else
		{
			if (!IsBootLoader())
			{
				if (m_MyHidDevice.ReadHid(RxBuffer, PackSize) == FALSE)
				{
					error_cnt++;
					Sleep(100);
				}
				//数据对比
				else if (DataComparison(TxBuffer, RxBuffer, PackSize) == FALSE)
				{
					error_cnt++;
					Sleep(100);
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}

		}
	}

	if (error_cnt >= 10)
	{
		//m_MyHidDevice.CloseHidDevice();
		//MessageBox(_T(String.HidSend_Error), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
		Sleep(2000);
		//FindDevice();
		return FALSE;
	}

	return	TRUE;
}

int CHid_Upgrade::FpgaBinCheckFileCrc(BYTE * FileBuff, DWORD FileSize)
{
	struct fx3_img_crc *file_crc;

	int file_crc_check = 0;

	file_crc = (struct fx3_img_crc *)((BYTE *)FileBuff + FileSize - sizeof(struct fx3_img_crc));
	if (file_crc->crc_flag == 0x10435243)
	{
		if (file_crc->crc_sum == cyg_crc16(FileBuff, file_crc->crc_len))
		{
			file_crc_check = 16;
			//MessageBox(_T("CRC File"),_T((String.Change ? "警告" : "Warning")),MB_ICONEXCLAMATION);
		}
		else
		{
			return -1;
		}
	}

	return file_crc_check;
}


int CHid_Upgrade::Fx3ImgChecksum(BYTE * FileBuff, DWORD FileSize)
{
	struct fx3_img_head img_head;
	struct fx3_img_end img_end;
	DWORD dCheckSum = 0;
	DWORD dLength, dAddress, i, dataLength;
	DWORD *dImageBuf;
	struct fx3_img_crc *img_crc;

	printf("Fx3ImgChecksum\n");

	int file_crc_check = 0;

	if (FileSize < sizeof(img_head))
	{
		//printf("error file size = %d\n", FileSize);
		return -1;
	}

	memcpy((BYTE *)&img_head, (BYTE *)FileBuff, sizeof(img_head));

	if (img_head.wSignature != 0x5943)
	{
		//printf("error wSignature = 0x%4x\n", img_head.wSignature);
		return -1;
	}

	//带校验和的普通 FW 二进制镜像 
	if (img_head.bImageType != 0xB0)
	{
		//printf("error bImageType = 0x%4x\n", img_head.bImageType);
		return -1;
	}

	dImageBuf = (DWORD *)(FileBuff + sizeof(img_head));

	dCheckSum = 0;
	dataLength = sizeof(img_head);
	while (1)
	{
		dLength = dImageBuf[0];
		printf("dLength = 0x%8x\n", dLength);
		dAddress = dImageBuf[1];
		if (dLength == 0) break;
		dImageBuf += 2;
		for (i = 0; i < dLength; i++) dCheckSum += dImageBuf[i];
		dImageBuf += dLength;
		dataLength += ((dLength + 2) << 2);
		if (dataLength > FileSize)
		{
			//printf("error file size = %d\n", FileSize);
			return -1;
		}
	}

	if (dataLength > FileSize - sizeof(img_end))
	{
		//printf("error file size = %d, dataLength = %d\n", FileSize, dataLength);
		return -1;
	}
	else if (dataLength < FileSize - sizeof(img_end))
	{
		img_crc = (struct fx3_img_crc *)((BYTE *)FileBuff + FileSize - sizeof(struct fx3_img_crc));
		if (img_crc->crc_flag == 0x10435243)
		{
			if (img_crc->crc_sum == cyg_crc16(FileBuff, img_crc->crc_len))
			{
				file_crc_check = 16;
				//MessageBox(_T("CRC File"),_T((String.Change ? "警告" : "Warning")),MB_ICONEXCLAMATION);
			}
			else
			{
				return -1;
			}
		}
	}

	memcpy((BYTE *)&img_end, (BYTE *)dImageBuf, sizeof(img_end));

	if (dCheckSum != img_end.checksum)
	{
		//printf("Fail to boot due to checksum error, dCheckSum = 0x%8x, dExpectedCheckSum = 0x%8x\n", dCheckSum, img_end.checksum);
		return -1;
	}

	if (img_end.val != 0)
	{
		printf("error img_end.val = 0x%8x\n", img_end.val);
		return -1;
	}

	if (img_end.entryAddr >= 0x40078000)
	{
		//return file_crc_check + 1;//0x40078000 Bootloader
	}


	return file_crc_check;
}

int CHid_Upgrade::VersionString2Number(const BYTE *VersionString, const char *StringHeader, version_t *Version)
{
	version_t VersionTemp;
	DWORD StrIndex = 0;

	//if (!CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	//{
	//	printf("Version error!\n");
	//	return -1;
	//}

#ifdef USB_UPGRADE_DEBUG
	printf("VersionString: %s\n", VersionString);
#endif

	StrIndex += strlen(StringHeader);

	if (VersionString[StrIndex] == ':')
		StrIndex++;

	if (VersionString[StrIndex] == 'V' || VersionString[StrIndex] == 'v')
		StrIndex++;

	//Mode
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
		VersionTemp.Mode = CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
	{
		VersionTemp.Mode = (VersionTemp.Mode * 10) + CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	}

	if (VersionString[StrIndex] == '.')
	{
		StrIndex++;

	}
	else
	{
		printf("Version error!\n");
		return -1;
	}

	//BigVersion
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
		VersionTemp.BigVersion = CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
	{
		VersionTemp.BigVersion = (VersionTemp.BigVersion * 10) + CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	}

	if (VersionString[StrIndex] == '.')
	{
		StrIndex++;

	}
	else
	{
		printf("Version error!\n");
		return -1;
	}

	//LittleVersion
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
		VersionTemp.LittleVersion = CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString[StrIndex]))
	{
		VersionTemp.LittleVersion = (VersionTemp.LittleVersion * 10) + CHAR_TO_DEC_NUMBER(VersionString[StrIndex++]);
	}

	//CustomCode
	if ((VersionString[StrIndex] >= 'A' && VersionString[StrIndex] <= 'Z') ||
		(VersionString[StrIndex] >= 'a' && VersionString[StrIndex] <= 'z')
		)
	{
		VersionTemp.CustomCode = VersionString[StrIndex];//必须为字母字符
	}
	else
	{
		VersionTemp.CustomCode = ' ';
	}

	if (StrIndex > strlen((char *)VersionString))
	{
		printf("Version error!\n");
		return -1;
	}

	Version->Mode = VersionTemp.Mode;
	Version->BigVersion = VersionTemp.BigVersion;
	Version->LittleVersion = VersionTemp.LittleVersion;
	Version->CustomCode = VersionTemp.CustomCode;

	return 0;

}

int CHid_Upgrade::String2VersionNumber(CString VersionString, DWORD StrStart, version_t *Version)
{
	version_t VersionTemp;
	DWORD StrIndex = StrStart;


	DWORD StrLength = VersionString.GetLength();


	//Mode
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
		VersionTemp.Mode = CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
	{
		VersionTemp.Mode = (VersionTemp.Mode * 10) + CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	}

	if (VersionString.GetAt(StrIndex) == '.')
	{
		StrIndex++;

	}
	else
	{
		printf("Version error!\n");
		return -1;
	}

	//BigVersion
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
		VersionTemp.BigVersion = CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
	{
		VersionTemp.BigVersion = (VersionTemp.BigVersion * 10) + CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	}

	if (VersionString.GetAt(StrIndex) == '.')
	{
		StrIndex++;

	}
	else
	{
		printf("Version error!\n");
		return -1;
	}

	//LittleVersion
	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
		VersionTemp.LittleVersion = CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	else
	{
		printf("Version error!\n");
		return -1;
	}

	if (CHECK_CHAR_IS_DEC_NUMBER(VersionString.GetAt(StrIndex)))
	{
		VersionTemp.LittleVersion = (VersionTemp.LittleVersion * 10) + CHAR_TO_DEC_NUMBER(VersionString.GetAt(StrIndex++));
	}

	//CustomCode
	if ((VersionString.GetAt(StrIndex) >= 'A' && VersionString.GetAt(StrIndex) <= 'Z') ||
		(VersionString.GetAt(StrIndex) >= 'a' && VersionString.GetAt(StrIndex) <= 'z')
		)
	{
		VersionTemp.CustomCode = VersionString.GetAt(StrIndex);//必须为字母字符
	}
	else
	{
		VersionTemp.CustomCode = ' ';
	}

	if (StrIndex > StrLength)
	{
		printf("Version error!\n");
		return -1;
	}

	Version->Mode = VersionTemp.Mode;
	Version->BigVersion = VersionTemp.BigVersion;
	Version->LittleVersion = VersionTemp.LittleVersion;
	Version->CustomCode = VersionTemp.CustomCode;

	return 0;

}

int CHid_Upgrade::GetFirmwareFileVersion(const unsigned char *fpbuff, DWORD FileSize, file_type FileType, version_t *FileVersion)
{
	DWORD i;
	CString str = "";
	CString temp_str = "";
	DWORD StrFind_start, temp;
	//version_t FileVersion = { 0 };


	if (!(FileType == FILE_TYPE_SOC_PKG || FileType == FILE_TYPE_FX3_USB || FileType == FILE_TYPE_FX3_FPGA || FileType == FILE_TYPE_FX3_USB_BOOTLOADER))
		return -1;

	//str.IsEmpty();

	for (i = 0; i < FileSize; i++)
	{
		if (fpbuff[i] == 0)
			str += ' ';
		else
			str += fpbuff[i];
	}

	switch (FileType)
	{
	case FILE_TYPE_FX3_USB_BOOTLOADER:
	{
		StrFind_start = str.Find("FX3_BOOTLOADER", 0);
		//printf("StrFind_start = %d\n", StrFind_start);
		if (StrFind_start != -1)
		{
			if (VersionString2Number((const BYTE *)fpbuff + StrFind_start, "FX3_BOOTLOADER", FileVersion) == -1)
				return -1;

			printf("USB BootLoader File Version : %d.%d.%d%c\n", FileVersion->Mode, FileVersion->BigVersion, FileVersion->LittleVersion, FileVersion->CustomCode);
			//printEventLog(EVENTLOG_INFORMATION_TYPE, "USB BootLoader File Version : %d.%d.%d%c\n", FileVersion->Mode, FileVersion->BigVersion, FileVersion->LittleVersion, FileVersion->CustomCode);
		}
		else
		{
			return -1;
		}
		break;
	}
	case FILE_TYPE_FX3_USB:
	{
		StrFind_start = str.Find("FX3_USB", 0);
		//printf("StrFind_start = %d\n", StrFind_start);
		if (StrFind_start != -1)
		{
			if (VersionString2Number((const BYTE *)fpbuff + StrFind_start, "FX3_USB", FileVersion) == -1)
				return -1;

			printf("USB File Version : %d.%d.%d%c\n", FileVersion->Mode, FileVersion->BigVersion, FileVersion->LittleVersion, FileVersion->CustomCode);
		}
		else
		{
			return -1;
		}
		break;
	}
	case FILE_TYPE_FX3_FPGA:
	{
		StrFind_start = str.Find("FX3_FPGA", 0);
		//printf("StrFind_start = %d\n", StrFind_start);
		if (StrFind_start != -1)
		{
			if (VersionString2Number((const BYTE *)fpbuff + StrFind_start, "FX3_FPGA", FileVersion) == -1)
				return -1;

			printf("FPGA File Version : %d.%d.%d\n", FileVersion->Mode, FileVersion->BigVersion, FileVersion->LittleVersion);
		}
		else
		{
			return -1;
		}

		break;
	}


	case FILE_TYPE_SOC_PKG:
	{
		if (fpbuff[FileSize - 1] != '>')
			return -1;

		temp = str.ReverseFind('<');
		if (temp == -1)
			return -1;

		temp_str = "";
		//temp_str += str.Mid(temp - 255, temp -1);
		for (i = temp - 255; i < temp; i++)
		{
			if (fpbuff[i] == 0)
				temp_str += ' ';
			else
				temp_str += fpbuff[i];
		}

		StrFind_start = temp_str.Find("<SOC", 0);
		//printf("StrFind_start = %d\n", StrFind_start);
		if (StrFind_start != -1)
		{
			StrFind_start += (temp - 255);

			if (VersionString2Number((const BYTE *)fpbuff + StrFind_start, "<SOC", FileVersion) == -1)
				return -1;

			printf("PKG File Version : %d.%d.%d\n", FileVersion->Mode, FileVersion->BigVersion, FileVersion->LittleVersion);
		}
		else
		{
			return -1;
		}
		break;
	}

	default:
		return -1;
	}

	return 0;
}

BOOL CHid_Upgrade::IsBootLoader()
{
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER ||
		m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int CHid_Upgrade::FindDevice()
{
	if (m_MyHidDevice.FindHid(vid1,pid1))
	{
		DeviceConnect();
	}
	else
	{
		DeviceDisconnect();
		return	-1;
	}

	return	0;

}

//设备连接时的响应函数
void CHid_Upgrade::DeviceConnect()
{
	CameraInfo.HIDUpgradeFirmwareID = m_MyHidDevice.HIDUpgradeFirmwareID;
	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID && CameraInfo.UpgradeFlag == TRUE)
	{
		return;
	}

	CameraInfo.MyDevFound = TRUE;
}

//设备断开时的响应函数
void CHid_Upgrade::DeviceDisconnect()
{
	//设置设备状态为未找到
	CameraInfo.MyDevFound = FALSE;
	CameraInfo.UpgradeFlag = FALSE;
	CameraInfo.AutoUpdateFlag = FALSE;
	CameraInfo.HIDUpgradeFirmwareID = DEVICE_UPGRADE_ERROR;
}

int CHid_Upgrade::QueryVersion()
{
	if (m_MyHidDevice.HidOutputReportByteLength == HID_REPORT_LEN_V1)
	{
		return QueryVersion_V1();

	}
	else if (m_MyHidDevice.HidOutputReportByteLength > HID_REPORT_LEN_V1)
	{
		return QueryVersion_V2();
	}
	else
	{
		return -1;
	}
}


int CHid_Upgrade::QueryVersion_V1()
{
	struct hid_version version;
	CString str;
	int StrFind_start, StrFind_end, StrTemp;
	int i;
	unsigned char cnt = 0;
	CString package_version_str = "";
	DWORD soc_version_num, package_version;

QueryAgain:
	CameraInfo.FPGAVersion = { 0xff, 0xff, 0xff };
	CameraInfo.SOCVersion = { 0 };
	CameraInfo.AFVersion = { 0 };
	CameraInfo.HWVersion = { 0 };
	CameraInfo.USBBootLoaderVersion = { 0 };
	CameraInfo.SocHidVersion = { 0 };
	CameraInfo.USBVersion.Mode = 0xff;
	CameraInfo.USBVersion.BigVersion = 0xff;
	CameraInfo.USBVersion.LittleVersion = 0xff;
	CameraInfo.USBVersion.CustomCode = ' ';
	CameraInfo.StringDeviceName = "";
	CameraInfo.StringUSBVersion = "";
	CameraInfo.StringFPGAVersion = "";
	CameraInfo.StringSOCVersion = "";
	CameraInfo.StringUSBBootLoaderVersion = "";
	CameraInfo.StringPackageVersion = "";
	CameraInfo.StringSOCVersionMode = "";
	CameraInfo.StringAFVersion = "";
	CameraInfo.StringHWVersion = "";
	CameraInfo.USBBootLoaderFound = 0;

	CameraInfo.StringDeviceName = m_MyHidDevice.GetProductString();

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{

		memset((BYTE *)&version, 0, sizeof(version));
		version.head = HID_HEAD_VERSION;
		version.operation = HID_CAM_OPERATION_QUERY;
		version.report_len = m_MyHidDevice.HidOutputReportByteLength;
		version.version_type = VERSION_TYPE_SOC;
		version.crc = cyg_crc16((BYTE *)&version + 2, m_MyHidDevice.HidOutputReportByteLength - 2);

		str = "";
		str = m_MyHidDevice.GetSOCVersionString();
		if (str != "")
		{
			StrFind_start = str.Find("SOC");
			if (StrFind_start != -1)
			{
				StrFind_end = str.GetLength() * sizeof(TCHAR);
				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (str.Find(":"))
					StrTemp++;

				//if(str.GetAt(StrTemp + 1) == 'V' || str.GetAt(StrTemp + 1) == 'v')
				//	StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				StrFind_end = CameraInfo.StringSOCVersion.Find("_");
				if (StrFind_end != -1)
				{
					CameraInfo.StringSOCVersionMode = CameraInfo.StringSOCVersion.Mid(0, StrFind_end);
					StrTemp = StrFind_end;
					StrFind_end = CameraInfo.StringSOCVersion.GetLength() * sizeof(TCHAR);
					while (CameraInfo.StringSOCVersion.GetAt(StrTemp) != '.')
					{
						if (StrTemp >= StrFind_end)
							break;

						CameraInfo.StringSOCVersionMode += CameraInfo.StringSOCVersion.GetAt(StrTemp);
						StrTemp++;
					}

					return 0;

				}
			}
		}

		if (SendQueryVersion(&version) == FALSE)
		{
			printf("Query version error!\n");
			return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("USB", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "USB", &CameraInfo.USBVersion) != 0)
					return -1;
			}

			StrFind_start = str.Find("SOC", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "SOC", &CameraInfo.SOCVersion) != 0)
					return -1;

			}
			else
			{
				printf("Query version error!\n");
				return -1;
			}
			return 0;
		}
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{

		str = ""; 
		str = m_MyHidDevice.GetSOCVersionString();
		if (str != "")
		{
			StrFind_start = str.Find("SOC");
			if (StrFind_start != -1)
			{
				StrFind_end = str.GetLength() * sizeof(TCHAR);
				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (str.Find(":"))
					StrTemp++;

				//if(str.GetAt(StrTemp + 1) == 'V' || str.GetAt(StrTemp + 1) == 'v')
				//	StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				StrFind_end = CameraInfo.StringSOCVersion.Find("_");
				if (StrFind_end != -1)
				{
					CameraInfo.StringSOCVersionMode = CameraInfo.StringSOCVersion.Mid(0, StrFind_end);
					StrTemp = StrFind_end;
					StrFind_end = CameraInfo.StringSOCVersion.GetLength() * sizeof(TCHAR);
					while (CameraInfo.StringSOCVersion.GetAt(StrTemp) != '.')
					{
						if (StrTemp >= StrFind_end)
							break;

						CameraInfo.StringSOCVersionMode += CameraInfo.StringSOCVersion.GetAt(StrTemp);
						StrTemp++;
					}
						
				}
			}
		}
		else
		{
			str = m_MyHidDevice.GetSerialNumberString();
			StrFind_start = str.Find("SOC");
			if (StrFind_start == -1)
			{
				StrFind_start = str.Find("USB");
				if (StrFind_start == -1)
					return -1;

				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				if (str.Find(":"))
					StrTemp++;

				if (str.Find('V') || str.Find('v'))
					StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);
			}
			else
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (str.Find(":"))
					StrTemp++;

				if (str.Find('V') || str.Find('v'))
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);
			}
		}

		return 0;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		memset((BYTE *)&version, 0, sizeof(version));
		version.head = HID_HEAD_VERSION_FX3;
		version.operation = HID_CAM_OPERATION_QUERY;
		version.report_len = m_MyHidDevice.HidOutputReportByteLength;
		version.version_type = VERSION_TYPE_USB;
		version.crc = cyg_crc16((BYTE *)&version + 2, m_MyHidDevice.HidOutputReportByteLength - 2);

		if (SendQueryVersion(&version) == FALSE)
		{
			printf("Query version error!\n");
			return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("USB", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "USB", &CameraInfo.USBVersion) != 0)
					return -1;
			}
			StrFind_start = str.Find("FPGA", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("FPGA");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringFPGAVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "FPGA", &CameraInfo.FPGAVersion) != 0)
					return -1;
			}

			StrFind_start = str.Find("SOC", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "SOC", &CameraInfo.SOCVersion) != 0)
					return -1;

			}
			else
			{
				cnt++;
				if (cnt < 3)
				{
					//等待SOC版本返回
					Sleep(1000);
					goto QueryAgain;

				}
			}

			//USB_BOOTLOADER VERSION
			memset((BYTE *)&version, 0, sizeof(version));
			version.head = HID_HEAD_VERSION_FX3;
			version.operation = HID_CAM_OPERATION_QUERY;
			version.report_len = m_MyHidDevice.HidOutputReportByteLength;
			version.version_type = VERSION_TYPE_USB_BOOTLOADER;
			version.crc = cyg_crc16((BYTE *)&version + 2, m_MyHidDevice.HidOutputReportByteLength - 2);

			if (SendQueryVersion(&version) != FALSE)
			{
				str = "";
				str = _T(version.data);
				StrFind_start = str.Find("BOOTLOADER", 0);
				if (StrFind_start != -1)
				{
					StrFind_end = str.Find("_", StrFind_start);
					if (StrFind_end == -1)
					{
						StrFind_end = str.GetLength()*sizeof(TCHAR);
					}

					StrTemp = StrFind_start;
					StrTemp += strlen("BOOTLOADER");

					if (version.data[StrTemp] == ':')
						StrTemp++;

					if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
						StrTemp++;

					CameraInfo.StringUSBBootLoaderVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

					if (VersionString2Number((const BYTE *)version.data + StrFind_start, "BOOTLOADER", &CameraInfo.USBBootLoaderVersion) != 0)
						return -1;

					CameraInfo.USBBootLoaderFound = 1;
				}
			}

		}
#if 0
		package_version_str = "";
		str = "";
		package_version_str = m_MyHidDevice.GetV61PUPackageVersionString();
		if (package_version_str != "")
		{
			str += ("Package " + package_version_str + "\n");
		}
		else
		{
			package_version_str = "";

			soc_version_num = (CameraInfo.SOCVersion.BigVersion * 100) + CameraInfo.SOCVersion.LittleVersion;

			if (soc_version_num < 233)
				package_version = 386;
			else
				package_version = (soc_version_num - 233) + 388;

			package_version_str.Format("Package Firmware:1.0.0.%d\n", package_version);

			str += package_version_str;

		}
		CameraInfo.StringPackageVersion = str;
		printf("Package %s\n", package_version_str);
#endif
		return 0;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1 || CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER)
	{
		CameraInfo.USBBootLoaderFound = 1;
		return 0;
	}
	else {
		//printf("Camera ID error! (%d)\n", CameraInfo.HIDUpgradeFirmwareID);
		return -1;
	}


	return 0;
}

int CHid_Upgrade::QueryVersion_V2()
{
	struct hid_version_v2 version;
	CString str;
	int StrFind_start, StrFind_end, StrTemp;
	int i;
	unsigned char cnt = 0;
	CString package_version_str = "";
	DWORD soc_version_num, package_version;

QueryAgain:
	CameraInfo.FPGAVersion = { 0xff, 0xff, 0xff };
	CameraInfo.SOCVersion = { 0 };
	CameraInfo.AFVersion = { 0 };
	CameraInfo.HWVersion = { 0 };
	CameraInfo.USBBootLoaderVersion = { 0 };
	CameraInfo.SocHidVersion = { 0 };
	CameraInfo.USBVersion.Mode = 0xff;
	CameraInfo.USBVersion.BigVersion = 0xff;
	CameraInfo.USBVersion.LittleVersion = 0xff;
	CameraInfo.USBVersion.CustomCode = ' ';
	CameraInfo.StringDeviceName = "";
	CameraInfo.StringUSBVersion = "";
	CameraInfo.StringFPGAVersion = "";
	CameraInfo.StringSOCVersion = "";
	CameraInfo.StringUSBBootLoaderVersion = "";
	CameraInfo.StringPackageVersion = "";
	CameraInfo.StringSOCVersionMode = "";
	CameraInfo.StringAFVersion = "";
	CameraInfo.StringHWVersion = "";
	CameraInfo.StringALLVersion = "";
	
	CameraInfo.StringDeviceName = m_MyHidDevice.GetProductString();
	
	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{

		memset((BYTE *)&version, 0, sizeof(version));
		version.head = HID_HEAD_VERSION;
		version.operation = HID_CAM_OPERATION_QUERY;
		version.report_len = m_MyHidDevice.HidOutputReportByteLength;
		version.version_type = VERSION_TYPE_SOC;
		version.crc = cyg_crc16(((BYTE *)&version) + 2, m_MyHidDevice.HidOutputReportByteLength - 2);


		str = "";
		str = m_MyHidDevice.GetSOCVersionString();
		if (str != "")
		{
			StrFind_start = str.Find("SOC");
			if (StrFind_start != -1)
			{
				StrFind_end = str.GetLength() * sizeof(TCHAR);
				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (str.Find(":"))
					StrTemp++;

				//if(str.GetAt(StrTemp + 1) == 'V' || str.GetAt(StrTemp + 1) == 'v')
				//	StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				StrFind_end = CameraInfo.StringSOCVersion.Find("_");
				if (StrFind_end != -1)
				{
					CameraInfo.StringSOCVersionMode = CameraInfo.StringSOCVersion.Mid(0, StrFind_end);
					StrTemp = StrFind_end;
					StrFind_end = CameraInfo.StringSOCVersion.GetLength() * sizeof(TCHAR);
					while (CameraInfo.StringSOCVersion.GetAt(StrTemp) != '.')
					{
						if (StrTemp >= StrFind_end)
							break;

						CameraInfo.StringSOCVersionMode += CameraInfo.StringSOCVersion.GetAt(StrTemp);
						StrTemp++;
					}

					return 0;

				}
			}
		}


		if (SendQueryVersion_V2(&version) == FALSE)
		{
			printf("Query version error!\n");
			return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("USB", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "USB", &CameraInfo.USBVersion) != 0)
					return -1;
			}

			StrFind_start = str.Find("SOC", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);
	
				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "SOC", &CameraInfo.SOCVersion) != 0)
					return -1;
				
			}
			else
			{
				printf("Query version error!\n");
				return -1;
			}
			//return 0;
		}

		version.version_type = VERSION_TYPE_HW;
		version.crc = cyg_crc16(((BYTE *)&version) + 2, m_MyHidDevice.HidOutputReportByteLength - 2);
		if (SendQueryVersion_V2(&version) == FALSE)
		{
			printf("Query version error!\n");
			//return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("HW", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("HW");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringHWVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "HW", &CameraInfo.HWVersion) != 0)
					printf("VersionString2Number fail\n");
					//return -1;

			}
			else
			{
				printf("Query version error!\n");
				//return -1;
			}
			//return 0;
		}

		version.version_type = VERSION_TYPE_AF;
		version.crc = cyg_crc16(((BYTE *)&version) + 2, m_MyHidDevice.HidOutputReportByteLength - 2);
		if (SendQueryVersion_V2(&version) == FALSE)
		{
			printf("Query version error!\n");
			//return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("AF", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("AF");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringAFVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "AF", &CameraInfo.AFVersion) != 0)
					printf("VersionString2Number fail\n");
				//return -1;

			}
			else
			{
				printf("Query version error!\n");
				//return -1;
			}
			return 0;
		}
		return 0;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{

		str = "";
		str = m_MyHidDevice.GetSOCVersionString();
		if (str != "")
		{
			StrFind_start = str.Find("SOC");
			if (StrFind_start != -1)
			{
				StrFind_end = str.GetLength() * sizeof(TCHAR);
				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (str.Find(":"))
					StrTemp++;

				//if(str.GetAt(StrTemp + 1) == 'V' || str.GetAt(StrTemp + 1) == 'v')
				//	StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				StrFind_end = CameraInfo.StringSOCVersion.Find("_");
				if (StrFind_end != -1)
				{
					CameraInfo.StringSOCVersionMode = CameraInfo.StringSOCVersion.Mid(0, StrFind_end);
					StrTemp = StrFind_end;
					StrFind_end = CameraInfo.StringSOCVersion.GetLength() * sizeof(TCHAR);
					while (CameraInfo.StringSOCVersion.GetAt(StrTemp) != '.')
					{
						if (StrTemp >= StrFind_end)
							break;

						CameraInfo.StringSOCVersionMode += CameraInfo.StringSOCVersion.GetAt(StrTemp);
						StrTemp++;
					}

				}
			}
		}
		else
		{
			str = m_MyHidDevice.GetSerialNumberString();
			StrFind_start = str.Find("SOC");
			if (StrFind_start == -1)
			{
				StrFind_start = str.Find("USB");
				if (StrFind_start == -1)
					return -1;

				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				//if (version.data[StrTemp] == ':')
				//	StrTemp++;

				//if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
				//	StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);
			}
			else
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);
			}
		}

		return 0;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		memset((BYTE *)&version, 0, sizeof(version));
		version.head = HID_HEAD_VERSION_FX3;
		version.operation = HID_CAM_OPERATION_QUERY;
		version.report_len = m_MyHidDevice.HidOutputReportByteLength;
		version.version_type = VERSION_TYPE_USB;
		version.crc = cyg_crc16((BYTE *)&version + 2, m_MyHidDevice.HidOutputReportByteLength - 2);

		if (SendQueryVersion_V2(&version) == FALSE)
		{
			printf("Query version error!\n");
			return -1;
		}
		else
		{
			str = "";
			str = _T(version.data);
			StrFind_start = str.Find("USB", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("USB");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringUSBVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "USB", &CameraInfo.USBVersion) != 0)
					return -1;
			}
			StrFind_start = str.Find("FPGA", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("FPGA");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringFPGAVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "FPGA", &CameraInfo.FPGAVersion) != 0)
					return -1;
			}

			StrFind_start = str.Find("SOC", 0);
			if (StrFind_start != -1)
			{
				StrFind_end = str.Find("_", StrFind_start);
				if (StrFind_end == -1)
				{
					StrFind_end = str.GetLength() * sizeof(TCHAR);
				}

				StrTemp = StrFind_start;
				StrTemp += strlen("SOC");

				if (version.data[StrTemp] == ':')
					StrTemp++;

				if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
					StrTemp++;

				CameraInfo.StringSOCVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

				if (VersionString2Number((const BYTE *)version.data + StrFind_start, "SOC", &CameraInfo.SOCVersion) != 0)
					return -1;

			}
			else
			{
				cnt++;
				if (cnt < 3)
				{
					//等待SOC版本返回
					Sleep(1000);
					goto QueryAgain;

				}
			}

			//USB_BOOTLOADER VERSION
			memset((BYTE *)&version, 0, sizeof(version));
			version.head = HID_HEAD_VERSION_FX3;
			version.operation = HID_CAM_OPERATION_QUERY;
			version.report_len = m_MyHidDevice.HidOutputReportByteLength;
			version.version_type = VERSION_TYPE_USB_BOOTLOADER;
			version.crc = cyg_crc16((BYTE *)&version + 2, m_MyHidDevice.HidOutputReportByteLength - 2);

			if (SendQueryVersion_V2(&version) != FALSE)
			{
				str = "";
				str = _T(version.data);
				StrFind_start = str.Find("BOOTLOADER", 0);
				if (StrFind_start != -1)
				{
					StrFind_end = str.Find("_", StrFind_start);
					if (StrFind_end == -1)
					{
						StrFind_end = str.GetLength()*sizeof(TCHAR);
					}

					StrTemp = StrFind_start;
					StrTemp += strlen("BOOTLOADER");

					if (version.data[StrTemp] == ':')
						StrTemp++;

					if (version.data[StrTemp] == 'V' || version.data[StrTemp] == 'v')
						StrTemp++;

					CameraInfo.StringUSBBootLoaderVersion += str.Mid(StrTemp, StrFind_end - StrTemp);

					if (VersionString2Number((const BYTE *)version.data + StrFind_start, "BOOTLOADER", &CameraInfo.USBBootLoaderVersion) != 0)
						return -1;
				}
			}

		}
#if 0
		package_version_str = "";
		str = "";
		package_version_str = m_MyHidDevice.GetV61PUPackageVersionString();
		if (package_version_str != "")
		{
			str += ("Package " + package_version_str + "\n");
		}
		else
		{
			package_version_str = "";

			soc_version_num = (CameraInfo.SOCVersion.BigVersion * 100) + CameraInfo.SOCVersion.LittleVersion;

			if (soc_version_num < 233)
				package_version = 386;
			else
				package_version = (soc_version_num - 233) + 388;

			package_version_str.Format("Package Firmware:1.0.0.%d\n", package_version);

			str += package_version_str;

		}
		CameraInfo.StringPackageVersion = str;
		printf("Package %s\n", package_version_str);
#endif
		return 0;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1 || CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER)
	{
		return 0;
	}
	else {
		//printf("Camera ID error! (%d)\n", CameraInfo.HIDUpgradeFirmwareID);
		return -1;
	}


	return 0;
}

int CHid_Upgrade::FX3Upgrade(CString File, CString strModeName, void *pParam)
{
	FILE *fp;
	unsigned char *fpbuff;
	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名
	CString	pos = "";
	CString str = "";
	CString FileNameStr = "";
	unsigned long int  i, j;
	unsigned long int  temp1, temp2 = 0;

	DWORD FileSize = 0;
	DWORD PackSize;
	DWORD LastPackLen;

	if (CameraInfo.USBBootLoaderFound && strModeName == UPGRADE_MODE_STRING_USB)
		return UsbUpgrade2FpgaFlash(File, UPGRADE_MODE_STRING_USB_BOOT, pParam);

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	if (!CameraInfo.MyDevFound)
	{
		return UPGRADE_ERROR_CONNECT;
	}

	if (File == "")
	{
		return UPGRADE_ERROR_FILE_NOT_FOUND;
	}

	_splitpath(File, NULL, NULL, FileName, FileExt);

	if (strcmp(FileExt, ".img") != 0)
	{
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//文件名和文件类型正确，打开文件
	fp = fopen(File, "rb");
	if (fp == NULL) {
		fclose(fp);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	FileSize = ftell(fp); /* 得到文件大小 */

	if ((FileSize % 32) != 0)
	{
		fpbuff = (unsigned char *)malloc(FileSize + (32 - FileSize % 32));
	}
	else
	{
		fpbuff = (unsigned char *)malloc(FileSize); /* 根据文件大小动态分配内存空间 */
	}

	if (fpbuff == NULL)
	{
		fclose(fp);
		return UPGRADE_ERROR_MEMORY_ERROR;
	}
	fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
	fread(fpbuff, FileSize, 1, fp); /* 一次性读取全部文件内容 */
	fclose(fp);


	//验证文件大小是否超出Flash容量
	if (FileSize > 128 * 1024 - 128)//128K Flash
	{
		free(fpbuff);	
		return UsbUpgrade2FpgaFlash(File, UPGRADE_MODE_STRING_USB_BOOT, pParam);
		//return UPGRADE_ERROR_FILE_SIZE;
	}

	//验证文件内容是否符合
	int file_crc_check = Fx3ImgChecksum(fpbuff, FileSize);
	if (file_crc_check < 0)
	{
		fclose(fp);
		free(fpbuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}

	for (i = 0; i < FileSize; i++)
	{
		if (fpbuff[i] == 0)
			str += ' ';
		else
			str += fpbuff[i];
	}

	int StrFind_FX3_BOOTLOADER = str.Find("FX3_BOOTLOADER", 0);
	int StrFind_FX3_USB = str.Find("FX3_USB", 0);


	QueryVersion();
	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	FileNameStr = FileName;

	if (CameraInfo.USBVersion.Mode == V50U && CameraInfo.USBVersion.Mode == V50U)
	{
		int StrFind1 = FileNameStr.Find("V50U_USB");
		int StrFind2 = FileNameStr.Find("V51U_USB");
		if (StrFind1 == -1 && StrFind2 == -1)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				if ((IDNO == AfxMessageBox("版本可能不兼容，是否要进行升级 ?", MB_ICONEXCLAMATION | MB_YESNO)))
				{
					free(fpbuff);
					return UPGRADE_ERROR_FILE_TYPE;
				}
				break;

			case LANGUAGE_ENGLISH:
			default:
				if ((IDNO == AfxMessageBox("The version may not be compatible with this version! Whether to upgrade ?", MB_ICONEXCLAMATION | MB_YESNO)))
				{
					free(fpbuff);
					return UPGRADE_ERROR_FILE_TYPE;
				}
				break;
			}
		}

	}
	else if ((CameraInfo.USBVersion.Mode == V60U || CameraInfo.USBVersion.Mode == V7xU) || IsBootLoader())
	{
		int StrFind1 = FileNameStr.Find("V60U_USB");
		int StrFind2 = FileNameStr.Find("V61U_USB");
		int StrFind3 = FileNameStr.Find("V600U_USB");
		int StrFind4 = FileNameStr.Find("V610U_USB");
		int StrFind5 = FileNameStr.Find("Unite 200_USB");

		int StrFind6 = FileNameStr.Find("V70U_USB");
		int StrFind7 = FileNameStr.Find("V71U_USB");
		int StrFind8 = FileNameStr.Find("V700U_USB");
		int StrFind9 = FileNameStr.Find("V710U_USB");

		int StrFind10 = FileNameStr.Find("USB_Bootloader");

		if (StrFind1 == -1 && StrFind2 == -1 && StrFind3 == -1 && StrFind4 == -1 && StrFind5 == -1
			&& StrFind6 == -1 && StrFind7 == -1 && StrFind8 == -1 && StrFind9 == -1 && StrFind10 == -1 
			&& StrFind_FX3_USB == -1 && StrFind_FX3_BOOTLOADER == -1)
		{

			free(fpbuff);
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else
	{
		switch (Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			if ((IDNO == AfxMessageBox("版本可能不兼容，是否要进行升级 ?", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				free(fpbuff);
				return UPGRADE_ERROR_FILE_TYPE;
			}
			break;

		case LANGUAGE_ENGLISH:
		default:
			if ((IDNO == AfxMessageBox("The version may not be compatible with this version! Whether to upgrade ?", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				free(fpbuff);
				return UPGRADE_ERROR_FILE_TYPE;
			}
			break;
		}
	}

	//开始升级
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 || m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		struct hid_file_data FileData;

		PackSize = FileSize / 32;
		if (FileSize % 32)
		{
			LastPackLen = FileSize % 32;
			PackSize += 1;
		}
		else
		{
			LastPackLen = 32;
		}

		FileData.head = HID_HEAD_FILE_FX3;
		FileData.pkg_size = PackSize;
		FileData.file_type = FILE_TYPE_FX3_USB;
		FileData.file_crc = file_crc_check;

		if (SendCharsCommand("go into update", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{


			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("usb update probe", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{

			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("usb update commit", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{
			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}

		for (i = 0; i < PackSize; i++)
		{
			FileData.pkg_num = i;
			if (i == PackSize - 1)
			{
				for (j = 0; j < 32; j++)
				{
					if (j < LastPackLen)
						FileData.data[j] = fpbuff[i * 32 + j];
					else
						FileData.data[j] = 0;
				}
				FileData.data_size = LastPackLen;
			}
			else
			{
				for (j = 0; j < 32; j++)
				{
					FileData.data[j] = fpbuff[i * 32 + j];
				}
				FileData.data_size = 32;
			}

			if (SendFileData(&FileData) == FALSE)
			{
				free(fpbuff);
				return UPGRADE_ERROR_TRANSFER;

			}
			else
			{
				temp1 = (100 * i / (PackSize - 1));
				if (temp2 != temp1)
				{
					pos.Format("%d%%", temp1);

					if (Language == LANGUAGE_SIMPLIFIED_CHINESE)
						pAppDlg->DisplayStatus("正在升级USB固件 ..." + pos);
					else
						pAppDlg->DisplayStatus("USB firmware upgrading ..." + pos);

					pAppDlg->PromptSetPos(temp1);
				}
				temp2 = temp1;
			}
		}
	}
	else
	{
		return UPGRADE_ERROR_VERSION;
	}

	free(fpbuff);

	return UPGRADE_SUCCESS;
}

int CHid_Upgrade::UsbUpgrade2FpgaFlash(CString File, CString strModeName, void *pParam)
{
	FILE *fp;
	unsigned char *fpbuff;
	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名
	CString	pos = "";
	CString str = "";
	CString FileNameStr = "";
	unsigned long int  i, j;
	unsigned long int  temp1, temp2 = 0;

	DWORD FileSize = 0;
	DWORD PackSize;
	DWORD LastPackLen;

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	if (!CameraInfo.MyDevFound)
	{
		return UPGRADE_ERROR_CONNECT;
	}

	if (File == "")
	{
		return UPGRADE_ERROR_FILE_NOT_FOUND;
	}

	_splitpath(File, NULL, NULL, FileName, FileExt);

	if (strcmp(FileExt, ".img") != 0)
	{
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//文件名和文件类型正确，打开文件
	fp = fopen(File, "rb");
	if (fp == NULL) {
		fclose(fp);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	FileSize = ftell(fp); /* 得到文件大小 */

	if ((FileSize % 4096) != 0)
	{
		fpbuff = (unsigned char *)malloc(FileSize + (4096 - FileSize % 4096));
	}
	else
	{
		fpbuff = (unsigned char *)malloc(FileSize); /* 根据文件大小动态分配内存空间 */
	}

	if (fpbuff == NULL)
	{
		fclose(fp);
		return UPGRADE_ERROR_MEMORY_ERROR;
	}
	fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
	fread(fpbuff, FileSize, 1, fp); /* 一次性读取全部文件内容 */
	fclose(fp);


	//验证文件大小是否超出Flash容量
	if (FileSize > 512 * 1024)//512K Flash
	{
		free(fpbuff);
		return UPGRADE_ERROR_FILE_SIZE;
	}

	//验证文件内容是否符合
	int file_crc_check = Fx3ImgChecksum(fpbuff, FileSize);
	if (file_crc_check < 0)
	{
		free(fpbuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}

	for (i = 0; i < FileSize; i++)
	{
		if (fpbuff[i] == 0)
			str += ' ';
		else
			str += fpbuff[i];
	}

	int StrFind_FX3_BOOTLOADER = str.Find("FX3_BOOTLOADER", 0);
	if (StrFind_FX3_BOOTLOADER != -1)
	{
		free(fpbuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}

	int StrFind_FX3_USB = str.Find("FX3_USB", 0);



	QueryVersion();
	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	FileNameStr = FileName;

	if (CameraInfo.USBVersion.Mode == V50U && CameraInfo.USBVersion.Mode == V50U)
	{
		int StrFind1 = FileNameStr.Find("V50U_USB");
		int StrFind2 = FileNameStr.Find("V51U_USB");
		if (StrFind1 == -1 && StrFind2 == -1)
		{
			free(fpbuff);
			return UPGRADE_ERROR_FILE_TYPE;
		}

	}
	else if ((CameraInfo.USBVersion.Mode == V60U || CameraInfo.USBVersion.Mode == V7xU) || IsBootLoader())
	{
		int StrFind1 = FileNameStr.Find("V60U_USB");
		int StrFind2 = FileNameStr.Find("V61U_USB");
		int StrFind3 = FileNameStr.Find("V600U_USB");
		int StrFind4 = FileNameStr.Find("V610U_USB");
		int StrFind5 = FileNameStr.Find("Unite 200_USB");

		int StrFind6 = FileNameStr.Find("V70U_USB");
		int StrFind7 = FileNameStr.Find("V71U_USB");
		int StrFind8 = FileNameStr.Find("V700U_USB");
		int StrFind9 = FileNameStr.Find("V710U_USB");

		if (StrFind1 == -1 && StrFind2 == -1 && StrFind3 == -1 && StrFind4 == -1 && StrFind5 == -1
			&& StrFind6 == -1 && StrFind7 == -1 && StrFind8 == -1 && StrFind9 == -1 && StrFind_FX3_USB == -1)
		{

			free(fpbuff);
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else
	{
		free(fpbuff);
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//开始升级FPGA
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 || m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		struct hid_file_data FileData;
		struct hid_file_info FileInfo;

		if (SendCharsCommand("go into update", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{


			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("fpga update probe", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{

			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("fpga update commit", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{
			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}

		if (strModeName == UPGRADE_MODE_STRING_USB_RESTORE)
		{
			FileInfo.file_size = BOOT_FORM_SPI_ADDR_RESTORE;
		}
		else
		{
			FileInfo.file_size = BOOT_FORM_SPI_ADDR;
		}

		//发送文件的信息
		FileInfo.head = HID_HEAD_FILE_FX3;
		//FileInfo.file_size = 0;
		FileInfo.file_type = FILE_TYPE_FX3_FPGA;

		if (SendFileInfo(&FileInfo) == FALSE)
		{
			free(fpbuff);
			return UPGRADE_ERROR_TRANSFER;
		}

		PackSize = FileSize / 32;
		if (FileSize % 32)
		{
			LastPackLen = FileSize % 32;
			PackSize += 1;
		}
		else
		{
			LastPackLen = 32;
		}

		FileData.head = HID_HEAD_FILE_FX3;
		FileData.pkg_size = PackSize;
		FileData.file_type = FILE_TYPE_FX3_FPGA;
		FileData.file_crc = file_crc_check;
		for (i = 0; i < PackSize; i++)
		{
			FileData.pkg_num = i;
			if (i == PackSize - 1)
			{
				for (j = 0; j < 32; j++)
				{
					if (j < LastPackLen)
						FileData.data[j] = fpbuff[i * 32 + j];
					else
						FileData.data[j] = 0;
				}
				FileData.data_size = LastPackLen;
			}
			else
			{
				for (j = 0; j < 32; j++)
				{
					FileData.data[j] = fpbuff[i * 32 + j];
				}
				FileData.data_size = 32;
			}

			if (SendFileData(&FileData) == FALSE)
			{
				free(fpbuff);
				//exit(0);
				return UPGRADE_ERROR_TRANSFER;

			}
			else
			{
				temp1 = (100 * i / (PackSize - 1));
				if (temp2 != temp1)
				{
					pos.Format("%d%%", temp1);

					if (Language == LANGUAGE_SIMPLIFIED_CHINESE)
						pAppDlg->DisplayStatus("正在升级USB-Boot固件 ..." + pos);
					else
						pAppDlg->DisplayStatus("USB-Boot firmware upgrading ..." + pos);

					pAppDlg->PromptSetPos(temp1);
				}
				temp2 = temp1;
			}
		}
	}
	else
	{
		return UPGRADE_ERROR_VERSION;
	}

	free(fpbuff);

	return UPGRADE_SUCCESS;
}

int CHid_Upgrade::FPGAUpgrade(CString File, CString strModeName, void *pParam)
{
	FILE *fp;
	unsigned char *fpbuff;
	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名
	CString	pos = "";
	CString str = "";
	CString FileNameStr = "";
	unsigned long int  i, j;
	unsigned long int  temp1, temp2 = 0;

	DWORD FileSize = 0;
	DWORD PackSize;
	DWORD LastPackLen;

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	if (!CameraInfo.MyDevFound)
	{
		return UPGRADE_ERROR_CONNECT;
	}

	if (File == "")
	{
		return UPGRADE_ERROR_FILE_NOT_FOUND;
	}

	_splitpath(File, NULL, NULL, FileName, FileExt);

	if (strcmp(FileExt, ".bin") != 0)
	{		
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//文件名和文件类型正确，打开文件
	fp = fopen(File, "rb");
	if (fp == NULL) {		
		fclose(fp);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	FileSize = ftell(fp); /* 得到文件大小 */

	if ((FileSize % 4096) != 0)
	{
		fpbuff = (unsigned char *)malloc(FileSize + (4096 - FileSize % 4096));
		//size + (4096 - size%4096);
	}
	else
	{
		fpbuff = (unsigned char *)malloc(FileSize); /* 根据文件大小动态分配内存空间 */
	}

	if (fpbuff == NULL)
	{
		fclose(fp);		
		return UPGRADE_ERROR_MEMORY_ERROR;
	}
	fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
	fread(fpbuff, FileSize, 1, fp); /* 一次性读取全部文件内容 */
	fclose(fp);

	//验证文件大小是否超出Flash容量
	if (FileSize > 8 * 1024 * 1024 + 4096)
	{		
		free(fpbuff);
		return UPGRADE_ERROR_FILE_SIZE;
	}

	//验证文件内容是否符合
	if (fpbuff[0] != 0xFF && fpbuff[1] != 0xFF && fpbuff[2] != 0xFF && fpbuff[3] != 0xFF \
		&& fpbuff[4] != 0xFF && fpbuff[5] != 0xFF && fpbuff[6] != 0xFF && fpbuff[7] != 0xFF)
	{		
		free(fpbuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}
	for (i = 0; i < FileSize; i++)
	{
		if (fpbuff[i] == 0)
			str += ' ';
		else
			str += fpbuff[i];
	}

	int StrFind_FX3_FPGA = str.Find("FX3_FPGA", 0);

	int file_crc_check = FpgaBinCheckFileCrc(fpbuff, FileSize);
	if (file_crc_check < 0)
	{		
		free(fpbuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}

	QueryVersion();
	if (m_MyHidDevice.HidOutputReportByteLength != HID_REPORT_LEN_V1)
	{
		return UPGRADE_ERROR_VERSION;

	}

	FileNameStr = FileName;

	if (CameraInfo.FPGAVersion.Mode == V50U && CameraInfo.FPGAVersion.Mode == V50U)
	{
		int StrFind1 = FileNameStr.Find("V50U_FPGA");
		int StrFind2 = FileNameStr.Find("V51U_FPGA");
		if (StrFind1 == -1 && StrFind2 == -1)
		{		
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				if ((IDNO == AfxMessageBox("版本可能不兼容，是否要进行升级 ?", MB_ICONEXCLAMATION | MB_YESNO)))
				{
					free(fpbuff);
					return UPGRADE_ERROR_FILE_TYPE;
				}
				break;

			case LANGUAGE_ENGLISH:
			default:
				if ((IDNO == AfxMessageBox("The version may not be compatible with this version! Whether to upgrade ?", MB_ICONEXCLAMATION | MB_YESNO)))
				{
					free(fpbuff);
					return UPGRADE_ERROR_FILE_TYPE;
				}
				break;
			}
		}

	}
	else if ((CameraInfo.USBVersion.Mode == V60U || CameraInfo.USBVersion.Mode == V7xU) || IsBootLoader())
	{
		int StrFind1 = FileNameStr.Find("V60U_FPGA");
		int StrFind2 = FileNameStr.Find("V61U_FPGA");
		int StrFind3 = FileNameStr.Find("V600U_FPGA");
		int StrFind4 = FileNameStr.Find("V610U_FPGA");
		int StrFind5 = FileNameStr.Find("Unite 200_FPGA");
		int StrFind6 = FileNameStr.Find("V70U_FPGA");
		int StrFind7 = FileNameStr.Find("V71U_FPGA");
		int StrFind8 = FileNameStr.Find("V700U_FPGA");
		int StrFind9 = FileNameStr.Find("V710U_FPGA");

		if (StrFind1 == -1 && StrFind2 == -1 && StrFind3 == -1 && StrFind4 == -1 && StrFind5 == -1
			&& StrFind6 == -1 && StrFind7 == -1 && StrFind8 == -1 && StrFind9 == -1 && StrFind_FX3_FPGA == -1)
		{
			
			free(fpbuff);
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else
	{	
		switch (Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			if ((IDNO == AfxMessageBox("版本可能不兼容，是否要进行升级 ?", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				free(fpbuff);
				return UPGRADE_ERROR_FILE_TYPE;
			}
			break;

		case LANGUAGE_ENGLISH:
		default:
			if ((IDNO == AfxMessageBox("The version may not be compatible with this version! Whether to upgrade ?", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				free(fpbuff);
				return UPGRADE_ERROR_FILE_TYPE;
			}
			break;
		}	
	}

	//开始升级FPGA
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 || m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		struct hid_file_data FileData;
		struct hid_file_info FileInfo;

		if (SendCharsCommand("go into update", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{


			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("fpga update probe", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{
			
			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}
		if (SendCharsCommand("fpga update commit", HID_HEAD_CAM_FX3, TRUE) != TRUE)
		{			
			free(fpbuff);
			return UPGRADE_ERROR_REQUEST;
		}

		if (strModeName == UPGRADE_MODE_STRING_FPGA_RESTORE)
		{
			FileInfo.file_size = FPGA_RESTORE_ADDR;
		}
		else
		{
			FileInfo.file_size = FPGA_BOOT_ADDR;
		}

		//发送文件的信息
		FileInfo.head = HID_HEAD_FILE_FX3;
		//FileInfo.file_size = 0;
		FileInfo.file_type = FILE_TYPE_FX3_FPGA;

		if (SendFileInfo(&FileInfo) == FALSE)
		{		
			free(fpbuff);
			return UPGRADE_ERROR_TRANSFER;
		}

		PackSize = FileSize / 32;
		if (FileSize % 32)
		{
			LastPackLen = FileSize % 32;
			PackSize += 1;
		}
		else
		{
			LastPackLen = 32;
		}

		FileData.head = HID_HEAD_FILE_FX3;
		FileData.pkg_size = PackSize;
		FileData.file_type = FILE_TYPE_FX3_FPGA;
		FileData.file_crc = file_crc_check;
		for (i = 0; i < PackSize; i++)
		{
			FileData.pkg_num = i;
			if (i == PackSize - 1)
			{
				for (j = 0; j < 32; j++)
				{
					if (j < LastPackLen)
						FileData.data[j] = fpbuff[i * 32 + j];
					else
						FileData.data[j] = 0;
				}
				FileData.data_size = LastPackLen;
			}
			else
			{
				for (j = 0; j < 32; j++)
				{
					FileData.data[j] = fpbuff[i * 32 + j];
				}
				FileData.data_size = 32;
			}

			if (SendFileData(&FileData) == FALSE)
			{				
				free(fpbuff);
				//exit(0);
				return UPGRADE_ERROR_TRANSFER;

			}
			else
			{
				temp1 = (100 * i / (PackSize - 1));
				if (temp2 != temp1)
				{
					pos.Format("%d%%", temp1);					

					if (Language == LANGUAGE_SIMPLIFIED_CHINESE)
						pAppDlg->DisplayStatus("正在升级FPGA固件 ..." + pos);
					else
						pAppDlg->DisplayStatus("FPGA firmware upgrading ..." + pos);

					pAppDlg->PromptSetPos(temp1);
				}
				temp2 = temp1;
			}
		}
	}
	else
	{
		return UPGRADE_ERROR_VERSION;
	}

	free(fpbuff);

	return UPGRADE_SUCCESS;
}


int CHid_Upgrade::SendFile(unsigned char FileType, CString File, void *pParam)
{
	if (FileType == FILE_TYPE_SOC_MTD || FileType == FILE_TYPE_SOC_PKG)
	{
		if (CameraInfo.StringSOCVersionMode != "")
		{
			if (File.Find(CameraInfo.StringSOCVersionMode) == -1)
				return UPGRADE_ERROR_FILE_CHECK;
		}
	}
	
	if (m_MyHidDevice.HidOutputReportByteLength == HID_REPORT_LEN_V1)
	{
		return SendFile_V1(FileType, File, pParam);

	}
	else if (m_MyHidDevice.HidOutputReportByteLength > HID_REPORT_LEN_V1)
	{
		return SendFile_V2(FileType, File, pParam);
	}
	else
	{
		return UPGRADE_ERROR_VERSION;
	}
}

int CHid_Upgrade::SendFile_V1(unsigned char FileType, CString File, void *pParam)
{
	BOOL states = TRUE;
	FILE *fp;
	unsigned char *fpbuff;
	DWORD  FileSize = 0;
	DWORD  i = 0;
	DWORD  j = 0;
	DWORD PackSize;
	DWORD LastPackLen;
	CString	pos = "";
	DWORD temp1 = 0;
	DWORD temp2 = 0;
	unsigned char cnt = 0;
	DWORD pBufferIndex = 0;

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	unsigned char file_type = FileType;
	unsigned char file_id = rand();//随机数

	struct hid_file_info FileInfo;
	struct hid_file_md5 FileMd5;
	struct hid_file_data FileData;

	int pack_data_size = m_MyHidDevice.HidOutputReportByteLength - HID_REPORT_DATA_HEAD_LEN_V1;


	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名
	char FileStr[_MAX_FNAME];

	if (!CameraInfo.MyDevFound)
	{
		return UPGRADE_ERROR_CONNECT;
	}

	if (File == "")
	{
		return UPGRADE_ERROR_FILE_NOT_FOUND;
	}

	//获取文件名和文件类型
	_splitpath(File, NULL, NULL, FileName, FileExt);

	if (file_type == FILE_TYPE_SOC_MTD)
	{
		if (strcmp(FileExt, ".img") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else if (file_type == FILE_TYPE_SOC_PKG)
	{
		if (strcmp(FileExt, ".pkg") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else if (file_type == FILE_TYPE_LINUX_FILE)
	{

	}
	else if (file_type == FILE_TYPE_SOC_DATA)
	{
		if (strcmp(FileExt, ".gz") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else
	{
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//文件名和文件类型正确，打开文件
	fp = fopen(File, "rb");
	if (fp == NULL) {		
		fclose(fp);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	FileSize = ftell(fp); /* 得到文件大小 */


	fpbuff = (unsigned char *)malloc(FileSize); /* 根据文件大小动态分配内存空间 */
	if (fpbuff == NULL)
	{
		fclose(fp);
		return UPGRADE_ERROR_MEMORY_ERROR;
	}
	fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
	fread(fpbuff, FileSize, 1, fp); /* 一次性读取全部文件内容 */
	fclose(fp);

	//验证文件内容是否符合

#if 0
	//验证文件大小是否超出Flash容量
	if (FileSize > 25 * 1024 * 1024)//25M
	{	
		return UPGRADE_ERROR_FILE_SIZE;
	}
#endif

connect_again:
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 0) == TRUE)
		{
			CameraInfo.Uvc2HidFlag = TRUE;
			while (m_MyHidDevice.FindLinuxHid() == FALSE);
			for (i = 0; i < 200; i++)
			{
				if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
					|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
				{
					Sleep(5000);
					break;
				}
				Sleep(500);
			}
			CameraInfo.Uvc2HidFlag = FALSE;
			CameraInfo.UpgradeFlag = TRUE;

		}
		else
		{
			return UPGRADE_ERROR_REQUEST;
		}

	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 0) != TRUE)
		{
			return UPGRADE_ERROR_REQUEST;
		}
		Sleep(5000);
	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 1) != TRUE)
		{
			return UPGRADE_ERROR_REQUEST;
		}
		//Sleep(1000);
	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID != DEVICE_UPGRADE_HI3516_HID)
	{		
		return UPGRADE_ERROR_VERSION;
	}



	if (!(m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID))
	{
		return UPGRADE_ERROR_VERSION;
	}

	//确认是否可以升级
	if (SendCharsCommand("update probe", HID_HEAD_CAM, 1) == FALSE)
	{
		cnt++;
		if (cnt > 3)
		{		
			return UPGRADE_ERROR_REQUEST;
		}
		Sleep(2000);
		goto connect_again;
	}

	//发送文件的信息
	FileInfo.head = HID_HEAD_FILE;
	FileInfo.file_crc = file_id;
	FileInfo.pkg_size = pack_data_size;
	FileInfo.file_size = FileSize;
	FileInfo.file_type = file_type;
	FileInfo.data_type = FILE_INFO_FILE_NAME;

	memset(FileStr, 0, sizeof(FileStr));
	memset(FileInfo.data, 0, sizeof(FileInfo.data));

	strcat(FileStr, FileName);
	strcat(FileStr, FileExt);

	if (strlen(FileStr) < pack_data_size)
		strcpy((char *)FileInfo.data, FileStr);
	else
		strcpy((char *)FileInfo.data, "File.File");


	if (SendFileInfo(&FileInfo) == FALSE)
	{
		return UPGRADE_ERROR_REQUEST;
	}


	//发送MD5值
	memset((BYTE *)&FileMd5, 0, sizeof(FileMd5));
	FileMd5.head = HID_HEAD_FILE;
	FileMd5.md5_len = 33;
	FileMd5.file_size = FileSize;
	FileMd5.file_type = file_type;
	FileMd5.file_id = file_id;

#if 1
	if (md5sum(fpbuff, FileSize, FileMd5.data) == FALSE)
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	FileMd5.data[33] = '\0';
#else
	CString strHash;
	if (GetMd5Hash(fpbuff, FileSize, strHash) == FALSE)
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	FileMd5.md5_len = strHash.GetLength();
	if (FileMd5.md5_len > sizeof(FileMd5.data))
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	for (i = 0; i < FileMd5.md5_len; i++)
	{
		FileMd5.data[i] = (unsigned char)strHash.GetAt(i);
	}
#endif
	if (SendFileMd5(&FileMd5) == FALSE)
	{		
		return UPGRADE_ERROR_REQUEST;
	}

	//开始升级
	if (SendCharsCommand("update commit", HID_HEAD_CAM, 1) == FALSE)
	{
		return UPGRADE_ERROR_REQUEST;
	}


	if (FileSize < pack_data_size)
	{
		PackSize = 1;
		LastPackLen = FileSize;
	}
	else
	{
		PackSize = FileSize / pack_data_size;
		if (FileSize % pack_data_size)
		{
			LastPackLen = FileSize % pack_data_size;
			PackSize += 1;
		}
		else
		{
			LastPackLen = pack_data_size;
		}
	}


	FileData.head = HID_HEAD_FILE;
	FileData.pkg_size = PackSize;
	FileData.file_type = file_type;
	FileData.file_crc = file_id;

	QueryPerformanceFrequency(&Frequency);

	for (i = 0; i < PackSize; i++)
	{
		FileData.pkg_num = i;
		if (i == PackSize - 1)
		{
			for (j = 0; j < pack_data_size; j++)
			{
				pBufferIndex = i * pack_data_size + j;
				if (j < LastPackLen && pBufferIndex < FileSize)
					FileData.data[j] = fpbuff[pBufferIndex];
				else
					FileData.data[j] = 0;
			}
			FileData.data_size = LastPackLen;
		}
		else
		{
			for (j = 0; j < pack_data_size; j++)
			{
				pBufferIndex = i * pack_data_size + j;
				if (pBufferIndex < FileSize)
					FileData.data[j] = fpbuff[pBufferIndex];
				else
					FileData.data[j] = 0;

			}
			FileData.data_size = pack_data_size;
		}

		if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			do {
				QueryPerformanceCounter(&StartingTime);
				ElapsedMicroseconds.QuadPart = StartingTime.QuadPart - EndingTime.QuadPart;
				ElapsedMicroseconds.QuadPart *= 1000000;//us
				ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
			} while (ElapsedMicroseconds.QuadPart <= 4000);

			QueryPerformanceCounter(&EndingTime);
		}

		if (SendFileData(&FileData) == FALSE)
		{
			return UPGRADE_ERROR_TRANSFER;
		}
		else
		{
			if (PackSize >= 2)
				temp1 = (100 * i / (PackSize - 1));
			else
				temp1 = 100;

			if (temp2 != temp1)
			{
				pos.Format("%d%%", temp1);
				
				if (Language == LANGUAGE_SIMPLIFIED_CHINESE)
					pAppDlg->DisplayStatus("文件发送中 ..." + pos);
				else
					pAppDlg->DisplayStatus("File Transmitting ..." + pos);

				pAppDlg->PromptSetPos(temp1);
			}
			temp2 = temp1;
		}
	}

	return UPGRADE_SUCCESS;
}

int CHid_Upgrade::SendFile_V2(unsigned char FileType, CString File, void *pParam)
{
	BOOL states = TRUE;
	FILE *fp;
	unsigned char *fpbuff;
	DWORD  FileSize = 0;
	DWORD  i = 0;
	DWORD  j = 0;
	DWORD PackSize;
	DWORD LastPackLen;
	CString	pos = "";
	DWORD temp1 = 0;
	DWORD temp2 = 0;
	unsigned char cnt = 0;
	DWORD pBufferIndex = 0;

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	unsigned char file_type = FileType;
	unsigned char file_id = rand();//随机数

	struct hid_file_info_v2 FileInfo;
	struct hid_file_md5_v2 FileMd5;
	struct hid_file_data_v2 FileData;

	int pack_data_size = m_MyHidDevice.HidOutputReportByteLength - HID_REPORT_DATA_HEAD_LEN_V2;


	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名
	char FileStr[_MAX_FNAME];

	if (!CameraInfo.MyDevFound)
	{
		return UPGRADE_ERROR_CONNECT;
	}

	if (File == "")
	{
		return UPGRADE_ERROR_FILE_NOT_FOUND;
	}

	//获取文件名和文件类型
	_splitpath(File, NULL, NULL, FileName, FileExt);

	if (file_type == FILE_TYPE_SOC_MTD)
	{
		if (strcmp(FileExt, ".img") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else if (file_type == FILE_TYPE_SOC_PKG)
	{
		if (strcmp(FileExt, ".pkg") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else if (file_type == FILE_TYPE_LINUX_FILE)
	{

	}
	else if (file_type == FILE_TYPE_SOC_DATA)
	{
		if (strcmp(FileExt, ".gz") != 0)
		{
			return UPGRADE_ERROR_FILE_TYPE;
		}
	}
	else
	{
		return UPGRADE_ERROR_FILE_TYPE;
	}

	//文件名和文件类型正确，打开文件
	fp = fopen(File, "rb");
	if (fp == NULL) {
		fclose(fp);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	FileSize = ftell(fp); /* 得到文件大小 */


	fpbuff = (unsigned char *)malloc(FileSize); /* 根据文件大小动态分配内存空间 */
	if (fpbuff == NULL)
	{
		fclose(fp);
		return UPGRADE_ERROR_MEMORY_ERROR;
	}
	fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
	fread(fpbuff, FileSize, 1, fp); /* 一次性读取全部文件内容 */
	fclose(fp);

	//验证文件内容是否符合

#if 0
	//验证文件大小是否超出Flash容量
	if (FileSize > 25 * 1024 * 1024)//25M
	{
		return UPGRADE_ERROR_FILE_SIZE;
	}
#endif 

connect_again:
	if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 0) == TRUE)
		{
			CameraInfo.Uvc2HidFlag = TRUE;
			while (m_MyHidDevice.FindLinuxHid() == FALSE);
			for (i = 0; i < 200; i++)
			{
				if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
					|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
				{
					Sleep(5000);
					break;
				}
				Sleep(500);
			}
			CameraInfo.Uvc2HidFlag = FALSE;
			CameraInfo.UpgradeFlag = TRUE;

		}
		else
		{
			return UPGRADE_ERROR_REQUEST;
		}

	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 0) != TRUE)
		{
			return UPGRADE_ERROR_REQUEST;
		}
		Sleep(1000);
	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (SendCharsCommand("go into update", HID_HEAD_CAM, 1) != TRUE)
		{
			return UPGRADE_ERROR_REQUEST;
		}
		//Sleep(1000);
	}
	else if (m_MyHidDevice.HIDUpgradeFirmwareID != DEVICE_UPGRADE_HI3516_HID)
	{
		return UPGRADE_ERROR_VERSION;
	}



	if (!(m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID))
	{
		return UPGRADE_ERROR_VERSION;
	}

	//确认是否可以升级
	if (SendCharsCommand("update probe", HID_HEAD_CAM, 1) == FALSE)
	{
		cnt++;
		if (cnt > 3)
		{
			return UPGRADE_ERROR_REQUEST;
		}
		Sleep(2000);
		goto connect_again;
	}

	//发送文件的信息
	FileInfo.head = HID_HEAD_FILE;
	FileInfo.file_id = file_id;
	FileInfo.pkg_size = pack_data_size;// sizeof(FileInfo.data);
	FileInfo.file_size = FileSize;
	FileInfo.file_type = file_type;
	FileInfo.data_type = FILE_INFO_FILE_NAME;

	memset(FileStr, 0, sizeof(FileStr));
	memset(FileInfo.data, 0, sizeof(FileInfo.data));

	strcat(FileStr, FileName);
	strcat(FileStr, FileExt);

	if (strlen(FileStr) < pack_data_size)
		strcpy((char *)FileInfo.data, FileStr);
	else
		strcpy((char *)FileInfo.data, "File.File");


	if (SendFileInfo_V2(&FileInfo) == FALSE)
	{
		return UPGRADE_ERROR_REQUEST;
	}


	//发送MD5值
	memset((BYTE *)&FileMd5, 0, sizeof(FileMd5));
	FileMd5.head = HID_HEAD_FILE;
	FileMd5.md5_len = 33;
	FileMd5.file_size = FileSize;
	FileMd5.file_type = file_type;
	FileMd5.file_id = file_id;

#if 1
	if (md5sum(fpbuff, FileSize, FileMd5.data) == FALSE)
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	FileMd5.data[33] = '\0';
#else
	CString strHash;
	if (GetMd5Hash(fpbuff, FileSize, strHash) == FALSE)
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	FileMd5.md5_len = strHash.GetLength();
	if(FileMd5.md5_len > sizeof(FileMd5.data))
	{
		return UPGRADE_ERROR_FILE_CHECK;
	}
	for (i = 0; i < FileMd5.md5_len; i++)
	{
		FileMd5.data[i] = (unsigned char)strHash.GetAt(i);
	}
#endif
	if (SendFileMd5_V2(&FileMd5) == FALSE)
	{
		return UPGRADE_ERROR_REQUEST;
	}

	//开始升级
	if (SendCharsCommand("update commit", HID_HEAD_CAM, 1) == FALSE)
	{
		return UPGRADE_ERROR_REQUEST;
	}


	if (FileSize < pack_data_size)
	{
		PackSize = 1;
		LastPackLen = FileSize;
	}
	else
	{
		PackSize = FileSize / pack_data_size;
		if (FileSize % pack_data_size)
		{
			LastPackLen = FileSize % pack_data_size;
			PackSize += 1;
		}
		else
		{
			LastPackLen = pack_data_size;
		}
	}

	FileData.head = HID_HEAD_FILE;
	FileData.pkg_size = PackSize;
	FileData.file_type = file_type;
	FileData.file_id = file_id;

	QueryPerformanceFrequency(&Frequency);

	for (i = 0; i < PackSize; i++)
	{
		FileData.pkg_num = i;
		if (i == PackSize - 1)
		{
			for (j = 0; j < pack_data_size; j++)
			{
				pBufferIndex = i * pack_data_size + j;
				if (j < LastPackLen && pBufferIndex < FileSize)
					FileData.data[j] = fpbuff[pBufferIndex];
				else
					FileData.data[j] = 0;
			}
			FileData.data_size = LastPackLen;
		}
		else
		{
			for (j = 0; j < pack_data_size; j++)
			{
				pBufferIndex = i * pack_data_size + j;
				if (pBufferIndex < FileSize)
					FileData.data[j] = fpbuff[pBufferIndex];
				else
					FileData.data[j] = 0;

			}
			FileData.data_size = pack_data_size;
		}

		if (m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			do {
				QueryPerformanceCounter(&StartingTime);
				ElapsedMicroseconds.QuadPart = StartingTime.QuadPart - EndingTime.QuadPart;
				ElapsedMicroseconds.QuadPart *= 1000000;//us
				ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
			} while (ElapsedMicroseconds.QuadPart <= 4000);

			QueryPerformanceCounter(&EndingTime);
		}

		if (SendFileData_V2(&FileData) == FALSE)
		{
			return UPGRADE_ERROR_TRANSFER;
		}
		else
		{
			if (PackSize >= 2)
				temp1 = (100 * i / (PackSize - 1));
			else
				temp1 = 100;

			if (temp2 != temp1)
			{
				pos.Format("%d%%", temp1);

				if (Language == LANGUAGE_SIMPLIFIED_CHINESE)
					pAppDlg->DisplayStatus("文件发送中 ..." + pos);
				else
					pAppDlg->DisplayStatus("File Transmitting ..." + pos);
	
				pAppDlg->PromptSetPos(temp1);
			}
			temp2 = temp1;
		}
	}

	return UPGRADE_SUCCESS;
}

int CHid_Upgrade::GetFile_V2(CString FileName, CString SavePath, void *pParam)
{
	struct hid_read_file_info_v2 ReadFileInfo;
	struct hid_read_file_md5_v2 ReadFileMd5;
	struct hid_read_file_data_v2 ReadFileData;
	DWORD i, FileNameLen;
	DWORD  FileSize = 0;
	BYTE FileMD5[64];
	BYTE *FileBuff;
	FILE *fp;
	CString	pos = "";
	DWORD temp1 = 0;
	DWORD temp2 = 0;

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	int pack_data_size = m_MyHidDevice.HidOutputReportByteLength - HID_REPORT_DATA_HEAD_LEN_V2;


	//1 打开/dev 下的文件，提取到BUFF
	memset(ReadFileInfo.data, 0, sizeof(ReadFileInfo.data));
	ReadFileInfo.crtl_type = READ_FILE_CRTL_TYPE_OPEN;

	FileNameLen = FileName.GetLength();
	if (FileNameLen > pack_data_size)
	{
		return UPGRADE_ERROR_FILE_OPEN;
	}
	for (i = 0; i < FileNameLen; i++)
	{
		ReadFileInfo.data[i] = (unsigned char)FileName.GetAt(i);
	}

	if (SendReadeFileInfo_V2(&ReadFileInfo) != TRUE)
	{
		return UPGRADE_ERROR_FILE_OPEN;
	}

	FileSize = ReadFileInfo.file_size;

	//2、获取MD5值
	if (GetReadFileMd5_V2(&ReadFileMd5) != TRUE)
	{
		return UPGRADE_ERROR_FILE_OPEN;
	}

	//3、获取文件数据
	FileBuff = (unsigned char *)malloc(FileSize);
	if (FileBuff == NULL)
	{
		return UPGRADE_ERROR_MEMORY_ERROR;
	}

	for (i = 0; i < FileSize; )
	{
		ReadFileData.read_start_address = i;
		ReadFileData.data_size = FileSize - ReadFileData.read_start_address;
		if(ReadFileData.data_size >= pack_data_size)
			ReadFileData.data_size = pack_data_size;

		if (GetReadFileData_V2(&ReadFileData) != TRUE)
		{
			free(FileBuff);
			return UPGRADE_ERROR_TRANSFER;
		}
		memcpy(&FileBuff[ReadFileData.read_start_address], ReadFileData.data, ReadFileData.data_size);
		i += ReadFileData.data_size;

		temp1 = (100 * i / (FileSize - 1));
		if (temp2 != temp1)
		{
			pos.Format("%d%%", temp1);

			if(Language == LANGUAGE_SIMPLIFIED_CHINESE)
				pAppDlg->DisplayStatus("文件接收中 ..." + pos);
			else
				pAppDlg->DisplayStatus("File Transmitting ..." + pos);
			pAppDlg->PromptSetPos(temp1);
		}
		temp2 = temp1;
	}

	//4、释放文件BUFF
	ReadFileInfo.crtl_type = READ_FILE_CRTL_TYPE_CLOSE;
	if (SendReadeFileInfo_V2(&ReadFileInfo) != TRUE)
	{
		free(FileBuff);
		return UPGRADE_ERROR_FILE_OPEN;
	}

	//5、校验MD5
	memset(FileMD5, 0, sizeof(FileMD5));
	if (md5sum(FileBuff, FileSize, FileMD5) == FALSE)
	{
		free(FileBuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}

	if (memcmp(ReadFileMd5.data, FileMD5, 32) != 0)
	{
		free(FileBuff);
		return UPGRADE_ERROR_FILE_CHECK;
	}


	//6、生成文件
	fp = fopen(SavePath + "\\" +FileName, "wb");
	if (fp == NULL) {
		fclose(fp);
		free(FileBuff);
		return UPGRADE_ERROR_FILE_CREATE;
	}
	if (fwrite(FileBuff, FileSize, 1, fp) != 1)
	{
		fclose(fp);
		free(FileBuff);
		return UPGRADE_ERROR_FILE_CREATE;
	}

	free(FileBuff);
	fclose(fp);

	return UPGRADE_SUCCESS;
}

int CHid_Upgrade::ResetOsdSetting(void)
{
	if (!CameraInfo.MyDevFound)
	{	
		return -1;
	}

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{	
		if (SendCharsCommand("SetResetOsdSetting", HID_HEAD_CAM_FX3, TRUE) == TRUE)
			return 0;
		else
			return -1;	
	}
	else
	{
		return -1;
	}
	return 0;
}

int CHid_Upgrade::MotorTestRotate(void)
{
	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (SendCharsCommand("SetMotorTestRotate", HID_HEAD_CAM_FX3, TRUE) == TRUE)
			return 0;
		else
			return -1;
	}
	else
	{
		return -1;
	}
	return 0;
}

UINT  CHid_Upgrade::QueryThread(void *pParam)
{

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	DWORD i, dwEvent;

	while (1)
	{
		// Wait for the thread to signal one of the event objects

		dwEvent = WaitForMultipleObjects(
			QUERY_EVENT_COUNT,     // number of objects in array
			QueryEvent,     // array of objects
			FALSE,       // wait for any object
			INFINITE);       // five-second wait

		switch (dwEvent)
		{
		case WAIT_OBJECT_0 + 0:

			while (pAppDlg->Hid_Upgrade.SendCharsCommand("GetLensCalibrationFlag", HID_HEAD_CAM_FX3, TRUE) == FALSE)
			{
				Sleep(1000);//1000毫秒
				i++;
				if (i > 100)
					break;
			}

			pAppDlg->EnableCtrl();						
			if (pAppDlg->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			{
				pAppDlg->DisplayStatus("曲线校正完成");
				AfxMessageBox(_T("曲线校正完成"), MB_ICONINFORMATION);
			}
			else
			{
				pAppDlg->DisplayStatus("The lens calibration is complete");
				AfxMessageBox(_T("The lens calibration is complete"), MB_ICONINFORMATION);
			}
			break;

		case WAIT_OBJECT_0 + 1:

			while (pAppDlg->Hid_Upgrade.SendCharsCommand("GetHotPixelsCalibrationFlag", HID_HEAD_CAM_FX3, TRUE) == FALSE)
			{
				Sleep(1000);//1000毫秒
				i++;
				if (i > 100)
					break;
			}

			pAppDlg->EnableCtrl();
			if (pAppDlg->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			{
				pAppDlg->DisplayStatus("坏点校正完成");
				AfxMessageBox(_T("坏点校正完成"), MB_ICONINFORMATION);
			}	
			else
			{
				pAppDlg->DisplayStatus("The hot pixels calibration is complete");
				AfxMessageBox(_T("The hot pixels calibration is complete"), MB_ICONINFORMATION);
			}
				
			break;

		case WAIT_TIMEOUT:
			//printf("Wait timed out.\n");
			break;

			// Return value is invalid.
		default:
			//printf("Wait error: %d\n", GetLastError()); 
			break;
		}

		for (i = 0; i < QUERY_EVENT_COUNT; i++)
			ResetEvent(QueryEvent[i]);
	}


	for (i = 0; i < QUERY_EVENT_COUNT; i++)
		CloseHandle(QueryEvent[i]);

	pQueryThread->Delete();


	return 0;
}

int CHid_Upgrade::LensCalibration(void *pParam)
{
	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (SendCharsCommand("SetLensCalibration", HID_HEAD_CAM_FX3, TRUE) == TRUE)
		{
			
			pAppDlg->DisableCtrl();
			if (pAppDlg->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
				pAppDlg->DisplayStatus("曲线校正中...");
			else
				pAppDlg->DisplayStatus("Lens Calibration ...");

			SetEvent(QueryEvent[0]);

			return 0;
		}		
		else
			return -1;
	}
	else
	{
		return -1;
	}
	return 0;
}
int CHid_Upgrade::HotPixelsCalibration(void *pParam)
{
	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (SendCharsCommand("SetHotPixelsCalibration", HID_HEAD_CAM_FX3, TRUE) == TRUE)
		{
			pAppDlg->DisableCtrl();
			if (pAppDlg->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
				pAppDlg->DisplayStatus("坏点校正中...");
			else
				pAppDlg->DisplayStatus("Hot Pixels Calibration ...");

			SetEvent(QueryEvent[1]);
			return 0;
		}
		else
			return -1;
	}
	else
	{
		return -1;
	}
	return 0;
}

int CHid_Upgrade::SetDeviceName(CString NameString, unsigned char *DeviceSettings)
{
	unsigned char StringBuff[HID_MAX_PACKETSIZE];
	unsigned char ucTxBuffer[HID_MAX_PACKETSIZE];   //发送缓冲    
	unsigned char ucRxBuffer[HID_MAX_PACKETSIZE];   //接收缓冲
	struct hid_data HidData;

	unsigned long int cnt = 0;
	unsigned long int  i, j;

	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (!CameraInfo.MyDevFound)
		{
			//MessageBox(_T(String.OnOpenDevice_error), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
			return -1;
		}

		if (CameraInfo.USBVersion.CustomCode == 'P')
		{
			if (NameString.GetLength() > 25)
			{
				//MessageBox(_T(String.Change ? "字符串长度超出范围! (<=25)" : "The length of the string is beyond range! (<=25)"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
				return -1;
			}
			else
			{
				DWORD length = NameString.GetLength();
				for (i = 0; i < length; i++)
				{
					if (CHECK_STRING_DESCR((unsigned char)NameString.GetAt(i)) == 0)
					{
						//MessageBox(_T(String.Change ? "序列号仅支持数字和字母字符!" : "The SN only support numbers and letters!"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
						return -1;
					}
				}
			}
		}
		else
		{
			if (NameString.GetLength() > 25 || NameString.GetLength() == 0)
			{
				//MessageBox(_T(String.Change ? "字符串长度超出范围! (1~25)" : "The length of the string is beyond range! (1~25)"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
				return -1;
			}
		}
		//DisableBtns();

		memset((BYTE *)&HidData, 0, HID_REPORT_LEN_V1);
		HidData.head = HID_HEAD_DATA_FX3;
		HidData.operation = HID_CAM_OPERATION_SET;
		HidData.data_type = DATA_TYPE_FX3_SETING;
		HidData.pkg_num = 0;
		HidData.data_size = 32;

		for (i = 0; i < 48; i++)
		{
			if (i < NameString.GetLength())
				StringBuff[i] = (unsigned char)NameString.GetAt(i);
			else
				StringBuff[i] = 0x00;
		}
		HidData.data[0] = 0x01;
		HidData.data[1] = DeviceSettings[0];
		HidData.data[2] = DeviceSettings[1];//pan speed
		HidData.data[3] = DeviceSettings[2];//tilt speed

		HidData.data[12] = NameString.GetLength() * 2 + 2;
		HidData.data[13] = 0x03;
		j = 14;
		for (i = 0; i < 9; i++)
		{
			HidData.data[j++] = StringBuff[i];
			HidData.data[j++] = 0x00;
		}
		if (HidDataTransfer(&HidData) != TRUE)
		{
			//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
			//EnableBtns();
			return -1;
		}

		memset((BYTE *)&HidData, 0, HID_REPORT_LEN_V1);
		HidData.head = HID_HEAD_DATA_FX3;
		HidData.operation = HID_CAM_OPERATION_SET;
		HidData.data_type = DATA_TYPE_FX3_SETING;
		HidData.pkg_num = 1;
		HidData.data_size = 32;
		j = 0;
		for (i = 0; i < 16; i++)
		{
			HidData.data[j++] = StringBuff[i + 9];
			HidData.data[j++] = 0x00;
		}
		if (HidDataTransfer(&HidData) != TRUE)
		{
			//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
			//EnableBtns();
			return -1;
		}


		//MessageBox(_T(String.Change ? "设置成功!\n重起后生效" : "Setup succeeded!\nAfter the restart to take effect"), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
		//EnableBtns();
		return 0;

	}
	else if (CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_FX3)
		return -1;

	//NameString = "PTZ Optics Camera";
	if (!CameraInfo.MyDevFound)
	{
		//MessageBox(_T(String.OnOpenDevice_error), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
		return -1;
	}

	if (NameString.GetLength() > 25 || NameString.GetLength() == 0)
	{
		//MessageBox(_T(String.Change ? "字符串长度超出范围! (1~25)" : "The length of the string is beyond range! (1~25)"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
		return -1;
	}


	for (i = 0; i < 48; i++)
	{
		if (i < NameString.GetLength())
			StringBuff[i] = (unsigned char)NameString.GetAt(i);
		else
			StringBuff[i] = 0x00;
	}

	//DisableBtns();

	ucTxBuffer[0] = 0x84;
	ucTxBuffer[1] = 0x01;

	ucTxBuffer[16] = 0x01;
	ucTxBuffer[17] = DeviceSettings[0];
	ucTxBuffer[18] = DeviceSettings[1];//pan speed
	ucTxBuffer[19] = DeviceSettings[2];//tilt speed


	cnt = 1;
	ucTxBuffer[8] = (unsigned char)(cnt & 0xFF);
	ucTxBuffer[9] = (unsigned char)((cnt & 0xFF00) >> 8);
	ucTxBuffer[10] = (unsigned char)((cnt & 0xFF0000) >> 16);
	ucTxBuffer[11] = (unsigned char)((cnt & 0xFF000000) >> 24);

	ucTxBuffer[28] = NameString.GetLength() * 2 + 2;
	ucTxBuffer[29] = 0x03;

	j = 30;
	for (i = 0; i < 9; i++)
	{
		ucTxBuffer[j++] = StringBuff[i];
		ucTxBuffer[j++] = 0x00;
	}

	if (HidSend(ucTxBuffer, ucRxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
	{
		//EnableBtns();
		return -1;
	}

	ucTxBuffer[0] = 0x84;
	ucTxBuffer[1] = 0x02;

	cnt = 2;
	ucTxBuffer[8] = (unsigned char)(cnt & 0xFF);
	ucTxBuffer[9] = (unsigned char)((cnt & 0xFF00) >> 8);
	ucTxBuffer[10] = (unsigned char)((cnt & 0xFF0000) >> 16);
	ucTxBuffer[11] = (unsigned char)((cnt & 0xFF000000) >> 24);

	j = 16;
	for (i = 0; i < 16; i++)
	{
		ucTxBuffer[j++] = StringBuff[i + 9];
		ucTxBuffer[j++] = 0x00;
	}

	if (HidSend(ucTxBuffer, ucRxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
	{
		return -1;
	}

	return 0;
}

int CHid_Upgrade::SetSNName(CString NameString, unsigned char *DeviceSettings)
{
	unsigned char StringBuff[HID_MAX_PACKETSIZE];
	struct hid_data_v2 HidData;

	unsigned long int cnt = 0;
	unsigned long int  i, j;

	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	if (NameString.GetLength() > 25)
	{
		//MessageBox(_T(String.Change ? "字符串长度超出范围! (<=25)" : "The length of the string is beyond range! (<=25)"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
		return -1;
	}
	else
	{
		DWORD length = NameString.GetLength();
		for (i = 0; i < length; i++)
		{
			if (CHECK_STRING_DESCR((unsigned char)NameString.GetAt(i)) == 0)
			{
				//MessageBox(_T(String.Change ? "序列号仅支持数字和字母字符!" : "The SN only support numbers and letters!"), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
				return -1;
			}
		}
	}

	//DisableBtns();

	memset((BYTE *)&HidData, 0, sizeof(struct hid_data_v2));
	HidData.head = HID_HEAD_DATA;
	HidData.operation = HID_CAM_OPERATION_SET;
	HidData.data_type = DATA_TYPE_FX3_SETING;
	HidData.pkg_num = 0;
	HidData.data_size = 32;
	j = 0;
	for (i = 0; i < HidData.data_size; i++)
	{
		if (i < NameString.GetLength())
			StringBuff[i] = (unsigned char)NameString.GetAt(i);
		else
			StringBuff[i] = 0x00;
	}
	
	for (i = 0; i < 16; i++)
	{
		HidData.data[j++] = StringBuff[i];
		HidData.data[j++] = 0x00;
	}
	if (HidDataTransfer_V2(&HidData) != TRUE)
	{
		//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
		//EnableBtns();
		return -1;
	}
	return 0;
}


int CHid_Upgrade::GetDeviceName(CString *NameString, unsigned char *DeviceSettings)
{
	unsigned char TxBuffer[HID_MAX_PACKETSIZE];   //发送缓冲    
	unsigned char RxBuffer[HID_MAX_PACKETSIZE];   //接收缓冲
	unsigned char DeviceNameBuffer[HID_MAX_PACKETSIZE];   //接收缓冲
	unsigned int i;
	struct hid_data HidData;

	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}
	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (!CameraInfo.MyDevFound)
		{
			//MessageBox(_T(String.OnOpenDevice_error), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
			return -1;
		}
		//DisableBtns();

		memset((BYTE *)&HidData, 0, HID_REPORT_LEN_V1);
		HidData.head = HID_HEAD_DATA_FX3;
		HidData.operation = HID_CAM_OPERATION_GET;
		HidData.data_type = DATA_TYPE_FX3_SETING;
		HidData.pkg_num = 0;
		HidData.data_size = 32;

		if (HidDataTransfer(&HidData) == TRUE)
		{

			DeviceSettings[0] = HidData.data[1];
			DeviceSettings[1] = HidData.data[2];
			DeviceSettings[2] = HidData.data[3];
		}
		else
		{
			//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
			//EnableBtns();
			return -1;
		}

		//EnableBtns();

		NameString[0] = m_MyHidDevice.GetProductString();

		return 0;

	}
	else if (CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_FX3)
		return -1;

	if (!CameraInfo.MyDevFound)
	{
		//MessageBox(_T(String.OnOpenDevice_error), _T((String.Change ? "警告" : "Warning")), MB_ICONEXCLAMATION);
		return -1;
	}
	//InfDeviceName = "";



	//DisableBtns();

	//设备名称查询
	TxBuffer[0] = 0xA0;
	TxBuffer[1] = 0x51;
	TxBuffer[2] = 0x01;
	TxBuffer[16] = 0xFF;
	TxBuffer[17] = 0x00;
	TxBuffer[18] = 0x00;
	TxBuffer[19] = 0x00;

	//写操作    
	if (m_MyHidDevice.WriteHid(TxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
	{
		//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
		//EnableBtns();
		return -1;
	}
	//读操作
	else
	{
		if (m_MyHidDevice.ReadHid(RxBuffer, m_MyHidDevice.HidOutputReportByteLength) != FALSE)
		{
			if (RxBuffer[0] == 0xA0 && RxBuffer[1] == 0x51 && RxBuffer[2] == 0x01)
			{
				for (i = 0; i < 32; i++)
				{
					DeviceNameBuffer[i] = RxBuffer[i + 16];
				}
			}
			else
			{
				//EnableBtns();
				return -1;
			}
		}
		else
		{
			//EnableBtns();
			return -1;
		}
	}

	//设备名称查询
	TxBuffer[0] = 0xA0;
	TxBuffer[1] = 0x51;
	TxBuffer[2] = 0x02;
	TxBuffer[16] = 0xFF;
	TxBuffer[17] = 0x00;
	TxBuffer[18] = 0x00;
	TxBuffer[19] = 0x00;

	//写操作    
	if (m_MyHidDevice.WriteHid(TxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
	{
		//MessageBox(_T(String.Version_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
		//EnableBtns();
		return -1;
	}
	//读操作
	else
	{
		if (m_MyHidDevice.ReadHid(RxBuffer, m_MyHidDevice.HidOutputReportByteLength) != FALSE)
		{
			if (RxBuffer[0] == 0xA0 && RxBuffer[1] == 0x51 && RxBuffer[2] == 0x02)
			{
				for (i = 0; i < 32; i++)
				{
					DeviceNameBuffer[i + 32] = RxBuffer[i + 16];
				}

				DeviceSettings[0] = DeviceNameBuffer[1];
				DeviceSettings[1] = DeviceNameBuffer[2];//pan speed
				DeviceSettings[2] = DeviceNameBuffer[3];//tilt speed

				if ((DeviceNameBuffer[0] & 0x01) == 1 && DeviceNameBuffer[13] == 0x03 && DeviceNameBuffer[0] != 0xFF)
				{
					for (i = 0; i < (DeviceNameBuffer[12] / 2 - 1); i++)
					{
						//InfDeviceName += (CString)DeviceNameBuffer[2 * i + 14];
						//if (i > 25) break;
					}
				}
				else if ((DeviceNameBuffer[0] & 0x01) != 1 && DeviceNameBuffer[0] != 0xFF)
				{
					//InfDeviceName = "HD Camera";

				}
				else
				{
					//InfDeviceName = "";//String.Change ? "设备名称未知！" : "Device name unknown!";
				}
			}
			else
			{
				//EnableBtns();
				return -1;
			}
		}
		else
		{
			//EnableBtns();
			return -1;
		}
	}


	//EnableBtns();

	NameString[0] = m_MyHidDevice.GetProductString();
	//NameString[0] = InfDeviceName;
	return 0;
}

int CHid_Upgrade::DeviceReboot(void)
{
	unsigned char TxBuffer[HID_MAX_PACKETSIZE];   //发送缓冲 
	unsigned char RxBuffer[HID_MAX_PACKETSIZE];   //接收缓冲

	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3)
	{
		//设备名称查询
		TxBuffer[0] = 0xB0;
		TxBuffer[1] = 'R';
		TxBuffer[2] = 'E';
		TxBuffer[3] = 'B';
		TxBuffer[4] = 'O';
		TxBuffer[5] = 'O';
		TxBuffer[6] = 'T';

		//写操作    
		if (m_MyHidDevice.WriteHid(TxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
		{		
			return -1;
		}
		//读操作
		else
		{
			if (m_MyHidDevice.ReadHid(RxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
			{
				//MessageBox(_T(String.HidSend_Error),_T((String.Change ? "提示" : "Prompt")),MB_ICONASTERISK);
				return -1;
			}
			else
			{
				if (DataComparison(TxBuffer, RxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
				{
					//MessageBox(_T(String.HidSend_Error),_T((String.Change ? "提示" : "Prompt")),MB_ICONASTERISK);
					return -1;
				}

			}
		}
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID)
	{
		if (SendCharsCommand("reboot", HID_HEAD_CAM, 0) == TRUE)
			return 0;
		else
			return -1;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 ||
		CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		if (SendCharsCommand("reboot", HID_HEAD_CAM_FX3, TRUE) == TRUE)
			return 0;
		else
			return -1;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (SendCharsCommand("reboot", HID_HEAD_CAM, TRUE) == TRUE)
			return 0;
		else
			return -1;
	}
	else
	{
		return -1;
	}

	return 0;
}

int CHid_Upgrade::DeviceReConnect(void)
{
	unsigned char TxBuffer[HID_MAX_PACKETSIZE];   //发送缓冲 
	unsigned char RxBuffer[HID_MAX_PACKETSIZE];   //接收缓冲

	if (!CameraInfo.MyDevFound)
	{
		return -1;
	}

	if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3)
	{
		//设备名称查询
		TxBuffer[0] = 0xC0;
		TxBuffer[1] = 'R';
		TxBuffer[2] = 'E';
		TxBuffer[3] = 'C';
		TxBuffer[4] = 'O';
		TxBuffer[5] = 'N';
		TxBuffer[6] = 'N';
		TxBuffer[7] = 'E';
		TxBuffer[8] = 'C';
		TxBuffer[9] = 'T';


		//写操作    
		if (m_MyHidDevice.WriteHid(TxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
		{
			//MessageBox(_T(String.HidSend_Error), _T((String.Change ? "提示" : "Prompt")), MB_ICONASTERISK);
			return -1;
		}
		//读操作
		else
		{
			if (m_MyHidDevice.ReadHid(RxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
			{
				//MessageBox(_T(String.HidSend_Error),_T((String.Change ? "提示" : "Prompt")),MB_ICONASTERISK);
				return -1;
			}
			else
			{
				if (DataComparison(TxBuffer, RxBuffer, m_MyHidDevice.HidOutputReportByteLength) == FALSE)
				{
					//MessageBox(_T(String.HidSend_Error),_T((String.Change ? "提示" : "Prompt")),MB_ICONASTERISK);
					return -1;
				}

			}
		}
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID)
	{
		if (SendCharsCommand("reconnect", HID_HEAD_CAM, 0))
			return 0;
		else
			return -1;
	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 ||
		CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		if (SendCharsCommand("reconnect", HID_HEAD_CAM_FX3, TRUE))
			return 0;
		else
			return -1;

	}
	else if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (SendCharsCommand("reconnect", HID_HEAD_CAM, TRUE))
			return 0;
		else
			return -1;
	}
	else
	{
		return -1;
	}

	return 0;
}

/*

USB 1.0.36开始支持PKG升级
SOC 2.13不支持USB升级PKG


V71C_G2.V_V7.2.30		// 开始启用SDK 071
V71U_G1.V_V7.2.30		// 开始启用SDK 071

V71C_G2.N_V7.2.28

*/

int CHid_Upgrade::VersionCheck(CString strModeName, CString FilePath)
{
	DWORD UsbVersion, SocVersion, FileVersion;
	DWORD StrFind_start;
	version_t File_Version = { 0,0,0 };


	char FileName[_MAX_FNAME];//文件名
	char FileExt[_MAX_EXT];//后缀名

	_splitpath(FilePath, NULL, NULL, FileName, FileExt);

	CString FileNameStr = FileName;

	UsbVersion = CameraInfo.USBVersion.BigVersion * 100 + CameraInfo.USBVersion.LittleVersion;
	SocVersion = CameraInfo.SOCVersion.BigVersion * 100 + CameraInfo.SOCVersion.LittleVersion;
	

	if (strModeName == UPGRADE_MODE_STRING_USB || strModeName == UPGRADE_MODE_STRING_USB_BOOT)
	{

	}
	else if (strModeName == UPGRADE_MODE_STRING_USB_BOOTLOADER)
	{

	}
	else if (strModeName == UPGRADE_MODE_STRING_PKG)
	{

		if (m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("当前SOC版本不支持该升级模式，请选择MTD模式进行升级！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("The current SOC version does not support the upgrade mode, Please select MTD model upgrade!"), MB_ICONINFORMATION);
				break;
			}

			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}

		if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			
			if (UsbVersion < 36)//1.0.36开始支持PKG升级
			{
				switch (Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("当前USB版本不支持该升级模式，请升级到0.36以上的版本！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("The current USB version does not support the upgrade mode, please upgrade to more than 0.36 version !"), MB_ICONINFORMATION);
					break;
				}
				return UPGRADE_ERROR_VERSION_CHECK_ERROR;
			}

			if (SocVersion <= 213)
			{
				switch (Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("当前SOC版本不支持该升级模式，请使用RS232来升级PKG！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("The current SOC version does not support the upgrade mode, Please through the RS232 port upgrade PKG!"), MB_ICONINFORMATION);
					break;
				}

				return UPGRADE_ERROR_VERSION_CHECK_ERROR;
			}
		}

		StrFind_start = FileNameStr.ReverseFind('V');
		if (StrFind_start == -1)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("获取文件版本号错误！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("Get file version error!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}
		if (String2VersionNumber(FileNameStr, StrFind_start + 1, &File_Version) != 0)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("获取文件版本号错误！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("Get file version error!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}

		if (File_Version.Mode != CameraInfo.SOCVersion.Mode)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("当前SOC版本不支持该固件升级！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("The current SOC version does not support the firmware update!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}

		FileVersion = File_Version.BigVersion * 100 + File_Version.LittleVersion;

		//71C 、71U、61U
		if (m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD)
		{
			if (SocVersion > 230 && FileVersion < 230)
			{
				switch (Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("当前SOC版本不支持通过PKG升级方式降到2.30以下的版本，请使用RS232来升级MTD！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("The current SOC version does not support upgrade to below V2.30 by the PKG upgrade mode, Please through the RS232 port upgrade MTD!"), MB_ICONINFORMATION);
					break;
				}

				return UPGRADE_ERROR_VERSION_CHECK_ERROR;
			}
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_MTD)
	{
		if (CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			if (UsbVersion < 36)//1.0.36开始支持PKG升级
			{
				switch (Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("当前USB版本不支持该升级模式，请升级到0.36以上的版本！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("The current USB version does not support the upgrade mode, please upgrade to more than 0.36 version !"), MB_ICONINFORMATION);
					break;
				}
				return UPGRADE_ERROR_VERSION_CHECK_ERROR;
			}


			if (SocVersion <= 213)
			{
				switch (Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("当前SOC版本不支持该升级模式，请使用RS232来升级PKG！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("The current SOC version does not support the upgrade mode, Please through the RS232 port upgrade PKG!"), MB_ICONINFORMATION);
					break;
				}
				return UPGRADE_ERROR_VERSION_CHECK_ERROR;
			}
		}

		StrFind_start = FileNameStr.ReverseFind('V');
		if (StrFind_start == -1)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("获取文件版本号错误！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("Get file version error!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}
		if (String2VersionNumber(FileNameStr, StrFind_start + 1, &File_Version) != 0)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("获取文件版本号错误！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("Get file version error!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}

		if (File_Version.Mode != CameraInfo.SOCVersion.Mode)
		{
			switch (Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("当前SOC版本不支持该固件升级！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				AfxMessageBox(_T("The current SOC version does not support the firmware update!"), MB_ICONINFORMATION);
				break;
			}
			return UPGRADE_ERROR_VERSION_CHECK_ERROR;
		}
	}
	else
	{

	}

	return UPGRADE_SUCCESS;
}
