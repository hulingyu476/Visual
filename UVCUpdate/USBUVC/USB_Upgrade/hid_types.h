#ifndef __HID_TYPES_H
#define __HID_TYPES_H

#include "config.h"
#pragma pack(1)//1BYTE对齐

struct MD5Context {
	DWORD buf[4];
	DWORD bits[2];
	unsigned char in[64];
};

typedef struct MD5Context MD5_CTX;

struct fx3_img_head {
	WORD wSignature;//两个字节标签使用 ASCII 文本 “CY” 初始化
	BYTE bImageCTL;
	//bImageType = 0xB0：带校验和的普通 FW 二进制镜像
	//bImageType = 0xB2：使用新的 VID 和 PID 进行 I2C/SPI 启动
	BYTE bImageType;
};

struct fx3_img_end {
	DWORD val;
	DWORD entryAddr;
	DWORD checksum;
};

struct fx3_img_crc {
	DWORD crc_flag;
	DWORD crc_sum;
	DWORD crc_len;
};

struct version_t {
	BYTE Mode;
	BYTE BigVersion;
	BYTE LittleVersion;
	char CustomCode;
};

struct camera_device_info {

	version_t USBVersion;
	version_t FPGAVersion;
	version_t SOCVersion;
	version_t SocHidVersion;
	version_t USBBootLoaderVersion;
	version_t AFVersion;
	version_t HWVersion;
	
	CString StringDeviceName;
	CString StringUSBVersion;
	CString StringFPGAVersion;
	CString StringSOCVersion;
	CString StringUSBBootLoaderVersion;
	CString StringPackageVersion;
	CString StringSOCVersionMode;
	CString StringAFVersion;
	CString StringHWVersion;
	CString StringALLVersion;
	//用来保存找到的设备路径
	CString MyDevPathName;
	//用来保存设备是否已经找到
	BOOL MyDevFound;

	WORD HIDUpgradeFirmwareID;
	BYTE UpgradeFlag;
	BYTE AutoUpdateFlag;
	BYTE Uvc2HidFlag;

	BYTE UpdateFlashFlag;

	BYTE USBBootLoaderFound;

	DWORD FlashId;
};


typedef enum UpgradeErrorCode_t
{
	UPGRADE_SUCCESS = 0,                 /**< 成功 */

	UPGRADE_ERROR_INTT,					 /**< 初始化失败*/
	UPGRADE_ERROR_SET,					 /**< 设置参数失败*/
	UPGRADE_ERROR_GET,					 /**< 获取参数失败*/
	UPGRADE_ERROR_QUERY,				 /**< 查询状态失败*/
	UPGRADE_ERROR_CONNECT,				 /**< 连接失败*/
	UPGRADE_ERROR_REQUEST,				 /**< 请求失败*/
	UPGRADE_ERROR_TRANSFER,				 /**< 传输失败*/
	UPGRADE_ERROR_MEMORY_ERROR,          /**< 内存申请失败*/
	UPGRADE_ERROR_FILE_TYPE,			 /**< 文件类型错误*/
	UPGRADE_ERROR_FILE_NOT_FOUND,		 /**< 文件Not found*/
	UPGRADE_ERROR_FILE_OPEN,			 /**< 文件打开错误*/
	UPGRADE_ERROR_FILE_SIZE,			 /**< 文件大小错误*/
	UPGRADE_ERROR_FILE_CHECK,			 /**< 文件校验错误*/
	UPGRADE_ERROR_FILE_CREATE,			 /**< 文件创建错误*/
	UPGRADE_ERROR_VERSION,				 /**< 版本错误*/
	UPGRADE_ERROR_VERSION_SAME,			 /**< 版本相同错误*/
	UPGRADE_ERROR_VERSION_CHECK_ERROR,	 /**< 版本校验失败/
	UPGRADE_ERROR_UNKNOWN				 /**< 未知的错误*/

} UpgradeErrorCode_t;


enum hid_head_type
{
	HID_HEAD_CAM = 'C',//COMMAND
	HID_HEAD_FILE = 'F',//FILE
	HID_HEAD_DATA = 'D',//DATA
	HID_HEAD_VERSION = 'V',//VERSION

	HID_HEAD_CAM_FX3 = 'c',//COMMAND
	HID_HEAD_FILE_FX3 = 'f',//FILE
	HID_HEAD_DATA_FX3 = 'd',//DATA
	HID_HEAD_VERSION_FX3 = 'v',//VERSION
};

enum hid_command_operation
{
	HID_CAM_OPERATION_SET = 'S',//SET
	HID_CAM_OPERATION_GET = 'G',//GET
	HID_CAM_OPERATION_END = 'E',//END
	HID_CAM_OPERATION_CHARS = 'C',
	HID_CAM_OPERATION_QUERY = 'Q',
	HID_CAM_OPERATION_REQUEST = 'R',
};

enum hid_file_operation
{
	HID_FILE_OPERATION_DATA = 'D',//DATA
	HID_FILE_OPERATION_INFO = 'I',//INFO
	HID_FILE_OPERATION_MD5 = 'M',//MD5

	HID_GET_FILE_OPERATION_DATA = 'd',//DATA
	HID_GET_FILE_OPERATION_INFO = 'i',//INFO
	HID_GET_FILE_OPERATION_MD5 = 'm',//MD5
};

typedef enum {
	FILE_TYPE_NULL = 0,
	FILE_TYPE_SOC_PKG = 1,
	FILE_TYPE_SOC_MTD,
	FILE_TYPE_SOC_ARM,
	FILE_TYPE_SOC_UBOOT,
	FILE_TYPE_SOC_KERNEL,
	FILE_TYPE_SOC_ROOTFS,
	FILE_TYPE_SOC_RESERVE,
	FILE_TYPE_SOC_DATA,
	FILE_TYPE_FX3_USB,
	FILE_TYPE_FX3_FPGA,
	FILE_TYPE_LINUX_FILE,
	FILE_TYPE_FX3_USB_BOOTLOADER,
	FILE_TYPE_FX3_FPGA_RESTORE = 13,
	FILE_TYPE_FX3_USB_RESTORE,
	FILE_TYPE_SOC_PKG_DFU,
} file_type;

typedef enum {
	DATA_TYPE_NULL = 0,
	DATA_TYPE_FX3_SETING = 1,
} data_type;

typedef enum {
	VERSION_TYPE_NULL = 0,
	VERSION_TYPE_SOC = 1,
	VERSION_TYPE_ARM,
	VERSION_TYPE_FPGA,
	VERSION_TYPE_AF,
	VERSION_TYPE_USB,
	VERSION_TYPE_PKG,
	VERSION_TYPE_USB_BOOTLOADER,
	VERSION_TYPE_HW,
} version_type;

typedef enum {
	FILE_INFO_NULL = 0,
	FILE_INFO_FILE_NAME = 1,
	FILE_INFO_FILE_TYPE  = 2,
} file_info;


typedef enum {
	HID_STATUS_NULL = 0,
	HID_STATUS_START = 1,
	HID_STATUS_ING,
	HID_STATUS_END,
	HID_STATUS_WAIT,
	HID_STATUS_BUSY,
	HID_STATUS_CANCELED,
	HID_STATUS_FINISH,
	
	HID_STATUS_ACK,
	HID_STATUS_NAK,
	HID_STATUS_ERROR,	
	HID_STATUS_NOT_EXECUTABLE,
} hid_status;

typedef enum {
	READ_FILE_CRTL_TYPE_NULL = 0,
	READ_FILE_CRTL_TYPE_OPEN = 1,
	READ_FILE_CRTL_TYPE_CLOSE = 2,
} read_file_crtl_type;


//老版本包大小48的定义
//#define HID_DATA_LEN_V1		(32)
#define HID_REPORT_LEN_V1	(0x30)
#define HID_REPORT_DATA_HEAD_LEN_V1	(16)

struct hid_report_data {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;
	BYTE data[HID_REPORT_LEN_V1 - 5];
};

struct hid_file_info {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;
	
	BYTE pkg_size;//package size 32
	DWORD file_size;
	BYTE file_type;
	BYTE file_crc;
	DWORD data_type;
	BYTE data[32];
};


struct hid_file_md5 {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;
	
	DWORD file_size;
	BYTE file_type;
	BYTE file_id;
	BYTE md5_len;
	WORD reserve;
	BYTE data[34];
};

struct hid_version {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;
	
	BYTE version_type;
	BYTE data_size;
	BYTE data[41];
};

struct hid_file_data {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;
	
	DWORD pkg_num;//package num
	DWORD pkg_size;//package size
	BYTE file_type;
	BYTE file_crc;	
	BYTE data_size;
	BYTE data[32];
};

struct hid_data {
	WORD crc;
	BYTE head;
	BYTE operation;
	BYTE report_len;

	DWORD pkg_num;//package num
	DWORD pkg_size;//package size
	BYTE data_type;
	BYTE data_id;	
	BYTE data_size;

	BYTE data[32];
};

//新版本包大小1024的定义

#define HID_REPORT_LEN_V2	(HID_MAX_PACKETSIZE)
#define HID_REPORT_DATA_HEAD_LEN_V2	(24)
struct hid_report_data_v2 {	
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 12];
};


struct hid_file_info_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD pkg_size;//package size 32
	DWORD file_size;
	BYTE file_type;
	BYTE file_id;
	WORD data_type;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];	
};

struct hid_file_md5_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD file_size;
	BYTE file_type;
	BYTE file_id;
	DWORD md5_len;
	WORD reserve;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];
};

struct hid_file_data_v2 {	
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD pkg_num;//package num
	DWORD pkg_size;//package size
	BYTE file_type;
	BYTE file_id;
	WORD reserve;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];
};

struct hid_data_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD pkg_num;//package num
	DWORD pkg_size;//package size
	BYTE data_type;
	BYTE data_id;
	WORD reserve;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];
};

struct hid_version_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	BYTE version_type;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 13];
};


struct hid_read_file_info_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD file_size;
	BYTE crtl_type;
	BYTE status;
	WORD reserve;
	DWORD reserve2;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];
};

struct hid_read_file_md5_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD file_size;
	BYTE file_type;
	BYTE status;
	DWORD md5_len;
	WORD reserve;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];

};

struct hid_read_file_data_v2 {
	WORD crc;
	BYTE head;
	BYTE operation;
	DWORD report_len;

	DWORD file_size;
	DWORD read_start_address;
	BYTE data_type;
	BYTE status;
	WORD reserve;
	DWORD data_size;
	BYTE data[HID_REPORT_LEN_V2 - 24];
};


#endif /*__HID_TYPES_H*/
