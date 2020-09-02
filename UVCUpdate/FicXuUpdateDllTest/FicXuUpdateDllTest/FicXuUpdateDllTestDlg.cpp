/*Firmware download process:
1. Get the current device from the device list.
2. Initialize the device that needs to upgrade the firmware.
3. Read the contents and size of the firmware to be downloaded.
4. Download the firmware content to the device.
*/

#include "stdafx.h"
#include "FicXuUpdateDllTest.h"
#include "FicXuUpdateDllTestDlg.h"
#include "afxdialogex.h"
#include "FicXuUpdate.h"
#include <Dbt.h>
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FW_LEN(M,N)  (((M) + (N) - 1) / (N) * (N))

int UpdateTheadIsRuning = 0;
int usb_device_insert_flag = 0;
load_info load_data_info;
HRESULT usb_load_fw(IBaseFilter *pVCap, ULONG NodeId, load_info *flash_data_info);;
UINT fic_xu_update_thread(LPVOID pParam);
// CFicXuUpdateDllTestDlg

CFicXuUpdateDllTestDlg::CFicXuUpdateDllTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFicXuUpdateDllTestDlg::IDD, pParent)
	, m_EditFwPath(_T(""))
	, m_EditDevManufacturer(_T(""))
	, m_EditDevProduct(_T(""))
	, m_EditDevPlatform(_T(""))
	, m_EditFwPlatform(_T(""))
	, m_EditDevUsbVid(_T(""))
	, m_EditDevUsbPid(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFicXuUpdateDllTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DEVICE_LIST, m_DeviceList);
	DDX_Text(pDX, IDC_EDIT_FW_PATH, m_EditFwPath);
	DDX_Text(pDX, IDC_EDIT_DEV_VER, m_DeviceVersion);
	DDX_Text(pDX, IDC_EDIT_FW_VER, m_FirmwareVersion);
	DDX_Text(pDX, IDC_EDIT_DEV_MFTR, m_EditDevManufacturer);
	DDX_Text(pDX, IDC_EDIT_DEV_PRO, m_EditDevProduct);
	DDX_Text(pDX, IDC_EDIT_DEV_PTM, m_EditDevPlatform);
	DDX_Text(pDX, IDC_EDIT_FW_PTM, m_EditFwPlatform);
	DDX_Text(pDX, IDC_EDIT_DEV_VID, m_EditDevUsbVid);
	DDX_Text(pDX, IDC_EDIT_DEV_PID, m_EditDevUsbPid);
	DDX_Control(pDX, IDC_PROGRESS_UPDATE, m_PrgUpdate);
}

BEGIN_MESSAGE_MAP(CFicXuUpdateDllTestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DEVICECHANGE()
	ON_BN_CLICKED(IDC_BTN_FW_PATH, &CFicXuUpdateDllTestDlg::OnBnClickedBtnFwPath)
	ON_BN_CLICKED(IDC_BTN_VERSION, &CFicXuUpdateDllTestDlg::OnBnClickedBtnVersion)
	ON_BN_CLICKED(IDC_BTN_FW_VER, &CFicXuUpdateDllTestDlg::OnBnClickedBtnFwVer)
	ON_BN_CLICKED(IDC_BTN_UPDATE, &CFicXuUpdateDllTestDlg::OnBnClickedBtnUpdate)
	ON_BN_CLICKED(IDC_BTN_DEV_MFTR, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevMftr)
	ON_BN_CLICKED(IDC_BTN_DEV_PRO, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPro)
	ON_BN_CLICKED(IDC_BTN_DEV_PTM, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPtm)
	ON_BN_CLICKED(IDC_BTN_FW_PTM, &CFicXuUpdateDllTestDlg::OnBnClickedBtnFwPtm)
	ON_BN_CLICKED(IDC_BTN_DEV_VID, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevVid)
	ON_BN_CLICKED(IDC_BTN_DEV_PID, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPid)
	ON_MESSAGE(WM_MSG_UPDATE_END, &CFicXuUpdateDllTestDlg::MsgUpdateEndHandler)
	ON_BN_CLICKED(IDC_BTN_DEV_REBOOT, &CFicXuUpdateDllTestDlg::OnBnClickedBtnDevReboot)
END_MESSAGE_MAP()


// CFicXuUpdateDllTestDlg

BOOL CFicXuUpdateDllTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	index = 0;
	ZeroMemory(m_pVideoMoniker, sizeof(m_pVideoMoniker));
	ZeroMemory(m_pVCapMult, sizeof(m_pVCapMult));
	ZeroMemory(m_pNodeId, sizeof(m_pNodeId));

	RefreshUvcDevices(m_pVideoMoniker, m_pVCapMult, m_pNodeId);
	RefreshDeviceList();
	UsbDetectRegister();
	return TRUE;
}

void CFicXuUpdateDllTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CFicXuUpdateDllTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
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

	FP = fopen(file_path,"rb");
	if(FP == NULL) 
	{
		return 0;
	}

	fstat(fileno(FP),&statbuf);
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
	fread(inbuffer,file_len,1,FP);
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


void CFicXuUpdateDllTestDlg::DlgWindowsEnable(BOOL isEnable)
{
	((CButton *)GetDlgItem(IDC_BTN_VERSION))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_DEV_MFTR))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_DEV_PRO))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_DEV_PTM))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_DEV_VID))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_DEV_PID))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_BTN_UPDATE))->EnableWindow(isEnable);
	((CButton *)GetDlgItem(IDC_COMBO_DEVICE_LIST))->EnableWindow(isEnable);
}
/************************************************************************
Desc			: Select the firmware file to download.
return			: void.
************************************************************************/
void CFicXuUpdateDllTestDlg::OnBnClickedBtnFwPath()
{
	UpdateData(TRUE);
	CString InitPath = _T("");
	if (!m_EditFwPath.IsEmpty())
	{
		int nPos1 = m_EditFwPath.ReverseFind('/');
		int nPos2 = m_EditFwPath.ReverseFind('\\'); 
		if (nPos1 > 0)
			InitPath = m_EditFwPath.Left(nPos1 + 1);
		else if (nPos2 > 0)
			InitPath = m_EditFwPath.Left(nPos2 + 1);
	}

	int nLen = InitPath.GetLength();
	TCHAR *fileNameBuf = NULL;
	if (nLen > 0)
	{
		fileNameBuf = (TCHAR *)malloc(sizeof(TCHAR)*(nLen + 2));
		if (fileNameBuf != NULL)
		{
			memset(fileNameBuf, 0, sizeof(TCHAR)*(nLen + 2));
			nLen *= sizeof(TCHAR);
			memcpy(fileNameBuf, InitPath.GetBuffer(0), nLen);
		}
	}
	TCHAR szFilters[]= _T("Bin Files (*.bin)|*.bin|All Files (*.*)|*.*||");
	CFileDialog objFileDialog(TRUE, NULL, fileNameBuf, OFN_HIDEREADONLY, szFilters, this);
	int nRet = objFileDialog.DoModal();
	if (fileNameBuf)
	{
		free(fileNameBuf);
	}
	if(nRet != IDOK)
		return;
	CString strFileName = objFileDialog.GetPathName();
	m_EditFwPath = strFileName;
	objFileDialog.DestroyWindow();
	m_EditFwPath.Replace(CString(_T('\\')), _T("\\\\"));
	UpdateData(FALSE);
}

/************************************************************************
Desc			: Get device version.
return			: void.
************************************************************************///
void CFicXuUpdateDllTestDlg::OnBnClickedBtnVersion()
{
	UpdateData(TRUE);
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG pNodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	UINT16 device_version;

	HRESULT hr = FicGetDeviceVersion(pVCap, pNodeId, &device_version);
	if (hr != S_OK)
	{
		return;
	}

	m_DeviceVersion.Format(_T("V%d.%d.%d"), device_version >> 8, (device_version >> 4) & 0x0f, device_version & 0x0f);
	UpdateData(FALSE);
}

/************************************************************************
Desc			: Get firmware version.
return			: void.
************************************************************************/
void CFicXuUpdateDllTestDlg::OnBnClickedBtnFwVer()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	load_info load_data_info;
	UINT32 fw_ver;

	if (m_EditFwPath.IsEmpty())
	{
		return;
	}
	memset(&load_data_info, 0, sizeof(load_data_info));
	load_data_info.len = read_file_to_mem(T2A(m_EditFwPath), &load_data_info.buf);//Gets the firmware content and size
	HRESULT hr = FwGetVersion(load_data_info.buf, load_data_info.len, &fw_ver);//Get firmware version
	if (hr != S_OK)
	{
		free_mem(&load_data_info);
		return;
	}
	free_mem(&load_data_info);

	m_FirmwareVersion.Format(_T("V%d.%d.%d"), (fw_ver >> 8) & 0x0F, (fw_ver >> 4) & 0x0F, fw_ver & 0x0F);
	UpdateData(FALSE);
}

UINT fic_xu_update_thread(LPVOID pParam)
{
	load_info *pInfo = (load_info *)pParam;
	CProgressCtrl *pPrgCtrl = pInfo->pPrgCtrl;
	int PrgMin, PrgMax;

	pPrgCtrl->GetRange(PrgMin, PrgMax);
	//Update firmware to devices
	usb_device_insert_flag = 0;
	HRESULT hr = usb_load_fw(pInfo->pVCap, pInfo->NodeId, pInfo);
	free_mem(pInfo);

	//Wait usb camera reboot.
	if (hr != S_OK)
	{
		::SendMessage(::AfxGetMainWnd()->m_hWnd, WM_MSG_UPDATE_END,0x02, 0);
		//::MessageBox(NULL, _T("Load Error"), _T("Error"), MB_OK);
		UpdateTheadIsRuning = 0;
		return 0;
	}
	while(usb_device_insert_flag == 0)
	{
		if (pPrgCtrl->GetPos() < PrgMax-1)
		{
			pPrgCtrl->StepIt();
		}
		Sleep(100);
	}
	pPrgCtrl->SetPos(PrgMax);
	::SendMessage(::AfxGetMainWnd()->m_hWnd,WM_MSG_UPDATE_END,0x01, 0);
	//::MessageBox(NULL, _T("Load SUCCESS"), _T("Success"), MB_OK);
	UpdateTheadIsRuning = 0;
	return 0;
}

/************************************************************************
Desc			: Update firmware.
return			: void.
************************************************************************/
void CFicXuUpdateDllTestDlg::OnBnClickedBtnUpdate()
{
	if (UpdateTheadIsRuning)
	{
		MessageBox(_T("Update Thread is Runing."), _T("Error"));
		return;
	}
	UpdateData(TRUE);
	USES_CONVERSION;
	memset(&load_data_info, 0, sizeof(load_data_info));

	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	//Gets the VCapMult and NodeId of the current device
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	load_data_info.NodeId = NodeId;
	load_data_info.pVCap = pVCap;
	load_data_info.pPrgCtrl = &m_PrgUpdate;
	//Initialization device
	HRESULT hr;
	hr = FicUpdateInitDevice(pVCap, NodeId);
	if (hr != S_OK)
	{
		return;
	}
	//Reads the firmware contents into memory
	load_data_info.len = read_file_to_mem(T2A(m_EditFwPath), &load_data_info.buf);
	CWinThread *ThreadHander = AfxBeginThread(fic_xu_update_thread, &load_data_info, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

	if (ThreadHander != NULL)
	{
		m_PrgUpdate.SetRange32(0, load_data_info.len * 5/4);
		m_PrgUpdate.SetPos(0);
		m_PrgUpdate.SetStep(1);
		DlgWindowsEnable(false);
		UpdateTheadIsRuning  = 1;
	}
	else
	{
		ULONG ErrCode = GetLastError();
		CString ErrStr;
		ErrStr.Format(_T("Create Update Thread Error. ErrCode(%08X)"), ErrCode);
		MessageBox(ErrStr, _T("Error"));
		free_mem(&load_data_info);
		return;
	}
#if 0
	//Update firmware to devices
	usb_device_insert_flag = 0;
	usb_load_fw(pVCap, NodeId, &load_data_info);
	free_mem(&load_data_info);
	
	//Wait usb camera reboot.
	while(usb_device_insert_flag == 0)
	{
		Sleep(100);
	}
#endif
	UpdateData(FALSE);
}

LRESULT CFicXuUpdateDllTestDlg::MsgUpdateEndHandler(WPARAM w,LPARAM l)
{
	UpdateData(TRUE);
	DlgWindowsEnable(true);
	switch(w)
	{
	case 0x01:
		MessageBox(_T("Load Success"), _T("SUC"));
		break;
	case 0x02:
		MessageBox(_T("Load Error"), _T("ERROR"));
		break;
	}
	UpdateData(FALSE);
	return S_OK;
}

/************************************************************************
Desc		    : Get the IMoniker *m_pVideoMoniker from the device tree.
@ppMoniker(in)	: UVC device Moniker.
@ppVCapMult(in)	: UVC device hander.
@NodeId(in)     : Xu ID.
return		    : Quantity of device.
************************************************************************/
int CFicXuUpdateDllTestDlg::RefreshUvcDevices(IMoniker **ppMoniker, IBaseFilter **ppVCapMult, ULONG *pNodeId)
{
	HRESULT hr;

	ICreateDevEnum *pCreateDevEnum = NULL;
	CoInitialize(NULL);
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,IID_ICreateDevEnum, (void**)&pCreateDevEnum);

	if(FAILED(hr)) 
	{
		return -1;
	}

	UINT uIndex = 0;
	IEnumMoniker *pEm=NULL;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, NULL);

	if(FAILED(hr)) {
		return -1;
	}

	if(pEm == NULL)
	{
		return -1;
	}

	hr = pEm->Reset();
	ULONG ulFetched = 0;
	IMoniker *pM;
	IBaseFilter *pVCapMult;

	while(hr = pEm->Next(1, &pM, &ulFetched), hr==S_OK)
	{
		IPropertyBag *pBag=NULL;
		pM->AddRef();
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"DevicePath", &var, NULL);
			if(SUCCEEDED(hr))
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
Desc		    : Get the IMoniker *m_pVideoMoniker from the device tree.
@strInfo(in)	: Device info string.
@pmVideo(in)	: UVC device Moniker.
return		    : true for success, and false for error.
************************************************************************/
bool CFicXuUpdateDllTestDlg::GetDeviceInfo(CString &strInfo, IMoniker *pmVideo)
{
	HRESULT hr = E_FAIL;

	if(pmVideo != 0)
	{
		IPropertyBag *pBag;

		hr = pmVideo->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr))
		{
			{
				VARIANT var;
				var.vt = VT_BSTR;

				hr = pBag->Read(L"FriendlyName", &var, NULL);
				if(hr == NOERROR)
				{
					WCHAR wachFriendlyName[256];
					memset(wachFriendlyName, 0,sizeof(wachFriendlyName));
					lstrcpyW(wachFriendlyName, var.bstrVal);
					SysFreeString(var.bstrVal);
					CString str = CString(wachFriendlyName);

					strInfo = str;
				}
			}
			{
				VARIANT var;
				var.vt = VT_BSTR;

				hr = pBag->Read(L"DevicePath", &var, NULL); //Description  DevicePath   FriendlyName
				if(hr == NOERROR)
				{
					WCHAR wachFriendlyName[256];
					memset(wachFriendlyName, 0,sizeof(wachFriendlyName));
					lstrcpyW(wachFriendlyName, var.bstrVal);
					SysFreeString(var.bstrVal);
					CString str = CString(wachFriendlyName);

					str.MakeUpper();

					int pos = 0;
					pos = str.Find(_T("VID"),pos);
					if (pos >= 0)
					{
						str = str.Mid(pos, 17);
					}
					strInfo += "    ";
					strInfo += str;
				}
			}
			pBag->Release();
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

/************************************************************************
Desc		    : Update device list control.
return		    : Device num.
************************************************************************/
int CFicXuUpdateDllTestDlg::RefreshDeviceList()
{
	CString DeviceInfo;
	bool is_ok;

	index = 0;
	while (m_pVideoMoniker[index] != NULL)
	{
		is_ok = GetDeviceInfo(DeviceInfo, m_pVideoMoniker[index]);
		if (is_ok == true)
		{
			m_DeviceList.InsertString(-1, DeviceInfo);
			index++;
		}
		else
		{
			break;
		}
	}
	if (index > 0)
	{
		m_DeviceList.SetCurSel(0);
	}
	return index;
}

/************************************************************************
Desc		    : Update device list.
return		    : Device num.
************************************************************************/
void CFicXuUpdateDllTestDlg::uvc_device_refresh(void)
{
	m_DeviceList.ResetContent();
	int i;
	for (i = 0; i < 10; i++)
	{
		if (m_pVCapMult[i] != NULL)
		{
			m_pVideoMoniker[i]->Release();
			m_pVideoMoniker[i] = NULL;
			m_pVCapMult[i]->Release();
			m_pVCapMult[i] = NULL;
			m_pNodeId[i] = 0;
		}
	}
	RefreshUvcDevices(m_pVideoMoniker, m_pVCapMult, m_pNodeId);
	RefreshDeviceList();
	UpdateData(FALSE);
}

/************************************************************************
Desc		    : Usb plug leave handler.
@nEventType(in)	: State of the usb.
@dwData(in)	    : Usb device type .
return		    : TURE for success, and FALSE for error.
************************************************************************/
BOOL CFicXuUpdateDllTestDlg::OnDeviceChange(UINT nEventType, DWORD dwData) 
{
	PDEV_BROADCAST_DEVICEINTERFACE pDbt; 

	pDbt = (PDEV_BROADCAST_DEVICEINTERFACE)dwData;
	CString str = _T("");
	switch (nEventType)
	{
	case DBT_DEVICEARRIVAL :  //Insert USB cable to device
		if (DBT_DEVTYP_DEVICEINTERFACE == pDbt->dbcc_devicetype)
		{
			uvc_device_refresh();
			if (m_DeviceList.GetCount() > 0)
			{
				usb_device_insert_flag = 1;
			}
		}
		break;
	case DBT_DEVICEREMOVECOMPLETE :   //Usb device unplug
		if (DBT_DEVTYP_DEVICEINTERFACE == pDbt->dbcc_devicetype)
		{
			uvc_device_refresh();
		}
		break;  
	default :
		break;
	}
	return TRUE;
}

/************************************************************************
Desc		    : Register for USB plug events.
return		    : void.
************************************************************************/
void CFicXuUpdateDllTestDlg::UsbDetectRegister()
{
	static const GUID GUID_DEVINTERFACE_LIST[] =
	{
		//camera
		{ 0x65e8773d, 0x8f56, 0x11d0, { 0xa3, 0xb9, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96 } },
	};

	//Register for USB plug events  
	HDEVNOTIFY hDevNotify;  
	DEV_BROADCAST_DEVICEINTERFACE NotifacationFiler;  
	ZeroMemory(&NotifacationFiler,sizeof(DEV_BROADCAST_DEVICEINTERFACE));  
	NotifacationFiler.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);  
	NotifacationFiler.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;  

	for(int i=0;i<sizeof(GUID_DEVINTERFACE_LIST)/sizeof(GUID);i++)  
	{  
		NotifacationFiler.dbcc_classguid = GUID_DEVINTERFACE_LIST[i];//GetCurrentUSBGUID();//m_usb->GetDriverGUID();  

		hDevNotify = RegisterDeviceNotification(this->m_hWnd,&NotifacationFiler,DEVICE_NOTIFY_WINDOW_HANDLE);  
		if(!hDevNotify)  
		{  
			int Err = GetLastError();    
		}  
	}
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
	CProgressCtrl *pPrgCtrl = flash_data_info->pPrgCtrl;

	if(pVCap == NULL)
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
	while(sent_byte < flash_data_info->len)
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

		if (hr != S_OK) break;
	
		sent_byte += packte_unit;
		pPrgCtrl->SetPos(sent_byte);
		
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
	return hr;
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevMftr()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG BufSize;
	char *pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_MANUFACTURER, NULL, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);
	if (pBuf == NULL)
	{
		MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_MANUFACTURER, pBuf, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		free(pBuf);
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	m_EditDevManufacturer.Format(_T("%s"), A2T(pBuf));
	UpdateData(FALSE);
	
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPro()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG BufSize;
	char *pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PRO, NULL, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);
	if (pBuf == NULL)
	{
		MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PRO, pBuf, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		free(pBuf);
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	m_EditDevProduct.Format(_T("%s"), A2T(pBuf));
	free(pBuf);
	UpdateData(FALSE);
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPtm()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG BufSize;
	char *pBuf;
	HRESULT hr;
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PLATFORM, NULL, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	pBuf = (char *)malloc(BufSize);
	if (pBuf == NULL)
	{
		MessageBox(_T("Get Mrmory Error. Code."), _T("Error"));
		return;
	}
	hr = FicGetStr(pVCap, NodeId, YHW_STR_ID_PLATFORM, pBuf, &BufSize);
	if (hr != S_OK)
	{
		CString StrErr;
		free(pBuf);
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	m_EditDevPlatform.Format(_T("%s"), A2T(pBuf));
	free(pBuf);
	UpdateData(FALSE);
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnFwPtm()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	load_info load_data_info;
	char BufPlatFrom[64];

	if (m_EditFwPath.IsEmpty())
	{
		return;
	}
	memset(BufPlatFrom, 0, sizeof(BufPlatFrom));
	memset(&load_data_info, 0, sizeof(load_data_info));
	load_data_info.len = read_file_to_mem(T2A(m_EditFwPath), &load_data_info.buf);//Gets the firmware content and size
	HRESULT hr = FwGetPlatform(load_data_info.buf, load_data_info.len, BufPlatFrom);
	if (hr != S_OK)
	{
		free_mem(&load_data_info);
		return;
	}
	free_mem(&load_data_info);

	m_EditFwPlatform.Format(_T("%s"), A2T(BufPlatFrom));
	UpdateData(FALSE);
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevVid()
{
	UpdateData(TRUE);
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG nId;
	HRESULT hr = FicGetReg(pVCap, NodeId, YHW_REG_USB_VID, &nId);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	m_EditDevUsbVid.Format(_T("%04X"), nId);
	UpdateData(FALSE);
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevPid()
{
	UpdateData(TRUE);
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG nId;
	HRESULT hr = FicGetReg(pVCap, NodeId, YHW_REG_USB_PID, &nId);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Get String From Device Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	m_EditDevUsbPid.Format(_T("%04X"), nId);
	UpdateData(FALSE);
}


void CFicXuUpdateDllTestDlg::OnBnClickedBtnDevReboot()
{
	UpdateData(TRUE);
	if (m_DeviceList.GetCurSel() < 0)
	{
		MessageBox(_T("Can't Find Device."), _T("Error"));
		return;
	}
	IBaseFilter *pVCap = (IBaseFilter *)m_pVCapMult[m_DeviceList.GetCurSel()];
	ULONG NodeId = m_pNodeId[m_DeviceList.GetCurSel()];
	ULONG reboot_flag = 0x15AE3946;
	HRESULT hr = FicSetReg(pVCap, NodeId, YHW_REG_SYS_REBOOT, reboot_flag);
	if (hr != S_OK)
	{
		CString StrErr;
		StrErr.Format(_T("Reboot Camera Error. Code(%08X)."), (ULONG)hr);
		MessageBox(StrErr, _T("Error"));
		return;
	}
	UpdateData(FALSE);
}
