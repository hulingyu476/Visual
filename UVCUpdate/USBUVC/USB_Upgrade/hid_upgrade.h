#if !defined(__HID_UPGRADE_H)
#define __HID_UPGRADE_H

#include "stdafx.h"
#include "hid.h"
#include "hid_types.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define FLASH_ID_MX25L64	(0xC2201700)
/*
* MX25L64
*2048 Equal Sectors with 4K byte each
* Any Sector can be erased individually
*128 Equal Blocks with 64K byte each
*Any Block can be erased individually
*Program Capability
* Byte base
*Page base (256 bytes)
*/
#define FLASH_ID_ST_M25P64	(0x20201700)
/*
* M25P64
*64Mbit of Flash Memory
*Page Program (up to 256 Bytes) in 1.4ms (typical)
*Sector Erase (512Kbit)
*Bulk Erase (64Mbit)
*/

#define MX25L64_SECTOR_SIZE	 	0x1000	//4K
#define ST_M25P64_SECTOR_SIZE	0x10000	//64K


#define	V50U  		(0)
#define	V60U 		(1)
//BOOTLOADER	(2)
#define	V7xU 		(3)

#define FPGA_BOOT_ADDR          	(0)
#define FPGA_RESTORE_ADDR          	(0x80000)
#define BOOT_FORM_SPI_ADDR          (0x780000)//8M - 512K = 8388608 - 524288 = 7864320
#define BOOT_FORM_SPI_ADDR_RESTORE  (BOOT_FORM_SPI_ADDR + 0x40000)// + 256K



#ifdef HID_UPGRADE_EXPORTS
#define HID_UPGRADE_API __declspec(dllexport)
#else
#define HID_UPGRADE_API __declspec(dllimport)
#endif

//#pragma warning(disable:4996)


#define LANGUAGE_MAX_NUM          	(2)
typedef enum {
	LANGUAGE_ENGLISH,
	LANGUAGE_SIMPLIFIED_CHINESE,
} language_type;


HID_UPGRADE_API class CHid_Upgrade
{
// Construction
public:
	CHid m_MyHidDevice;	
	struct camera_device_info CameraInfo;
	unsigned int Language;

	//MD5
	void setByteOrder(void);
	void byteReverse(unsigned char *buf, unsigned longs);
	void MD5Init(struct MD5Context *context);
	void MD5Update(struct MD5Context *context, unsigned char const *buf, unsigned len);
	void MD5Final(unsigned char digest[16], struct MD5Context *context);
	void MD5Transform(DWORD buf[4], DWORD const in[16]);
	BOOL mdfile(unsigned char *file_buff, unsigned  file_buff_size, unsigned char *digest);
	BOOL md5sum(unsigned char *file_buff, unsigned file_buff_size, unsigned char *digest);
	BOOL GetMd5Hash(CONST BYTE *pbData, DWORD dwDataLen, CString &strHash/*, DWORD &err*/);
	//CRC
	DWORD crc32_no_comp(DWORD crc, const BYTE *buf, DWORD len);
	DWORD crc32(DWORD crc, const BYTE *p, DWORD len);
	WORD cyg_crc16(unsigned char *buf, int len);

	
	//Transmit
	BOOL HidSendReport(unsigned char *buff, unsigned int PackSize);
	BOOL HidGetReport(unsigned char *buff, unsigned int PackSize, unsigned int Timeout_ms = 5000);
	unsigned char CheckReportStatus(unsigned char *buff, unsigned int Timeout_ms = 5000);
	BOOL SendCharsCommand_V1(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms = 5000);
	BOOL SendCharsCommand(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms = 5000);
	BOOL SendFileInfo(struct hid_file_info *FileInfo);
	BOOL SendFileMd5(struct hid_file_md5 *FileMd5);
	BOOL SendFileData(struct hid_file_data *FileData);
	BOOL HidDataTransfer(struct hid_data *HidData);
	unsigned char QueryHidStatus(void);
	BOOL SendQueryVersion(struct hid_version *version);
	
	//Transmit V2
	unsigned char CheckReportStatus_V2(unsigned char *buff, unsigned int Timeout_ms = 5000);
	BOOL SendCharsCommand_V2(char *String, unsigned char report_head, unsigned char CheckStatusFlag, unsigned int Timeout_ms = 5000);
	BOOL SendFileInfo_V2(struct hid_file_info_v2 *FileInfo);
	BOOL SendFileMd5_V2(struct hid_file_md5_v2 *FileMd5);
	BOOL SendFileData_V2(struct hid_file_data_v2 *FileData);
	BOOL HidDataTransfer_V2(struct hid_data_v2 *HidData);
	unsigned char QueryHidStatus_V2(void);
	BOOL SendQueryVersion_V2(struct hid_version_v2 *version);
	BOOL SendReadeFileInfo_V2(struct hid_read_file_info_v2 *ReadFileInfo);
	BOOL GetReadFileMd5_V2(struct hid_read_file_md5_v2 *ReadFileMd5);
	BOOL GetReadFileData_V2(struct hid_read_file_data_v2 *ReadFileData);
	
	
	//API
	int Hid_UpgradeInit(void *pParam);
	void CameraInfoInit(void);
	//UINT QueryThread(void *pParam);
	CString BrowPathFiles(CString path_ext_name);
	BOOL DataComparison(unsigned char *buff1, unsigned char *buff2, unsigned long int  size);
	BOOL HidSend(unsigned char *TxBuffer, unsigned char *RxBuffer, unsigned char PackSize);
	int FpgaBinCheckFileCrc(BYTE * FileBuff, DWORD FileSize);
	int Fx3ImgChecksum(BYTE * FileBuff, DWORD FileSize);
	int VersionString2Number(const BYTE *VersionString, const char *StringHeader, version_t *Version);
	int String2VersionNumber(CString VersionString, DWORD StrStart, version_t *Version);
	int GetFirmwareFileVersion(const unsigned char *fpbuff, DWORD FileSize, file_type FileType, version_t *FileVersion);
	

	BOOL IsBootLoader();
	int CHid_Upgrade::FindDevice();
	void DeviceConnect(void);
	void DeviceDisconnect(void);
	int QueryVersion();
	int QueryVersion_V1();
	int QueryVersion_V2();


	int FX3Upgrade(CString File, CString strModeName, void *pParam);
	int UsbUpgrade2FpgaFlash(CString File, CString strModeName, void *pParam);
	int FPGAUpgrade(CString File, CString strModeName, void *pParam);
	int SendFile(unsigned char FileType, CString File, void *pParam);
	int SendFile_V1(unsigned char FileType, CString File, void *pParam);
	int SendFile_V2(unsigned char FileType, CString File, void *pParam);
	int GetFile_V2(CString FileName, CString SavePath, void *pParam);

	int ResetOsdSetting(void);
	int MotorTestRotate(void);
	int LensCalibration(void *pParam);
	int HotPixelsCalibration(void *pParam);

	int SetDeviceName(CString NameString, unsigned char *DeviceSettings);
	int SetSNName(CString NameString, unsigned char *DeviceSettings);
	int GetDeviceName(CString *NameString, unsigned char *DeviceSettings);

	int DeviceReboot(void);
	int DeviceReConnect(void);

	int VersionCheck(CString strModeName, CString FilePath);
	
protected:
	static UINT QueryThread(void *pParam);
};

#define	CHAR_TO_DEC_NUMBER(x)		(x - 0x30)
#define CHECK_CHAR_IS_DEC_NUMBER(x) (((x - 0x30) > 10) ? 0 : (((x - 0x30) < 0) ? 0 : 1))

//#define CHECK_STRING_DESCR(x)	(((x >= 32 && x <= 126) && !(x == '\' || x == '/' || x == ':' || x == '*' || x == '?'|| x == '"' || x == '<'|| x == '>' || x == '|')) ? 1 : 0)
#define CHECK_STRING_DESCR(x)	(((x >= 32 && x <= 126) && !(x == 92 || x == 47 || x == 58 || x == 42 || x == 63 || x == 34 || x == 60|| x == 62 || x == 124)) ? 1 : 0)

#endif // !defined(__HID_UPGRADE_H)
