// device_settings_dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USB_Upgrade.h"
#include "device_settings_dlg.h"
#include "afxdialogex.h"
#include "USB_UpgradeDlg.h"
#include "osd_control_dlg.h"


// device_settings_dlg 对话框
extern BOOL cmdstart;

IMPLEMENT_DYNAMIC(device_settings_dlg, CDialogEx)

device_settings_dlg::device_settings_dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_DEVICE_SETTINGS, pParent)
{

}

device_settings_dlg::~device_settings_dlg()
{
}

void device_settings_dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_SETTING_REBOOT, m_ctrlBtn_Settings_Reboot);
	DDX_Control(pDX, IDC_BUTTON_SETTINGS_RECONNECT, m_ctrlBtn_Settings_Reconnect);
}


BEGIN_MESSAGE_MAP(device_settings_dlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SETTING_REBOOT, &device_settings_dlg::OnBnClickedButtonSettingReboot)
	ON_BN_CLICKED(IDC_BUTTON_SETTINGS_RECONNECT, &device_settings_dlg::OnBnClickedButtonSettingsReconnect)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDOK, &device_settings_dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_SETTINGS_OSD, &device_settings_dlg::OnBnClickedButtonSettingsOsd)
	ON_BN_CLICKED(IDC_BUTTON_HOT_PIXELS, &device_settings_dlg::OnBnClickedButtonHotPixels)
	ON_EN_CHANGE(IDC_EDIT_USB_SN, &device_settings_dlg::OnEnChangeEditUsbSn)
	ON_BN_CLICKED(IDC_BUTTON_LENS_CAL, &device_settings_dlg::OnBnClickedButtonLensCal)
	ON_BN_CLICKED(IDC_BUTTON_MOTOR_TEST, &device_settings_dlg::OnBnClickedButtonMotorTest)
END_MESSAGE_MAP()


// device_settings_dlg 消息处理程序


void device_settings_dlg::OnBnClickedButtonSettingReboot()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();
	if (pWnd->Hid_Upgrade.DeviceReboot() == 0)
	{
		CDialogEx::OnOK();
	}
	else
	{
		if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
		else
			AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
	}
}


void device_settings_dlg::OnBnClickedButtonSettingsReconnect()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	if (pWnd->Hid_Upgrade.DeviceReConnect() == 0)
	{
		CDialogEx::OnOK();
	}
	else
	{
		if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
		else
			AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
	}
}


void device_settings_dlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();
	
	if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
	{
		SetDlgItemText(IDC_BUTTON_SETTING_REBOOT, "重启设备");
		SetDlgItemText(IDC_BUTTON_SETTINGS_RECONNECT, "重连USB");
		SetDlgItemText(IDC_BUTTON_SETTINGS_OSD, "打开菜单");
		SetDlgItemText(IDC_BUTTON_HOT_PIXELS, "坏点矫正");

		SetDlgItemText(IDC_BUTTON_LENS_CAL, "曲线矫正");
		SetDlgItemText(IDC_BUTTON_MOTOR_TEST, "云台测试");

		SetDlgItemText(IDOK, "确认");
		SetDlgItemText(IDCANCEL, "取消");
	}
	else
	{
		SetDlgItemText(IDC_BUTTON_SETTING_REBOOT, "Dvice Reboot");
		SetDlgItemText(IDC_BUTTON_SETTINGS_RECONNECT, "Reconnect USB");
		SetDlgItemText(IDC_BUTTON_SETTINGS_OSD, "Open OSD");
		SetDlgItemText(IDC_BUTTON_HOT_PIXELS, "Hot Pixels Calibration");

		SetDlgItemText(IDC_BUTTON_LENS_CAL, "Lens Calibration");
		SetDlgItemText(IDC_BUTTON_MOTOR_TEST, "Motor Test");

		SetDlgItemText(IDOK, "Confirm");
		SetDlgItemText(IDCANCEL, "Cancel");
	}

	if ((pWnd->Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD))
	{
		((CButton*)GetDlgItem(IDC_BUTTON_HOT_PIXELS))->EnableWindow(TRUE);
		((CButton*)GetDlgItem(IDC_BUTTON_SETTINGS_OSD))->EnableWindow(TRUE);
		((CButton*)GetDlgItem(IDC_EDIT_USB_SN))->EnableWindow(TRUE);//禁用修改SN选项

		CString SNString = "";
		SNString = pWnd->Hid_Upgrade.m_MyHidDevice.GetSerialNumberString();
		if (SNString == "")
			((CEdit*)GetDlgItem(IDC_EDIT_USB_SN))->SetWindowText("000000000000000");
		else
			((CEdit*)GetDlgItem(IDC_EDIT_USB_SN))->SetWindowText(SNString);
	}
	//else if (pWnd->Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_POLYCOM &&pWnd->Hid_Upgrade.m_MyHidDevice.usb_id.usb_pid == USB_PID_V61PU)
	else if (pWnd->Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD)
	{
		((CButton*)GetDlgItem(IDC_BUTTON_HOT_PIXELS))->EnableWindow(TRUE);
		((CButton*)GetDlgItem(IDC_BUTTON_SETTINGS_OSD))->EnableWindow(FALSE);

		((CButton*)GetDlgItem(IDC_BUTTON_LENS_CAL))->EnableWindow(TRUE);
		((CButton*)GetDlgItem(IDC_BUTTON_MOTOR_TEST))->EnableWindow(TRUE);
		((CButton*)GetDlgItem(IDC_EDIT_USB_SN))->EnableWindow(TRUE);//禁用修改SN选项

		CString SNString = "";
		SNString = pWnd->Hid_Upgrade.m_MyHidDevice.GetSerialNumberString();
		if (SNString == "")
			((CEdit*)GetDlgItem(IDC_EDIT_USB_SN))->SetWindowText("000000000000000");
		else
			((CEdit*)GetDlgItem(IDC_EDIT_USB_SN))->SetWindowText(SNString);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_BUTTON_HOT_PIXELS))->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_BUTTON_SETTINGS_OSD))->EnableWindow(FALSE);

		((CButton*)GetDlgItem(IDC_BUTTON_LENS_CAL))->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_BUTTON_MOTOR_TEST))->EnableWindow(FALSE);
		((CButton*)GetDlgItem(IDC_EDIT_USB_SN))->EnableWindow(FALSE);
	}
	//if (!cmdstart)
		((CButton*)GetDlgItem(IDOK))->EnableWindow(FALSE);

	CDialogEx::OnShowWindow(bShow, nStatus);


}


void device_settings_dlg::OnBnClickedOk()
{
	
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

		if ((pWnd->Hid_Upgrade.m_MyHidDevice.usb_id.usb_vid == USB_VID_VHD))
	{
		CString SNString;
		CString SpeedString;
		unsigned char DeviceSettings[3] = { 0xFF, 0xFF, 0xFF };

		((CButton*)GetDlgItem(IDC_EDIT_USB_SN))->GetWindowText(SNString);
		DWORD i;
		for (i = 0; i < SNString.GetLength(); i++)
		{
			if (CHECK_STRING_DESCR((unsigned char)SNString.GetAt(i)) == 0)
			{
				if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
					AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
				else
					AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);

				return;
			}
		}

		{
			if (pWnd->Hid_Upgrade.SetDeviceName(SNString, DeviceSettings) == 0)
			{
				CDialog::OnOK();
			}
			else
			{
				if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
					AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
				else
					AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
			}
		}
		

	}

	CDialogEx::OnOK();
}




void device_settings_dlg::OnBnClickedButtonSettingsOsd()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();


#ifdef _DEBUG
	if (pUSB_UpgradeDlg->pOsdControlDlg)
	{
		pUSB_UpgradeDlg->pOsdControlDlg->ShowWindow(SW_SHOW);
		CDialogEx::OnOK();
	}
#else
	if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("OSD-OPEN", HID_HEAD_CAM, 1) == FALSE)
	{
		AfxMessageBox(_T("Open OSD Error!"), MB_ICONINFORMATION);		
	}
	else
	{
		if (pUSB_UpgradeDlg->pOsdControlDlg)
		{
			pUSB_UpgradeDlg->pOsdControlDlg->ShowWindow(SW_SHOW);
			CDialogEx::OnOK();
		}		
	}
#endif
}


void device_settings_dlg::OnBnClickedButtonHotPixels()
{
	CUSB_UpgradeDlg *pUSB_UpgradeDlg = (CUSB_UpgradeDlg*)GetParent();

	{
		if (pUSB_UpgradeDlg->Hid_Upgrade.SendCharsCommand("SetHotPixelsCalibration", HID_HEAD_CAM, 1) == FALSE)
		{
			if (pUSB_UpgradeDlg->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
				AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
			else
				AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
		}
	}
}


void device_settings_dlg::OnEnChangeEditUsbSn()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	CString TextString;
	DWORD i,length;

}


void device_settings_dlg::OnBnClickedButtonLensCal()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	if (pWnd->Hid_Upgrade.LensCalibration(pWnd) == 0)
	{
		CDialog::OnOK();
	}
	else
	{
		if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
		else
			AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
	}
}


void device_settings_dlg::OnBnClickedButtonMotorTest()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	if (pWnd->Hid_Upgrade.MotorTestRotate() == 0)
	{
		CDialog::OnOK();
	}
	else
	{
		if (pWnd->Hid_Upgrade.Language == LANGUAGE_SIMPLIFIED_CHINESE)
			AfxMessageBox(_T("设置失败!"), MB_ICONINFORMATION);
		else
			AfxMessageBox(_T("Setup failed!"), MB_ICONINFORMATION);
	}

}
