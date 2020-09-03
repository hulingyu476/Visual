// enter_read_file_name.cpp : 实现文件
//

#include "stdafx.h"
#include "USB_Upgrade.h"
#include "USB_UpgradeDlg.h"
#include "enter_read_file_name.h"
#include "afxdialogex.h"


// enter_read_file_name 对话框

IMPLEMENT_DYNAMIC(enter_read_file_name, CDialogEx)

enter_read_file_name::enter_read_file_name(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_SET_READ_FILE_NAME, pParent)
{

}

enter_read_file_name::~enter_read_file_name()
{
}

void enter_read_file_name::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_READ_FILE_NAME, m_EditFileName);
}


BEGIN_MESSAGE_MAP(enter_read_file_name, CDialogEx)
	ON_BN_CLICKED(IDOK, &enter_read_file_name::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_READ_FILE_NAME, &enter_read_file_name::OnEnChangeEditReadFileName)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// enter_read_file_name 消息处理程序


void enter_read_file_name::OnBnClickedOk()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();
	CString FileName;
	// TODO: 在此添加控件通知处理程序代码
	m_EditFileName.GetWindowText(FileName);
	pWnd ->strGetFileName = FileName;
	CDialogEx::OnOK();
}


void enter_read_file_name::OnEnChangeEditReadFileName()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void enter_read_file_name::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	SetDlgItemText(IDC_EDIT_READ_FILE_NAME, pWnd->strGetFileName);

	CDialogEx::OnShowWindow(bShow, nStatus);
}
