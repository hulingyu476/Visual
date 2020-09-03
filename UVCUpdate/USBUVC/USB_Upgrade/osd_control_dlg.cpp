// osd_control_dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USB_Upgrade.h"
#include "osd_control_dlg.h"
#include "afxdialogex.h"
#include "USB_UpgradeDlg.h"


// osd_control_dlg 对话框

IMPLEMENT_DYNAMIC(osd_control_dlg, CDialogEx)

osd_control_dlg::osd_control_dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_OSD_CTRL, pParent)
{

}

osd_control_dlg::~osd_control_dlg()
{
}

void osd_control_dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(osd_control_dlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_OSD_OK, &osd_control_dlg::OnBnClickedButtonOsdOk)
	ON_BN_CLICKED(IDC_BUTTON_OSD_UP, &osd_control_dlg::OnBnClickedButtonOsdUp)
	ON_BN_CLICKED(IDC_BUTTON_OSD_DOWN, &osd_control_dlg::OnBnClickedButtonOsdDown)
	ON_BN_CLICKED(IDC_BUTTON_OSD_LEFT, &osd_control_dlg::OnBnClickedButtonOsdLeft)
	ON_BN_CLICKED(IDC_BUTTON_OSD_RIGHT, &osd_control_dlg::OnBnClickedButtonOsdRight)
	ON_BN_CLICKED(IDC_BUTTON_OSD_OPEN_CLOSE, &osd_control_dlg::OnBnClickedButtonOsdOpenClose)
	ON_BN_CLICKED(IDC_BUTTON_OSD_BACK, &osd_control_dlg::OnBnClickedButtonOsdBack)
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// osd_control_dlg 消息处理程序


void osd_control_dlg::OnBnClickedButtonOsdOk()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-OK", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdUp()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-UP", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdDown()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-DOWN", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdLeft()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-LEFT", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdRight()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-RIGHT", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdOpenClose()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-OPEN_OR_CLOSE", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnBnClickedButtonOsdBack()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-BACK", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
}


void osd_control_dlg::OnClose()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-CLOSE", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Settings Error!"), MB_ICONINFORMATION);
	}
	CDialogEx::OnClose();
}


void osd_control_dlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
#if 0
	CDialogEx::OnShowWindow(bShow, nStatus);

	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-OPEN", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Open OSD Error!"), MB_ICONINFORMATION);
		//DestroyWindow();
	}
#endif
}
