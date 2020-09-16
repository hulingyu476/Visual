1:获取版本号
dfu-util.ext -l
显示内容中
Found Runtime: [2757:2001] ver=0182, devnum=8, cfg=1, intf=0, path="1-1", alt=0, name="DFU Run-Time Interface", serial="1111111111111111"
的ver就是版本号
执行version.bat会生成uvcversion.txt的文件

2：安装DFU驱动(正式镜像会安装，调试需要手动安装)
备注：此固件使用DFU模式升级
手动安装：设备管理器中，手动选择dfu_vhd driver文件更新驱动

3：升级
同文件夹下放置JX1701U*.img的升级文件，
执行dfu_upgrade_vx.bat批处理命令


说明：
dfudriver.bat是安装dfu驱动的尝试