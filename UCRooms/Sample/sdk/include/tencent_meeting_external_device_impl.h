// Tencent Meeting External Device SDK V0.2
#ifndef TENCENT_MEETING_EXTERNAL_DEVICE_IMPL_H_
#define TENCENT_MEETING_EXTERNAL_DEVICE_IMPL_H_

#include <stdint.h>
#include <stdbool.h>

enum TMEDResult {
  kTMEDSuccess = 0,
  kTMEDError,
  kTMEDNotImpl,
};

enum TMEDDevType {
  kTMEDDevScreen = 1 << 0,
  kTMEDDevCamera = 1 << 1,
  kTMEDDevMicrophone = 1 << 2,
};

enum TMEDFirmwareUpdateStatus {
  kTMEDFwNoNeedUpdate = 0,
  kTMEDFwPreparing,
  kTMEDFwUpdating,
  kTMEDFwUpdateSuccess,
  kTMEDFwUpdateFailed,
};

enum TMEDVideoFormat {
  kTMEDVideoFormatUnknown = 0,
  kTMEDVideoFormatUncompressed,
  kTMEDVideoFormatMJPEG,
  kTMEDVideoFormatH264,
};

enum TMEDCamPanMovement {
  kTMEDCamMoveToCounterClockwiseDirection = -1,
  kTMEDCamPanStop = 0,
  kTMEDCamMoveToClockwiseDirection = 1,
};

enum TMEDCamTiltMovement {
  kTMEDCamPointTheImagingPlaneDown = -1,
  kTMEDCamTiltStop = 0,
  kTMEDCamPointTheImagingPlaneUp = 1,
};

enum TMEDCamZoomMovement {
  kTMEDCamMoveToWideAngleDirection = -1,
  kTMEDCamZoomStop = 0,
  kTMEDCamMoveToTelephotoDirection = 1,
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The callback for firmware update
 *
 * @param context the context which set by TMEDUpdateFirmware
 * @param status see the definition of TMEDFirmwareUpdateStatus.
 * @param progress the progress for updating, it will be set when status is kTMEDFwUpdating.
 */
typedef void (* OnTMEDFirmwareUpdateCallback)(void* context, TMEDFirmwareUpdateStatus status, int progress);

/**
 * Get the device type.
 *
 * @return see the definition of TMEDDevType, support multi-type device.
 */
typedef int32_t (* TMEDGetDeviceType)();

/**
 * Get the manufacture name.
 *
 * @return the name of manufacture.
 */
typedef const char* (* TMEDGetManufactureName)();

/**
 * Get the product name.
 *
 * @return the name of product.
 */
typedef const char* (* TMEDGetProductName)();

/**
 * Get the serial number.
 *
 * @return the serial number of the device.
 */
typedef const char* (* TMEDGetSerialNumber)();

/**
 * Get the firmware version.
 *
 * @return the firmware version.
 */
typedef const char* (* TMEDGetFirmwareVersion)();

/**
 * Update the firmware
 *
 * @param context the sdk context.
 * @param cb the callback of the update process.
 * @return see the definition of TMEDResult.
 */
typedef TMEDResult (* TMEDUpdateFirmware)(void* context, OnTMEDFirmwareUpdateCallback cb);

/**
 * Cancel firmware update
 *
 * @return see the definition of TMEDResult.
 */
typedef TMEDResult(*TMEDCancelUpdateFirmware)();

/**
 * Set the app id of the customer
 *
 * @param appid the customer app id, usually defined by the device vendor.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetAppId)(const char* appid);

/**
 * Set the username and password for auto login the system.
 *
 * @param un username
 * @param pw password
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetAutoLoginAccount)(const char* un, const char* pw);

/**
 * Shutdown the device.
 *
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDShutdown)();

/**
 * Reboot the device.
 *
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDReboot)();

/**
 * Set the screen status
 *
 * @param on true means screen on, false means screen off.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetScreenStatus)(bool on);

/**
 * Set the microphone mute status.
 *
 * @param on true means mute on, false means mute off.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetMuteStatus)(bool on);

/**
 * Set the hook status
 *
 * @param on true means on-hook, false means off-hook.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetHookStatus)(bool on);

/**
 * Probe the video format, resolution and fps.
 *
 * @param format see the definition of TMEDVideoFormat.
 * @param width the width of video.
 * @param height the height of video.
 * @param fps the fps of video.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDProbeVideoControl)(TMEDVideoFormat format, uint16_t width, uint16_t height, uint8_t fps);

/**
 * Set pan movement of the camera.
 *
 * @param pan_movement see the definition of TMEDCamPanMovement
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetPan)(TMEDCamPanMovement pan_movement);

/**
 * Set tilt movement of the camera.
 *
 * @param tilt_movement see the definition of TMEDCamTiltMovement
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetTilt)(TMEDCamTiltMovement tilt_movement);

/**
 * Set zoom movement of the camera.
 *
 * @param zoom_movement see the definition of TMEDCamZoomMovement
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetZoom)(TMEDCamZoomMovement zoom_movement);

/**
 * Enable/Disable the participates count detect feature.
 *
 * @param enable true means enable, false means disable.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetParticipatesCountDetectEnable)(bool enable);

/**
 * Get the participates count.
 *
 * @param count
 * the participates count.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDGetParticipatesCount)(uint8_t* count);

/**
 * Enable/Disable the auto framing feature.
 *
 * @param enable true means enable, false means disable.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetAutoFramingEnable)(bool enable);

/**
 * Get the auto framing feature state.
 *
 * @param enable true means enable, false means disable.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDIsAutoFramingEnable)(bool* enable);

/**
 * Enable/Disable the speaker tracking detect feature.
 *
 * @param enable true means enable, false means disable.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSetSpeakerTrackingEnable)(bool enable);

/**
 * Get the speaker tracking detect feature state.
 *
 * @param enable true means enable, false means disable.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDIsSpeakerTrackingEnable)(bool* enable);

/**
 * Clear all preset.
 *
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDClearPreset)();

/**
 * Save preset to the specific index.
 *
 * @param index the preset index.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDSavePresetTo)(uint8_t index);

/**
 * Apply preset from the specific index.
 *
 * @param index the preset index.
 * @return see the definition of TMEDResult, please return kTMEDNotImpl if this feature is unsupported.
 */
typedef TMEDResult (* TMEDApplyPresetFrom)(uint8_t index);

typedef struct TMEDEntryPoints {
  TMEDGetDeviceType  getDeviceType;
  TMEDGetManufactureName getManufactureName;
  TMEDGetProductName getProductName;
  TMEDGetSerialNumber getSerialNumber;
  TMEDGetFirmwareVersion getFirmwareVersion;
  TMEDUpdateFirmware updateFirmware;
  TMEDCancelUpdateFirmware cancelUpdateFirmware;
  TMEDSetAppId setAppId;
  TMEDSetAutoLoginAccount setAutoLoginAccount;
  TMEDShutdown shutdown;
  TMEDReboot reboot;
  TMEDSetScreenStatus setScreenStatus;
  TMEDSetMuteStatus setMuteStatus;
  TMEDSetHookStatus setHookStatus;
  TMEDProbeVideoControl probeVideoControl;
  TMEDSetPan setPan;
  TMEDSetTilt setTilt;
  TMEDSetZoom setZoom;
  TMEDSetParticipatesCountDetectEnable setParticipatesCountDetectEnable;
  TMEDGetParticipatesCount getParticipatesCount;
  TMEDSetAutoFramingEnable setAutoFramingEnable;
  TMEDIsAutoFramingEnable isAutoFramingEnable;
  TMEDSetSpeakerTrackingEnable setSpeakerTrackingEnable;
  TMEDIsSpeakerTrackingEnable isSpeakerTrackingEnable;
  TMEDClearPreset clearPreset;
  TMEDSavePresetTo savePresetTo;
  TMEDApplyPresetFrom applyPresetFrom;
} TMEDEntryPoints;

typedef enum TMEDLogLevel {
  kTMEDLogLevelFault = 0x01,
  kTMEDLogLevelCpe = 0x02,
  kTMEDLogLevelError = 0x03,
  kTMEDLogLevelWarning = 0x04,
  kTMEDLogLevelCp = 0x05,
  kTMEDLogLevelInfo = 0x06,
  kTMEDLogLevelDebug = 0x07,
} TMEDLogLevel;

typedef struct TMEDConfig {
  TMEDLogLevel logLevel;
  bool enableConsoleLog;
  const char* logDir;
} TMEDConfig;

/**
 * Initialize the SDK.
 *
 * @param sdk_id the SDK ID generated by Tencent Meeting.
 * @param license the license generated by Tencent Meeting.
 * @param entries the function entries, if an entry has no implementation, it should be set as NULL.
 * @param config the sdk configuration.
 * @return see the definition of TMEDResult.
 */
TMEDResult TMEDInitSdkWithLicense(const char* sdk_id, const char* license, const TMEDEntryPoints& entries, const TMEDConfig& config);

#ifdef __cplusplus
}
#endif

#endif //TENCENT_MEETING_EXTERNAL_DEVICE_IMPL_H_
