
// FicXuUpdateDllTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CFicXuUpdateDllTestApp:
// �йش����ʵ�֣������ FicXuUpdateDllTest.cpp
//

class CFicXuUpdateDllTestApp : public CWinApp
{
public:
	CFicXuUpdateDllTestApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CFicXuUpdateDllTestApp theApp;