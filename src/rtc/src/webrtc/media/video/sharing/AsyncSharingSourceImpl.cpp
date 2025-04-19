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
#include "AsyncSharingSourceImpl.h"
#include "DesktopConfiguration.h"
#include "DesktopCapturer.h"
#include "VideoUtils.h"

namespace LiveKitCpp
{

AsyncSharingSourceImpl::AsyncSharingSourceImpl(std::unique_ptr<DesktopCapturer> capturer,
                                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger)
    , _capturer(std::move(capturer))
{
    if (_capturer) {
        setOptions(VideoOptions{._maxFPS = DesktopConfiguration::maxFramerate(_capturer->window())});
    }
}

AsyncSharingSourceImpl::~AsyncSharingSourceImpl()
{
    close();
    if (_capturer) {
        _capturer->setOutputSink(nullptr);
    }
}

void AsyncSharingSourceImpl::requestCapturer()
{
    if (_capturer && frameWanted()) {
        const auto device = this->deviceInfo();
        if (_capturer->selectSource(device._guid)) {
            _capturer->setOutputSink(this);
            if (enabled()) {
                startCapturer();
            }
        }
        else {
            // TODO: add error details
            notify(&MediaDeviceListener::onMediaCreationFailed, std::string{});
        }
    }
}

void AsyncSharingSourceImpl::resetCapturer()
{
    if (_capturer) {
        stopCapturer();
        _capturer->setOutputSink(nullptr);
    }
}

void AsyncSharingSourceImpl::onOptionsChanged(const VideoOptions& options)
{
    AsyncVideoSourceImpl::onOptionsChanged(options);
    if (active() && _capturer) {
        _capturer->setPreviewMode(options.preview());
        _capturer->setTargetResolutuon(options._width, options._height);
        _capturer->setTargetFramerate(options._maxFPS);
    }
}

MediaDeviceInfo AsyncSharingSourceImpl::validate(MediaDeviceInfo info) const
{
    if (_capturer) {
        if (_capturer->window()) {
            if (_capturer->windowIdFromString(info._guid).has_value()) {
                return info;
            }
        }
        if (_capturer->screenIdFromString(info._guid).has_value()) {
            return info;
        }
    }
    return {};
}

VideoOptions AsyncSharingSourceImpl::validate(VideoOptions options) const
{
    if (_capturer) {
        if (!_capturer->window()) {
            const auto res = _capturer->screenResolution(deviceInfo()._guid);
            if (!res.is_empty()) {
                options._width = std::min(options._width, res.width());
                options._height = std::min(options._height, res.height());
            }
        }
        return options;
    }
    return {};
}

void AsyncSharingSourceImpl::startCapturer()
{
    if (_capturer) {
        const auto captureOptions = this->options();
        _capturer->setPreviewMode(captureOptions.preview());
        _capturer->setTargetResolutuon(captureOptions._width, captureOptions._height);
        _capturer->setTargetFramerate(captureOptions._maxFPS);
        if (_capturer->start()) {
            notify(&MediaDeviceListener::onMediaStarted);
        }
        else {
            // TODO: add error details
            notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
        }
    }
}

void AsyncSharingSourceImpl::stopCapturer()
{
    if (_capturer && _capturer->started()) {
        _capturer->stop();
        notify(&MediaDeviceListener::onMediaStopped);
    }
}

} // namespace LiveKitCpp
