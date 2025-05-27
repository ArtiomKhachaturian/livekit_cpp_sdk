// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "CameraManager.h"
#include "MacCameraCapturer.h"
#include "Utils.h"
#include <modules/video_capture/device_info_impl.h>
#import <components/capturer/RTCCameraVideoCapturer.h>
#import <Cocoa/Cocoa.h>

@interface CameraSettingsWindow : NSWindowController<NSWindowDelegate>
@property (nonatomic, strong) AVCaptureDevice* cameraDevice;
- (instancetype) initWithParent:(void*) parentWindow
                      positionX:(uint32_t) positionX
                      positionY:(uint32_t) positionY
                          title:(NSString*) title;
- (void) runModalWindow;
@end

namespace
{

class MacOSDeviceInfoImpl : public webrtc::videocapturemodule::DeviceInfoImpl
{
public:
    MacOSDeviceInfoImpl() = default;
    // impl. of webrtc::VideoCaptureModule
    uint32_t NumberOfDevices() final;
    int32_t GetDeviceName(uint32_t deviceNumber,
                          char* deviceNameUTF8,
                          uint32_t deviceNameLength,
                          char* deviceUniqueIdUTF8,
                          uint32_t deviceUniqueIdUTF8Length,
                          char* productUniqueIdUTF8 = 0,
                          uint32_t productUniqueIdUTF8Length = 0) final;
    int32_t DisplayCaptureSettingsDialogBox(const char* deviceUniqueIdUTF8,
                                            const char* dialogTitleUTF8,
                                            void* parentWindow,
                                            uint32_t positionX,
                                            uint32_t positionY) final;
protected:
    // impl. of webrtc::videocapturemodule::DeviceInfoImpl
    int32_t Init() final { return 0; }
    int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8) final;
};

inline NSArray<AVCaptureDevice*>* availableDevices()
{
    AVCaptureDeviceDiscoverySession* video = [AVCaptureDeviceDiscoverySession
                                              discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInWideAngleCamera,
                                                                                AVCaptureDeviceTypeExternal]
                                              mediaType:AVMediaTypeVideo
                                              position:AVCaptureDevicePositionUnspecified];
    return [video devices];
}

}

namespace LiveKitCpp
{

std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> CameraManager::createDeviceInfo()
{
    return std::make_unique<MacOSDeviceInfoImpl>();
}

webrtc::scoped_refptr<CameraCapturer> CameraManager::createCapturer(const MediaDeviceInfo& dev,
                                                                    VideoFrameBufferPool framesPool,
                                                                    const std::shared_ptr<Bricks::Logger>& /*logger*/) const
{
    return MacCameraCapturer::create(dev, std::move(framesPool));
}

} // namespace LiveKitCpp

namespace
{

using namespace LiveKitCpp;

uint32_t MacOSDeviceInfoImpl::NumberOfDevices()
{
    @autoreleasepool {
        auto devs = availableDevices();
        if (devs) {
            return static_cast<uint32_t>([devs count]);
        }
    }
    return 0U;
}

int32_t MacOSDeviceInfoImpl::GetDeviceName(uint32_t deviceNumber,
                                           char* deviceNameUTF8,
                                           uint32_t deviceNameLength,
                                           char* deviceUniqueIdUTF8,
                                           uint32_t deviceUniqueIdUTF8Length,
                                           char* productUniqueIdUTF8,
                                           uint32_t productUniqueIdUTF8Length)
{
    if (deviceNameUTF8 && deviceNameLength > 0U) {
        @autoreleasepool {
            if (auto devs = availableDevices()) {
                if (deviceNumber < [devs count]) {
                    auto dev = [devs objectAtIndex:deviceNumber];
                    if (dev) {
                        const auto deviceName = MacCameraCapturer::localizedDeviceName(dev);
                        std::strncpy(deviceNameUTF8, deviceName.c_str(), deviceNameLength);
                        if (deviceUniqueIdUTF8 && deviceUniqueIdUTF8Length > 0U) {
                            const auto deviceGuid = fromNSString(dev.uniqueID);
                            std::strncpy(deviceUniqueIdUTF8, deviceGuid.c_str(), deviceUniqueIdUTF8Length);
                        }
                        if (productUniqueIdUTF8 && productUniqueIdUTF8Length > 0U) {
                            const auto model = fromNSString(dev.modelID);
                            std::strncpy(productUniqueIdUTF8, model.c_str(), productUniqueIdUTF8Length);
                        }
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}

int32_t MacOSDeviceInfoImpl::CreateCapabilityMap(const char* deviceUniqueIdUTF8)
{
    _captureCapabilities = MacCameraCapturer::capabilities(deviceUniqueIdUTF8);
    return static_cast<int32_t>(_captureCapabilities.size());
}

int32_t MacOSDeviceInfoImpl::DisplayCaptureSettingsDialogBox(const char* deviceUniqueIdUTF8,
                                                             const char* dialogTitleUTF8,
                                                             void* parentWindow,
                                                             uint32_t positionX,
                                                             uint32_t positionY)
{
    if (deviceUniqueIdUTF8) {
        @autoreleasepool {
            AVCaptureDevice* device = [AVCaptureDevice deviceWithUniqueID:toNSString(deviceUniqueIdUTF8)];
            if (device) {
                CameraSettingsWindow* window = [[CameraSettingsWindow alloc] initWithParent:parentWindow
                                                                                  positionX:positionX
                                                                                  positionY:positionY
                                                                                      title:toNSString(dialogTitleUTF8)];
                window.cameraDevice = device;
                [window runModalWindow];
                return 0;
            }
        }
    }
    return -1;
}

}

@implementation CameraSettingsWindow {
    NSPopUpButton* _exposureDropdown;
    NSPopUpButton* _focusDropdown;
    NSPopUpButton* _whiteBalanceDropdown;
    NSTextField* _exposureLabel;
    NSTextField* _focusLabel;
    NSTextField* _whiteBalanceLabel;
}

@synthesize cameraDevice = _cameraDevice;

- (instancetype)initWithParent:(void*) parentWindow
                     positionX:(uint32_t) positionX
                     positionY:(uint32_t) positionY
                         title:(NSString*) title {
    self = [super init];
    if (self) {
        NSWindow *parentNSWindow = nil;
        if (parentWindow) {
            NSWindow *parentNSWindow = nil;
            NSResponder* parent = (__bridge NSResponder*)parentWindow;
            if ([parent isKindOfClass: [NSWindow class]]) {
                parentNSWindow = (NSWindow*)parent;
            }
            else if ([parent isKindOfClass: [NSView class]]){
                parentNSWindow = ((NSView*)parent).window;
            }
        }
        
        NSRect windowRect = NSMakeRect(positionX, positionY, 300, 100);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect
                                                       styleMask:(NSWindowStyleMaskTitled |
                                                                  NSWindowStyleMaskClosable)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        self.window = window;
        self.window.title = title;
        self.window.delegate = self;

        if (parentNSWindow) {
            [parentNSWindow addChildWindow:self.window ordered:NSWindowAbove];
        }
        [self setupUI];
    }
    return self;
}

- (void) setupUI {
    NSView *contentView = self.window.contentView;

    NSRect labelRect = NSMakeRect(10, contentView.frame.size.height - 30, 120, 20);
    _exposureLabel = [[NSTextField alloc] initWithFrame:labelRect];
    [_exposureLabel setStringValue:@"Exposure Mode:"];
    [_exposureLabel setBezeled:NO];
    [_exposureLabel setEditable:NO];
    [_exposureLabel setBackgroundColor:[NSColor clearColor]];
    [contentView addSubview:_exposureLabel];

    NSRect dropdownRect = NSMakeRect(labelRect.origin.x + labelRect.size.width,
                                     labelRect.origin.y - 2,
                                     160,
                                     labelRect.size.height + 4);
    _exposureDropdown = [[NSPopUpButton alloc] initWithFrame:dropdownRect];
    [_exposureDropdown setAction:@selector(exposureModeChanged:)];
    [_exposureDropdown setTarget:self];
    [_exposureDropdown setEnabled:NO];
    [contentView addSubview:_exposureDropdown];

    labelRect.origin.y -= dropdownRect.size.height + 4;
    _focusLabel = [[NSTextField alloc] initWithFrame:labelRect];
    [_focusLabel setStringValue:@"Focus Mode:"];
    [_focusLabel setBezeled:NO];
    [_focusLabel setEditable:NO];
    [_focusLabel setBackgroundColor:[NSColor clearColor]];
    [contentView addSubview:_focusLabel];

    dropdownRect.origin.y = labelRect.origin.y - 2;
    _focusDropdown = [[NSPopUpButton alloc] initWithFrame:dropdownRect];
    [_focusDropdown setAction:@selector(focusModeChanged:)];
    [_focusDropdown setTarget:self];
    [_focusDropdown setEnabled:NO];
    [contentView addSubview:_focusDropdown];
    
    labelRect.origin.y -= dropdownRect.size.height + 4;
    _whiteBalanceLabel = [[NSTextField alloc] initWithFrame:labelRect];
    [_whiteBalanceLabel setStringValue:@"White Balance:"];
    [_whiteBalanceLabel setBezeled:NO];
    [_whiteBalanceLabel setEditable:NO];
    [_whiteBalanceLabel setBackgroundColor:[NSColor clearColor]];
    [contentView addSubview:_whiteBalanceLabel];
    
    dropdownRect.origin.y = labelRect.origin.y - 2;
    _whiteBalanceDropdown = [[NSPopUpButton alloc] initWithFrame:dropdownRect];
    [_whiteBalanceDropdown setAction:@selector(whiteBalanceChanged:)];
    [_whiteBalanceDropdown setTarget:self];
    [_whiteBalanceDropdown setEnabled:NO];
    [contentView addSubview:_whiteBalanceDropdown];
}

- (void) exposureModeChanged:(NSPopUpButton*) sender {
    NSString* selectedMode = sender.selectedItem.title;
    if ([self.cameraDevice lockForConfiguration:nil]) {
        AVCaptureExposureMode mode = [CameraSettingsWindow exposureModeFromString:selectedMode];
        if ([self.cameraDevice isExposureModeSupported: mode]) {
            self.cameraDevice.exposureMode = mode;
        }
        [self.cameraDevice unlockForConfiguration];
    }
}

- (void) focusModeChanged:(NSPopUpButton*) sender {
    NSString* selectedMode = sender.selectedItem.title;
    if ([self.cameraDevice lockForConfiguration:nil]) {
        AVCaptureFocusMode mode = [CameraSettingsWindow focusModeFromString:selectedMode];
        if ([self.cameraDevice isFocusModeSupported: mode]) {
            self.cameraDevice.focusMode = mode;
        }
        [self.cameraDevice unlockForConfiguration];
    }
}

- (void) whiteBalanceChanged:(NSPopUpButton*) sender {
    NSString* selectedMode = sender.selectedItem.title;
    if ([self.cameraDevice lockForConfiguration:nil]) {
        AVCaptureWhiteBalanceMode mode = [CameraSettingsWindow whiteBalanceModeFromString:selectedMode];
        if ([self.cameraDevice isWhiteBalanceModeSupported: mode]) {
            self.cameraDevice.whiteBalanceMode = mode;
        }
        [self.cameraDevice unlockForConfiguration];
    }
}

- (void) setCameraDevice:(AVCaptureDevice*) cameraDevice {
    _cameraDevice = cameraDevice;
    [_exposureDropdown removeAllItems];
    [_focusDropdown removeAllItems];
    [_whiteBalanceDropdown removeAllItems];
    if (_cameraDevice && [_cameraDevice lockForConfiguration:nil]) {
        const auto s1 = _cameraDevice.whiteBalanceMode;
        NSInteger exposureIndex = -1, focusIndex = -1, whiteBalanceIndex = -1;
        for (NSNumber* mode in @[@(AVCaptureExposureModeLocked),
                                 @(AVCaptureExposureModeAutoExpose),
                                 @(AVCaptureExposureModeContinuousAutoExposure)]) {
            AVCaptureExposureMode exposureMode = (AVCaptureExposureMode)mode.intValue;
            if (exposureMode == _cameraDevice.exposureMode || [_cameraDevice isExposureModeSupported: exposureMode]) {
                [_exposureDropdown addItemWithTitle:[CameraSettingsWindow exposureModeToString:exposureMode]];
                if (exposureMode == _cameraDevice.exposureMode) {
                    exposureIndex = _exposureDropdown.numberOfItems - 1;
                }
            }
        }
        for (NSNumber* mode in @[@(AVCaptureFocusModeLocked),
                                 @(AVCaptureFocusModeAutoFocus),
                                 @(AVCaptureFocusModeContinuousAutoFocus)]) {
            AVCaptureFocusMode focusMode = (AVCaptureFocusMode)mode.intValue;
            if (focusMode == _cameraDevice.focusMode || [_cameraDevice isFocusModeSupported: focusMode]) {
                [_focusDropdown addItemWithTitle:[CameraSettingsWindow focusModeToString:focusMode]];
                if (focusMode == _cameraDevice.focusMode) {
                    focusIndex = _focusDropdown.numberOfItems - 1;
                }
            }
        }
        for (NSNumber* mode in @[@(AVCaptureWhiteBalanceModeLocked),
                                 @(AVCaptureWhiteBalanceModeAutoWhiteBalance),
                                 @(AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance)]) {
            AVCaptureWhiteBalanceMode wbMode = (AVCaptureWhiteBalanceMode)mode.intValue;
            if (wbMode == _cameraDevice.whiteBalanceMode || [_cameraDevice isWhiteBalanceModeSupported: wbMode]) {
                [_whiteBalanceDropdown addItemWithTitle:[CameraSettingsWindow whiteBalanceModeToString:wbMode]];
                if (wbMode == _cameraDevice.whiteBalanceMode) {
                    whiteBalanceIndex = _whiteBalanceDropdown.numberOfItems - 1;
                }
            }
        }
        [_cameraDevice unlockForConfiguration];
        [_exposureDropdown selectItemAtIndex:exposureIndex];
        [_focusDropdown selectItemAtIndex:focusIndex];
        [_whiteBalanceDropdown selectItemAtIndex:whiteBalanceIndex];
    }
    _exposureDropdown.enabled = _exposureDropdown.numberOfItems > 1;
    _focusDropdown.enabled = _focusDropdown.numberOfItems > 1;
    _whiteBalanceDropdown.enabled = _whiteBalanceDropdown.numberOfItems > 1;
    _exposureLabel.enabled = _exposureDropdown.enabled;
    _focusLabel.enabled = _focusDropdown.enabled;
    _whiteBalanceLabel.enabled = _whiteBalanceDropdown.enabled;
}

- (void) windowWillClose:(NSNotification*) notification {
    if (notification.object == self.window) {
        [NSApplication.sharedApplication stopModal];
    }
}

- (void) runModalWindow {
    NSApplication *app = [NSApplication sharedApplication];
    [app runModalForWindow:self.window];
}

+ (nullable NSString*) focusModeToString:(AVCaptureFocusMode) mode {
    switch (mode) {
        case AVCaptureFocusModeLocked:
            return @"Locked";
        case AVCaptureFocusModeAutoFocus:
            return @"Auto";
        case AVCaptureFocusModeContinuousAutoFocus:
            return @"Continuous Auto";
        default:
            break;
    }
    return nil;
}

+ (nullable NSString*) exposureModeToString:(AVCaptureExposureMode) mode {
    switch (mode) {
        case AVCaptureExposureModeLocked:
            return @"Locked";
        case AVCaptureExposureModeAutoExpose:
            return @"Auto";
        case AVCaptureExposureModeContinuousAutoExposure:
            return @"Continuous Auto";
        default:
            break;
    }
    return nil;
}

+ (nullable NSString*) whiteBalanceModeToString:(AVCaptureWhiteBalanceMode) mode {
    switch (mode) {
        case AVCaptureWhiteBalanceModeLocked:
            return @"Locked";
        case AVCaptureWhiteBalanceModeAutoWhiteBalance:
            return @"Auto";
        case AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance:
            return @"Continuous Auto";
        default:
            break;
    }
    return nil;
}

+ (AVCaptureFocusMode) focusModeFromString:(nullable NSString*) mode {
    if (mode) {
        if ([mode isEqualToString:[CameraSettingsWindow focusModeToString:AVCaptureFocusModeLocked]]) {
            return AVCaptureFocusModeLocked;
        }
        if ([mode isEqualToString:[CameraSettingsWindow focusModeToString:AVCaptureFocusModeContinuousAutoFocus]]) {
            return AVCaptureFocusModeContinuousAutoFocus;
        }
    }
    return AVCaptureFocusModeAutoFocus;
}

+ (AVCaptureExposureMode) exposureModeFromString:(nullable NSString*) mode {
    if (mode) {
        if ([mode isEqualToString:[CameraSettingsWindow exposureModeToString:AVCaptureExposureModeLocked]]) {
            return AVCaptureExposureModeLocked;
        }
        if ([mode isEqualToString:[CameraSettingsWindow exposureModeToString:AVCaptureExposureModeContinuousAutoExposure]]) {
            return AVCaptureExposureModeContinuousAutoExposure;
        }
    }
    return AVCaptureExposureModeAutoExpose;
}

+ (AVCaptureWhiteBalanceMode) whiteBalanceModeFromString:(nullable NSString*) mode {
    if (mode) {
        if ([mode isEqualToString:[CameraSettingsWindow whiteBalanceModeToString:AVCaptureWhiteBalanceModeLocked]]) {
            return AVCaptureWhiteBalanceModeLocked;
        }
        if ([mode isEqualToString:[CameraSettingsWindow whiteBalanceModeToString:AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance]]) {
            return AVCaptureWhiteBalanceModeContinuousAutoWhiteBalance;
        }
    }
    return AVCaptureWhiteBalanceModeAutoWhiteBalance;
}
@end
