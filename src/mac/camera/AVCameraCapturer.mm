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
#ifdef WEBRTC_MAC
#include "AVCameraCapturer.h"
#include "Utils.h"
#include "VTSupportedPixelFormats.h"
#include <rtc_base/system/gcd_helpers.h>
#include <cmath>

#define CAPTURER_DOMAIN "org.livekitcpp.cameracapturer.video"

using namespace LiveKitCpp;

typedef NS_ENUM(NSInteger, Error) {
    WrongOutputDevice = 1,
    CannotAddOutputDevice,
    CannotAddCameraAsInput,
    CannotSetDesiredFps
};

@implementation AVCameraCapturer {
    id<AVCameraCapturerDelegate> _delegate;
    dispatch_queue_t _frameQueue;
    AVCaptureVideoDataOutput* _videoDataOutput;
    AVCaptureSession* _captureSession;
    FourCharCode _preferredOutputPixelFormat;
    std::atomic<uint64_t> _framesCounter;
}

@synthesize fps = _fps;

+ (NSArray<AVCaptureDeviceFormat*>*) formatsForDevice:(AVCaptureDevice*) device
{
    // support opening the device in any format. We make sure it's converted to a format we
    // can handle, if needed, in the method `-setupVideoDataOutput`.
    return [device formats];
}

+ (NSArray<AVCaptureDeviceFormat*>*) formatsForDeviceUniqueID:(NSString*) deviceUniqueID
{
    if (AVCaptureDevice* device = [AVCameraCapturer deviceWithUniqueID:deviceUniqueID]) {
        return [AVCameraCapturer formatsForDevice: device];
    }
    return nil;
}

+ (NSArray<AVCaptureDeviceFormat*>*) formatsForDeviceUniqueIDUTF8:(const char*) deviceUniqueIDUTF8
{
    if (deviceUniqueIDUTF8) {
        return [AVCameraCapturer formatsForDeviceUniqueID:LiveKitCpp::toNSString(deviceUniqueIDUTF8)];
    }
    return nil;
}

+ (AVCaptureDevice*) deviceWithUniqueID:(NSString*) deviceUniqueID
{
    return [AVCaptureDevice deviceWithUniqueID:deviceUniqueID];
}

+ (AVCaptureDevice*) deviceWithUniqueIDUTF8:(const char*) deviceUniqueIDUTF8
{
    if (deviceUniqueIDUTF8) {
        return [AVCameraCapturer deviceWithUniqueID:LiveKitCpp::toNSString(deviceUniqueIDUTF8)];
    }
    return nil;
}

+ (NSArray<AVCaptureDevice*>*) availableDevices
{
    if (@available(macOS 10.15,*)) {
        AVCaptureDeviceDiscoverySession* video = [AVCaptureDeviceDiscoverySession
                                                  discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeBuiltInWideAngleCamera,
                                                                                    AVCaptureDeviceTypeExternalUnknown]
                                                  mediaType:AVMediaTypeVideo
                                                  position:AVCaptureDevicePositionUnspecified];
        return [video devices];
    }
    return [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
}

- (instancetype) initWithDelegate:(id<AVCameraCapturerDelegate>) delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;
        AVCaptureVideoDataOutput* videoDataOutput = [AVCaptureVideoDataOutput new];
        if (const auto availablePixelFormats = [AVCameraCapturer supportedPixelFormats:videoDataOutput]) {
            AVCaptureSession* captureSession = [AVCaptureSession new];
            if ([captureSession canAddOutput:videoDataOutput]) {
                [captureSession addOutput:videoDataOutput];
                _captureSession = captureSession;
                _videoDataOutput = videoDataOutput;
                _frameQueue = RTCDispatchQueueCreateWithTarget(CAPTURER_DOMAIN,
                                                               DISPATCH_QUEUE_SERIAL,
                                                               dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
                {
                    NSNumber* prefferedPixelFormat = availablePixelFormats.firstObject;
                    _preferredOutputPixelFormat = [prefferedPixelFormat unsignedIntValue];
                    _videoDataOutput.videoSettings = @{(NSString*)kCVPixelBufferPixelFormatTypeKey : prefferedPixelFormat};
                }
                _videoDataOutput.alwaysDiscardsLateVideoFrames = NO;
                [_videoDataOutput setSampleBufferDelegate:self queue:_frameQueue];
                // direct subscribe to running state
                // https://developer.apple.com/documentation/avfoundation/avcapturesession/isrunning?language=objc
                [_captureSession addObserver:self
                                  forKeyPath:@"running"
                                     options:NSKeyValueObservingOptionNew
                                     context:nil];
                // subscribe to capturer session notifications
                NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
                [center addObserver:self
                           selector:@selector(handleCaptureSessionRuntimeError:)
                               name:AVCaptureSessionRuntimeErrorNotification
                             object:_captureSession];
                [center addObserver:self
                           selector:@selector(handleCaptureSessionDidStartRunning:)
                               name:AVCaptureSessionDidStartRunningNotification
                             object:_captureSession];
                [center addObserver:self
                           selector:@selector(handleCaptureSessionDidStopRunning:)
                               name:AVCaptureSessionDidStopRunningNotification
                             object:_captureSession];
                return self;
            }
            else {
                [self sendFatalError:[AVCameraCapturer createError:Error::CannotAddOutputDevice
                                                            reason:@"Cannot add output device to the capture session"]];
            }
        }
        else {
            [self sendFatalError:[AVCameraCapturer createError:Error::WrongOutputDevice
                                                        reason:@"Output device has no supported formats"]];
        }
    }
    return nil;
}

- (void) dealloc
{
    [self stopCapture];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL) startCaptureWithDevice:(AVCaptureDevice*)device
                         format:(AVCaptureDeviceFormat*)format
                            fps:(int32_t)fps
{
    BOOL started = NO;
    [self stopCapture];
    NSError* error = nil;
    if ([device lockForConfiguration:&error]) {
        if ([self reconfigureCaptureSessionInput:device]) {
            [self updateDeviceCaptureFormat:device format:format fps:fps];
            [self updateVideoDataOutputPixelFormat:format];
            @synchronized (_captureSession) {
                if (_captureSession) {
                    _framesCounter = 0ULL;
                    [_captureSession startRunning];
                    started = YES;
                }
            }
        }
        [device unlockForConfiguration];
    }
    else {
        [self sendFatalError:error];
    }
    return started;
}

- (BOOL) stopCapture
{
    @synchronized (_captureSession) {
        if (_captureSession) {
            if (_captureSession.running) {
                [self removeCaptureSessionAllInputs:YES];
                [_captureSession stopRunning];
                _framesCounter = 0ULL;
            }
            return YES;
        }
    }
    return NO;
}

- (void) captureOutput:(AVCaptureOutput*) output
 didOutputSampleBuffer:(CMSampleBufferRef) sampleBuffer
        fromConnection:(AVCaptureConnection*) connection
{
    NSParameterAssert(output == _videoDataOutput);
    // DVC-19632 & DVC-21294:
    // drop video frames during of first ~ 0.3 sec for initial camera stabilization,
    // required for suppression of black/dark frames after start capturing,
    // according to advice from https://stackoverflow.com/a/22995641
    if (_framesCounter.fetch_add(1U) > unsigned(_fps / 3)) {
        [_delegate didCapture:sampleBuffer
               timestampMicro:[AVCameraCapturer timestampMicro:sampleBuffer]
                     capturer:self];
    }
}

- (void) observeValueForKeyPath:(NSString*) keyPath
                       ofObject:(id) object
                         change:(NSDictionary<NSKeyValueChangeKey,id>*) change
                        context:(void*) context {
    if ([keyPath isEqualToString:@"running"]) {
        BOOL running = [change[NSKeyValueChangeNewKey] boolValue];
        if (running) {
            [_delegate didStarted:self];
        }
        else {
            [_delegate didStopped:self];
        }
    }
}

- (void) handleCaptureSessionRuntimeError:(NSNotification*) notification
{
    [self removeCaptureSessionAllInputs:YES];
    [self sendFatalError:[notification.userInfo objectForKey:AVCaptureSessionErrorKey]];
}

- (void) handleCaptureSessionDidStartRunning:(NSNotification*) notification
{
    [_delegate didStarted:self];
}

- (void) handleCaptureSessionDidStopRunning:(NSNotification*) notification
{
    [_delegate didStopped:self];
}

+ (NSArray<NSNumber*>* _Nullable) supportedPixelFormats:(AVCaptureVideoDataOutput* _Nonnull) output
{
    if (const auto availablePixelFormats = [NSMutableOrderedSet orderedSetWithArray:output.availableVideoCVPixelFormatTypes]) {
        NSMutableArray<NSNumber*>* supportedPixelFormats = [NSMutableArray new];
        for (NSNumber* format in availablePixelFormats) {
            if (isSupportedFormat(format.unsignedIntValue)) {
                [supportedPixelFormats addObject:format];
            }
        }
        if ([supportedPixelFormats count] > 0U) {
            return supportedPixelFormats;
        }
    }
    return nil;
}

- (void) sendError:(BOOL) fatal error:(NSError* _Nonnull) error
{
    [_delegate didError:fatal error:error capturer:self];
}

- (void) sendFatalError:(NSError* _Nonnull) error
{
    [self sendError:YES error:error];
}

- (void) sendNotFatalError:(NSError* _Nonnull) error
{
    [self sendError:NO error:error];
}

- (BOOL) reconfigureCaptureSessionInput:(AVCaptureDevice* _Nonnull) device
{
    BOOL result = NO;
    NSError* error = nil;
    if (AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error]) {
        [self removeCaptureSessionAllInputs: NO];
        @synchronized (_captureSession) {
            if (_captureSession) {
                [_captureSession beginConfiguration];
                if ([_captureSession canAddInput:input]) {
                    [_captureSession addInput:input];
                    result = YES;
                }
                [_captureSession commitConfiguration];
                if (!result) {
                    [self sendFatalError:[AVCameraCapturer createError:Error::CannotAddCameraAsInput
                                                                reason:@"Cannot add camera as an input to the session"]];
                }
            }
        }
    }
    else {
        [self sendFatalError:error];
    }
    return result;
}

- (void) removeCaptureSessionAllInputs:(BOOL) configure
{
    @synchronized (_captureSession) {
        if (_captureSession) {
            if (configure) {
                [_captureSession beginConfiguration];
            }
            for (AVCaptureDeviceInput* oldInput in [_captureSession.inputs copy]) {
                [_captureSession removeInput:oldInput];
            }
            if (configure) {
                [_captureSession commitConfiguration];
            }
        }
    }
    _fps = 0U;
}

- (void) updateDeviceCaptureFormat:(AVCaptureDevice* _Nonnull) device
                            format:(AVCaptureDeviceFormat* _Nonnull) format
                               fps:(int32_t) fps
{
    BOOL framerateApplied = FALSE;
    device.activeFormat = format;
    for (AVFrameRateRange* range in format.videoSupportedFrameRateRanges) {
        if (fps >= ::lround(range.minFrameRate) && fps <= ::lround(range.maxFrameRate)) {
            @try {
                device.activeVideoMinFrameDuration = CMTimeMake(1, fps);
                framerateApplied = YES;
            }
            @catch (NSException*) { // suppress 'invalid_arg' exception
                const auto minFrameDuration = device.activeVideoMinFrameDuration;
                if (0 != CMTimeCompare(kCMTimeInvalid, minFrameDuration)) {
                    _fps = static_cast<NSInteger>(std::round(minFrameDuration.timescale / minFrameDuration.value));
                }
                framerateApplied = fps == _fps;
            }
            break;
        }
    }
    if (framerateApplied) {
        _fps = fps;
    }
    else {
        NSString* reason = [NSString stringWithFormat: @"Requested framerate %ld fps is not supported", (long)fps];
        [self sendNotFatalError:[AVCameraCapturer createError:Error::CannotSetDesiredFps reason:reason]];
    }
}

- (void) updateVideoDataOutputPixelFormat:(AVCaptureDeviceFormat* _Nonnull) format
{
    FourCharCode mediaSubType = CMFormatDescriptionGetMediaSubType(format.formatDescription);
    if (!isSupportedFormat(mediaSubType)) {
        mediaSubType = _preferredOutputPixelFormat;
    }
    // update videoSettings with dimensions, as some virtual cameras, e.g. Snap Camera,
    // may not work otherwise
    CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
    _videoDataOutput.videoSettings = @{
        (id)kCVPixelBufferWidthKey : @(dimensions.width),
        (id)kCVPixelBufferHeightKey : @(dimensions.height),
        (id)kCVPixelBufferPixelFormatTypeKey : @(mediaSubType),
    };
}

+ (NSError*) createError:(Error)code
                  reason:(NSString* _Nonnull) reason
{
    return [AVCameraCapturer createErrorWithUserInfo:code userInfo:@{@"Error reason": reason}];
}

+ (NSError*) createErrorWithUserInfo:(Error)code
                            userInfo:(NSDictionary<NSErrorUserInfoKey, id>* _Nullable) userInfo
{
    return [[NSError alloc] initWithDomain:@CAPTURER_DOMAIN code:code userInfo:userInfo];
}

+ (int64_t) timestampMicro:(CMSampleBufferRef) sampleBuffer
{
    if (sampleBuffer) {
        return LiveKitCpp::cmTimeToMicro(CMSampleBufferGetOutputPresentationTimeStamp(sampleBuffer));
    }
    return 0LL; // auto
}

@end
#endif
