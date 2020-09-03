#include "stdafx.h"
#include "config.h"
#include "hid_upgrade.h"
#include "Hid.h"
#include "Windows.h"



BOOL CHid_Upgrade::HidSendReport(unsigned char *buff, unsigned int PackSize)
{
	unsigned char error_cnt = 1;
	unsigned char i = 0;

#if 0
	for(i = 0; i < 3; i++)
	{
		//写操作    
		if(m_MyHidDevice.WriteHid(buff,PackSize)== FALSE)
		{	
			error_cnt ++;
			Sleep(300);
		}
		else
		{
			return TRUE;
		}
	}

	if(error_cnt >= 3)
	{
		return FALSE;
	}
#else
	//写操作    
	if (m_MyHidDevice.WriteHid(buff, PackSize) == FALSE)
	{
		FindDevice();
		return FALSE;
	}
	else
	{
		return TRUE;
	}
#endif
	return TRUE;
}

BOOL CHid_Upgrade::HidGetReport(unsigned char *buff, unsigned int PackSize, unsigned int Timeout_ms)
{
	unsigned char error_cnt = 1;
	unsigned char i = 0;

	if(IsBootLoader())
	{
		return TRUE;
	}

#if 0
	for(i = 0; i < 5; i++)
	{
		//读操作
		if(m_MyHidDevice.ReadHid(buff,PackSize,Timeout_ms)== FALSE)
		{	
			error_cnt ++;
			Sleep(200);
		}
		else
		{
			return TRUE;
		}
	}

	if(error_cnt >= 5)
	{
		return FALSE;
	}
#else
	//读操作
	if (m_MyHidDevice.ReadHid(buff, PackSize, Timeout_ms) == FALSE)
	{
		FindDevice();
		return FALSE;
	}
	else
	{
		return TRUE;
	}
#endif

	return TRUE;
}

unsigned char CHid_Upgrade::CheckReportStatus(unsigned char *buff, unsigned int Timeout_ms)
{

	struct hid_report_data *SendReportData = (struct hid_report_data *) buff;
	struct hid_report_data GetReportData;
	WORD checksum;
	
	BYTE * data = (BYTE *)&GetReportData + 2;

	if(IsBootLoader())
	{
		return HID_STATUS_ACK;
	}

	if(HidGetReport((unsigned char *)&GetReportData, m_MyHidDevice.HidOutputReportByteLength, Timeout_ms) == TRUE)
	{	
		checksum = cyg_crc16(data, m_MyHidDevice.HidOutputReportByteLength - 2);
		if(GetReportData.crc != checksum)
		{	
			return HID_STATUS_NULL;
		}
		if(GetReportData.head == SendReportData->head && GetReportData.operation == SendReportData->operation)
		{
			return GetReportData.data[0];
		}
		else
		{
			return HID_STATUS_NULL;
		}
	}
	else
	{
		return HID_STATUS_NULL;
	}	
}

BOOL CHid_Upgrade::SendCharsCommand_V1(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms)
{
	BOOL states = TRUE;
	struct hid_report_data report_data;
	BYTE *data = (BYTE * )&report_data + 2;
	unsigned char error_cnt = 0;

again:

	if(!(report_head == HID_HEAD_CAM || report_head == HID_HEAD_CAM_FX3))
	{
		return FALSE;
	}

	memset((BYTE * )&report_data, 0, HID_REPORT_LEN_V1);
	report_data.head = report_head;//HID_HEAD_CAM;
	report_data.operation = HID_CAM_OPERATION_CHARS;
	strcpy((char *)report_data.data, (LPCTSTR)String);
	report_data.report_len = m_MyHidDevice.HidOutputReportByteLength;
	report_data.crc = cyg_crc16(data, report_data.report_len - 2);

	states = HidSendReport((BYTE * )&report_data,report_data.report_len);
	if(CheckStatusFlag && states == TRUE)
	{
		if(CheckReportStatus((BYTE * )&report_data, Timeout_ms) == HID_STATUS_ACK)
		{
			return TRUE;
		}
		else
		{
			error_cnt++;
			if (error_cnt <= 3)
			{
				Sleep(100);
				goto again;
			}
			return FALSE;
		}
	}

	return states;

}

BOOL CHid_Upgrade::SendCharsCommand(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms)
{
	if (sizeof(String) > m_MyHidDevice.HidOutputReportByteLength)
		return FALSE;

	if (m_MyHidDevice.HidOutputReportByteLength == HID_REPORT_LEN_V1)
	{
		if (sizeof(String) > m_MyHidDevice.HidOutputReportByteLength - HID_REPORT_DATA_HEAD_LEN_V1)
			return FALSE;

		return SendCharsCommand_V1(String, report_head, CheckStatusFlag, Timeout_ms);

	}
	else if (m_MyHidDevice.HidOutputReportByteLength > HID_REPORT_LEN_V1)
	{
		if (sizeof(String) > m_MyHidDevice.HidOutputReportByteLength - HID_REPORT_DATA_HEAD_LEN_V2)
			return FALSE;

		return SendCharsCommand_V2(String, report_head, CheckStatusFlag, Timeout_ms);
	}
	else
	{
		return FALSE;
	}
}

//发送文件的信息
BOOL CHid_Upgrade::SendFileInfo(struct hid_file_info *FileInfo)
{
	BOOL states = TRUE;
	BYTE *data = (BYTE *)FileInfo + 2;
	unsigned char error_cnt = 0;

again:

	//FileInfo->head = HID_HEAD_FILE; `
	FileInfo->operation = HID_FILE_OPERATION_INFO; 
	FileInfo->report_len = m_MyHidDevice.HidOutputReportByteLength;
	FileInfo->crc = cyg_crc16(data, FileInfo->report_len - 2);
	states = HidSendReport((BYTE * )FileInfo, m_MyHidDevice.HidOutputReportByteLength);
	if(states == TRUE)
	{	
		if(CheckReportStatus((BYTE * )FileInfo) == HID_STATUS_ACK)
		{
			return TRUE;
		}
		else
		{
			error_cnt++;
			if (error_cnt <= 3)
			{
				Sleep(100);
				goto again;
			}
			return FALSE;
		}
	}

	return states;
}

//发送文件的MD5
BOOL CHid_Upgrade::SendFileMd5(struct hid_file_md5 *FileMd5)
{
	BOOL states = TRUE;

	BYTE *data = (BYTE * )FileMd5 + 2;
	unsigned char error_cnt = 0;

again:
	//FileMd5->head = HID_HEAD_FILE; 
	FileMd5->operation = HID_FILE_OPERATION_MD5; 
	FileMd5->report_len = m_MyHidDevice.HidOutputReportByteLength;
	FileMd5->crc = cyg_crc16(data, FileMd5->report_len - 2);

	states = HidSendReport((BYTE *)FileMd5, m_MyHidDevice.HidOutputReportByteLength);
	if(states == TRUE)
	{	
		if(CheckReportStatus((BYTE * )FileMd5) == HID_STATUS_ACK)
		{
			return TRUE;
		}
		else
		{
			error_cnt++;
			if (error_cnt <= 3)
			{
				Sleep(100);
				goto again;
			}
			return FALSE;
		}
	}

	return states;
}

//发送文件的DATA
BOOL CHid_Upgrade::SendFileData(struct hid_file_data *FileData)
{
	BOOL states = TRUE;
	unsigned char ReportStatus = HID_STATUS_ACK;
	unsigned int Timeout_ms = 5000;

	BYTE *data = (BYTE *)FileData + 2;
	unsigned char error_cnt = 0;

again:

	//FileData->head = HID_HEAD_FILE; 
	FileData->operation = HID_FILE_OPERATION_DATA; 
	FileData->report_len = m_MyHidDevice.HidOutputReportByteLength;
	FileData->crc = cyg_crc16(data, FileData->report_len - 2);

	//写操作
	if(m_MyHidDevice.WriteHid((BYTE * )FileData, m_MyHidDevice.HidOutputReportByteLength)== FALSE)
	{	
		error_cnt ++;
#if 0
		if((GetHIDUpgradeFirmwareID() == DEVICE_UPGRADE_HI3519_HID
			|| GetHIDUpgradeFirmwareID() == DEVICE_UPGRADE_LINUX_HID)&& error_cnt <= 3)
			{
				Sleep(300);
				goto again;
			}
#endif
			
				
		return FALSE;
	}
	else if(IsBootLoader())
	{
		return states;
	}
	else
	{
		if(FileData->head == HID_HEAD_FILE_FX3 || m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
			|| m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
		{
			if(FileData->pkg_num == FileData->pkg_size - 1)
				Timeout_ms = 300000;
			else
				Timeout_ms = 5000;
				
			if(CheckReportStatus((BYTE * )FileData, Timeout_ms) == HID_STATUS_ACK)
			{
				return TRUE;
			}
			else
			{
#if 0
				error_cnt ++;
				if((GetHIDUpgradeFirmwareID() == DEVICE_UPGRADE_HI3519_HID
					|| GetHIDUpgradeFirmwareID() == DEVICE_UPGRADE_LINUX_HID)&& error_cnt <= 3)
				{
					Sleep(300);
					goto again;
				}
#endif				
				return FALSE;
			}
		}
		else if(FileData->head != HID_HEAD_FILE_FX3 && FileData->pkg_num == FileData->pkg_size - 1)
		{
			ReportStatus = CheckReportStatus((BYTE * )FileData, 300000);
			if(ReportStatus == HID_STATUS_ACK)
			{
				return TRUE;
			}
			else if(ReportStatus == HID_STATUS_ERROR)
			{
				return FALSE;
			}
		}
		else
		{
			return TRUE;
		}
	}

	return states;
}

//DATA传输
BOOL CHid_Upgrade::HidDataTransfer(struct hid_data *HidData)
{
	BOOL states = TRUE;
	WORD checksum;
	BYTE *data = (BYTE *)HidData + 2;
	
	if(!(HidData->head == HID_HEAD_DATA || HidData->head == HID_HEAD_DATA_FX3))
	{
		return FALSE;
	}

	//HidData->head = HID_HEAD_DATA; 
	//HidData->operation = HID_CAM_OPERATION_SET; 
	HidData->report_len = m_MyHidDevice.HidOutputReportByteLength;
	HidData->crc = cyg_crc16(data, m_MyHidDevice.HidOutputReportByteLength - 2);

	if(HidData->operation == HID_CAM_OPERATION_SET)
	{
		if(HidSendReport((BYTE * )HidData, m_MyHidDevice.HidOutputReportByteLength)== FALSE)
		{	
			return FALSE;
		}
		else
		{
			if(CheckReportStatus((BYTE * )HidData) != HID_STATUS_ACK)
				return FALSE;
		}
	}
	else if(HidData->operation == HID_CAM_OPERATION_GET)
	{
		//写操作
		if(HidSendReport((BYTE * )HidData, m_MyHidDevice.HidOutputReportByteLength)== FALSE)
		{	
			return FALSE;
		}
		else if(IsBootLoader())
		{
			return states;
		}
		else
		{

			if(HidGetReport((BYTE * )HidData, m_MyHidDevice.HidOutputReportByteLength)== FALSE)
			{
				return FALSE;
			}
			else
			{
				checksum = cyg_crc16(data, m_MyHidDevice.HidOutputReportByteLength - 2);
				if(HidData->crc != checksum)
				{	
					return FALSE;
				}
				if((!(HidData->head == HID_HEAD_DATA || HidData->head == HID_HEAD_DATA_FX3)) || HidData->operation != HID_CAM_OPERATION_GET)
				{
					return FALSE;
				}
			
			}			
		}
	}
	else
	{
		return FALSE;
	}



	return states;
}

//查询状态
unsigned char CHid_Upgrade::QueryHidStatus(void)
{
	struct hid_report_data report_data;

	memset((BYTE * )&report_data, 0, HID_REPORT_LEN_V1);
	report_data.head = HID_HEAD_CAM;
	report_data.operation = HID_CAM_OPERATION_CHARS;

	if(SendCharsCommand("query hid status", HID_HEAD_CAM, 0) == TRUE)
	{
		return CheckReportStatus((BYTE * )&report_data);
	}

	return HID_STATUS_NULL;
}


//查询版本
BOOL CHid_Upgrade::SendQueryVersion(struct hid_version *version)
{
	WORD checksum;
	BYTE *data = (BYTE *)version + 2;

	if(IsBootLoader())
	{
		return TRUE;
	}

	//写操作    
	if(m_MyHidDevice.WriteHid((BYTE *)version, m_MyHidDevice.HidOutputReportByteLength)== FALSE)
	{	
		return FALSE;
	}
	else
	{
		if(HidGetReport((unsigned char *)version, m_MyHidDevice.HidOutputReportByteLength, 1000) == TRUE)
		{	
			checksum = cyg_crc16(data, m_MyHidDevice.HidOutputReportByteLength - 2);
			if(version->crc != checksum 
				&& !(version->head == HID_HEAD_VERSION || version->head == HID_HEAD_VERSION_FX3))
			{	
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
		else
		{
			return FALSE;
		}
		
	}

	return TRUE;
}
