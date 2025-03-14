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
#pragma once
#ifdef WEBRTC_MAC
#import "AVCameraCapturerDelegate.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>


@interface AVCameraCapturer : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
+ (NSArray<AVCaptureDeviceFormat*>* _Nonnull) formatsForDevice:(AVCaptureDevice* _Nonnull) device;
+ (NSArray<AVCaptureDeviceFormat*>* _Nullable) formatsForDeviceUniqueID:(NSString* _Nonnull) deviceUniqueID;
+ (NSArray<AVCaptureDeviceFormat*>* _Nullable) formatsForDeviceUniqueIDUTF8:(const char* _Nonnull) deviceUniqueIDUTF8;
+ (AVCaptureDevice* _Nullable) deviceWithUniqueID:(NSString* _Nonnull) deviceUniqueID;
+ (AVCaptureDevice* _Nullable) deviceWithUniqueIDUTF8:(const char* _Nonnull) deviceUniqueIDUTF8;
+ (NSArray<AVCaptureDevice*>* _Nullable) availableDevices;
- (instancetype _Nullable)initWithDelegate:(id<AVCameraCapturerDelegate> _Nonnull) delegate;
// Starts the capture session asynchronously.
- (BOOL) startCaptureWithDevice:(AVCaptureDevice* _Nonnull)device
                         format:(AVCaptureDeviceFormat* _Nonnull)format
                            fps:(int32_t)fps;
// Stops the capture session asynchronously.
- (BOOL) stopCapture;
@property (atomic, readonly) NSInteger fps;
@end
#endif
