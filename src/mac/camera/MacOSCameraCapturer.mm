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
#include "MacOSCameraCapturer.h"
#include "MacOSCameraCapturerDelegate.h"
#include "AVCameraCapturer.h"
#include "CoreVideoPixelBuffer.h"
#include "VTSupportedPixelFormats.h"
#include "VideoFrameBuffer.h"
#include "Utils.h"
#include <rtc_base/ref_counted_object.h>

namespace
{

NSArray<AVCaptureDeviceFormat*>* eligibleDeviceFormats(const AVCaptureDevice* device,
                                                       int32_t supportedFps);
int32_t minFrameRate(AVFrameRateRange* range);
int32_t maxFrameRate(AVFrameRateRange* range);

}

namespace LiveKitCpp
{

MacOSCameraCapturer::MacOSCameraCapturer(const MediaDevice& deviceInfo,
                                         AVCaptureDevice* device,
                                         const std::shared_ptr<Bricks::Logger>& logger)
    : CameraCapturer(deviceInfo)
    , _device(device)
    , _delegate([[MacOSCameraCapturerDelegate alloc] initWithCapturer:this andLogger:logger])
    , _impl([[AVCameraCapturer alloc] initWithDelegate:_delegate])
{
}

MacOSCameraCapturer::~MacOSCameraCapturer()
{
    MacOSCameraCapturer::StopCapture();
    [_delegate reset];
}

rtc::scoped_refptr<MacOSCameraCapturer> MacOSCameraCapturer::
    create(const MediaDevice& deviceInfo,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!deviceInfo._guid.empty()) {
        @autoreleasepool {
            const auto guid = deviceInfo._guid.c_str();
            if (AVCaptureDevice* device = [AVCameraCapturer deviceWithUniqueIDUTF8:guid]) {
                auto capturer = rtc::make_ref_counted<MacOSCameraCapturer>(deviceInfo,
                                                                           device,
                                                                           logger);
                if (capturer->_impl) {
                    return capturer;
                }
            }
        }
    }
    return {};
}

void MacOSCameraCapturer::deliverFrame(int64_t timestampMicro, CMSampleBufferRef sampleBuffer)
{
    if (sampleBuffer && hasSink()) {
        if (const auto nativeFrame = createVideoFrame(createBuffer(sampleBuffer), timestampMicro)) {
            sendFrame(nativeFrame.value());
        }
        else {
            discardFrame();
        }
    }
}

std::vector<webrtc::VideoCaptureCapability> MacOSCameraCapturer::capabilities(AVCaptureDevice* device)
{
    std::vector<webrtc::VideoCaptureCapability> capabilities;
    if (device) {
        @autoreleasepool {
            if (auto formats = [AVCameraCapturer formatsForDevice:device]) {
                // calculate expected count of capabilities
                size_t expectedCount = 0UL;
                for (AVCaptureDeviceFormat* format in formats) {
                    enumerateFramerates(format, [&expectedCount](AVFrameRateRange* range){
                        expectedCount += 1 + ((maxFrameRate(range) - minFrameRate(range)) / _frameRateStep);
                    });
                }
                // fill
                if (expectedCount > 0UL) {
                    webrtc::VideoCaptureCapability pattern;
                    // reserve memory
                    capabilities.reserve(expectedCount);
                    for (AVCaptureDeviceFormat* format in formats) {
                        pattern.videoType = fromMediaSubType(format.formatDescription);
                        if (webrtc::VideoType::kUnknown != pattern.videoType) {
                            const auto dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
                            pattern.width = dimensions.width;
                            pattern.height = dimensions.height;
                            pattern.interlaced = interlaced(format.formatDescription);
                            enumerateFramerates(format, [&capabilities, &pattern](AVFrameRateRange* range) {
                                for (int32_t fps = minFrameRate(range), maxFps = maxFrameRate(range);
                                     fps <= maxFps; fps += _frameRateStep) {
                                    pattern.maxFPS = fps;
                                    capabilities.push_back(pattern);
                                }
                            });
                        }
                    }
                    capabilities.shrink_to_fit();
                }
            }
        }
    }
    return capabilities;
}

std::vector<webrtc::VideoCaptureCapability> MacOSCameraCapturer::capabilities(const char* deviceUniqueIdUTF8)
{
    @autoreleasepool {
        return capabilities([AVCameraCapturer deviceWithUniqueIDUTF8:deviceUniqueIdUTF8]);
    }
}

std::string MacOSCameraCapturer::localizedDeviceName(AVCaptureDevice* device)
{
    if (device) {
        return fromNSString(device.localizedName);
    }
    return {};
}

std::string MacOSCameraCapturer::deviceUniqueIdUTF8(AVCaptureDevice* device)
{
    if (device) {
        return fromNSString(device.uniqueID);
    }
    return {};
}

void MacOSCameraCapturer::setObserver(CameraObserver* observer)
{
    [_delegate setObserver:observer];
}

int32_t MacOSCameraCapturer::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    if (_impl) {
        switch (_delegate.state) {
            case CameraState::Stopped:
            case CameraState::Stopping:
                @autoreleasepool {
                    auto format = findClosestFormat(capability);
                    if (format) {
                        if ([_impl startCaptureWithDevice:_device format:format fps:capability.maxFPS]) {
                            return 0;
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
    return -1;
}

int32_t MacOSCameraCapturer::StopCapture()
{
    if (_impl) {
        switch (_delegate.state) {
            case CameraState::Starting:
            case CameraState::Started:
                if ([_impl stopCapture]) {
                    return 0;
                }
                break;
            case CameraState::Stopping:
            case CameraState::Stopped:
                return 0;
            default:
                break;
        }
    }
    return -1;
}

bool MacOSCameraCapturer::CaptureStarted()
{
    return CameraState::Started == _delegate.state;
}

int32_t MacOSCameraCapturer::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    @autoreleasepool {
        if (_device.activeFormat) {
            const auto dimensions = CMVideoFormatDescriptionGetDimensions(_device.activeFormat.formatDescription);
            settings.videoType = fromMediaSubType(_device.activeFormat.formatDescription);
            settings.interlaced = interlaced(_device.activeFormat.formatDescription);
            settings.width = dimensions.width;
            settings.height = dimensions.height;
            settings.maxFPS = static_cast<int32_t>([_impl fps]);
            return 0;
        }
    }
    return -1;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> MacOSCameraCapturer::createBuffer(CMSampleBufferRef sampleBuffer) const
{
    return CoreVideoPixelBuffer::createFromSampleBuffer(sampleBuffer);
}

AVCaptureDeviceFormat* MacOSCameraCapturer::findClosestFormat(const webrtc::VideoCaptureCapability& capability) const
{
    @autoreleasepool {
        auto eligibleFormats = eligibleDeviceFormats(_device, capability.maxFPS);
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

std::optional<webrtc::ColorSpace> MacOSCameraCapturer::activeColorSpace() const
{
    return std::nullopt; // not yet supported now
}

webrtc::VideoType MacOSCameraCapturer::fromMediaSubType(OSType type)
{
    switch (type) {
        case pixelFormatNV12Full():
        case pixelFormatNV12Video():
            break;
        case pixelFormatRGB24():
            return webrtc::VideoType::kRGB24;
        case pixelFormatBGRA32():
            return webrtc::VideoType::kBGRA;
        case pixelFormatARGB32():
            return webrtc::VideoType::kARGB;
        default:
            break;
    }
    return webrtc::VideoType::kYV12; // NV12 is always supported on MacOS
}

webrtc::VideoType MacOSCameraCapturer::fromMediaSubType(CMFormatDescriptionRef format)
{
    if (format) {
        return fromMediaSubType(CMFormatDescriptionGetMediaSubType(format));
    }
    return webrtc::VideoType::kUnknown;
}

bool MacOSCameraCapturer::interlaced(CMFormatDescriptionRef format)
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

template<typename Callback>
void MacOSCameraCapturer::enumerateFramerates(AVCaptureDeviceFormat* format, Callback callback)
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


} // namespace LiveKitCpp

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
