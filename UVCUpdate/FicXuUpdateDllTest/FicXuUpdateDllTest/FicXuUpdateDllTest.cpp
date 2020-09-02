#include "stdafx.h"
#include "FicXuUpdateDllTest.h"
#include "FicXuUpdateDllTestDlg.h"
#include "FicXuUpdate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFicXuUpdateDllTestApp

BEGIN_MESSAGE_MAP(CFicXuUpdateDllTestApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CFicXuUpdateDllTestApp::CFicXuUpdateDllTestApp()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

}


CFicXuUpdateDllTestApp theApp;


// CFicXuUpdateDllTestApp init

BOOL CFicXuUpdateDllTestApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	CShellManager *pShellManager = new CShellManager;

	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	CFicXuUpdateDllTestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{

	}
	else if (nResponse == IDCANCEL)
	{

	}

	if (pShellManager != NULL)
	{
		delete pShellManager;
	}
	return FALSE;
}

