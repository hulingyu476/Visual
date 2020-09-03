#pragma once


// enter_read_file_name 对话框

class enter_read_file_name : public CDialogEx
{
	DECLARE_DYNAMIC(enter_read_file_name)

public:
	enter_read_file_name(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~enter_read_file_name();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SET_READ_FILE_NAME };
#endif

	CEdit	m_EditFileName;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEditReadFileName();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
