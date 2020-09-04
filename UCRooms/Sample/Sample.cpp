#include <iostream>
#include <Windows.h>
#include "sdk/include/tencent_meeting_external_device_impl.h"

#define SAMPLE_SDK_ID  "1234567890"
#define SAMPLE_LICENSE "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"

static int32_t SampleGetDeviceType() {
  std::cout << "get device type" << std::endl;
  return kTMEDDevScreen | kTMEDDevCamera | kTMEDDevMicrophone;
}

static const char* SampleGetManufactureName() {
  std::cout << "get manufacture name" << std::endl;
  return "Sample Manufacture";
}

static const char* SampleGetProductName() {
  std::cout << "get product name" << std::endl;
  return "Sample Product";
}

static const char* SampleGetSerialNumber() {
  std::cout << "get serial number" << std::endl;
  return "0123456789";
}

static const char* SampleGetFirmwareVersion() {
  std::cout << "get firmware version" << std::endl;
  return "1.0.0";
}

static TMEDResult SampleUpdateFirmware(void* context, OnTMEDFirmwareUpdateCallback cb) {
  std::cout << "firmware update" << std::endl;
  cb(context, kTMEDFwPreparing, 0);
  return kTMEDSuccess;
}

static TMEDResult SampleCancelUpdateFirmware() {
  std::cout << "cancel firmware update" << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetAppId(const char* appid) {
  std::cout << "set appid: " << appid << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetAutoLoginAccount(const char* un, const char* pw) {
  std::cout << "set auto login account, user name: " << un << ", password: " << pw << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleShutdown() {
  std::cout << "shutdown" << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleReboot() {
  std::cout << "reboot" << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetScreenStatus(bool on) {
  std::cout << "set screen status: " << on << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetMuteStatus(bool on) {
  std::cout << "set mute status:" << on << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetHookStatus(bool on) {
  std::cout << "set hook status" << on << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleProbeVideoControl(TMEDVideoFormat format, uint16_t width, uint16_t height, uint8_t fps) {
  std::cout << "probe video control. format: " << format << ", w:" << width << ", h: " << height << ", fps: " << fps << std::endl;
  return kTMEDError;
}

static TMEDResult SampleSetPan(TMEDCamPanMovement pan_movement) {
  std::cout << "set pan:" << pan_movement << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetTilt(TMEDCamTiltMovement tilt_movement) {
  std::cout << "set tilt:" << tilt_movement << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetZoom(TMEDCamZoomMovement zoom_movement) {
  std::cout << "set zoom:" << zoom_movement << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSetParticipatesCountDetectEnable(bool enable) {
  std::cout << "set participates count detect enable " << enable << std::endl;
  return kTMEDNotImpl;
}

static TMEDResult SampleGetParticipatesCount(uint8_t* count) {
  std::cout << "get participates count" << std::endl;
  *count = 8;
  return kTMEDSuccess;
}

static TMEDResult SampleSetAutoFramingEnable(bool enable) {
  std::cout << "set auto framing enable " << enable << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleIsAutoFramingEnable(bool* enable) {
  std::cout << "get auto framing enable" << std::endl;
  *enable = false;
  return kTMEDNotImpl;
}

static TMEDResult SampleSetSpeakerTrackingEnable(bool enable) {
  std::cout << "set speaker tracking enable " << enable << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleIsSpeakerTrackingEnable(bool* enable) {
  std::cout << "get speaker tracking enable" << std::endl;
  *enable = false;
  return kTMEDNotImpl;
}

static TMEDResult SampleClearPreset() {
  std::cout << "clear preset" << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleSavePresetTo(uint8_t index) {
  std::cout << "save preset to " << index << std::endl;
  return kTMEDSuccess;
}

static TMEDResult SampleApplyPresetFrom(uint8_t index) {
  std::cout << "apply preset from" << index << std::endl;
  return kTMEDSuccess;
}

static TMEDEntryPoints entries = {
  SampleGetDeviceType,
  SampleGetManufactureName,
  SampleGetProductName,
  SampleGetSerialNumber,
  SampleGetFirmwareVersion,
  SampleUpdateFirmware,
  SampleCancelUpdateFirmware,
  SampleSetAppId,
  SampleSetAutoLoginAccount,
  SampleShutdown,
  SampleReboot,
  SampleSetScreenStatus,
  SampleSetMuteStatus,
  SampleSetHookStatus,
  SampleProbeVideoControl,
  SampleSetPan,
  SampleSetTilt,
  SampleSetZoom,
  SampleSetParticipatesCountDetectEnable,
  SampleGetParticipatesCount,
  SampleSetAutoFramingEnable,
  SampleIsAutoFramingEnable,
  SampleSetSpeakerTrackingEnable,
  SampleIsSpeakerTrackingEnable,
  SampleClearPreset,
  SampleSavePresetTo,
  SampleApplyPresetFrom,
};

int main(int argc, char* argv[]) {
  TMEDConfig config = {kTMEDLogLevelDebug, true, "D:\\SdkSample\\Logs"};
  TMEDInitSdkWithLicense(SAMPLE_SDK_ID, SAMPLE_LICENSE, entries, config);
  std::cout << "SDK is Running..." << std::endl;

  MSG msg = {};
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
}
