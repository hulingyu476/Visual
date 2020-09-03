// Hid.h: interface for the CHid class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HID_H__A5700CF5_9580_4B1F_B9A6_5FC546E0EF3B__INCLUDED_)
#define AFX_HID_H__A5700CF5_9580_4B1F_B9A6_5FC546E0EF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma pack(1)//1BYTE对齐

//VHD

#define USB_VID_VHD			(0x2E7E)

#define USB_PID_JX1700U		(0x080C)


struct usb_id_info {
	USHORT  usb_vid;
	USHORT  usb_pid;
	USHORT  usb_type;
	USHORT  usb_bcd;
};


//
typedef enum
{
	DEVICE_UPGRADE_ERROR = 0,

	DEVICE_UPGRADE_TYPE_FX3,
	DEVICE_UPGRADE_FX3,
	DEVICE_UPGRADE_FX3_V1,

	DEVICE_UPGRADE_TYPE_FX3_BOOT_LOADER,
	DEVICE_UPGRADE_FX3_BOOT_LOADER,//Boot Loader
	DEVICE_UPGRADE_FX3_BOOT_LOADER_V1,//Boot Loader

	DEVICE_UPGRADE_TYPE_HISI_UVC,
	DEVICE_UPGRADE_HISI_UVC_HID,

	DEVICE_UPGRADE_TYPE_HI3516,
	DEVICE_UPGRADE_HI3516_HID,

	DEVICE_UPGRADE_TYPE_HI3519,
	DEVICE_UPGRADE_HI3519_HID,
	
	DEVICE_UPGRADE_TYPE_LINUX,
	DEVICE_UPGRADE_LINUX_HID,
} device_upgrade_firmware;



#define HID_MAX_PACKETSIZE	4096

extern "C" {

// This file is in the Windows DDK available from Microsoft.
#include "hidsdi.h"

#include <setupapi.h>
#include <dbt.h>
}

class CHid  
{
public:
	HANDLE m_ReportEvent;
	HANDLE *m_hParentWnd;
	BOOL m_bMyDeviceDetected;
	USHORT HIDUpgradeFirmwareID;
	unsigned int HidInputReportByteLength = 0;
	unsigned int HidOutputReportByteLength = 0;
	struct usb_id_info usb_id;

	BOOL FindHid(USHORT vid, USHORT pid);
	BOOL FindLinuxHid();
	USHORT GetDeviceUpgradeFirmware();
	BOOL WriteHid(unsigned char ucTxBuffer[],unsigned int ucTxLength);
	BOOL CHid::ReadHid(unsigned char ucDataBuffer[], unsigned int ucDataLength, unsigned int Timeout_ms = 5000);
	CHid(HANDLE *hParenWnd);
	CHid();
	void CloseHidDevice();

    CString GetSerialNumberString(void);
	CString GetProductString(void);	
	CString GetV61PUPackageVersionString(void);	
	CString GetSOCVersionString(void);

	virtual ~CHid();

protected:
	OVERLAPPED m_HidOverlapped;
	PHIDP_PREPARSED_DATA m_HidParsedData;
	HIDP_CAPS m_Capabilities;
	HANDLE m_hDevice;
	BOOL ReadyForWR();
	
	BOOL   m_bDeviceNoficationRegistered;
	HDEVNOTIFY m_hDevNotify;					/* notification results */

	BOOL RegisterHidDeviceNotify(/*HWND hWnd, HDEVNOTIFY hDevNotify*/);
	BOOL OpenHidDevice(HANDLE *HidDevHandle, USHORT vid, USHORT pid,int index);

};

#endif // !defined(AFX_HID_H__A5700CF5_9580_4B1F_B9A6_5FC546E0EF3B__INCLUDED_)
