#项目说明
##FicXuUpdateDllTest
    UI版本的升级工具
##CameraUpgradeDll
    生成dll
##ConsoleTest
    控制台测试程序

***
#项目遇到的问题
`CreateThread(NULL, 1000, (LPTHREAD_START_ROUTINE)fic_xu_update_thread(&load_data_info), this, 1, &threadID); //TODO`
在dll不能进入多线程？
使用下面替代
`(HANDLE)_beginthreadex(0, 0, (unsigned int(__stdcall *)(void *))fic_xu_update_thread, (LPVOID)(&load_data_info), 0, 0);`
