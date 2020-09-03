// enter_read_file_name.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USB_Upgrade.h"
#include "USB_UpgradeDlg.h"
#include "enter_read_file_name.h"
#include "afxdialogex.h"


// enter_read_file_name �Ի���

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


// enter_read_file_name ��Ϣ�������


void enter_read_file_name::OnBnClickedOk()
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();
	CString FileName;
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	m_EditFileName.GetWindowText(FileName);
	pWnd ->strGetFileName = FileName;
	CDialogEx::OnOK();
}


void enter_read_file_name::OnEnChangeEditReadFileName()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void enter_read_file_name::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CUSB_UpgradeDlg *pWnd = (CUSB_UpgradeDlg*)GetParent();

	SetDlgItemText(IDC_EDIT_READ_FILE_NAME, pWnd->strGetFileName);

	CDialogEx::OnShowWindow(bShow, nStatus);
}
