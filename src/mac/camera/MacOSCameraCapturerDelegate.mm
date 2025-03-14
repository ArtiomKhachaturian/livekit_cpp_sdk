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
#include "MacOSCameraCapturerDelegate.h"
#include "MacOSCameraCapturer.h"
#include "Utils.h"
#include "Logger.h"
#include "CameraObserver.h"
#include "CameraManager.h"

@implementation MacOSCameraCapturerDelegate {
    LiveKitCpp::MacOSCameraCapturer* _capturer;
    LiveKitCpp::CameraObserver* _observer;
    std::shared_ptr<Bricks::Logger> _logger;
    std::atomic<LiveKitCpp::CameraState> _state;
}

- (instancetype) initWithCapturer:(LiveKitCpp::MacOSCameraCapturer*) capturer
                        andLogger:(const std::shared_ptr<Bricks::Logger>&) logger
{
    self = [super init];
    if (self) {
        _capturer = capturer;
        _observer = nullptr;
        _logger = logger;
        _state = LiveKitCpp::CameraState::Stopped;
    }
    return self;
}

- (void) reset
{
    @synchronized (self) {
        _capturer = nullptr;
        _observer = nullptr;
        _logger = nullptr;
    }
}

- (LiveKitCpp::CameraState) state
{
    return _state;
}

- (void) setObserver:(LiveKitCpp::CameraObserver* _Nullable) observer
{
    @synchronized (self) {
        _observer = observer;
    }
}

- (void)didCapture:(CMSampleBufferRef) sampleBuffer
    timestampMicro:(int64_t) timeStamp
          capturer:(AVCameraCapturer*) capturer {
#pragma unused(capturer)
    @synchronized (self) {
        if (_capturer) {
            _capturer->deliverFrame(timeStamp, sampleBuffer);
        }
    }
}

- (void)didError:(BOOL) fatal
           error:(NSError*) error
        capturer:(AVCameraCapturer*) capturer {
#pragma unused(capturer)
    @synchronized (self) {
        if (_logger) {
            const auto errorStr = LiveKitCpp::toString(error);
            if (fatal) {
                _logger->logError(errorStr, LiveKitCpp::CameraManager::logCategory());
            }
            else {
                _logger->logWarning(errorStr, LiveKitCpp::CameraManager::logCategory());
            }
        }
    }
    if (fatal) {
        [self notifyAboutChanges:LiveKitCpp::CameraState::Stopped];
    }
}

- (void)didStarted:(AVCameraCapturer*) capturer {
#pragma unused(capturer)
    [self notifyAboutChanges:LiveKitCpp::CameraState::Started];
}

- (void)didStopped:(AVCameraCapturer*) capturer {
#pragma unused(capturer)
    [self notifyAboutChanges:LiveKitCpp::CameraState::Stopped];
}

- (void) notifyAboutChanges:(LiveKitCpp::CameraState) state
{
    if (state != _state.exchange(state)) {
        @synchronized (self) {
            if (_observer) {
                _observer->onStateChanged(state);
            }
        }
    }
}

@end
#endif
