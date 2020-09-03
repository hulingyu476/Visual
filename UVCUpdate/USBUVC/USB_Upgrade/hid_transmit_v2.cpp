#include "stdafx.h"
#include "config.h"
#include "hid_upgrade.h"
#include "Hid.h"
#include "Windows.h"



unsigned char CHid_Upgrade::CheckReportStatus_V2(unsigned char *buff, unsigned int Timeout_ms)
{

	struct hid_report_data_v2 *SendReportData = (struct hid_report_data_v2 *) buff;
	struct hid_report_data_v2 GetReportData;
	WORD checksum;
	
	BYTE * data = (BYTE *)&GetReportData +2;

	if(IsBootLoader())
	{
		return HID_STATUS_ACK;
	}

	if(HidGetReport((unsigned char *)&GetReportData, m_MyHidDevice.HidInputReportByteLength, Timeout_ms) == TRUE)
	{	
		checksum = cyg_crc16(data, m_MyHidDevice.HidInputReportByteLength - 2);
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

BOOL CHid_Upgrade::SendCharsCommand_V2(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms)
{
	BOOL states = TRUE;
	struct hid_report_data_v2 report_data;
	BYTE *data = (BYTE *)&report_data +2;
	unsigned char error_cnt = 0;

again:

	if(!(report_head == HID_HEAD_CAM || report_head == HID_HEAD_CAM_FX3))
	{
		return FALSE;
	}

	memset((BYTE * )&report_data, 0, m_MyHidDevice.HidOutputReportByteLength);
	report_data.head = report_head;//HID_HEAD_CAM;
	report_data.operation = HID_CAM_OPERATION_CHARS;
	strcpy((char *)report_data.data, (LPCTSTR)String);
	report_data.report_len = m_MyHidDevice.HidOutputReportByteLength;
	report_data.crc = cyg_crc16(data, report_data.report_len - 2);

	states = HidSendReport((BYTE * )&report_data,report_data.report_len);
	if(CheckStatusFlag && states == TRUE)
	{
		if(CheckReportStatus_V2((BYTE * )&report_data, Timeout_ms) == HID_STATUS_ACK)
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

//发送文件的信息
BOOL CHid_Upgrade::SendFileInfo_V2(struct hid_file_info_v2 *FileInfo)
{
	BOOL states = TRUE;
	BYTE *data = (BYTE *)FileInfo +2;
	unsigned char error_cnt = 0;

again:

	//FileInfo->head = HID_HEAD_FILE; 
	FileInfo->operation = HID_FILE_OPERATION_INFO; 
	FileInfo->report_len = m_MyHidDevice.HidOutputReportByteLength;
	FileInfo->crc = cyg_crc16(data, FileInfo->report_len - 2);
	states = HidSendReport((BYTE * )FileInfo, FileInfo->report_len);
	if(states == TRUE)
	{	
		if(CheckReportStatus_V2((BYTE * )FileInfo) == HID_STATUS_ACK)
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
BOOL CHid_Upgrade::SendFileMd5_V2(struct hid_file_md5_v2 *FileMd5)
{
	BOOL states = TRUE;

	BYTE *data = (BYTE *)FileMd5 +2;
	unsigned char error_cnt = 0;

again:
	//FileMd5->head = HID_HEAD_FILE; 
	FileMd5->operation = HID_FILE_OPERATION_MD5; 
	FileMd5->report_len = m_MyHidDevice.HidOutputReportByteLength;
	FileMd5->crc = cyg_crc16(data, FileMd5->report_len - 2);

	states = HidSendReport((BYTE *)FileMd5, FileMd5->report_len);
	if(states == TRUE)
	{	
		if(CheckReportStatus_V2((BYTE * )FileMd5) == HID_STATUS_ACK)
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
BOOL CHid_Upgrade::SendFileData_V2(struct hid_file_data_v2 *FileData)
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
	if(m_MyHidDevice.WriteHid((BYTE * )FileData, FileData->report_len)== FALSE)
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
				
			if(CheckReportStatus_V2((BYTE * )FileData, Timeout_ms) == HID_STATUS_ACK)
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
			ReportStatus = CheckReportStatus_V2((BYTE * )FileData, 300000);
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
BOOL CHid_Upgrade::HidDataTransfer_V2(struct hid_data_v2 *HidData)
{
	BOOL states = TRUE;
	WORD checksum;
	BYTE *data = (BYTE *)HidData +2;
	
	if(!(HidData->head == HID_HEAD_DATA || HidData->head == HID_HEAD_DATA_FX3))
	{
		return FALSE;
	}

	//HidData->head = HID_HEAD_DATA; 
	//HidData->operation = HID_CAM_OPERATION_SET; 
	HidData->report_len = m_MyHidDevice.HidOutputReportByteLength;
	HidData->crc = cyg_crc16(data, HidData->report_len - 2);

	if(HidData->operation == HID_CAM_OPERATION_SET)
	{
		if(HidSendReport((BYTE * )HidData, HidData->report_len)== FALSE)
		{	
			return FALSE;
		}
		else
		{
			if(CheckReportStatus_V2((BYTE * )HidData) != HID_STATUS_ACK)
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

			if(HidGetReport((BYTE * )HidData, m_MyHidDevice.HidInputReportByteLength)== FALSE)
			{
				return FALSE;
			}
			else
			{
				checksum = cyg_crc16(data, m_MyHidDevice.HidInputReportByteLength - 2);
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
unsigned char CHid_Upgrade::QueryHidStatus_V2(void)
{
	struct hid_report_data report_data;

	memset((BYTE * )&report_data, 0, m_MyHidDevice.HidOutputReportByteLength);
	report_data.head = HID_HEAD_CAM;
	report_data.operation = HID_CAM_OPERATION_CHARS;

	if(SendCharsCommand_V2("query hid status", HID_HEAD_CAM, 0) == TRUE)
	{
		return CheckReportStatus_V2((BYTE * )&report_data);
	}

	return HID_STATUS_NULL;
}


//查询版本
BOOL CHid_Upgrade::SendQueryVersion_V2(struct hid_version_v2 *version)
{
	WORD checksum;
	BYTE *data = (BYTE *)version +2;

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
		if(HidGetReport((unsigned char *)version, m_MyHidDevice.HidInputReportByteLength, 1000) == TRUE)
		{	
			checksum = cyg_crc16(data, m_MyHidDevice.HidInputReportByteLength - 2);
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

BOOL CHid_Upgrade::SendReadeFileInfo_V2(struct hid_read_file_info_v2 *ReadFileInfo)
{
	BOOL states = TRUE;

	ReadFileInfo->head = HID_HEAD_FILE;
	ReadFileInfo->operation = HID_GET_FILE_OPERATION_INFO;
	ReadFileInfo->report_len = m_MyHidDevice.HidOutputReportByteLength;
	ReadFileInfo->crc = cyg_crc16((BYTE *)ReadFileInfo + 2, ReadFileInfo->report_len - 2);
	states = HidSendReport((BYTE *)ReadFileInfo, ReadFileInfo->report_len);
	if (states == TRUE)
	{
		if (HidGetReport((unsigned char *)ReadFileInfo, m_MyHidDevice.HidInputReportByteLength,10000) == TRUE)
		{
			WORD checksum = cyg_crc16((BYTE *)ReadFileInfo + 2, m_MyHidDevice.HidInputReportByteLength - 2);
			if (ReadFileInfo->crc != checksum)
			{
				return FALSE;
			}

			if (ReadFileInfo->status != HID_STATUS_ACK)
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	return states;
}

BOOL CHid_Upgrade::GetReadFileMd5_V2(struct hid_read_file_md5_v2 *ReadFileMd5)
{
	BOOL states = TRUE;

	ReadFileMd5->head = HID_HEAD_FILE;
	ReadFileMd5->operation = HID_GET_FILE_OPERATION_MD5;
	ReadFileMd5->report_len = m_MyHidDevice.HidOutputReportByteLength;
	ReadFileMd5->crc = cyg_crc16((BYTE *)ReadFileMd5 + 2, ReadFileMd5->report_len - 2);
	states = HidSendReport((BYTE *)ReadFileMd5, ReadFileMd5->report_len);
	if (states == TRUE)
	{
		if (HidGetReport((unsigned char *)ReadFileMd5, m_MyHidDevice.HidInputReportByteLength) == TRUE)
		{
			WORD checksum = cyg_crc16((BYTE *)ReadFileMd5 + 2, m_MyHidDevice.HidInputReportByteLength - 2);
			if (ReadFileMd5->crc != checksum)
			{
				return FALSE;
			}

			if (ReadFileMd5->status != HID_STATUS_ACK)
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}


	return states;
}

BOOL CHid_Upgrade::GetReadFileData_V2(struct hid_read_file_data_v2 *ReadFileData)
{
	BOOL states = TRUE;

	ReadFileData->head = HID_HEAD_FILE;
	ReadFileData->operation = HID_GET_FILE_OPERATION_DATA;
	ReadFileData->report_len = m_MyHidDevice.HidOutputReportByteLength;
	ReadFileData->crc = cyg_crc16((BYTE *)ReadFileData + 2, ReadFileData->report_len - 2);
	states = HidSendReport((BYTE *)ReadFileData, ReadFileData->report_len);
	if (states == TRUE)
	{
		if (HidGetReport((unsigned char *)ReadFileData, m_MyHidDevice.HidInputReportByteLength) == TRUE)
		{
			WORD checksum = cyg_crc16((BYTE *)ReadFileData + 2, m_MyHidDevice.HidInputReportByteLength - 2);
			if (ReadFileData->crc != checksum)
			{
				return FALSE;
			}

			if (ReadFileData->status != HID_STATUS_ACK)
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
	return states;
}


