// CameraUpgradeDll.cpp : 定义 DLL 应用程序的导出函数。
//
//#define CAMERAUPGRADEDLL_API  _declspec(dllexport)

#include "stdafx.h"
#include "FicXuUpdate.h"
#include <Dbt.h>
#include <sys/stat.h>
#include "CameraUpgradeDll.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FW_LEN(M,N)  (((M) + (N) - 1) / (N) * (N))
//CCameraUpgradeDll cu;

CAMERAUPGRADEDLL_API int UpdateTheadIsRuning = 0;
int usb_device_insert_flag = 0;
load_info load_data_info;
HRESULT usb_load_fw(IBaseFilter *pVCap, ULONG NodeId, load_info *flash_data_info);



//silas add 
//char* FwPath="C:\\newlinecamera.bin";
UINT updatingProgress = 0;
UINT preupdatingProgress = 0;

int updateStatus = 0;

// 这是导出变量的一个示例
//CAMERAUPGRADEDLL_API int nCameraUpgradeDll=0;

// 这是导出函数的一个示例。
//CAMERAUPGRADEDLL_API int fnCameraUpgradeDll(void)
//{
//    return 42;
//}



// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 CameraUpgradeDll.h
CCameraUpgradeDll::CCameraUpgradeDll()
{
	//InitDeviceList();
	
    return;
}

BOOL CCameraUpgradeDll::InitDeviceList()
{
	index = 0;
	ZeroMemory(m_pVideoMoniker, sizeof(m_pVideoMoniker));
	ZeroMemory(m_pVCapMult, sizeof(m_pVCapMult));
	ZeroMemory(m_pNodeId, sizeof(m_pNodeId));
	
	int ret = RefreshUvcDevices(m_pVideoMoniker, m_pVCapMult, m_pNodeId);
	if (ret <= 0) {
		//can not fine uvc devices
		updateStatus = 5;
		return false;
	}

	//RefreshDeviceList();
	//UsbDetectRegister(); //hu no used tmp

	return true;
}



/************************************************************************
Desc		    : Get the IMoniker *m_pVideoMoniker from the device tree.
@ppMoniker(in)	: UVC device Moniker.
@ppVCapMult(in)	: UVC device hander.
@NodeId(in)     : Xu ID.
return		    : Quantity of device.
************************************************************************/
int CCameraUpgradeDll::RefreshUvcDevices(IMoniker **ppMoniker, IBaseFilter **ppVCapMult, ULONG *pNodeId)
{
	HRESULT hr;
	USES_CONVERSION;
	ICreateDevEnum *pCreateDevEnum = NULL;
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	if (FAILED(hr))
	{
		return -1;
	}

	UINT uIndex = 0;
	IEnumMoniker *pEm = NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, NULL);

	if (FAILED(hr)) {
		return -1;
	}

	if (pEm == NULL)
	{
		return -1;
	}

	hr = pEm->Reset();
	ULONG ulFetched = 0;
	IMoniker *pM;
	IBaseFilter *pVCapMult;

	while (hr = pEm->Next(1, &pM, &ulFetched), hr == S_OK)
	{
		IPropertyBag *pBag = NULL;
		pM->AddRef();
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"DevicePath", &var, NULL);
			if (SUCCEEDED(hr))
			{
				pBag->Release();
				SysFreeString(var.bstrVal);
				hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&pVCapMult);
				if (SUCCEEDED(hr))
				{
					pVCapMult->AddRef();
					ULONG NodeId;
					if (SUCCEEDED(FicCheckDevice(pVCapMult, &NodeId)))
					{
						ppMoniker[uIndex] = pM;
						ppVCapMult[uIndex] = pVCapMult;
						pNodeId[uIndex] = NodeId;
						
						CString ComName;
						uIndex++;
					}
					else
					{
						pVCapMult->Release();
						pM->Release();
					}
				}
				else
				{
					pM->Release();
				}
			}
			else
			{
				pM->Release();
			}

		}
		/* Can't release for use */
		//pM->Release();
	}

	pEm->Release();
	
	return uIndex;
}

/************************************************************************
Desc		    : Update device list control.
return		    : Device num.
************************************************************************/
int CCameraUpgradeDll::RefreshDeviceList()
{
	CString DeviceInfo;
	bool is_ok;

	index = 0;
	while (m_pVideoMoniker[index] != NULL)
	{
		is_ok = true;//GetDeviceInfo(DeviceInfo, m_pVideoMoniker[index]); 
		if (is_ok == true)
		{
			//m_DeviceList.InsertString(-1, DeviceInfo);
			index++;
		}
		else
		{
			break;
		}
	}
	if (index > 0)
	{
		//m_DeviceList.SetCurSel(0);
	}
	return index;
	//return 0;
}

/************************************************************************
Desc			: Read the firmware file into memory.
@file_path(in)	: Firmware path.
@mem(in)		: Firmware store buf.
return			: Firmware size.
************************************************************************/
int read_file_to_mem(char *file_path, unsigned char **mem)
{
	FILE *FP;
	struct stat statbuf;
	unsigned char *inbuffer;
	int	file_len;

	//FP = fopen(file_path, "rb");
	fopen_s(&FP,file_path, "rb");
	if (FP == NULL)
	{
		updateStatus = 1;//打开文件失败
		return 0;
	}

	//fstat(fileno(FP), &statbuf);
	fstat(_fileno(FP), &statbuf);
	file_len = statbuf.st_size;
	if (mem == NULL)
	{
		return file_len;
	}

	inbuffer = (unsigned char *)malloc(file_len);
	if (inbuffer == NULL)
	{
		fclose(FP);
		return 0;
	}
	memset(inbuffer, 0, file_len);
	fseek(FP, 0, SEEK_SET);
	fread(inbuffer, file_len, 1, FP);
	fclose(FP);
	*mem = inbuffer;
	return file_len;
}

/************************************************************************
Desc			: Free Firmware info.
@pinfo(in)	    : Firmware info.
return			: void
************************************************************************/
void free_mem(load_info *pinfo)
{
	load_info info_tmp;

	memcpy(&info_tmp, pinfo, sizeof(load_info));
	memset(pinfo, 0, sizeof(load_info));

	if (info_tmp.len)
	{
		free(info_tmp.buf);
	}
}


//UINT fic_xu_update_thread(LPVOID pParam)
unsigned int __stdcall fic_xu_update_thread(LPVOID *pParam)
{	
	load_info *pInfo = (load_info *)pParam;

	CString ComNameChild;
	ComNameChild.Format(_T("dll child PID =(%d) file_len=(%d)"), GetCurrentThreadId(),pInfo->len);
	_ftprintf_s(stdout, _T("%s\r\n"), ComNameChild);

	//CProgressCtrl *pPrgCtrl = pInfo->pPrgCtrl;
	//int PrgMin, PrgMax;

	//pPrgCtrl->GetRange(PrgMin, PrgMax);
	updatingProgress = 1;

	//Update firmware to devices
	usb_device_insert_flag = 0;
	HRESULT hr = usb_load_fw(pInfo->pVCap, pInfo->NodeId, pInfo);
	
	free_mem(pInfo);
	
	//Wait usb camera reboot.
	if (hr != S_OK)
	{
		//::SendMessage(::AfxGetMainWnd()->m_hWnd, WM_MSG_UPDATE_END, 0x02, 0);
		//::MessageBox(NULL, _T("Load Error"), _T("Error"), MB_OK);
		UpdateTheadIsRuning = 0;
		return 0;
	}

	//while (usb_device_insert_flag == 0)
	{
		//if (pPrgCtrl->GetPos() < PrgMax - 1)
		//{
		//	pPrgCtrl->StepIt(); //步长更新进度条
		//}
		Sleep(100);
		Sleep(60*1000);//delay 
	}
	//pPrgCtrl->SetPos(PrgMax);
	//::SendMessage(::AfxGetMainWnd()->m_hWnd, WM_MSG_UPDATE_END, 0x01, 0);
	//::MessageBox(NULL, _T("Load SUCCESS"), _T("Success"), MB_OK);
	UpdateTheadIsRuning = 0;
	updatingProgress = 100;
	updateStatus = 4;

	CString ComName = CString("dll SUCCESS updatingProgress thread final ");
	_ftprintf_s(stdout, _T("%s\r\n"), ComName);

	return 0;
}

/************************************************************************
Desc		         : Download firmware to device.
@pVCap(in)	         : UVC device hander.
@NodeId(in)          : Xu ID.
@flash_data_info(in) : Load data info.
return		         : S_OK for success, and others for error.
************************************************************************/
HRESULT usb_load_fw(IBaseFilter *pVCap, ULONG NodeId, load_info *flash_data_info)
{
	HRESULT hr = E_FAIL;
	int sent_byte = 0;
	UINT16 packte_unit;
	BYTE *BufFree = NULL;
	BYTE *BufTmp = NULL;
	//CProgressCtrl *pPrgCtrl = flash_data_info->pPrgCtrl;
	
	if (pVCap == NULL)
	{
		return E_HANDLE;
	}
	if ((flash_data_info == NULL) || (flash_data_info->buf == NULL))
	{
		return E_POINTER;
	}

	//get device packet
	hr = FicUpdateGetPacketUnit(pVCap, NodeId, &packte_unit);
	if (!SUCCEEDED(hr)) return hr;

	hr = FicUpdateStart(pVCap, NodeId);
	if (!SUCCEEDED(hr)) return hr;
	hr = FicUpdateSetFwLen(pVCap, NodeId, flash_data_info->len);
	if (!SUCCEEDED(hr)) return hr;
	Sleep(2);
	
	//Download by section
	while (sent_byte < flash_data_info->len)
	{
		if ((sent_byte + packte_unit) > flash_data_info->len)
		{
			BufFree = (BYTE *)malloc(packte_unit);
			if (BufFree == NULL)
			{
				hr = CO_E_INIT_MEMORY_ALLOCATOR;
				break;
			}
			memcpy(BufFree, &(flash_data_info->buf[sent_byte]), flash_data_info->len - sent_byte);
			BufTmp = BufFree;
		}
		else
		{
			BufTmp = (BYTE *)&(flash_data_info->buf[sent_byte]);
		}
		hr = FicUpdateWriteDataUnit(pVCap, NodeId, BufTmp, packte_unit);
		
		if (hr != S_OK) { 
			updateStatus = 3;//写数据失败
			break; 
		}

		sent_byte += packte_unit;
		//pPrgCtrl->SetPos(sent_byte);

		updatingProgress = (sent_byte*100) / (flash_data_info->len);
		
		if (updatingProgress > preupdatingProgress) {
			CString ComName;
			ComName.Format(_T("dll updatingProgress = (%d) sent_byte= (%d)  len=(%d)"), updatingProgress, sent_byte, flash_data_info->len);
			_ftprintf_s(stdout, _T("%s\r\n"), ComName);
			preupdatingProgress = updatingProgress;
		}

	}
	if (BufFree != NULL)
	{
		free(BufFree);
	}

	if (hr != S_OK)
	{
		return hr;
	}
	hr = FicUpdateStop(pVCap, NodeId);
	
	CString ComName;
	ComName.Format(_T("dll FicUpdateStop "));
	_ftprintf_s(stdout, _T("%s\r\n"), ComName);
	return hr;
}

 BOOL CCameraUpgradeDll::SetFwPath(char* path)
{
	UINT32 len = strlen(path);
	if (0 == len) {
		return FALSE;
	}
	memcpy(FwPath, path,len );

	return TRUE;
}

 void CCameraUpgradeDll::GetFwVersion(UINT32 *fw_ver)
{
	//UpdateData(TRUE);
	USES_CONVERSION;
	load_info load_data_info;
	//UINT32 fw_ver;

	/*if (m_EditFwPath.IsEmpty())
	{
		return;
	}*/

	memset(&load_data_info, 0, sizeof(load_data_info));
	//load_data_info.len = read_file_to_mem(T2A(m_EditFwPath), &load_data_info.buf);//Gets the firmware content and size
	//HRESULT hr = FwGetVersion(load_data_info.buf, load_data_info.len, &fw_ver);//Get firmware version

	
	load_data_info.len = read_file_to_mem(FwPath, &load_data_info.buf);//Gets the firmware content and size
	HRESULT hr = FwGetVersion(load_data_info.buf, load_data_info.len, fw_ver);//Get firmware version
	if (hr != S_OK)
	{
		free_mem(&load_data_info);
		return;
	}
	free_mem(&load_data_info);

	//m_FirmwareVersion.Format(_T("V%d.%d.%d"), (fw_ver >> 8) & 0x0F, (fw_ver >> 4) & 0x0F, fw_ver & 0x0F);
	//UpdateData(FALSE);
}
 void CCameraUpgradeDll::GetFwPlatform(char* fw_flatform)
{
	//UpdateData(TRUE);
	USES_CONVERSION;
	load_info load_data_info;
	char BufPlatFrom[64];

	/*if (m_EditFwPath.IsEmpty())
	{
		return;
	}*/
	memset(BufPlatFrom, 0, sizeof(BufPlatFrom));
	memset(&load_data_info, 0, sizeof(load_data_info));
	//load_data_info.len = read_file_to_mem(T2A(m_EditFwPath), &load_data_info.buf);//Gets the firmware content and size
	load_data_info.len = read_file_to_mem(FwPath, &load_data_info.buf);//Gets the firmware content and size
	HRESULT hr = FwGetPlatform(load_data_info.buf, load_data_info.len, BufPlatFrom);
	strncpy_s(fw_flatform, load_data_info.len+1,BufPlatFrom, load_data_info.len+1);
	

	if (hr != S_OK)
	{
		free_mem(&load_data_info);
		return;
	}
	free_mem(&load_data_info);

	//m_EditFwPlatform.Format(_T("%s"), A2T(BufPlatFrom));
	//UpdateData(FALSE);
}


 void CCameraUpgradeDll::GetDevVersion(UINT32 *dev_version)
{
	//UpdateData(TRUE);
	/*if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}*/
	//IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	//ULONG pNodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG pNodeId = m_pNodeId[index];
	UINT16 device_version;

	HRESULT hr = FicGetDeviceVersion(pVCap, pNodeId, &device_version);
	
	*dev_version = device_version;

	if (hr != S_OK)
	{
		return;
	}

	//m_DeviceVersion.Format(_T("V%d.%d.%d"), device_version >> 8, (device_version >> 4) & 0x0f, device_version & 0x0f);
	//UpdateData(FALSE);
}
 void CCameraUpgradeDll::GetDevManufacturer(char* manufacturer)
{
	//UpdateData(TRUE);
	//USES_CONVERSION;
	//if (m_DeviceList.GetCurSel() < 0)
	//{
		//MessageBox(_T("Can't Find Device."), _T("Error"));
		//return;
	//}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG NodeId = m_pNodeId[index];
	ULONG BufSize;
	char *pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_MANUFACTURER, NULL, &BufSize);
	if (hr != S_OK)
	{
		//CString StrErr;
		//StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		//MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);

	

	if (pBuf == NULL)
	{
		//MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_MANUFACTURER, pBuf, &BufSize);

	int len = strlen(pBuf);
	strcpy_s(manufacturer, (len + 1), pBuf);

	//CString ComName = CString(pBuf);
	//_ftprintf_s(stdout, _T("%s\r\n"), ComName);

	if (hr != S_OK)
	{
		//CString StrErr;
		free(pBuf);
		//StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		//MessageBox(StrErr, _T("Error"));
		return;
	}
	//m_EditDevManufacturer.Format(_T("%s"), A2T(pBuf));
	//UpdateData(FALSE);
	free(pBuf);
}
 void CCameraUpgradeDll::GetDevProduct(char* product)
{
	//UpdateData(TRUE);
	//USES_CONVERSION;
	//if (m_DeviceList.GetCurSel() < 0)
	//{
	//MessageBox(_T("Can't Find Device."), _T("Error"));
	//return;
	//}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG NodeId = m_pNodeId[index];
	ULONG BufSize;
	char *pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PRO, NULL, &BufSize);
	if (hr != S_OK)
	{
		//CString StrErr;
		//StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		//MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);
	if (pBuf == NULL)
	{
		//MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PRO, pBuf, &BufSize);

	int len = strlen(pBuf);
	strcpy_s(product, (len + 1), pBuf);
	//CString ComName =CString(pBuf);
	//_ftprintf_s(stdout, _T("%s\r\n"), ComName);

	if (hr != S_OK)
	{
		//CString StrErr;
		free(pBuf);
		//StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		//MessageBox(StrErr, _T("Error"));
		return;
	}
	//m_EditDevManufacturer.Format(_T("%s"), A2T(pBuf));
	//UpdateData(FALSE);
	free(pBuf);
}
 void CCameraUpgradeDll::GetDevPlatform(char* platform)
{
	//UpdateData(TRUE);
	//USES_CONVERSION;
	//if (m_DeviceList.GetCurSel() < 0)
	//{
	//MessageBox(_T("Can't Find Device."), _T("Error"));
	//return;
	//}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG NodeId = m_pNodeId[index];
	ULONG BufSize;
	char* pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PLATFORM, NULL, &BufSize);

	if (hr != S_OK)
	{
		//CString StrErr;
		//StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		//MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);
	if (pBuf == NULL)
	{
		//MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PLATFORM, pBuf, &BufSize);
	
	int len = strlen(pBuf);
	strcpy_s(platform,(len+1), pBuf);
	

	if (hr != S_OK)
	{
		//CString StrErr;
		free(pBuf);
		return;
	}
	//m_EditDevManufacturer.Format(_T("%s"), A2T(pBuf));
	//UpdateData(FALSE);
	free(pBuf);
	
 }
 ULONG CCameraUpgradeDll::GetDevVid()
{
	//UpdateData(TRUE);
	//if (m_DeviceList.GetCurSel() < 0)
	//{
		//MessageBox(_T("Can't Find Device."), _T("Error"));
	//	return;
	//}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG NodeId = m_pNodeId[index];
	ULONG nId;
	HRESULT hr = FicGetReg(pVCap, NodeId, YHW_REG_USB_VID, &nId);
	

	if (hr != S_OK)
	{
		return 0;
	}
	//m_EditDevUsbPid.Format(_T("%04X"), nId);
	//UpdateData(FALSE);
	return nId;
}
 ULONG CCameraUpgradeDll::GetDevPid()
 {
	 USES_CONVERSION;
	 //UpdateData(TRUE);
	 //if (m_DeviceList.GetCurSel() < 0)
	 //{
	 //MessageBox(_T("Can't Find Device."), _T("Error"));
	 //	return;
	 //}
	 IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	 ULONG NodeId =  m_pNodeId[index];
	 ULONG nId;
	 HRESULT hr = FicGetReg(pVCap, NodeId, YHW_REG_USB_PID, &nId);
	 
	 if (hr != S_OK)
	 {
		 //CString StrErr;
		 //StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		 //MessageBox(StrErr, _T("Error"));
		 
		 return 0;
	 }
	 //m_EditDevUsbPid.Format(_T("%04X"), nId);
	 //UpdateData(FALSE);
	 return nId;
 }

HRESULT CCameraUpgradeDll::AutoUpdate()
{
	CString ComName;
	if (UpdateTheadIsRuning)
	{
		ComName.Format(_T("dll UpdateTheadIsRuning"));
		_ftprintf_s(stdout, _T("%s\r\n"), ComName);
		return E_ACCESSDENIED;
	}

	//USES_CONVERSION;
	memset(&load_data_info, 0, sizeof(load_data_info));

	//Gets the VCapMult and NodeId of the current device
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[index];
	ULONG NodeId = m_pNodeId[index];
	load_data_info.NodeId = NodeId;
	load_data_info.pVCap = pVCap;
	//load_data_info.pPrgCtrl = &m_PrgUpdate;

	//Initialization device
	HRESULT hr;
	hr = FicUpdateInitDevice(pVCap, NodeId);
	if (hr != S_OK)
	{
		free_mem(&load_data_info);
		return E_FAIL;
	}

	//Reads the firmware contents into memory
	load_data_info.len = read_file_to_mem(FwPath, &load_data_info.buf);

	//TODO
	HANDLE hThread;
	//DWORD threadID;
	unsigned int threadID;
	//CWinThread *ThreadHander = AfxBeginThread(fic_xu_update_thread, &load_data_info, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
	//CreateThread(NULL, 1000, (LPTHREAD_START_ROUTINE)fic_xu_update_thread(&load_data_info), this, 1, &threadID); //TODO
	//hThread = (HANDLE)_beginthreadex(NULL, 0, &func, (LPVOID)this, 0, &threadID); //this is ok
	
	hThread = (HANDLE)_beginthreadex(0, 0, (unsigned int(__stdcall *)(void *))fic_xu_update_thread, (LPVOID)(&load_data_info), 0, 0);

	CString ComNameMain;
	ComNameMain.Format(_T("dll main Thread   PID=(%d) hThread=(%d)"), GetCurrentThreadId(), hThread);
	_ftprintf_s(stdout, _T("%s\r\n"), ComNameMain);

	//last 
	//free_mem(&load_data_info); //move to thread and delete

	return S_OK;
}
