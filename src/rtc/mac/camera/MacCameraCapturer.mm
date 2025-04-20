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
#include "MacCameraCapturer.h"
#include "CapturerObserver.h"
#include "CameraManager.h"
#include "Listener.h"
#include "VideoUtils.h"
#include "VTSupportedPixelFormats.h"
#include "Utils.h"
#include "livekit/rtc/media/VideoContentHint.h"
#include <modules/video_capture/video_capture_config.h>
#include <rtc_base/ref_counted_object.h>
#include <rtc_base/time_utils.h>
#include <components/capturer/RTCCameraVideoCapturer.h>
#include <native/api/video_frame_buffer.h>
#include <helpers/RTCDispatcher+Private.h>
#include <set>
#include <unordered_set>

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

namespace
{

NSArray<AVCaptureDeviceFormat*>* eligibleDeviceFormats(const AVCaptureDevice* device,
                                                       int32_t supportedFps);
int32_t minFrameRate(AVFrameRateRange* range);
int32_t maxFrameRate(AVFrameRateRange* range);

inline size_t hashCode(const webrtc::VideoCaptureCapability& cap) {
    return LiveKitCpp::hashCombine(cap.width, cap.height,
                                   cap.interlaced, cap.maxFPS,
                                   cap.videoType);
}

}

@interface CapturerDelegate : NSObject<RTC_OBJC_TYPE(RTCVideoCapturerDelegate)>

- (instancetype) init NS_DESIGNATED_INITIALIZER;
- (void) reportAboutError:(NSError* _Nullable) error;
- (void) reportAboutErrorMessage:(NSString* _Nonnull) error;
- (BOOL) changeState:(LiveKitCpp::CapturerState) state;
@property (nullable, nonatomic) rtc::VideoSinkInterface<webrtc::VideoFrame>* sink;
@property (nullable, nonatomic) LiveKitCpp::CapturerObserver* observer;
@property (nonatomic) LiveKitCpp::CapturerState state;
@property (weak) RTCCameraVideoCapturer* capturerRef;

@end

namespace LiveKitCpp
{

class MacCameraCapturer::Impl
{
public:
    Impl(AVCaptureDevice* device);
    ~Impl();
    AVCaptureDevice* device() const noexcept { return _device; }
    void setContentHint(VideoContentHint hint);
    bool startCapture(AVCaptureDeviceFormat* format, NSInteger fps);
    void stopCapture();
    void setSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) { _delegate.sink = sink; }
    void setObserver(CapturerObserver* observer) { _delegate.observer = observer; }
    CapturerState state() const { return _delegate.state; }
    bool changeState(CapturerState state) { return [_delegate changeState:state]; }
    void reportAboutError(const std::string& error);
private:
    AVCaptureDevice* const _device;
    CapturerDelegate* const _delegate;
    RTC_OBJC_TYPE(RTCCameraVideoCapturer)* const _capturer;
};

MacCameraCapturer::MacCameraCapturer(const MediaDeviceInfo& deviceInfo, std::unique_ptr<Impl> impl)
    : CameraCapturer(deviceInfo)
    , _impl(std::move(impl))
{
    _impl->setSink(this);
}

MacCameraCapturer::~MacCameraCapturer()
{
    const auto ok = 0 == MacCameraCapturer::StopCapture();
    _impl->setSink(nullptr);
    if (ok) {
        _impl->changeState(CapturerState::Stopped);
    }
    _impl->setObserver(nullptr);
}

rtc::scoped_refptr<MacCameraCapturer> MacCameraCapturer::create(const MediaDeviceInfo& deviceInfo)
{
    if (!deviceInfo._guid.empty()) {
        @autoreleasepool {
            const auto guid = deviceInfo._guid.c_str();
            AVCaptureDevice* device = deviceWithUniqueIDUTF8(guid);
            if (device) {
                auto impl = std::make_unique<Impl>(device);
                return rtc::make_ref_counted<MacCameraCapturer>(deviceInfo, std::move(impl));
            }
        }
    }
    return {};
}

AVCaptureDevice* MacCameraCapturer::deviceWithUniqueIDUTF8(const char* deviceUniqueIdUTF8)
{
    return [AVCaptureDevice deviceWithUniqueID:toNSString(deviceUniqueIdUTF8)];
}

std::vector<webrtc::VideoCaptureCapability> MacCameraCapturer::capabilities(AVCaptureDevice* device)
{
    if (device) {
        @autoreleasepool {
            if (auto formats = [RTCCameraVideoCapturer supportedFormatsForDevice:device]) {
                // fill
                std::list<webrtc::VideoCaptureCapability> caps;
                std::unordered_set<size_t> enumerated;
                // reserve memory
                for (AVCaptureDeviceFormat* format in formats) {
                    webrtc::VideoCaptureCapability pattern;
                    pattern.videoType = fromMediaSubType(format.formatDescription);
                    if (webrtc::VideoType::kUnknown != pattern.videoType) {
                        const auto dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
                        pattern.width = dimensions.width;
                        pattern.height = dimensions.height;
                        pattern.interlaced = interlaced(format.formatDescription);
                        const auto code = hashCode(pattern);
                        if (!enumerated.count(code)) {
                            std::set<int32_t> availableFps;
                            enumerateFramerates(format, [&availableFps](AVFrameRateRange* range) {
                                const auto minFps = minFrameRate(range);
                                const auto maxFps = maxFrameRate(range);
                                for (int32_t fps = minFps; fps <= maxFps; ++fps) {
                                    availableFps.insert(fps);
                                }
                            });
                            if (availableFps.empty()) {
                                pattern.maxFPS = webrtc::videocapturemodule::kDefaultFrameRate;
                            }
                            else {
                                for (auto fps : availableFps) {
                                    if (0 == (fps % _frameRateStep)) {
                                        pattern.maxFPS = fps;
                                        caps.push_back(pattern);
                                    }
                                }
                            }
                            enumerated.insert(code);
                        }
                    }
                }
                return {std::make_move_iterator(caps.begin()),
                        std::make_move_iterator(caps.end())};
            }
        }
    }
    return {};
}

std::vector<webrtc::VideoCaptureCapability> MacCameraCapturer::capabilities(const char* deviceUniqueIdUTF8)
{
    @autoreleasepool {
        const auto device = deviceWithUniqueIDUTF8(deviceUniqueIdUTF8);
        return capabilities(device);
    }
}

std::string MacCameraCapturer::localizedDeviceName(AVCaptureDevice* device)
{
    if (device) {
        return fromNSString(device.localizedName);
    }
    return {};
}

std::string MacCameraCapturer::deviceUniqueIdUTF8(AVCaptureDevice* device)
{
    if (device) {
        return fromNSString(device.uniqueID);
    }
    return {};
}

void MacCameraCapturer::setContentHint(VideoContentHint hint)
{
    CameraCapturer::setContentHint(hint);
    _impl->setContentHint(hint);
}

void MacCameraCapturer::setObserver(CapturerObserver* observer)
{
    CameraCapturer::setObserver(observer);
    _impl->setObserver(observer);
}

int32_t MacCameraCapturer::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    int result = -1;
    switch (_impl->state()) {
        case CapturerState::Stopped:
        case CapturerState::Stopping:
            @autoreleasepool {
                auto format = findClosestFormat(capability);
                if (format) {
                    if (_impl->startCapture(format, capability.maxFPS)) {
                        result = 0;
                    }
                }
                else {
                    _impl->reportAboutError("unable to find closest format for [" + toString(capability) + "]");
                }
            }
            break;
        default:
            break;
    }
    return result;
}

int32_t MacCameraCapturer::StopCapture()
{
    _impl->stopCapture();
    return 0;
}

int32_t MacCameraCapturer::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    @autoreleasepool {
        if (_impl->device().activeFormat) {
            const auto desc = _impl->device().activeFormat.formatDescription;
            const auto dimensions = CMVideoFormatDescriptionGetDimensions(desc);
            settings.videoType = fromMediaSubType(desc);
            settings.interlaced = interlaced(desc);
            settings.width = dimensions.width;
            settings.height = dimensions.height;
            const auto fps = 1. / CMTimeGetSeconds(_impl->device().activeVideoMinFrameDuration);
            settings.maxFPS = static_cast<int32_t>(std::round(fps));
            return 0;
        }
    }
    return -1;
}

bool MacCameraCapturer::CaptureStarted()
{
    return CapturerState::Started == _impl->state();
}

webrtc::VideoType MacCameraCapturer::fromMediaSubType(OSType type)
{
    switch (type) {
        case pixelFormatNV12Full():
        case pixelFormatNV12Video():
            return webrtc::VideoType::kNV12;
        case pixelFormatYUY2():
            return webrtc::VideoType::kYUY2;
        case pixelFormatI420():
            return webrtc::VideoType::kI420;
        case pixelFormatUYVY():
            return webrtc::VideoType::kUYVY;
        case pixelFormatRGB24(): // kCVPixelFormatType_24RGB
            return webrtc::VideoType::kRGB24;
        case pixelFormatBGRA32(): // kCVPixelFormatType_32BGRA
            return webrtc::VideoType::kBGRA;
        case pixelFormatARGB32():
            return webrtc::VideoType::kARGB;
        default:
            break;
    }
    return webrtc::VideoType::kUnknown;
}

webrtc::VideoType MacCameraCapturer::fromMediaSubType(CMFormatDescriptionRef format)
{
    if (format) {
        return fromMediaSubType(CMFormatDescriptionGetMediaSubType(format));
    }
    return webrtc::VideoType::kUnknown;
}

bool MacCameraCapturer::interlaced(CMFormatDescriptionRef format)
{
    if (format) {
        @autoreleasepool {
            auto props = CMFormatDescriptionGetExtension(format, kCMFormatDescriptionExtension_FieldCount);
            NSNumber* fieldCount = (__bridge NSNumber*)props;
            return fieldCount && fieldCount.integerValue > 1U;
        }
    }
    return false;
}

template <typename Callback>
void MacCameraCapturer::enumerateFramerates(AVCaptureDeviceFormat* format, Callback callback)
{
    if (format) {
        @autoreleasepool {
            const auto ranges = format.videoSupportedFrameRateRanges;
            if (ranges && [ranges count] > 0U) {
                for (AVFrameRateRange* rate in ranges) {
                    callback(rate);
                }
            }
        }
        format = nil;
    }
}

AVCaptureDeviceFormat* MacCameraCapturer::findClosestFormat(const webrtc::VideoCaptureCapability& capability) const
{
    @autoreleasepool {
        auto eligibleFormats = eligibleDeviceFormats(_impl->device(), capability.maxFPS);
        if (eligibleFormats) {
            AVCaptureDeviceFormat* desiredDeviceFormat = nil;
            for (AVCaptureDeviceFormat* deviceFormat in eligibleFormats) {
                CMVideoDimensions dimension = CMVideoFormatDescriptionGetDimensions(deviceFormat.formatDescription);
                if (dimension.width == capability.width && dimension.height == capability.height) {
                    // this is the preferred format so no need to wait for better option.
                    return deviceFormat; // exact the same as required
                }
                if (dimension.width < capability.width && dimension.height < capability.height) {
                    // this is good candidate, but let's wait for something better.
                    desiredDeviceFormat = deviceFormat;
                }
            }
            return desiredDeviceFormat;
        }
    }
    return nil;
}

MacCameraCapturer::Impl::Impl(AVCaptureDevice* device)
    : _device(device)
    , _delegate([CapturerDelegate new])
    , _capturer([[RTCCameraVideoCapturer alloc] initWithDelegate:_delegate])
{
    @autoreleasepool {
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
        [center addObserver:_delegate
                   selector:@selector(handleCaptureSessionRuntimeError:)
                       name:AVCaptureSessionRuntimeErrorNotification
                     object:_capturer.captureSession];
        _delegate.capturerRef = _capturer;
    }
}

MacCameraCapturer::Impl::~Impl()
{
    stopCapture();
    @autoreleasepool {
        [[NSNotificationCenter defaultCenter] removeObserver:_delegate];
    }
}

void MacCameraCapturer::Impl::setContentHint(VideoContentHint hint)
{
    @autoreleasepool {
        AVCaptureSession* session = _capturer.captureSession;
        if (session) {
            switch (hint) {
                case VideoContentHint::Fluid:
                    session.sessionPreset = AVCaptureSessionPresetHigh;
                    break;
                case VideoContentHint::Detailed:
                    session.sessionPreset = AVCaptureSessionPresetPhoto;
                    break;
                default:
                    session.sessionPreset = AVCaptureSessionPresetMedium;
                    break;
            }
        }
    }
}

bool MacCameraCapturer::Impl::startCapture(AVCaptureDeviceFormat* format, NSInteger fps)
{
    if (format && _device) {
        @autoreleasepool {
            if ([_delegate changeState:CapturerState::Starting]) {
                CapturerDelegate* __weak weakDelegate = _delegate;
                [_capturer startCaptureWithDevice:_device
                                           format:format
                                              fps:fps
                                completionHandler:^(NSError* _Nullable error) {
                    CapturerDelegate* __strong delegate = weakDelegate;
                    if (delegate) {
                        if (error) {
                            delegate.state = CapturerState::Stopped;
                            [delegate reportAboutError:error];
                        }
                        else {
                            delegate.state = CapturerState::Started;
                        }
                    }
                }];
                return true;
            }
        }
    }
    return false;
}

void MacCameraCapturer::Impl::stopCapture()
{
    @autoreleasepool {
        if ([_delegate changeState:CapturerState::Stopping]) {
            CapturerDelegate* __weak weakDelegate = _delegate;
            [_capturer stopCaptureWithCompletionHandler:^{
                CapturerDelegate* __strong delegate = weakDelegate;
                if (delegate) {
                    delegate.state = CapturerState::Stopped;
                }
            }];
        }
    }
}

void MacCameraCapturer::Impl::reportAboutError(const std::string& error)
{
    if (!error.empty()) {
        [_delegate reportAboutErrorMessage:toNSString(error)];
    }
}

} // namespace LiveKitCpp

@implementation CapturerDelegate {
    Bricks::Listener<rtc::VideoSinkInterface<webrtc::VideoFrame>*> _sink;
    Bricks::Listener<LiveKitCpp::CapturerObserver*> _observer;
    Bricks::SafeObj<LiveKitCpp::CapturerState> _state;
}

@synthesize capturerRef = _capturerRef;

- (instancetype) init
{
    if (self = [super init]) {
        _state(LiveKitCpp::CapturerState::Stopped);
    }
    return self;
}

- (void) reportAboutError:(NSError*) error
{
    if (error) {
        [self reportAboutErrorMessage: [error localizedDescription]];
    }
}

- (void) reportAboutErrorMessage:(NSString*) error
{
    if (error) {
        _observer.invoke(&LiveKitCpp::CapturerObserver::onCapturingError,
                         LiveKitCpp::fromNSString(error), true);
    }
}

- (BOOL) changeState:(LiveKitCpp::CapturerState) state
{
    using namespace LiveKitCpp;
    bool accepted = false;
    {
        LOCK_WRITE_SAFE_OBJ(_state);
        if (state != _state.constRef()) {
            accepted = acceptState(_state.constRef(), state);
            if (accepted) {
                _state = state;
            }
        }
    }
    if (accepted) {
        _observer.invoke(&CapturerObserver::onStateChanged, state);
        return YES;
    }
    return NO;
}

- (rtc::VideoSinkInterface<webrtc::VideoFrame>*) sink
{
    return _sink.listener();
}

- (void) setSink:(rtc::VideoSinkInterface<webrtc::VideoFrame>*) sink
{
    _sink = sink;
}

- (LiveKitCpp::CapturerObserver*) observer
{
    return _observer.listener();
}

- (void) setObserver:(LiveKitCpp::CapturerObserver*) observer
{
    _observer = observer;
}

- (LiveKitCpp::CapturerState) state
{
    return _state();
}

- (void) setState:(LiveKitCpp::CapturerState) state
{
    [self changeState:state];
}

- (void) capturer:(RTCVideoCapturer*) capturer didCaptureVideoFrame:(RTCVideoFrame*) frame
{
    if (_sink) {
        if (frame) {
            if (const auto buffer = webrtc::ObjCToNativeVideoFrameBuffer(frame.buffer)) {
                if (auto rtcFrame = LiveKitCpp::createVideoFrame(buffer)) {
                    rtcFrame->set_rotation(webrtc::VideoRotation(frame.rotation));
                    rtcFrame->set_rtp_timestamp(frame.timeStamp);
                    rtcFrame->set_timestamp_us(frame.timeStampNs / rtc::kNumNanosecsPerMicrosec);
                    _sink.invoke(&rtc::VideoSinkInterface<webrtc::VideoFrame>::OnFrame, rtcFrame.value());
                    return;
                }
            }
        }
        _sink.invoke(&rtc::VideoSinkInterface<webrtc::VideoFrame>::OnDiscardedFrame);
    }
}

- (void)handleCaptureSessionRuntimeError:(NSNotification *)notification {
    NSError *error = [notification.userInfo objectForKey:AVCaptureSessionErrorKey];

    [RTC_OBJC_TYPE(RTCDispatcher) dispatchAsyncOnType:RTCDispatcherTypeCaptureSession
                                                block:^{
        [self reportAboutError:error];
        [_capturerRef stopCapture];
        self.state = LiveKitCpp::CapturerState::Stopped;
    }];
}

@end

namespace {

inline bool isFrameRateWithinRange(int32_t fps, AVFrameRateRange* range)
{
    return range != nil && fps >= minFrameRate(range) && fps <= maxFrameRate(range);
}

NSArray<AVCaptureDeviceFormat*>* eligibleDeviceFormats(const AVCaptureDevice* device, int32_t supportedFps)
{
    if (device) {
        @autoreleasepool {
            NSMutableArray<AVCaptureDeviceFormat*>* accepted = [NSMutableArray array];
            for (AVCaptureDeviceFormat* format in device.formats) {
                // Filter out frame rate ranges that we currently don't support in the stack
                for (AVFrameRateRange* frameRateRange in format.videoSupportedFrameRateRanges) {
                    if (isFrameRateWithinRange(supportedFps, frameRateRange)) {
                        [accepted addObject:format];
                        break;
                    }
                }
            }
            if ([accepted count] > 0U) {
                return accepted;
            }
        }
    }
    return nullptr;
}

int32_t minFrameRate(AVFrameRateRange* range)
{
    if (range) {
        const auto min = std::min(range.minFrameRate, range.maxFrameRate);
        return static_cast<int32_t>(std::round(min));
    }
    return 0;
}

int32_t maxFrameRate(AVFrameRateRange* range)
{
    if (range) {
        const auto max = std::max(range.minFrameRate, range.maxFrameRate);
        return static_cast<int32_t>(std::round(max));
    }
    return 0;
}

}
#endif
