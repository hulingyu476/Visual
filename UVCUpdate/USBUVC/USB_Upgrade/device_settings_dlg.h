#pragma once


// device_settings_dlg 对话框

class device_settings_dlg : public CDialogEx
{
	DECLARE_DYNAMIC(device_settings_dlg)

public:
	device_settings_dlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~device_settings_dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DEVICE_SETTINGS };
#endif

	CButton	m_ctrlBtn_Settings_Reboot;
	CButton	m_ctrlBtn_Settings_Reconnect;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSettingReboot();
	afx_msg void OnBnClickedButtonSettingsReconnect();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonSettingsOsd();
	afx_msg void OnBnClickedButtonHotPixels();
	afx_msg void OnEnChangeEditUsbSn();
	afx_msg void OnBnClickedButtonLensCal();
	afx_msg void OnBnClickedButtonMotorTest();
};
