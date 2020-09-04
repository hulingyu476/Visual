
// USB_UpgradeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USB_Upgrade.h"
#include "USB_UpgradeDlg.h"
#include "hid_upgrade.h"
#include "afxdialogex.h"
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dbt.h>
#include <strsafe.h>

#include "enter_read_file_name.h"
#include "device_settings_dlg.h"
#include "osd_control_dlg.h"

#include "hid.h"

extern "C" {
#include "conio.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

USHORT vid1 = 0, pid1 = 0;
int devindex=0;
BOOL cmdstart = false;
int times = 0;

// CUSB_UpgradeDlg 对话框

#define QUERY_EVENT_COUNT			(2)
static HANDLE UpgradeEvent[QUERY_EVENT_COUNT];


CString AutoUSBFile;
CString AutoFPGAFile;
CString AutoUSBBootLoaderFile;
CString AutoPKGFile;
CString AutoMTDFile;
CString AutoCFGFile;

bool uvc_to_hid_flag = 0;

#define SEND_FILE_MAX_BUFF			(255)
struct SendFile_t {
	WORD file_num;
	CString file_path[SEND_FILE_MAX_BUFF];
};

struct SendFile_t send_file_path;


CUSB_UpgradeDlg::CUSB_UpgradeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_USB_UPGRADE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUSB_UpgradeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPGRADE_TYPE, m_ctrlCombo_Mode);
	DDX_Control(pDX, IDC_BUTTON_LOAD_FILE, m_ctrlBtn_AddFile);
	DDX_Control(pDX, IDC_BUTTON_UPGRADE, m_ctrlBtn_UpgradeStar);
	DDX_Control(pDX, IDC_BUTTON_AUTO_UPGRADE, m_ctrlBtn_AutoUpgrade);
	DDX_Control(pDX, IDC_BUTTON_QUERY_VERSION, m_ctrlBtn_QueryVersion);
	DDX_Control(pDX, IDC_EDIT_FILE_PATH, m_StaticFilePath);
	DDX_Control(pDX, IDC_PROGRESS_UPGRADE, m_ctrlUpgradeProgress);
	DDX_Control(pDX, IDC_BUTTON_DEVICE_SETTINGS, m_ctrlBtn_Settings);
	DDX_Control(pDX, IDC_BUTTON_LANGUAGE, m_ctrlBtn_Language);


}

BEGIN_MESSAGE_MAP(CUSB_UpgradeDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_DEVICECHANGE, OnDeviceChange)
	//	ON_CBN_SELCHANGE(IDC_COMBO_UPGRADE_TYPE, &CUSB_UpgradeDlg::OnCbnSelchangeComboUpgradeType)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_FILE, &CUSB_UpgradeDlg::OnBnClickedButtonLoadFile)
	ON_BN_CLICKED(IDC_BUTTON_UPGRADE, &CUSB_UpgradeDlg::OnBnClickedButtonUpgrade)
	ON_BN_CLICKED(IDC_BUTTON_AUTO_UPGRADE, &CUSB_UpgradeDlg::OnBnClickedButtonAutoUpgrade)
	ON_BN_CLICKED(IDC_BUTTON_QUERY_VERSION, &CUSB_UpgradeDlg::OnBnClickedButtonQueryVersion)
	ON_CBN_DROPDOWN(IDC_COMBO_UPGRADE_TYPE, &CUSB_UpgradeDlg::OnDropdownComboUpgradeType)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO_UPGRADE_TYPE, &CUSB_UpgradeDlg::OnCbnSelchangeComboUpgradeType)
	ON_BN_CLICKED(IDC_BUTTON_DEVICE_SETTINGS, &CUSB_UpgradeDlg::OnBnClickedButtonDeviceSettings)
	ON_BN_CLICKED(IDC_BUTTON_LANGUAGE, &CUSB_UpgradeDlg::OnBnClickedButtonLanguage)
END_MESSAGE_MAP()


//int   WINAPI WinMain(HINSTANCE   hInstance, HINSTANCE   hPrevInstance, LPSTR  szCmdLine, int   iCmdShow)
//{
//	for (;;)
//	{
//	}
//	return 0;
//}

// CUSB_UpgradeDlg 消息处理程序

//int m = 0;
//__declspec(dllexport)int Add(int a, int b)
//{
//	m = 100;
//	return a + b;
//}
//
//
//__declspec(dllexport)int Add1(int a)
//{
//	return a + m;
//}
//
//__declspec(dllexport)char * __stdcall test()
//{
//	return "你好";
//}

BOOL CUSB_UpgradeDlg::OnInitDialog()
{

	//是否检查多运行
	BOOL checkmulti = true;
	//定义接受参数数据结构
	LPWSTR *szArglist = NULL;
	int nArgs;
	//获取参数 以及参数个数。
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (NULL != szArglist && nArgs>1)
	{
		checkmulti = false;
		//for (int i = 1; i<nArgs; ++i)
			//AfxMessageBox((CString)szArglist[i]);//测试
		char* str;
		if (nArgs >= 2)
		{
			vid1 = strtol((CString)szArglist[1], &str, 16);
			cmdstart = true;
		}
		if (nArgs >= 3)
			pid1 = strtol((CString)szArglist[2], &str, 16);
		//printf("%d %d", vid1,pid1);
		if (nArgs >= 4)
			devindex = strtol((CString)szArglist[3], &str, 16);
		//if ((CString)szArglist[3]!="")

		/*CString s;
		s.Format("%d %d %d", vid1, pid1, devindex);
		AfxMessageBox(s);*/
	}
	//释放szArglist
	LocalFree(szArglist);

	SetWindowText(_T(APP_VERSION));

	LCID lcid = GetSystemDefaultLCID();

	if (checkmulti)//检查多运行
	{
		HANDLE hMutex = CreateMutex(NULL, FALSE, _T("CSTS"));
		if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hMutex);

			switch (lcid)
			{
			case 2052:
			case 1028:
			case 3076:
				AfxMessageBox(_T("已经有一个程序正在运行，不允许多个程序同时使用！"), MB_ICONASTERISK);
				break;

			default:
				MessageBoxEx(NULL, _T("There is already one program running, and you can not run two or more programs at the same time "), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("There is already one program running, and you can not run two or more programs at the same time "), MB_ICONINFORMATION);
				break;
			}
			ExitProcess(0);
		}
	}

	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

									// TODO: 在此添加额外的初始化代码

	int i;
	for (i = 0; i < QUERY_EVENT_COUNT; i++)
	{
		UpgradeEvent[i] = CreateEvent(
			NULL,   // default security attributes
			FALSE,  // auto-reset event object
			FALSE,  // initial state is nonsignaled
			NULL);  // unnamed object

		if (UpgradeEvent[i] == NULL)
		{
			//printf("CreateEvent error: %d\n", GetLastError() ); 
			ExitProcess(0);
		}
	}

	pDeviceSettingsDlg = new device_settings_dlg;
	if (pDeviceSettingsDlg)
	{
		BOOL ret = pDeviceSettingsDlg->Create(IDD_DIALOG_DEVICE_SETTINGS, this);
	}

	pEnterReadFileNameDlg = new enter_read_file_name;
	if (pEnterReadFileNameDlg)
	{
		BOOL ret = pEnterReadFileNameDlg->Create(IDD_DIALOG_SET_READ_FILE_NAME, this);
	}

	pOsdControlDlg = new osd_control_dlg;
	if (pOsdControlDlg)
	{
		BOOL ret = pOsdControlDlg->Create(IDD_DIALOG_OSD_CTRL, this);
	}

	AfxBeginThread(UpgradeThread, (LPVOID) this);

	send_file_path.file_num = 0;
	DisplayStatus("");
	strGetFileName = "";
	//m_ctrlBtn_CopySocFile.EnableWindow(0);
	//m_ctrlBtn_Upgrade.EnableWindow(0);
	m_ctrlUpgradeProgress.SetStep(1);
	m_ctrlUpgradeProgress.SetRange(0, 100);
	m_ctrlUpgradeProgress.SetPos(0);

	Hid_Upgrade.Hid_UpgradeInit((LPVOID) this);

	LANGID lid = GetSystemDefaultLangID();

	if ((lid == 0x0804 || lid == 3076 || lid == 1028)&& (lcid == 2052 || lcid == 3076 || lcid == 1028))
	{
		Hid_Upgrade.Language = LANGUAGE_SIMPLIFIED_CHINESE;
	}
	else
	{
		Hid_Upgrade.Language = LANGUAGE_ENGLISH;
	}

	//if (lcid == 2052 || lcid == 3076 || lcid == 1028)//中华人民共和国 zh-cn 0x0804 
	//{
	//	Hid_Upgrade.Language = LANGUAGE_SIMPLIFIED_CHINESE;
	//}
	//else
	//{
	Hid_Upgrade.Language = LANGUAGE_ENGLISH;//强制英文
	//}
	LanguageChange();
	Hid_Upgrade.FindDevice();
	if (Hid_Upgrade.CameraInfo.MyDevFound)
	{
		DeviceConnect();
	}
	else
	{
		DeviceDisconnect();
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CUSB_UpgradeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CUSB_UpgradeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CUSB_UpgradeDlg::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam;

	switch (LOWORD(wParam))
	{
		//设备连接事件
	case DBT_DEVICEARRIVAL:
	{
		if (Hid_Upgrade.CameraInfo.UpgradeFlag == 0)
		{
			if (Hid_Upgrade.m_MyHidDevice.FindHid(vid1,pid1))
			{
				if (Hid_Upgrade.CameraInfo.UpdateFlashFlag)
				{
					Hid_Upgrade.CameraInfo.UpdateFlashFlag = 0;

					switch (Hid_Upgrade.Language)
					{
					case LANGUAGE_SIMPLIFIED_CHINESE:
						AfxMessageBox(_T("升级完成！"), MB_ICONINFORMATION);
						break;

					case LANGUAGE_ENGLISH:
					default:
						MessageBoxEx(NULL, _T("Upgrade is completed!"), "USB Upgrade", MB_OK| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
						//AfxMessageBox(_T("Upgrade is completed!"), MB_ICONINFORMATION);
						break;
					}
				}
				
				DeviceConnect();				
			}
			else
			{
				if (Hid_Upgrade.CameraInfo.UpdateFlashFlag == 0)
					DeviceDisconnect();
			}
		}

		break;
	}

	//设备拔出事件
	case DBT_DEVICEREMOVECOMPLETE:
	{
		if (Hid_Upgrade.CameraInfo.UpgradeFlag == 0 && Hid_Upgrade.CameraInfo.UpdateFlashFlag == 0)
		{
			if (!Hid_Upgrade.m_MyHidDevice.FindHid(vid1,pid1))
			{
				if (Hid_Upgrade.CameraInfo.MyDevFound && Hid_Upgrade.CameraInfo.UpdateFlashFlag == 0)
					DeviceDisconnect();
			}
		}

		break;
	}

	/* HID Device Change - Note that Win2k tends to use this messages
	instead of the previous two. */
	case DBT_DEVNODES_CHANGED://USB插拔都会进入
	{
		if (Hid_Upgrade.CameraInfo.UpgradeFlag == 0)
		{
			if (Hid_Upgrade.m_MyHidDevice.FindHid(vid1,pid1))
			{
				Hid_Upgrade.DeviceConnect();
				if (Hid_Upgrade.QueryVersion() != 0)//版本查询错误
				{
					times++;
					if (times >= 5)
					{
						times = 0;
						switch (Hid_Upgrade.Language)
						{
						case LANGUAGE_SIMPLIFIED_CHINESE:
							AfxMessageBox(_T("版本查询错误！"), MB_ICONINFORMATION);
							break;

						case LANGUAGE_ENGLISH:
						default:
							MessageBoxEx(NULL, _T("Query version fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
							//AfxMessageBox(_T("Query version fail!"), MB_ICONINFORMATION);
							break;
						}
					}
					return false;
				}

				if (Hid_Upgrade.CameraInfo.UpdateFlashFlag)
				{
					Hid_Upgrade.CameraInfo.UpdateFlashFlag = 0;

					switch (Hid_Upgrade.Language)
					{
					case LANGUAGE_SIMPLIFIED_CHINESE:
						AfxMessageBox(_T("升级完成！"), MB_ICONINFORMATION);
						break;

					case LANGUAGE_ENGLISH:
					default:
						MessageBoxEx(NULL, _T("Upgrade is completed!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
						//AfxMessageBox(_T("Upgrade is completed!"), MB_ICONINFORMATION);
						break;
					}
				}
				/*times++;
				if (times >= 2)
				{
					times = 0;*/
					DeviceConnect();
				//}
			}
			else //if (USB_Upgrade.CameraInfo.MyDevFound)
			{
				if (Hid_Upgrade.CameraInfo.UpdateFlashFlag == 0)
					DeviceDisconnect();
			}
		}
		break;
	}

	default:
		//return DefWindowProc(message, wParam, lParam);
		break;
	}
	return true;
}

UINT CUSB_UpgradeDlg::UpgradeThread(void *pParam)
{
	CUSB_UpgradeDlg *pAppDlg;
	pAppDlg = (CUSB_UpgradeDlg*)pParam;

	DWORD i, dwEvent;

	while (1)
	{
		//等待事件触发
		dwEvent = WaitForMultipleObjects(
			QUERY_EVENT_COUNT,     // number of objects in array
			UpgradeEvent,     // array of objects
			FALSE,       // wait for any object
			INFINITE);       // five-second wait

		switch (dwEvent)
		{
		case WAIT_OBJECT_0 + 0:
			pAppDlg->UpgradeHandle(pParam);
			break;

		case WAIT_OBJECT_0 + 1:
			pAppDlg->AutoUpgradeHandle(pParam);
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
			ResetEvent(UpgradeEvent[i]);
		//设置事件为无效状态
	}

	for (i = 0; i < QUERY_EVENT_COUNT; i++)
		CloseHandle(UpgradeEvent[i]);

	return 0;
}

void CUSB_UpgradeDlg::ErrorInfoHandle(CString strModeName, int ret)
{
	switch (ret)
	{
	case UPGRADE_SUCCESS:
		if (strModeName == UPGRADE_MODE_STRING_PKG || strModeName == UPGRADE_MODE_STRING_MTD || strModeName == UPGRADE_MODE_STRING_AUTO)
		{
			if (strModeName == UPGRADE_MODE_STRING_AUTO &&
					(Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1
						|| (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 
							&& (AutoMTDFile == "" && AutoPKGFile == "")
							)
					)
				)
			{
				switch (Hid_Upgrade.Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					DisplayStatus(_T("升级完成，需要手动重启摄像机电源"));
					break;

				case LANGUAGE_ENGLISH:
				default:
					DisplayStatus("Upgrade completed, Please restart the camera power.");
					break;
				}
			}			
			else
			{
				Hid_Upgrade.CameraInfo.UpdateFlashFlag = 1;

				switch (Hid_Upgrade.Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					DisplayStatus(_T("正在升级中...请等待升级完成，请勿断开电源和USB！！！"));
					break;

				case LANGUAGE_ENGLISH:
				default:
					DisplayStatus("Upgrading... please wait for upgrade to complete and do not disconnect power and USB!!!");
					break;
				}
			}
		}
		else if (strModeName == UPGRADE_MODE_STRING_GET_LOG || strModeName == UPGRADE_MODE_STRING_GET_FILE)
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				DisplayStatus(_T("文件接收完成"));
				break;

			case LANGUAGE_ENGLISH:
			default:
				DisplayStatus("File receive completed");
				break;
			}
		}
		else if (strModeName == UPGRADE_MODE_STRING_FILE || strModeName == UPGRADE_MODE_STRING_CONFIG)
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				DisplayStatus("文件发送完成");
				break;

			case LANGUAGE_ENGLISH:
			default:
				DisplayStatus("File send completed");
				break;
			}
		}
		else 
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				DisplayStatus(_T("升级完成，需要手动重启摄像机电源"));
				break;

			case LANGUAGE_ENGLISH:
			default:
				DisplayStatus("Upgrade completed, Please restart the camera power.");
				break;
			}
		}
		break;
	case UPGRADE_ERROR_CONNECT:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("连接失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Connect Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Connect Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_VERSION:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("版本错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Version Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Version Error!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_QUERY:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("版本查询错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Query Version Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Query Version Error!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_TRANSFER:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("数据传输错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Transfer Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Transfer Error!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_REQUEST:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("请求失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Request Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Request Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_MEMORY_ERROR:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("内存申请失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Memory Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Memory Error!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_TYPE:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("文件类型错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("File Type Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("File Type Error!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_NOT_FOUND:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Not Found The Upgrade File!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Not Found The Upgrade File!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_OPEN:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("文件打开失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("File Open Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("File Open Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_SIZE:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("文件大小错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("File Size Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("File Size Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_CHECK:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("文件校验失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("File Check Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("File Check Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	case UPGRADE_ERROR_FILE_CREATE:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("文件创建失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("File Creare Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("File Creare Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	default:
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("升级失败！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Upgrade Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Upgrade Fail!"), MB_ICONINFORMATION);
			break;
		}
		break;
	}
}

void CUSB_UpgradeDlg::UpgradeHandle(void *pParam)
{
	CString strModeName;
	CString PathNameString;
	int ret;
	int i;


	m_ctrlCombo_Mode.GetWindowText(strModeName);
	m_StaticFilePath.GetWindowText(PathNameString);

	if (strModeName == "")
	{
		if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
		{
			if (Hid_Upgrade.SendCharsCommand("go into update", HID_HEAD_CAM, 0) != TRUE)
			{
				MessageBoxEx(NULL, _T("Open Device Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Open Device Fail!"), MB_ICONASTERISK);
				return;
			}
		}
		else
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("请选择升级模式！"), MB_ICONASTERISK);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Please select an upgrade type!"), "USB Upgrade", MB_OK|MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Please select an upgrade type!"), MB_ICONASTERISK);
				break;
			}
		}

		return;
	}

	if (PathNameString == "" && strModeName != UPGRADE_MODE_STRING_FILE)
	{

		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("请选择升级文件！"), MB_ICONASTERISK);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Please select a firmware!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Please select a firmware!"), MB_ICONASTERISK);
			break;
		}
		return;
	}

	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{
		DisableCtrl();
		uvc_to_hid_flag = 1;
		if (Hid_Upgrade.SendCharsCommand("go into update", HID_HEAD_CAM, 0) == TRUE)
		{
			while (Hid_Upgrade.m_MyHidDevice.FindLinuxHid() == FALSE);
			for (i = 0; i < 200; i++)
			{
				if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_HISI_UVC_HID)
				{
					Sleep(5000);
					break;
				}
				Sleep(500);
			}

			if (Hid_Upgrade.VersionCheck(strModeName, PathNameString) != UPGRADE_SUCCESS)
			{
				Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
				//EnableCtrl();
				return;
			}
		}
		else
		{
			MessageBoxEx(NULL, _T("Open Device Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Open Device Fail!"), MB_ICONASTERISK);
			uvc_to_hid_flag = 0;
			EnableCtrl();
			return;
		}

		uvc_to_hid_flag = 0;

	}

	DisableCtrl();
	Hid_Upgrade.CameraInfo.UpgradeFlag = 1;
	Hid_Upgrade.CameraInfo.UpdateFlashFlag = 0;
	PromptSetPos(0);

	if (strModeName == UPGRADE_MODE_STRING_PKG || strModeName == UPGRADE_MODE_STRING_MTD || 
		strModeName == UPGRADE_MODE_STRING_FPGA || strModeName == UPGRADE_MODE_STRING_USB || strModeName == UPGRADE_MODE_STRING_USB_BOOT || 
		strModeName == UPGRADE_MODE_STRING_USB_BOOTLOADER || strModeName == UPGRADE_MODE_STRING_FPGA_RESTORE || strModeName == UPGRADE_MODE_STRING_USB_RESTORE)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("升级过程中请勿断开电源和USB！！！"), MB_ICONWARNING);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Please do not disconnect the power and disconnect USB when upgrading!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Please do not disconnect the power and disconnect USB when upgrading!"), MB_ICONWARNING);
			break;
		}
	}


	if (strModeName == UPGRADE_MODE_STRING_PKG)
	{
		if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			if (Hid_Upgrade.VersionCheck(strModeName, PathNameString) != UPGRADE_SUCCESS)
			{
				Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
				EnableCtrl();
				return;
			}
		}
		ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_PKG, PathNameString, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_MTD)
	{
		if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
		{
			if (Hid_Upgrade.VersionCheck(strModeName, PathNameString) != UPGRADE_SUCCESS)
			{
				Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
				EnableCtrl();
				return;
			}
		}

		ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_MTD, PathNameString, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_FILE)
	{
		if (send_file_path.file_num > 0)
		{
			for (i = 0; i < send_file_path.file_num; i++)
			{
				if (send_file_path.file_path[i] != "")
				{
					ret = Hid_Upgrade.SendFile(FILE_TYPE_LINUX_FILE, send_file_path.file_path[i], pParam);
					if (ret == UPGRADE_SUCCESS)
						Sleep(200);
					else
						break;
				}
				else
				{
					ret = LANGUAGE_SIMPLIFIED_CHINESE;
					break;
				}
			}
		}
	}
	else if(strModeName == UPGRADE_MODE_STRING_CONFIG)
	{
		ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_DATA, PathNameString, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_GET_LOG)
	{
		ret = Hid_Upgrade.GetFile_V2("vhd.log", PathNameString, pParam);
		if (ret == UPGRADE_SUCCESS)
			ret = Hid_Upgrade.GetFile_V2("vhd_ucam.log", PathNameString, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_GET_FILE)
	{
		if (strGetFileName != "")
			ret = Hid_Upgrade.GetFile_V2(strGetFileName, PathNameString, pParam);
		else
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("请输入要获取的文件名称！"), MB_ICONASTERISK);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Please enter the file name!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Please enter the file name!"), MB_ICONASTERISK);
				break;
			}


			Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
			EnableCtrl();
			return;
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_FPGA)
	{
		ret = Hid_Upgrade.FPGAUpgrade(PathNameString, strModeName, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_FPGA_RESTORE)
	{
		ret = Hid_Upgrade.FPGAUpgrade(PathNameString, strModeName, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_USB)
	{
		ret = Hid_Upgrade.FX3Upgrade(PathNameString, strModeName, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_USB_BOOT)
	{
		ret = Hid_Upgrade.UsbUpgrade2FpgaFlash(PathNameString, strModeName, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_USB_RESTORE)
	{
		ret = Hid_Upgrade.UsbUpgrade2FpgaFlash(PathNameString, strModeName, pParam);
	}
	else if (strModeName == UPGRADE_MODE_STRING_USB_BOOTLOADER)
	{
		ret = Hid_Upgrade.FX3Upgrade(PathNameString, strModeName, pParam);
	}
	else
	{
		Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
		EnableCtrl();
		return;
	}


	ErrorInfoHandle(strModeName, ret);



	Hid_Upgrade.CameraInfo.UpgradeFlag = 0;

	if (strModeName == UPGRADE_MODE_STRING_PKG || strModeName == UPGRADE_MODE_STRING_MTD)
	{
		if (ret != UPGRADE_SUCCESS)
		{
			EnableCtrl();
		}
	}
	else
	{
		EnableCtrl();
	}

}

void CUSB_UpgradeDlg::AutoUpgradeHandle(void *pParam)
{
	int ret;
	int i;

	if (Hid_Upgrade.CameraInfo.UpgradeFlag)
		return;

	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		if (AutoMTDFile == "" && AutoPKGFile == "" &&
			AutoUSBFile == "" && AutoFPGAFile == "" && AutoUSBBootLoaderFile == "")
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Not Found The Upgrade File!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Not Found The Upgrade File!"), MB_ICONINFORMATION);
				break;
			}
			return;
		}
	}
	else if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		if (AutoUSBFile == "" && AutoFPGAFile == "" && AutoUSBBootLoaderFile == "")
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Not Found The Upgrade File!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Not Found The Upgrade File!"), MB_ICONINFORMATION);
				break;
			}
			return;
		}
	}
	else
	{
		if (AutoMTDFile == "" && AutoPKGFile == "")
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Not Found The Upgrade File!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Not Found The Upgrade File!"), MB_ICONINFORMATION);
				break;
			}
			return;
		}
	}


	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID)
	{

		DisableCtrl();

		uvc_to_hid_flag = 1;
		if (Hid_Upgrade.SendCharsCommand("go into update", HID_HEAD_CAM, 0) == TRUE)
		{
			while (Hid_Upgrade.m_MyHidDevice.FindLinuxHid() == FALSE);
			for (i = 0; i < 200; i++)
			{
				if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_HISI_UVC_HID)
				{
					Sleep(5000);
					break;
				}
				Sleep(500);
			}

			if (AutoMTDFile != "")
			{
				if (Hid_Upgrade.VersionCheck(UPGRADE_MODE_STRING_MTD, AutoMTDFile) != UPGRADE_SUCCESS)
				{
					Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
					uvc_to_hid_flag = 0;
					//EnableCtrl();
					return;
				}
			}
			else if (AutoPKGFile != "")
			{
				if (Hid_Upgrade.VersionCheck(UPGRADE_MODE_STRING_PKG, AutoPKGFile) != UPGRADE_SUCCESS)
				{
					Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
					uvc_to_hid_flag = 0;
					//EnableCtrl();
					return;
				}
			}
		}
		else
		{
			MessageBoxEx(NULL, _T("Open Device Fail!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Open Device Fail!"), MB_ICONASTERISK);
			uvc_to_hid_flag = 0;
			EnableCtrl();
			return;
		}

		uvc_to_hid_flag = 0;

	}

	DisableCtrl();
	Hid_Upgrade.CameraInfo.UpgradeFlag = 1;
	Hid_Upgrade.CameraInfo.UpdateFlashFlag = 0;
	PromptSetPos(0);

	//if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		//if (AutoMTDFile != "" || AutoPKGFile != "")
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("升级过程中请勿断开电源和USB！！！"), MB_ICONWARNING);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Please do not disconnect the power and disconnect USB when upgrading!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Please do not disconnect the power and disconnect USB when upgrading!"), MB_ICONWARNING);
				break;
			}
		}
	}

	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1 ||
		Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{

		if (AutoUSBBootLoaderFile != "")
		{
			ret = Hid_Upgrade.FX3Upgrade(AutoUSBBootLoaderFile, UPGRADE_MODE_STRING_USB_BOOTLOADER, pParam);
			if (ret != UPGRADE_SUCCESS)
				goto out;
		}

		if (AutoFPGAFile != "")
		{
			ret = Hid_Upgrade.FPGAUpgrade(AutoFPGAFile, UPGRADE_MODE_STRING_FPGA, pParam);
			if (ret != UPGRADE_SUCCESS)
				goto out;
		}

		if (AutoUSBFile != "")
		{
			if (AutoUSBBootLoaderFile != "")
			{
				ret = Hid_Upgrade.UsbUpgrade2FpgaFlash(AutoUSBFile, UPGRADE_MODE_STRING_USB_BOOT, pParam);
				if (ret != UPGRADE_SUCCESS)
					goto out;
			}
			else
			{
				ret = Hid_Upgrade.FX3Upgrade(AutoUSBFile, UPGRADE_MODE_STRING_USB, pParam);
				if (ret != UPGRADE_SUCCESS)
					goto out;
			}
		}

		if (AutoMTDFile != "")//MTD 自动升级
		{
			if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
			{
				if (Hid_Upgrade.VersionCheck(UPGRADE_MODE_STRING_MTD, AutoMTDFile) != UPGRADE_SUCCESS)
				{
					Hid_Upgrade.CameraInfo.UpgradeFlag = 0;
					EnableCtrl();
					return;
				}
			}

			ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_MTD, AutoMTDFile, pParam);
			if (ret != UPGRADE_SUCCESS)
				goto out;
		}
		


		if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID != DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
		{
			if (AutoPKGFile != "")
			{

				if (Hid_Upgrade.VersionCheck(UPGRADE_MODE_STRING_PKG, AutoPKGFile) != UPGRADE_SUCCESS)
				{
					return;
				}
				ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_PKG, AutoPKGFile, pParam);
			}		
		}
	}
	else 
	{
		if (AutoMTDFile != "")
		{
			ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_MTD, AutoMTDFile, pParam);
			if (AutoCFGFile != "")
				ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_DATA, AutoCFGFile, pParam);
		}
		else if (AutoPKGFile != "")
		{
			ret = Hid_Upgrade.SendFile(FILE_TYPE_SOC_PKG, AutoPKGFile, pParam);
		}		
	}

out:
	Hid_Upgrade.CameraInfo.UpgradeFlag = 0;


	if (ret != UPGRADE_SUCCESS)
	{
		EnableCtrl();
	}

	ErrorInfoHandle(UPGRADE_MODE_STRING_AUTO, ret);
}

void CUSB_UpgradeDlg::DeviceConnect(void)
{
	Hid_Upgrade.DeviceConnect();
	PromptSetPos(0);

	switch (Hid_Upgrade.Language)
	{
	case LANGUAGE_SIMPLIFIED_CHINESE:
		DisplayStatus("摄像机连接成功");
		break;

	case LANGUAGE_ENGLISH:
	default:
		DisplayStatus("Camera connection is successful");
		break;
	}

	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3 || 
		Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("请使用老版本的升级工具进行升级！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Please use the old version of the upgrade tool to upgrade !"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Please use the old version of the upgrade tool to upgrade !"), MB_ICONINFORMATION);
			break;
		}

		return;
	}

	if (Hid_Upgrade.QueryVersion() == 0)
	{
		PromptStringInit();
	}
	else
	{

		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("版本查询错误！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Query Version Error!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Query Version Error!"), MB_ICONINFORMATION);
			break;
		}
	}

	if(uvc_to_hid_flag == 0)
		EnableCtrl();

}

void CUSB_UpgradeDlg::DeviceDisconnect(void)
{
	DisableCtrl();
	Hid_Upgrade.DeviceDisconnect();
	PromptSetPos(0);

	switch (Hid_Upgrade.Language)
	{
	case LANGUAGE_SIMPLIFIED_CHINESE:
		DisplayStatus("摄像机连接失败");
		break;

	case LANGUAGE_ENGLISH:
	default:
		DisplayStatus("Camera connection failed");
		break;
	}
}

void CUSB_UpgradeDlg::EnableCtrl(void)
{
	CString strModeName;

	m_ctrlCombo_Mode.GetWindowText(strModeName);


	m_ctrlBtn_UpgradeStar.EnableWindow(TRUE);
	//m_ctrlBtn_AutoUpgrade.EnableWindow(TRUE);
	m_ctrlCombo_Mode.EnableWindow(TRUE);
	m_ctrlBtn_QueryVersion.EnableWindow(TRUE);
	m_ctrlBtn_AddFile.EnableWindow(TRUE);
	m_StaticFilePath.EnableWindow(TRUE);
	m_ctrlBtn_Settings.EnableWindow(TRUE);
	m_ctrlBtn_Language.EnableWindow(TRUE);

	if (!(strModeName == UPGRADE_MODE_STRING_GET_LOG || strModeName == UPGRADE_MODE_STRING_GET_FILE))
	{
		m_ctrlBtn_AutoUpgrade.EnableWindow(TRUE);
	}
}

void CUSB_UpgradeDlg::DisableCtrl(void)
{
#ifndef _DEBUG
	m_ctrlBtn_UpgradeStar.EnableWindow(FALSE);
	m_ctrlBtn_AutoUpgrade.EnableWindow(FALSE);
	m_ctrlCombo_Mode.EnableWindow(FALSE);
	m_ctrlBtn_QueryVersion.EnableWindow(FALSE);
	m_ctrlBtn_AddFile.EnableWindow(FALSE);
	m_StaticFilePath.EnableWindow(FALSE);
	m_ctrlBtn_Settings.EnableWindow(FALSE);
	m_ctrlBtn_Language.EnableWindow(FALSE);
#endif
}

void CUSB_UpgradeDlg::DisplayStatus(const char *string)
{
	SetDlgItemText(IDC_STATIC_STATUS, string);
}

void CUSB_UpgradeDlg::PromptSetPos(int pos)
{
	m_ctrlUpgradeProgress.SetPos(pos);
}

//将信息添加到信息显示框的函数
void CUSB_UpgradeDlg::PromptStringInit(void)
{	
	PromptString = "";

	if (Hid_Upgrade.CameraInfo.MyDevFound)
	{
		if (Hid_Upgrade.CameraInfo.StringDeviceName != "")
		{
			PromptString += (Hid_Upgrade.CameraInfo.StringDeviceName + " Information :");
			//PromptString += "\r\n";
		}
		else
		{
			PromptString += ("Camera Information :");
			//PromptString += "\r\n";
		}
		

		if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER
			|| Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
		{
			PromptString += "\r\n";
			PromptString += ("USB BootLoader:" + Hid_Upgrade.m_MyHidDevice.GetSerialNumberString());
		}
		else
		{
			if (Hid_Upgrade.CameraInfo.StringPackageVersion != "")
			{
				PromptString += "\r\n";
				PromptString += (Hid_Upgrade.CameraInfo.StringPackageVersion);
			}

			if (Hid_Upgrade.CameraInfo.StringSOCVersion != "")
			{
				PromptString += "\r\n";
				
				{
					PromptString += ("SOC Version : " + Hid_Upgrade.CameraInfo.StringSOCVersion);
				}
			}
			if (Hid_Upgrade.CameraInfo.StringUSBVersion != "")
			{
				PromptString += "\r\n";
				PromptString += ("USB Version : " + Hid_Upgrade.CameraInfo.StringUSBVersion);
			}

			if (Hid_Upgrade.CameraInfo.StringFPGAVersion != "")
			{
				PromptString += "\r\n";
				PromptString += ("FPGA Version : " + Hid_Upgrade.CameraInfo.StringFPGAVersion);
			}

			if (Hid_Upgrade.CameraInfo.StringAFVersion != "")
			{
				PromptString += "\r\n";
				PromptString += ("AF Version : " + Hid_Upgrade.CameraInfo.StringAFVersion);
			}

			if (Hid_Upgrade.CameraInfo.StringHWVersion != "")
			{
				PromptString += "\r\n";
				PromptString += ("HW Version : " + Hid_Upgrade.CameraInfo.StringHWVersion);
			}
			
			if (Hid_Upgrade.CameraInfo.StringUSBBootLoaderVersion != "")
			{
				PromptString += "\r\n";
				PromptString += ("USB BootLoader Version : " + Hid_Upgrade.CameraInfo.StringUSBBootLoaderVersion);
			}

			
			if (Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD)
			{
				CString TempString = "";
				TempString = "";
				TempString = Hid_Upgrade.m_MyHidDevice.GetSerialNumberString();
				if(TempString != ""){
					PromptString += "\r\n";
					PromptString += ("SN :" + TempString);
				}
			}
			
			//if (Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_POLYCOM && Hid_Upgrade.m_MyHidDevice.usb_id.usb_pid == USB_PID_V61PU)
			//{
			//	CString TempString = "";

			//	TempString = Hid_Upgrade.m_MyHidDevice.GetV61PUPackageVersionString();
			//	if (TempString != "")
			//	{
			//		PromptString += "\r\n";
			//		PromptString += ("Package " + TempString);
			//	}
			//	//str += (String.DeviceName + m_MyHidDevice.GetProductString() + "\r\n");
			//	//			CString SNString = "";

			//	TempString = "";
			//	TempString = Hid_Upgrade.m_MyHidDevice.GetSerialNumberString();
			//	PromptString += "\r\n";
			//	PromptString += ("SN :" + TempString);

			//}
		}
	}

	//设置新的文本
	SetDlgItemText(IDC_EDIT_INFO, PromptString);
	//滚动条自动滚动到最后一行
	//i = ((CEdit*)GetDlgItem(IDC_EDIT_INFO))->GetLineCount();
	//((CEdit*)GetDlgItem(IDC_EDIT_INFO))->LineScroll(i, 0);
}

void CUSB_UpgradeDlg::PromptStringAdd(CString str)
{
	UINT i;

	PromptString += str;
	//PromptString += "\r\n";
	//PromptString += "\r\n";

	SetDlgItemText(IDC_EDIT_INFO, PromptString);
	//滚动条自动滚动到最后一行
	i = ((CEdit*)GetDlgItem(IDC_EDIT_INFO))->GetLineCount();
	((CEdit*)GetDlgItem(IDC_EDIT_INFO))->LineScroll(i, 0);
}



//void CUSB_UpgradeDlg::OnCbnSelchangeComboUpgradeType()
//{
//	// TODO: 在此添加控件通知处理程序代码
//}


void CUSB_UpgradeDlg::OnBnClickedButtonLoadFile()
{
	CString strModeName;

	m_ctrlCombo_Mode.GetWindowText(strModeName);

	if (strModeName == UPGRADE_MODE_STRING_USB || strModeName == UPGRADE_MODE_STRING_USB_BOOTLOADER || strModeName == UPGRADE_MODE_STRING_USB_BOOT || strModeName == UPGRADE_MODE_STRING_USB_RESTORE)
	{
		CFileDialog fileDlg(TRUE, "*.img", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "USB Upgrade File(*.img)|*.img|", 0);
		if (fileDlg.DoModal() == IDOK)
		{
			//设置新的文本
			SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_FPGA || strModeName == UPGRADE_MODE_STRING_FPGA_RESTORE)
	{
		CFileDialog fileDlg(TRUE, "*.bin", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "FPGA Upgrade File(*.bin)|*.bin|", 0);
		if (fileDlg.DoModal() == IDOK)
		{
			//设置新的文本
			SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_PKG)
	{
		CFileDialog fileDlg(TRUE, "*.pkg", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "PKG Upgrade File(*.pkg)|*.pkg|", 0);
		if (fileDlg.DoModal() == IDOK)
		{
			//设置新的文本
			SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_MTD)
	{
		CFileDialog fileDlg(TRUE, "*.img", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "MTD Upgrade File(*.img)|*.img|", 0);
		if (fileDlg.DoModal() == IDOK)
		{
			//设置新的文本
			SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_FILE)
	{
#if 0
		CFileDialog fileDlg(TRUE, "*.EXE", "", NULL, "Send File(*)|*|", 0);
		if (fileDlg.DoModal() == IDOK)
		{
			//设置新的文本
			SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
		}
#else
		CString szFilter = "All Files (*.*)|*.*| Bin Files (*.bin)|*.bin| Img Files (*.img)|*.img| ko Files (*.ko)|*.ko||";
		CFileDialog   dlg(TRUE, "*", NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, szFilter);
		dlg.m_ofn.lpstrTitle = _T("Select Send Files");//设置标题  
		CString PathName;
		CArray<CString, CString> aryFilename;
		if (dlg.DoModal() == IDOK)
		{
			POSITION posFile = dlg.GetStartPosition();
			while (posFile != NULL)
			{
				aryFilename.Add(dlg.GetNextPathName(posFile));
			}

			send_file_path.file_num = aryFilename.GetSize();//获取选择的文件数  
			if (send_file_path.file_num >= SEND_FILE_MAX_BUFF)
			{
				send_file_path.file_num = 0;
				switch (Hid_Upgrade.Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("最大只支持255个文件！"), MB_ICONASTERISK);
					break;

				case LANGUAGE_ENGLISH:
				default:
					AfxMessageBox(_T("Maximum support only 255 files!"), MB_ICONASTERISK);
					break;
				}
			}
			else
			{
				int i;
				for (i = 0; i < SEND_FILE_MAX_BUFF; i++)
				{
					send_file_path.file_path[i] = "";
				}

				for (i = 0; i < send_file_path.file_num; i++)
				{
					send_file_path.file_path[i] = aryFilename[i];
					PathName += aryFilename[i];
					PathName += ";";
				}
				SetDlgItemText(IDC_EDIT_FILE_PATH, PathName);
			}
		}
#endif
	}
	else if (strModeName == UPGRADE_MODE_STRING_GET_LOG || strModeName == UPGRADE_MODE_STRING_GET_FILE || strModeName == UPGRADE_MODE_STRING_AUTO)
	{
		//选择输出路径  
		TCHAR szDir[MAX_PATH];
		BROWSEINFO bi;
		ITEMIDLIST *pidl;
		bi.hwndOwner = this->m_hWnd;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = szDir;//这个是输出缓冲区 
		if (strModeName == UPGRADE_MODE_STRING_AUTO)
			bi.lpszTitle = _T("Choose a path"); //标题 
		else
			bi.lpszTitle = _T("Choose a folder to save"); //标题  
		bi.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
		bi.lpfn = NULL;
		bi.lParam = 0;
		bi.iImage = 0;
		pidl = SHBrowseForFolder(&bi);//弹出对话框   
		if (pidl == NULL)//点了取消，或者选择了无效的文件夹则返回NULL  
			return;

		if (SHGetPathFromIDList(pidl, szDir))
			SetDlgItemText(IDC_EDIT_FILE_PATH, szDir);
	}
	else if (strModeName == UPGRADE_MODE_STRING_CONFIG)
	{
		CFileDialog fileDlg(TRUE, "*.tar.gz", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Config File(*.tar.gz)|*.tar.gz|", 0);
		if (fileDlg.DoModal() == IDOK)
		{		

			if(fileDlg.GetFileName().MakeLower().Find("config")==0 && fileDlg.GetFileName().MakeLower().Find(".tar.gz"))
				//设置新的文本
				SetDlgItemText(IDC_EDIT_FILE_PATH, fileDlg.GetPathName());
			else
			{
				switch (Hid_Upgrade.Language)
				{
				case LANGUAGE_SIMPLIFIED_CHINESE:
					AfxMessageBox(_T("文件名称错误！"), MB_ICONINFORMATION);
					break;

				case LANGUAGE_ENGLISH:
				default:
					MessageBoxEx(NULL, _T("Please select an right file name!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
					//AfxMessageBox(_T("Please select an right file name!"), MB_ICONINFORMATION);
					break;
				}
				
			}
		}
	}
	else
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("请选择模式！"), MB_ICONASTERISK);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Please select an upgrade type!"), "USB Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Please select an upgrade type!"), MB_ICONASTERISK);
			break;
		}
	}
}


void CUSB_UpgradeDlg::OnBnClickedButtonUpgrade()
{
	times = 0;
	if (Hid_Upgrade.CameraInfo.MyDevFound)
		SetEvent(UpgradeEvent[0]);
}


void CUSB_UpgradeDlg::OnBnClickedButtonAutoUpgrade()
{
	times = 0;
	if (!Hid_Upgrade.CameraInfo.MyDevFound)
		return;

	//DisableCtrl();
	CString str = "Upgrade Files : ";
	AutoUSBFile = "";
	AutoFPGAFile = "";
	AutoUSBBootLoaderFile = "";
	AutoPKGFile = "";
	AutoMTDFile = "";
	AutoCFGFile = "";

	CString PathNameString;
	m_StaticFilePath.GetWindowText(PathNameString);

	if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID || Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (PathNameString == "")
			AutoMTDFile = Hid_Upgrade.BrowPathFiles("JX1700U*.img");
		else
			AutoMTDFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "JX1700U*.img");
		if (AutoMTDFile == "")
		{
			if (PathNameString == "")
				AutoMTDFile = Hid_Upgrade.BrowPathFiles("J1702C*.img");
			else
				AutoMTDFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "J1702C*.img");
		}

		if (AutoMTDFile != "")
		{
			str += "\r\n";
			str += "MTD: " + AutoMTDFile;

			AutoCFGFile = Hid_Upgrade.BrowPathFiles("config*.tar.gz");
			if (AutoCFGFile != "")
				{
					str += "\r\n";
					str += "CFG: " + AutoCFGFile;
				}
			if (IDNO == ::MessageBoxEx(NULL, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO| MB_SYSTEMMODAL| MB_TOPMOST, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
			//if (IDNO == ::MessageBox(this->m_hWnd, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO))
			{
				//选择了否，退出升级
				return;
			}
			

			if (Hid_Upgrade.CameraInfo.MyDevFound)
				SetEvent(UpgradeEvent[1]);

			return;
		}
		else
		{	

			if (PathNameString == "")
				AutoPKGFile = Hid_Upgrade.BrowPathFiles("JX1700U*.pkg");
			else
				AutoPKGFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "JX1700U*.pkg");
			if(AutoPKGFile=="")
				AutoPKGFile = Hid_Upgrade.BrowPathFiles("Lenovo_360*.pkg");//添加 PKG头检测 20200727

			if (AutoPKGFile != "")
			{
				str += "\r\n";
				str += "PKG: " + AutoPKGFile;
				if (IDNO == ::MessageBoxEx(NULL, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO| MB_SYSTEMMODAL| MB_TOPMOST, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
				//if (IDNO == ::MessageBox(this->m_hWnd, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO))
				{
					//选择了否，退出升级
					return;
				}

				if (Hid_Upgrade.CameraInfo.MyDevFound)
					SetEvent(UpgradeEvent[1]);
				return;
			}
		}

		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL,_T("Not find the upgrade files!"), "Auto Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Not find the upgrade files!"), MB_ICONINFORMATION);
			break;
		}


	}
	else if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1)
	{
		if (PathNameString == "")
		{
			AutoUSBFile = Hid_Upgrade.BrowPathFiles("*_USB_*.img");
			AutoFPGAFile = Hid_Upgrade.BrowPathFiles("*FPGA*.bin");
			AutoUSBBootLoaderFile = Hid_Upgrade.BrowPathFiles("*USB_Bootloader_*.img");
		}	
		else
		{
			AutoUSBFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*_USB_*.img");
			AutoFPGAFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*FPGA*.bin");
			AutoUSBBootLoaderFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*USB_Bootloader_*.img");
		}
			

		if (AutoUSBFile != "" || AutoFPGAFile != "" || AutoUSBBootLoaderFile != "")
		{
			if (AutoUSBFile != "")
			{
				str += "\r\n";
				str += "USB: " + AutoUSBFile;
			}


			if (AutoFPGAFile != "")
			{
				str += "\r\n";
				str += "FPGA: " + AutoFPGAFile;
			}


			if (AutoUSBBootLoaderFile != "")
			{
				str += "\r\n";
				str += "USB Bootloader: " + AutoUSBBootLoaderFile;
			}

			if (AutoPKGFile != "")
			{
				str += "\r\n";
				str += "PKG: " + AutoPKGFile;
			}
			if (IDNO == ::MessageBoxEx(NULL, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
			//if (IDNO == ::MessageBox(this->m_hWnd, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO))
			{
				//选择了否，退出升级
				return;
			}

			if (Hid_Upgrade.CameraInfo.MyDevFound)
				SetEvent(UpgradeEvent[1]);

			return;
		}

		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Not find the upgrade files!"), "Auto Upgrade", MB_OK | MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Not find the upgrade files!"), MB_ICONINFORMATION);
			break;
		}
	}
	else if (Hid_Upgrade.CameraInfo.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{
		{
			AutoMTDFile = Hid_Upgrade.BrowPathFiles("*.img");
			if (AutoMTDFile != "")
			{
				str += "\r\n";
				str += "MTD: " + AutoMTDFile;
				if (IDNO == ::MessageBoxEx(NULL, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
				//if (IDNO == ::MessageBox(this->m_hWnd, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO))
				{
					//选择了否，退出升级
					return;
				}


				if (Hid_Upgrade.CameraInfo.MyDevFound)
					SetEvent(UpgradeEvent[1]);

				return;
			}
		}
		if (PathNameString == "")
		{
			AutoUSBFile = Hid_Upgrade.BrowPathFiles("*_USB_*.img");
			AutoFPGAFile = Hid_Upgrade.BrowPathFiles("*FPGA*.bin");
			AutoUSBBootLoaderFile = Hid_Upgrade.BrowPathFiles("*USB_Bootloader_*.img");

			//AutoMTDFile = Hid_Upgrade.BrowPathFiles("*.img");
			AutoPKGFile = Hid_Upgrade.BrowPathFiles("*.pkg");
		}
		else
		{
			AutoUSBFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*_USB_*.img");
			AutoFPGAFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*FPGA*.bin");
			AutoUSBBootLoaderFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*USB_Bootloader_*.img");
			AutoPKGFile = Hid_Upgrade.BrowPathFiles(PathNameString + "\\" + "*.pkg");
		}


		if (AutoUSBFile != "" || AutoFPGAFile != "" || AutoUSBBootLoaderFile != "" ||
			AutoMTDFile != "" || AutoPKGFile != "")
		{
			if (AutoUSBFile != "")
			{
				str += "\r\n";
				str += "USB: "+ AutoUSBFile;
			}


			if (AutoFPGAFile != "")
			{
				str += "\r\n";
				str += "FPGA: " + AutoFPGAFile;
			}


			if (AutoUSBBootLoaderFile != "")
			{
				str += "\r\n";
				str += "USB Bootloader: " + AutoUSBBootLoaderFile;
			}

			if (AutoPKGFile != "")
			{
				str += "\r\n";
				str += "PKG: " + AutoPKGFile;
			}
				
			if (IDNO == ::MessageBoxEx(NULL, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
			//if (IDNO == ::MessageBox(this->m_hWnd, _T(str), "Auto Upgrade", MB_ICONASTERISK | MB_YESNO))
			{
				//选择了否，退出升级
				return;
			}

			if (Hid_Upgrade.CameraInfo.MyDevFound)
				SetEvent(UpgradeEvent[1]);

			return;
		}

		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("未发现升级文件！"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("Not find the upgrade files!"), "Auto Upgrade", MB_OK| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("Not find the upgrade files!"), MB_ICONINFORMATION);			
			break;
		}
	}
	else
	{
		
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			AfxMessageBox(_T("摄像机版本不支持这个功能"), MB_ICONINFORMATION);
			break;

		case LANGUAGE_ENGLISH:
		default:
			MessageBoxEx(NULL, _T("The camera version not support!"), "Auto Upgrade", MB_OK| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
			//AfxMessageBox(_T("The camera version not support!"), MB_ICONINFORMATION);
			break;
		}
	}

}


void CUSB_UpgradeDlg::OnBnClickedButtonQueryVersion()
{
	if (Hid_Upgrade.CameraInfo.MyDevFound)
	{
		if (Hid_Upgrade.QueryVersion() == 0)
		{
			PromptStringInit();
		}
		else
		{
			switch (Hid_Upgrade.Language)
			{
			case LANGUAGE_SIMPLIFIED_CHINESE:
				AfxMessageBox(_T("版本查询错误！"), MB_ICONINFORMATION);
				break;

			case LANGUAGE_ENGLISH:
			default:
				MessageBoxEx(NULL, _T("Query Version Error!"), "USB Upgrade", MB_OK| MB_SYSTEMMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
				//AfxMessageBox(_T("Query Version Error!"), MB_ICONINFORMATION);
				break;
			}
		}
	}
	else
	{

	}
}


void CUSB_UpgradeDlg::OnDropdownComboUpgradeType()
{
	int cnt = 0;
	((CComboBox*)GetDlgItem(IDC_COMBO_UPGRADE_TYPE))->ResetContent();//消除Combo Box现有所有内容

	if (Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_V1)
	{

		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FPGA);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_PKG);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_MTD);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB_BOOT);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB_BOOTLOADER);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB_RESTORE);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FPGA_RESTORE);

	}
	else if (Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER
		|| Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3_BOOT_LOADER_V1
		|| Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_FX3)
	{
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FPGA);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB_BOOTLOADER);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_USB_BOOT);
	}
	else if (Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HISI_UVC_HID
		|| Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3516_HID)
	{
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_PKG);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_MTD);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FILE);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_GET_LOG);
		m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_GET_FILE);
		//m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_AUTO);
	}
	else if (Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_HI3519_HID
		|| Hid_Upgrade.m_MyHidDevice.HIDUpgradeFirmwareID == DEVICE_UPGRADE_LINUX_HID)
	{
		if (Hid_Upgrade.m_MyHidDevice.HidInputReportByteLength == HID_REPORT_LEN_V1)
		{
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_PKG);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_MTD);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FILE);
		}
		else if (Hid_Upgrade.m_MyHidDevice.HidInputReportByteLength > HID_REPORT_LEN_V1)
		{
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_MTD);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_PKG);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_FILE);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_GET_LOG);
			m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_GET_FILE);

			//if (Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD && Hid_Upgrade.m_MyHidDevice.usb_id.usb_pid == USB_PID_JX1700U)
				m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_CONFIG);
			//m_ctrlCombo_Mode.InsertString(cnt++, UPGRADE_MODE_STRING_AUTO);
		}
	}
}


void CUSB_UpgradeDlg::OnClose()
{
	if (Hid_Upgrade.CameraInfo.UpgradeFlag)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			if ((IDNO == ::MessageBox(this->m_hWnd, "正在升级中，是否要退出 ?", \
				"Close Warning", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				return;
			}
			break;

		case LANGUAGE_ENGLISH:
		default:
			if (IDNO == ::MessageBoxEx(NULL, _T("The firmware is upgrading ,Do you really want to exit ?"), "USB Upgrade", MB_ICONASTERISK | MB_YESNO, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
			//if ((IDNO == ::MessageBox(this->m_hWnd, "The firmware is upgrading ,Do you really want to exit ?", \
				"Close Warning", MB_ICONEXCLAMATION | MB_YESNO)))
			{
				return;
			}
			break;
		}
	}

	CDialogEx::OnClose();
}


void CUSB_UpgradeDlg::OnCbnSelchangeComboUpgradeType()
{
	CString strModeName;

	m_ctrlCombo_Mode.GetWindowText(strModeName);

	if (strModeName == UPGRADE_MODE_STRING_GET_LOG || strModeName == UPGRADE_MODE_STRING_GET_FILE)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "保存路径");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "接收文件");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Save Path");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "Receive File");
			break;
		}


		m_ctrlBtn_AutoUpgrade.EnableWindow(FALSE);

		if (strModeName == UPGRADE_MODE_STRING_GET_FILE)
		{
			if (pEnterReadFileNameDlg)
				pEnterReadFileNameDlg->ShowWindow(SW_SHOW);
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_AUTO)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "路径");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Path");
			break;
		}


	}
	else
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "选择文件");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "升级");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Load File");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "Upgrade");
			break;
		}

		m_ctrlBtn_AutoUpgrade.EnableWindow(TRUE);
	}
}


void CUSB_UpgradeDlg::OnBnClickedButtonDeviceSettings()
{
	if (pDeviceSettingsDlg)
		pDeviceSettingsDlg->ShowWindow(SW_SHOW);

}

void CUSB_UpgradeDlg::LanguageChange()
{
	CString strModeName;

	m_ctrlCombo_Mode.GetWindowText(strModeName);

	switch (Hid_Upgrade.Language)
	{
	case LANGUAGE_SIMPLIFIED_CHINESE:
		SetDlgItemText(IDC_BUTTON_LANGUAGE, "英文");
		SetDlgItemText(IDC_STATIC_UPGRADE_TYPE, "模式:");
		SetDlgItemText(IDC_BUTTON_DEVICE_SETTINGS, "设置");
		SetDlgItemText(IDC_BUTTON_QUERY_VERSION, "版本查询");
		SetDlgItemText(IDC_BUTTON_AUTO_UPGRADE, "一键升级");
		SetDlgItemText(IDC_BUTTON_UPGRADE, "升级");
		SetDlgItemText(IDC_BUTTON_LOAD_FILE, "选择文件");
		if (Hid_Upgrade.CameraInfo.UpgradeFlag == 0)
		{
			if (Hid_Upgrade.CameraInfo.MyDevFound)
				DisplayStatus("摄像机连接成功");
			else
				DisplayStatus("摄像机连接失败");
		}
		break;

	case LANGUAGE_ENGLISH:
	default:
		SetDlgItemText(IDC_BUTTON_LANGUAGE, "Chinese");
		SetDlgItemText(IDC_STATIC_UPGRADE_TYPE, "Type:");
		SetDlgItemText(IDC_BUTTON_DEVICE_SETTINGS, "Settings");
		SetDlgItemText(IDC_BUTTON_QUERY_VERSION, "Query Version");
		SetDlgItemText(IDC_BUTTON_AUTO_UPGRADE, "Auto Upgrade");
		SetDlgItemText(IDC_BUTTON_UPGRADE, "Upgrade");
		SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Load File");
		if (Hid_Upgrade.CameraInfo.UpgradeFlag == 0)
		{
			if (Hid_Upgrade.CameraInfo.MyDevFound)
				DisplayStatus("Camera connection is successful");
			else
				DisplayStatus("Camera connection failed");
		}

		break;
	}


	if (strModeName == UPGRADE_MODE_STRING_GET_LOG || strModeName == UPGRADE_MODE_STRING_GET_FILE)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "保存路径");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "接收文件");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Save Path");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "Receive File");
			break;
		}


		m_ctrlBtn_AutoUpgrade.EnableWindow(FALSE);

		if (strModeName == UPGRADE_MODE_STRING_GET_FILE)
		{
			enter_read_file_name* pOneDlgObj = new enter_read_file_name;
			if (pOneDlgObj)
			{
				BOOL ret = pOneDlgObj->Create(IDD_DIALOG_SET_READ_FILE_NAME, this);
			}

			pOneDlgObj->ShowWindow(SW_SHOW);
		}
	}
	else if (strModeName == UPGRADE_MODE_STRING_AUTO)
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "路径");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Path");
			break;
		}


	}
	else
	{
		switch (Hid_Upgrade.Language)
		{
		case LANGUAGE_SIMPLIFIED_CHINESE:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "选择文件");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "升级");
			break;

		case LANGUAGE_ENGLISH:
		default:
			SetDlgItemText(IDC_BUTTON_LOAD_FILE, "Load File");
			SetDlgItemText(IDC_BUTTON_UPGRADE, "Upgrade");
			break;
		}

		m_ctrlBtn_AutoUpgrade.EnableWindow(TRUE);
	}
}

void CUSB_UpgradeDlg::OnBnClickedButtonLanguage()
{
	Hid_Upgrade.Language++;
	if (Hid_Upgrade.Language > LANGUAGE_MAX_NUM - 1)
		Hid_Upgrade.Language = 0;

	LanguageChange();

}


