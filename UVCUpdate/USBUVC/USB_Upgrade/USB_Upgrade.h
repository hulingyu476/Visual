
// USB_Upgrade.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CUSB_UpgradeApp: 
// �йش����ʵ�֣������ USB_Upgrade.cpp
//

class CUSB_UpgradeApp : public CWinApp
{
public:
	CUSB_UpgradeApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CUSB_UpgradeApp theApp;