
// USB_UpgradeDlg.h : 头文件
//

#pragma once

#include "hid_upgrade.h"
#include "enter_read_file_name.h"
#include "device_settings_dlg.h"
#include "osd_control_dlg.h"


#define APP_VERSION	"USB Upgrade V1.1.0.56"


#define UPGRADE_MODE_STRING_USB				"USB"
#define UPGRADE_MODE_STRING_FPGA			"FPGA"
#define UPGRADE_MODE_STRING_PKG				"PKG"
#define UPGRADE_MODE_STRING_MTD				"MTD"
#define UPGRADE_MODE_STRING_USB_BOOT		"USB-Boot"
#define UPGRADE_MODE_STRING_USB_BOOTLOADER	"Bootloader"
#define UPGRADE_MODE_STRING_USB_RESTORE		"USB-Restore"
#define UPGRADE_MODE_STRING_FPGA_RESTORE	"FPGA-Restore"
#define UPGRADE_MODE_STRING_FILE			"FILE"
#define UPGRADE_MODE_STRING_GET_FILE		"GET FILE"
#define UPGRADE_MODE_STRING_GET_LOG			"GET LOG"
#define UPGRADE_MODE_STRING_AUTO			"AUTO"
#define UPGRADE_MODE_STRING_CONFIG			"CONFIG"

// CUSB_UpgradeDlg 对话框
class CUSB_UpgradeDlg : public CDialogEx
{
// 构造
public:
	CUSB_UpgradeDlg(CWnd* pParent = NULL);	// 标准构造函数

	LRESULT OnDeviceChange(WPARAM wParam, LPARAM lParam);

	void EnableCtrl(void);
	void DisableCtrl(void);

	void DisplayStatus(const char *string);
	void PromptSetPos(int pos);

	CHid_Upgrade Hid_Upgrade;
	CString strGetFileName;

	device_settings_dlg* pDeviceSettingsDlg;
	enter_read_file_name* pEnterReadFileNameDlg;
	osd_control_dlg* pOsdControlDlg;

	
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_USB_UPGRADE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;
	//CTabCtrl TabCtrlUpgrade;
	CComboBox m_ctrlCombo_Mode;
	CButton	m_ctrlBtn_AddFile;
	CButton	m_ctrlBtn_UpgradeStar;
	CButton	m_ctrlBtn_AutoUpgrade;
	CButton	m_ctrlBtn_QueryVersion;
	CProgressCtrl 	m_ctrlUpgradeProgress;
	CEdit	m_StaticFilePath;
	CButton	m_ctrlBtn_Settings;
	CButton	m_ctrlBtn_Language;

	CString PromptString;

	
	void ErrorInfoHandle(CString strModeName, int ret);
	void UpgradeHandle(void *pParam);

	void AutoUpgradeHandle(void *pParam);

	void DeviceConnect(void);
	void DeviceDisconnect(void);

	//void DisplayStatus(const char *string);
	//void PromptSetPos(int pos);
	void PromptStringInit(void);
	void PromptStringAdd(CString str);
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	static UINT UpgradeThread(void *pParam);
	void LanguageChange();
public:
//	afx_msg void OnCbnSelchangeComboUpgradeType();
	afx_msg void OnBnClickedButtonLoadFile();
	afx_msg void OnBnClickedButtonUpgrade();
	afx_msg void OnBnClickedButtonAutoUpgrade();
	afx_msg void OnBnClickedButtonQueryVersion();
	afx_msg void OnDropdownComboUpgradeType();
	afx_msg void OnClose();
	afx_msg void OnCbnSelchangeComboUpgradeType();
	afx_msg void OnBnClickedButtonDeviceSettings();
	afx_msg void OnBnClickedButtonLanguage();
};
