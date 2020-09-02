
// FicXuUpdateDllTestDlg.h : Í·ÎÄ¼þ
//
#pragma once

#include <dshow.h>
#include <Vidcap.h>
#include <KsProxy.h>
#define DATA_PACKET_UNIT		1024

typedef struct 
{
	UINT8 *buf;
	int len;
	//Device HANDER
	IBaseFilter	 *pVCap;
	ULONG NodeId;
	CProgressCtrl *pPrgCtrl;
}load_info;

#define MAX_ADDR_FIELD      10

#define WM_MSG_UPDATE_END	WM_USER+1

// CFicXuUpdateDllTestDlg
class CFicXuUpdateDllTestDlg : public CDialogEx
{
public:
	CFicXuUpdateDllTestDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_FICXUUPDATEDLLTEST_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD dwData);
	afx_msg void OnBnClickedBtnFwPath();
	afx_msg void OnBnClickedBtnVersion();
	afx_msg void OnBnClickedBtnFwVer();
	afx_msg void OnBnClickedBtnUpdate();
	CString m_EditFwPath;
	CString m_DeviceVersion;
	CString m_FirmwareVersion;
	CComboBox m_DeviceList;
public:
	/************************************************************************
	Usb device handle
	************************************************************************/
	IMoniker *m_pVideoMoniker[10];
	IBaseFilter	 *m_pVCapMult[10];
	ULONG	m_pNodeId[10];
	int index;

	void DlgWindowsEnable(BOOL isEnable);
	int RefreshUvcDevices(IMoniker **ppMoniker, IBaseFilter **ppVCapMult, ULONG *pNodeId);
	int RefreshDeviceList();
	bool GetDeviceInfo(CString &strInfo, IMoniker *pmVideo);
	void UsbDetectRegister();
	void uvc_device_refresh();

	CString m_EditDevManufacturer;
	CString m_EditDevProduct;
	CString m_EditDevPlatform;
	afx_msg void OnBnClickedBtnDevMftr();
	afx_msg void OnBnClickedBtnDevPro();
	afx_msg void OnBnClickedBtnDevPtm();
	CString m_EditFwPlatform;
	afx_msg void OnBnClickedBtnFwPtm();
	afx_msg void OnBnClickedBtnDevVid();
	CString m_EditDevUsbVid;
	CString m_EditDevUsbPid;
	afx_msg void OnBnClickedBtnDevPid();
	CProgressCtrl m_PrgUpdate;
	afx_msg LRESULT MsgUpdateEndHandler(WPARAM,LPARAM);
	afx_msg void OnBnClickedBtnDevReboot();
};
