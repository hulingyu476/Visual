// Hid.cpp: implementation of the CHid class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "hid.h"
#include "Windows.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern USHORT vid1 , pid1;
extern int devindex;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHid::CHid(HANDLE *hParentWnd)
{
	m_hParentWnd	= hParentWnd;
}

CHid::CHid()
{
	m_bMyDeviceDetected				= FALSE;
	m_bDeviceNoficationRegistered	= FALSE;
}

CHid::~CHid()
{
	/* end HID device plug/unplug notifications */
	UnregisterDeviceNotification( m_hDevNotify );
}



/**********************************************************
*
*	Function: OpenHidDevice
*	Purpose: tries to open a HID device based on VID and PID
*	Parameters: vid - HID device's vendor ID
*				pid - HID device's product ID
*				HidDevHandle - pointer to a handle to the HID device
*	Returns: TRUE, if device is found
*			 FALSE, if device is not found
*
**********************************************************/
BOOL CHid::OpenHidDevice(HANDLE *HidDevHandle, USHORT vid, USHORT pid,int index)
{
static GUID HidGuid;						/* HID Globally Unique ID: windows supplies us with this value */
HDEVINFO HidDevInfo;						/* handle to structure containing all attached HID Device information */
SP_DEVICE_INTERFACE_DATA devInfoData;		/* Information structure for HID devices */
BOOLEAN Result;								/* result of getting next device information structure */
DWORD Index;								/* index of HidDevInfo array entry */
int count = 0;
DWORD DataSize;								/* size of the DeviceInterfaceDetail structure */		
BOOLEAN GotRequiredSize;					/* 1-shot got device info data structure size flag */
PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;/* device info data */
DWORD RequiredSize;							/* size of device info data structure */
BOOLEAN DIDResult;							/* get device info data result */
HIDD_ATTRIBUTES HIDAttrib;					/* HID device attributes */

	Index = 0;									/* init to first index of array */
	count = 0;

again_fine_next_mi:

				/* initialize variables */
				GotRequiredSize = FALSE;
				detailData = NULL;



				/* 1) Get the HID Globally Unique ID from the OS */
				HidD_GetHidGuid(&HidGuid);


				/* 2) Get an array of structures containing information about
				all attached and enumerated HIDs */
				HidDevInfo = SetupDiGetClassDevs(	&HidGuid, 
													NULL, 
													NULL, 
													DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);


				/* 3) Step through the attached device list 1 by 1 and examine
				each of the attached devices.  When there are no more entries in
				the array of structures, the function will return FALSE. */
				

				devInfoData.cbSize = sizeof(devInfoData);	/* set to the size of the structure
															that will contain the device info data */
				
				do {
					/* Get information about the HID device with the 'Index' array entry */
					Result = SetupDiEnumDeviceInterfaces(	HidDevInfo, 
															0, 
															&HidGuid, 
															Index, 
															&devInfoData);
					
					/* If we run into this condition, then there are no more entries
					to examine, we might as well return FALSE at point */
					if(Result == FALSE)
					{
						/* free the memory allocated for DetailData */
						if(detailData != NULL)
							free(detailData);
						
						/* free HID device info list resources */
						SetupDiDestroyDeviceInfoList(HidDevInfo);
						
						return FALSE;
					}


					//if(GotRequiredSize == FALSE)
					{
						/* 3) Get the size of the DEVICE_INTERFACE_DETAIL_DATA
						structure.  The first call will return an error condition, 
						but we'll get the size of the strucure */
						DIDResult = SetupDiGetDeviceInterfaceDetail(HidDevInfo,
																	&devInfoData,
																	NULL,
																	0,
																	&DataSize,
																	NULL);
						GotRequiredSize = TRUE;

						/* allocate memory for the HidDevInfo structure */
						detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(DataSize);
						
						/* set the size parameter of the structure */
						detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					}
							
						
					/* 4) Now call the function with the correct size parameter.  This 
					function will return data from one of the array members that 
					Step #2 pointed to.  This way we can start to identify the
					attributes of particular HID devices.  */
					DIDResult = SetupDiGetDeviceInterfaceDetail(	HidDevInfo,
																&devInfoData,
																detailData,
																DataSize,
																&RequiredSize,
																NULL);
						

					/* 5) Open a file handle to the device.  Make sure the
					attibutes specify overlapped transactions or the IN
					transaction may block the input thread. */
					*HidDevHandle = CreateFile( detailData->DevicePath,
												GENERIC_READ | GENERIC_WRITE,
												FILE_SHARE_READ | FILE_SHARE_WRITE,
												(LPSECURITY_ATTRIBUTES)NULL,
												OPEN_EXISTING,
												FILE_FLAG_OVERLAPPED,
												NULL);
		
						
					/* 6) Get the Device VID & PID to see if it's the device we want */
					if(*HidDevHandle != INVALID_HANDLE_VALUE)
					{
						HIDAttrib.Size = sizeof(HIDAttrib);
						HidD_GetAttributes(	*HidDevHandle, &HIDAttrib);

						if((HIDAttrib.VendorID == vid) && (HIDAttrib.ProductID == pid))
						{
							
							if (count == index)
							{								
								/* free HID device info list resources */
								SetupDiDestroyDeviceInfoList(HidDevInfo);


								HidD_GetPreparsedData(*HidDevHandle, &m_HidParsedData);
								/* extract the capabilities info */
								HidP_GetCaps(m_HidParsedData, &m_Capabilities);
								if (m_Capabilities.FeatureReportByteLength == 0)
								{
									HidInputReportByteLength = m_Capabilities.InputReportByteLength - 1;
									HidOutputReportByteLength = m_Capabilities.OutputReportByteLength - 1;
								}
								else
								{
									HidInputReportByteLength = m_Capabilities.FeatureReportByteLength - 1;
									HidOutputReportByteLength = m_Capabilities.FeatureReportByteLength - 1;
								}
								HidD_FreePreparsedData(m_HidParsedData);

								/* free the memory allocated for DetailData */
								if (detailData != NULL)
									free(detailData);

								if (m_Capabilities.UsagePage != 0xFF00 || m_Capabilities.Usage != 0 ||
									HidInputReportByteLength > HID_MAX_PACKETSIZE || HidInputReportByteLength < 48 
									|| HidOutputReportByteLength > HID_MAX_PACKETSIZE || HidOutputReportByteLength < 48)
								{
									CloseHandle(*HidDevHandle);
									Index++;	/* increment the array index to search the next entry */
									goto again_fine_next_mi;
								}
								else
								{								
									return TRUE;	/* found HID device */
								}
									
							}
							count++;
						}
						
						/* 7) Close the Device Handle because we didn't find the device
						with the correct VID and PID */
						CloseHandle(*HidDevHandle);
					}

					free(detailData);
					detailData = NULL;

					Index++;	/* increment the array index to search the next entry */

				} while(Result == TRUE);

	/* free the memory allocated for DetailData */
	if(detailData != NULL)
		free(detailData);

	/* free HID device info list resources */
	SetupDiDestroyDeviceInfoList(HidDevInfo);

	return FALSE;

}

/**********************************************************
*
*	Function: HidDeviceNotify
*	Purpose: Sets up the HID device notification events.  The 
*			message contains the events
*			DBT_DEVICEARRIVAL and DBT_DEVICEREMOVALCOMPLETE.  It
*			is then up to the application to find out if the 
*			event is refering to the device it is connected with by
*			parsing a structure that the event points to.
*	Parameters: hWnd - handle to window that notifications should be
*				sent to.
*	Returns: TRUE, if device notification is set up
*			 FALSE, if device notification setup fails
*
**********************************************************/

BOOL CHid::RegisterHidDeviceNotify(/*HWND hWnd, HDEVNOTIFY hDevNotify*/)	//HDEVNOTIFY结构需要在StdAfx.h中定义#deinfe WINVER 0x0500 
{
	GUID HidGuid;										/* temporarily stores Windows HID Class GUID */
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;	/* un/plug notification filter */

	/* Set up device notification (i.e. plug or unplug of HID Devices) */
	
	/* 1) get the HID GUID */
	HidD_GetHidGuid(&HidGuid);
				
	/* 2) clear the notification filter */
	ZeroMemory( &NotificationFilter, sizeof(NotificationFilter));
				
	/* 3) assign the previously cleared structure with the correct data
	so that the application is notified of HID device un/plug events */
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = HidGuid;
				


	/* 4) register device notifications for this application */
	m_hDevNotify = RegisterDeviceNotification(	m_hParentWnd,
												&NotificationFilter, 
												DEVICE_NOTIFY_WINDOW_HANDLE);

	/* 5) notify the calling procedure if the HID device will not be recognized */
	if(!m_hDevNotify)
		return FALSE;

	return TRUE;
}

/**********************************************************
*
*	Function ReadHid
*	Purpose: This thread data from the hid device
*	Parameters: ucDataBuffer->the data buffer to read
*				ucDataLength->the data length to read 
*	Returns: BOOL->Readed some data
*
**********************************************************/
BOOL CHid::ReadHid(unsigned char ucDataBuffer[], unsigned int ucDataLength, unsigned int Timeout_ms)
{
//	HWND			hDlg = (HWND) lpParameter;
//	HWND			hOutputBox;
//	HWND			hTempGraph;
//	HANDLE			hDevice;
	unsigned long	numBytesReturned;
	unsigned char	inbuffer[HID_MAX_PACKETSIZE + 1];		/* input buffer*/
//	unsigned char   outbuffer[9];		/* output buffer */
	BOOL			bResult;
//	HIDP_CAPS		Capabilities;
//	PHIDP_PREPARSED_DATA		HidParsedData;
//	OVERLAPPED		HidOverlapped;
//	HANDLE			ReportEvent;
	BOOL			bSuccess		= FALSE;
//	short			temperature = 0;	/* temperature measurement */

//	char msgstring[64];

	if (ucDataLength > HidInputReportByteLength)
		ucDataLength = HidInputReportByteLength;


//	TMThreadActive = TRUE;

	/* get a handle to the dialog box controls */
//	hOutputBox = GetDlgItem (hDlg, IDC_TESTMSG);
//	hTempGraph = GetDlgItem (hDlg, IDC_TEMPGRAPH);
	
	
//	SendMessage( hOutputBox, LB_INSERTSTRING, 0, (LPARAM) "Started Temperature monitor");

	if(!m_bMyDeviceDetected)
	{
		//printf("USB设备连接失败\r");
		//SetDlgItemText(IDC_Device,"disconnect");
		return	FALSE;
	}

	/* Open the HID device handle */
	//if(OpenHidDevice( &hDevice, USB_VID, USB_PID) == TRUE)
	{
		
		
		
		
		/* get a temperature report from the HID device.  Note that
		this call to ReadFile only kicks off the reading of the report.
		If the input buffer is full then this routine will cause an
		event to happen and data will be presented immediately.  If not
		the event will occur when the HID decide provides a HID report
		on the IN endpoint. */	
		inbuffer[0] = 0;
		if (m_Capabilities.FeatureReportByteLength > 0)
		{
			bSuccess = HidD_GetFeature(m_hDevice, &inbuffer[0], ucDataLength + 1);
			if(bSuccess = TRUE)
				memcpy(ucDataBuffer, inbuffer + 1, ucDataLength);
		}
		else
		{
			bResult = ReadFile(m_hDevice,							/* handle to device */
				&inbuffer[0],						/* IN report buffer to fill */
				//m_Capabilities.InputReportByteLength,	/* input buffer size */ 
				ucDataLength + 1,						//如果ucDataLength的值为64的话,Capabilities.InputReportByteLength就为65
				&numBytesReturned,					/* returned buffer size */
				(LPOVERLAPPED)&m_HidOverlapped);	/* long pointer to an OVERLAPPED structure */

			/* wait for IN report event.  Note that if a report does not occur
			in the time set by the 'dwMilliseconds' parameter, then this function
			returns with a FALSE result and the HID device did not provide a
			report.  If the function returns TRUE, then a report did occur and the
			data is ready to be read from the buffer specified in the ReadFile
			function call.*/

			bResult = WaitForSingleObject(m_ReportEvent, Timeout_ms);//5S	
			/* if the transaction timed out, then we have to manually cancel
			the request */
			if (bResult == WAIT_TIMEOUT || bResult == WAIT_ABANDONED)
			{
				CloseHidDevice();
				FindHid(vid1, pid1);
			}

			/* Process in the input data.  Note that the first byte (i.e.
			inbuffer[0] is the report ID for the HID report.  We can just
			ignore this value.  The first data byte of interest is
			inbuffer[1] */

			if (bResult == WAIT_OBJECT_0)
			{
				//Copy data to ucDataBuffer(this parameter will return by this funciotn)
				memcpy(ucDataBuffer, inbuffer + 1, ucDataLength);
				//			printf("\n");
				bSuccess = TRUE;
			}
		}

		/* close the HID device handle */
//		CloseHandle(hDevice);
	}

//	ResetEvent(m_ReportEvent);

	return	bSuccess;
//	TMThreadActive = FALSE;
}

/**********************************************************
*
*	Function WriteAndReadHid
*	Purpose: This thread write data to hid device
*	Parameters: ucTxBuffer->the data buffer to send
*				ucTxLength->the data length to send(If zero,don't send any data) 
				ucRxBuffer->the data buffer to read
*				ucRxLength->the data length to read(If zero,don't read any data) 
*	Returns: none
*
**********************************************************/
BOOL CHid::WriteHid(unsigned char ucTxBuffer[], unsigned int ucTxLength)
{
	//	HWND			hDlg = (HWND) lpParameter;
	//	HWND			hOutputBox;
	//	HWND			hTempGraph;
	//	HANDLE			hDevice;
	unsigned long	numBytesReturned;
	//	unsigned char	inbuffer[HID_MAX_PACKETSIZE * 2];		/* input buffer*/
	unsigned char   outbuffer[HID_MAX_PACKETSIZE * 2];		/* output buffer */
	BOOL			bResult;
	//	HIDP_CAPS		Capabilities;
	//	PHIDP_PREPARSED_DATA		HidParsedData;
	//	OVERLAPPED		HidOverlapped;
	//	HANDLE			ReportEvent;
	//	short			temperature = 0;	/* temperature measurement */

	//	char msgstring[64];

	if (ucTxLength > HidOutputReportByteLength)
		ucTxLength = HidOutputReportByteLength;

	BOOL			bSuccess = TRUE;

	//	TMThreadActive = TRUE;

		/* get a handle to the dialog box controls */
	//	hOutputBox = GetDlgItem (hDlg, IDC_TESTMSG);
	//	hTempGraph = GetDlgItem (hDlg, IDC_TEMPGRAPH);

	//	SendMessage( hOutputBox, LB_INSERTSTRING, 0, (LPARAM) "Started Temperature monitor");

	if (!m_bMyDeviceDetected)
	{
		//printf("USB设备连接失败\r");
		return FALSE;
	}


	/* Open the HID device handle */
	//if(OpenHidDevice( &hDevice, USB_VID, USB_PID) == TRUE)
	{
		/* get a handle to a buffer that describes the device's capabilities.  This
		line plus the following two lines of code extract the report length the
		device is claiming to support */
		//		HidD_GetPreparsedData(hDevice, &HidParsedData);

				/* extract the capabilities info */
		//		HidP_GetCaps( HidParsedData ,&Capabilities);

				/* Free the memory allocated when getting the preparsed data */
		//		HidD_FreePreparsedData(HidParsedData);	

				/* Create a new event for report capture */
		//		ReportEvent = CreateEvent(NULL, TRUE, TRUE, "");

				/* fill the HidOverlapped structure so that Windows knows which
				event to cause when the device sends an IN report */
				//		HidOverlapped.hEvent = ReportEvent;
				//		HidOverlapped.Offset = 0;
				//		HidOverlapped.OffsetHigh = 0;

						/* Use WriteFile to send an output report to the HID device.  Firt we should
						fill the outbuffer*/
		outbuffer[0] = 0;	/* this is used as the report ID */
		//for(unsigned int i=0;i<ucTxLength;i++)		//copy data form ucDataBuffer
		//	outbuffer[i+1]	= ucTxBuffer[i];
		memcpy(outbuffer + 1, ucTxBuffer, ucTxLength);

		if (m_Capabilities.FeatureReportByteLength > 0)
		{
			bSuccess = HidD_SetFeature(m_hDevice, &outbuffer[0], ucTxLength + 1);
		}
		else
		{
			bResult = WriteFile(m_hDevice,
				&outbuffer[0],
				//m_Capabilities.OutputReportByteLength, 
				ucTxLength + 1,
				&numBytesReturned,
				(LPOVERLAPPED)&m_HidOverlapped);

			bResult = WaitForSingleObject(m_ReportEvent, 1000);

			/* if the transaction timed out, then we have to manually cancel
			the request */
			if (bResult == WAIT_TIMEOUT || bResult == WAIT_ABANDONED)
			{
				CloseHidDevice();
				FindHid(vid1, pid1);
				bSuccess = FALSE;
			}
			//ResetEvent(ReportEvent);
			/* close the HID device handle */
			//CloseHandle(hDevice);

			//ResetEvent(m_ReportEvent);

	//		TRACE("%s\n",outbuffer);
		}

		//	TMThreadActive = FALSE;

		return bSuccess;

	}
}

void CHid::CloseHidDevice()
{
	ResetEvent(m_ReportEvent);
	CancelIo(&m_hDevice);
	CloseHandle(m_hDevice);

}

//获取USB的SerialNumberString
CString CHid::GetSerialNumberString(void)
{
	char Buffer[255],i;
	CString SerialNumberString = "";

	if(HidD_GetSerialNumberString(m_hDevice ,Buffer ,255) == TRUE)
	{
		for(i = 0; i < 127; i++)
		{
			if(Buffer[2*i] == 0)
				break;
			SerialNumberString += (CString)Buffer[2*i];
		}
	}

	return (SerialNumberString);
}

//获取USB的ProductString
CString CHid::GetProductString(void)
{
	char Buffer[255],i;
	CString ProductString = "";

	if(HidD_GetProductString (m_hDevice ,Buffer ,255) == TRUE)
	{
		for(i = 0; i < 127; i++)
		{
			if(Buffer[2*i] == 0)
				break;
			ProductString += (CString)Buffer[2*i];
		}
	}

	return (ProductString);
}

CString CHid::GetV61PUPackageVersionString(void)
{
	char Buffer[255],i;
	CString VersionString = "";

	if(HidD_GetIndexedString (m_hDevice , 0x10, Buffer ,255) == TRUE)
	{
		for(i = 0; i < 127; i++)
		{
			if(Buffer[2*i] == 0)
				break;
			VersionString += (CString)Buffer[2*i];
		}
	}

	return (VersionString);	
}

CString CHid::GetSOCVersionString(void)
{
	char Buffer[255], i;
	CString VersionString = "";

#if 1 //还没支持该功能
	if ((usb_id.usb_vid == USB_VID_VHD&& usb_id.usb_bcd >= 35) || (usb_id.usb_pid == 0x0000 ))
	{
		if (HidD_GetIndexedString(m_hDevice, 0x11, Buffer, 255) == TRUE)
		{
			for (i = 0; i < 127; i++)
			{
				if (Buffer[2 * i] == 0)
					break;
				VersionString += (CString)Buffer[2 * i];
			}
		}
	}
#endif
	return (VersionString);
}



/*usb_id_info_list*/

//old id
static const struct usb_id_info usb_id_v71uc_1 = {
	0x0000,
	0x0021,
	DEVICE_UPGRADE_TYPE_FX3,
};

static const struct usb_id_info usb_id_v71uc_2 = {
	0x0000,
	0x0031,
	DEVICE_UPGRADE_TYPE_FX3,
};

static const struct usb_id_info usb_id_fx3_1 = {
	0x0000,
	0x0020,
	DEVICE_UPGRADE_TYPE_FX3,
};

static const struct usb_id_info usb_id_fx3_2 = {
	0x0000,
	0x0030,
	DEVICE_UPGRADE_TYPE_FX3,

};

static const struct usb_id_info usb_id_fx3_3 = {
	0x04B4,
	0x00F8,
	DEVICE_UPGRADE_TYPE_FX3,

};

static const struct usb_id_info usb_id_fx3_4 = {
	0x04B4,
	0x00F9,
	DEVICE_UPGRADE_TYPE_FX3,

};

//new id
static const struct usb_id_info usb_id_linux_hid = {
	0x2E7E,
	0x0000,
	DEVICE_UPGRADE_TYPE_LINUX,
};

static const struct usb_id_info usb_id_jx1700u = {
	USB_VID_VHD,
	USB_PID_JX1700U,
	DEVICE_UPGRADE_TYPE_HI3519,
};



static const struct usb_id_info usb_id_hi3516_hid_old = {
	0x6171,
	0x3516,
	DEVICE_UPGRADE_TYPE_HI3516,
};

static const struct usb_id_info usb_id_v71c_old = {
	0x0000,
	0x071C,
	DEVICE_UPGRADE_TYPE_HISI_UVC,
};

static const struct usb_id_info usb_id_v61c_old = {
	0x0000,
	0x061C,
	DEVICE_UPGRADE_TYPE_HISI_UVC,
};

static const struct usb_id_info usb_id_v71u = {
	0x2E7E,
	0x0701,
	DEVICE_UPGRADE_TYPE_FX3,
};

static const struct usb_id_info usb_id_v60u = {
	0x2E7E,
	0x0600,
	DEVICE_UPGRADE_TYPE_FX3,

};

static const struct usb_id_info usb_id_v61u = {
	0x2E7E,
	0x0601,
	DEVICE_UPGRADE_TYPE_FX3,

};


static const struct usb_id_info usb_id_v71uc = {
	0x2E7E,
	0x0707,
	DEVICE_UPGRADE_TYPE_FX3,
};

static const struct usb_id_info usb_id_v610uc = {
	0x2E7E,
	0x0603,
	DEVICE_UPGRADE_TYPE_FX3,
};



static const struct usb_id_info usb_id_fx3_boot_loader = {
	0x0000,
	0x00F0,
	DEVICE_UPGRADE_TYPE_FX3_BOOT_LOADER,
};



static const struct usb_id_info usb_id_L360V2 = {
	USB_VID_VHD,
	0x0819,
	DEVICE_UPGRADE_TYPE_HI3519,
};

static const struct usb_id_info * const usb_id_info_list[] = {
	(const struct usb_id_info *) &usb_id_linux_hid,
	(const struct usb_id_info *) &usb_id_jx1700u,


	(const struct usb_id_info *) &usb_id_hi3516_hid_old,
	(const struct usb_id_info *) &usb_id_v71c_old,
	(const struct usb_id_info *) &usb_id_v61c_old,

	(const struct usb_id_info *) &usb_id_v71uc,
	(const struct usb_id_info *) &usb_id_v610uc,

	//old id
	(const struct usb_id_info *) &usb_id_v71uc_1,
	(const struct usb_id_info *) &usb_id_v71uc_2,
	(const struct usb_id_info *) &usb_id_fx3_1,
	(const struct usb_id_info *) &usb_id_fx3_2,
	(const struct usb_id_info *) &usb_id_fx3_3,
	(const struct usb_id_info *) &usb_id_fx3_4,

	(const struct usb_id_info *) &usb_id_L360V2,

	NULL,
};





BOOL CHid::FindHid(USHORT vid,USHORT pid)
{
	HANDLE HidDevHandle = INVALID_HANDLE_VALUE;	
	BOOL bFindHid = FALSE;
	const struct usb_id_info * const *src;

	if (vid != 0 || pid !=0)
	{
		bFindHid = OpenHidDevice(&HidDevHandle, vid, pid,devindex);
		if (bFindHid)
		{
			usb_id.usb_pid = pid;
			usb_id.usb_vid = vid;			
		}
	}
	else
	{
	for (src = usb_id_info_list; *src; ++src) {	
		
			bFindHid = OpenHidDevice(&HidDevHandle, (*src)->usb_vid, (*src)->usb_pid, devindex);
			if (bFindHid)
			{
				usb_id.usb_pid = (*src)->usb_pid;
				usb_id.usb_vid = (*src)->usb_vid;
				break;
			}
		}
		
		
	}


	if(!m_bDeviceNoficationRegistered)
	{
		RegisterHidDeviceNotify(/*m_hParentWnd,m_hDevNotify*/);
		m_bDeviceNoficationRegistered		= TRUE;
	}

	if(bFindHid)
	{
//		TRACE("My Device detected.");

		

		CloseHandle(HidDevHandle);

		if (ReadyForWR() == FALSE)
		{
			CloseHidDevice();
			return FALSE;
		}
		m_bMyDeviceDetected = TRUE;
		HIDUpgradeFirmwareID = GetDeviceUpgradeFirmware();

		return TRUE;
	}
	else
	{
//		TRACE("My Device not detected.");

		//if(m_bMyDeviceDetected)
		//	CloseHandle(HidDevHandle);	//未找到指定设备，故设备指针没有用，可以释放

		m_bMyDeviceDetected	= FALSE;
		usb_id.usb_vid = 0;
		usb_id.usb_pid = 0;
		HIDUpgradeFirmwareID = 0;

		return FALSE;
	}
}

BOOL CHid::FindLinuxHid()
{
	HANDLE HidDevHandle = NULL;	//The handle of the hid device return by OpenHidDevice

	//USHORT usPid,usVid;
	BOOL bFindHid = FALSE;

	const struct usb_id_info * const *src;


	for (src = usb_id_info_list; *src; ++src) {		
		bFindHid	= OpenHidDevice(&HidDevHandle,(*src)->usb_vid,(*src)->usb_pid, devindex);
		if(bFindHid) 
		{
			if((*src)->usb_type == DEVICE_UPGRADE_TYPE_LINUX
				|| (*src)->usb_type == DEVICE_UPGRADE_TYPE_HI3516)
			{
				usb_id.usb_pid = (*src)->usb_pid;
				usb_id.usb_vid = (*src)->usb_vid;
				break;
			}
		}
		
	}



	if(!m_bDeviceNoficationRegistered)
	{
		RegisterHidDeviceNotify(/*m_hParentWnd,m_hDevNotify*/);
		m_bDeviceNoficationRegistered		= TRUE;
	}

	if(bFindHid)
	{
//		TRACE("My Device detected.");

		

		CloseHandle(HidDevHandle);

		if (ReadyForWR() == FALSE)
		{
			CloseHidDevice();
			return FALSE;
		}
		m_bMyDeviceDetected = TRUE;

		HIDUpgradeFirmwareID = GetDeviceUpgradeFirmware();

		return TRUE;
	}
	else
	{
//		TRACE("My Device not detected.");

//		if(m_bMyDeviceDetected)
//			CloseHandle(HidDevHandle);	//未找到指定设备，故设备指针没有用，可以释放

		m_bMyDeviceDetected	= FALSE;
		usb_id.usb_vid = 0;
		usb_id.usb_pid = 0;
		HIDUpgradeFirmwareID = 0;

		return FALSE;
	}


}

//获取设备型号或者升级固件的类型
USHORT CHid::GetDeviceUpgradeFirmware()
{
	struct _HIDD_ATTRIBUTES Attributes = { 0 };
	const struct usb_id_info * const *src;


	//请求获得 HID 设备的厂商 ID、产品 ID 和版本号
	HidD_GetAttributes (m_hDevice ,&Attributes);
	
	usb_id.usb_bcd = Attributes.VersionNumber;
	
	for (src = usb_id_info_list; *src; ++src) 
	{	
		if((*src)->usb_vid == Attributes.VendorID && (*src)->usb_pid == Attributes.ProductID) 
		{
			if((*src)->usb_type == DEVICE_UPGRADE_TYPE_FX3)
			{
				if(Attributes.VersionNumber == 0x0001 || Attributes.VersionNumber > 0x0002)
					return DEVICE_UPGRADE_FX3_V1;
				else if (Attributes.VersionNumber == 0x0000)
				{
					return DEVICE_UPGRADE_FX3;
				}	
				else if(Attributes.VersionNumber == 0x0002 )
					return DEVICE_UPGRADE_FX3_BOOT_LOADER_V1;
			}
			else if((*src)->usb_type == DEVICE_UPGRADE_TYPE_FX3_BOOT_LOADER)
			{
				if(Attributes.VersionNumber >= 0x0001)
					return DEVICE_UPGRADE_FX3_BOOT_LOADER_V1;
				else
					return DEVICE_UPGRADE_FX3_BOOT_LOADER;
			}
			else if((*src)->usb_type == DEVICE_UPGRADE_TYPE_HISI_UVC)
			{
				return DEVICE_UPGRADE_HISI_UVC_HID;
			}
			else if((*src)->usb_type == DEVICE_UPGRADE_TYPE_HI3516)
			{
				return DEVICE_UPGRADE_HI3516_HID;
			}
			else if((*src)->usb_type == DEVICE_UPGRADE_TYPE_HI3519)
			{
				return DEVICE_UPGRADE_HI3519_HID;
			}
			else if((*src)->usb_type == DEVICE_UPGRADE_TYPE_LINUX)
			{
				return DEVICE_UPGRADE_LINUX_HID;
			}
			else
			{
				//return DEVICE_UPGRADE_ERROR;
				continue;
			}
		}
	}


	return DEVICE_UPGRADE_ERROR;

}


BOOL CHid::ReadyForWR()
{
	OpenHidDevice( &m_hDevice, usb_id.usb_vid, usb_id.usb_pid, devindex);
/* get a handle to a buffer that describes the device's capabilities.  This
line plus the following two lines of code extract the report length the
	device is claiming to support */
	HidD_GetPreparsedData(m_hDevice, &m_HidParsedData);
	
	/* extract the capabilities info */
	HidP_GetCaps( m_HidParsedData ,&m_Capabilities);
	
	if (m_Capabilities.FeatureReportByteLength == 0)
	{
		HidInputReportByteLength = m_Capabilities.InputReportByteLength - 1;
		HidOutputReportByteLength = m_Capabilities.OutputReportByteLength - 1;
	}
	else
	{
		HidInputReportByteLength = m_Capabilities.FeatureReportByteLength - 1;
		HidOutputReportByteLength = m_Capabilities.FeatureReportByteLength - 1;
	}

	
	/* Free the memory allocated when getting the preparsed data */
	HidD_FreePreparsedData(m_HidParsedData);		
	
	/* Create a new event for report capture */
	m_ReportEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	/* fill the HidOverlapped structure so that Windows knows which
	event to cause when the device sends an IN report */
	m_HidOverlapped.hEvent = m_ReportEvent;
	m_HidOverlapped.Offset = 0;
	m_HidOverlapped.OffsetHigh = 0;

	if (HidInputReportByteLength > HID_MAX_PACKETSIZE || HidOutputReportByteLength > HID_MAX_PACKETSIZE)
		return FALSE;
	else
		return TRUE;

}

