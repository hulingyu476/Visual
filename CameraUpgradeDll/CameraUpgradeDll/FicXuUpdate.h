#ifdef FICXUUPDATE_EXPORTS
#define FICXUUPDATE_API __declspec(dllexport)
#else
#define FICXUUPDATE_API __declspec(dllimport)
#endif

#include <dshow.h>
#include <Vidcap.h>
#include <KsProxy.h>

#define YHW_STR_ID_PRO				0x09
#define YHW_STR_ID_MANUFACTURER		0x0C
#define YHW_REG_USB_VID				0x0F
#define YHW_REG_USB_PID				0x10
#define YHW_STR_ID_PLATFORM			0x11
#define YHW_REG_SYS_REBOOT			0x14

/************************************************************************
Desc		: Check whether the device is our device, and if so set NodeId.
@pVCap(in)	: UVC device hander.
@NodeId(out): UVC Extension Unit ID.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicCheckDevice(IBaseFilter *pVCap, ULONG *pNodeId);

/************************************************************************
Desc		: Get Device Version.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@pVer(out)	: version pointer.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicGetDeviceVersion(IBaseFilter *pVCap, ULONG NodeId, UINT16 *pVer);

/************************************************************************
Desc		: Initialize device and get device's version.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@pVer(out)   : the version number of the device.
@pPacketUnit(out): Number of bytes transmitted at one time during upgrade.
return		: S_OK for success, and others for error.
*************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateInitDevice(IBaseFilter *pVCap, ULONG NodeId);

/************************************************************************
Desc		: Get Device Version.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@pPackerSize(out)	: A pointer to the number of data bytes transferred at a time.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateGetPacketUnit(IBaseFilter *pVCap, ULONG NodeId, UINT16 *pPackerSize);

/************************************************************************
Desc		: Notify the device to start the upgrade process.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateStart(IBaseFilter *pVCap, ULONG NodeId);

/************************************************************************
Desc		: Set Firmware Length.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@len(in)	: Firmware length by Byte. Must be aligned by package size, 
			  witch can get from FicUpdateInitDevice().
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateSetFwLen(IBaseFilter *pVCap, ULONG NodeId, UINT32 len);

/************************************************************************
Desc		: Write Data to device. Transmit PacketUnit bytes at a time.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@Buf(in)	: Firmware Buffer.
@PacketUnit(in)	: Transmit size of byte. must be equal to *pPacketUnit. 
				  see FicUpdateInitDevice();
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateWriteDataUnit(IBaseFilter *pVCap, ULONG NodeId, 
												void *pBuf, int PacketUnit);

/************************************************************************
Desc		: Notify the device that the firmware has been transferred.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicUpdateStop(IBaseFilter *pVCap, ULONG NodeId);

/************************************************************************
Desc		: Get Firmware's Version.
@pBuf(in)	: Firmware
@size(in)	: Firmware size of byte.
@pVer(out)	: version pointer.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FwGetVersion(const unsigned char *pBuf, int size, UINT32 *pVer);

/************************************************************************
Desc		: Get special string form device. code by UTF8 or ASCII.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@StrId(in)	: String ID.
@pBuf(in)	: Buffer for store String, OR NULL for get String length.
@pSize(in/out): Buffer length. or String length for return. Can't be NULL.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicGetStr(IBaseFilter *pVCap, ULONG NodeId, 
	ULONG StrId, char *pBuf, ULONG *pSize);

/************************************************************************
Desc		: Get Firmware's Platform String.
@pBuf(in)	: Firmware
@size(in)	: Firmware size of byte.
@pPlatform(out)	: Buffer for store Platform String. must be greater than 64 bytes.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FwGetPlatform(const unsigned char *pBuf, int size, char *pPlatform);

/************************************************************************
Desc		: Read special function's Register from Device.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@RegAddr(in): the index of Register.
@pRegData(out)	: A memory for store Register data.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicGetReg(IBaseFilter *pVCap, ULONG NodeId, 
	ULONG RegAddr, ULONG *pRegData);

/************************************************************************
Desc		: Write Register to Device.
@pVCap(in)	: UVC device hander.
@NodeId(in) : Xu ID.
@RegAddr(in): the index of Register.
@RegData(in): A memory for store Register data.
return		: S_OK for success, and others for error.
************************************************************************/
FICXUUPDATE_API HRESULT FicSetReg(IBaseFilter *pVCap, ULONG NodeId, 
	ULONG RegAddr, ULONG RegData);

