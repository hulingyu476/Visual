#pragma once


// osd_control_dlg 对话框

class osd_control_dlg : public CDialogEx
{
	DECLARE_DYNAMIC(osd_control_dlg)

public:
	osd_control_dlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~osd_control_dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_OSD_CTRL };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOsdOk();
	afx_msg void OnBnClickedButtonOsdUp();
	afx_msg void OnBnClickedButtonOsdDown();
	afx_msg void OnBnClickedButtonOsdLeft();
	afx_msg void OnBnClickedButtonOsdRight();
	afx_msg void OnBnClickedButtonOsdOpenClose();
	afx_msg void OnBnClickedButtonOsdBack();
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
