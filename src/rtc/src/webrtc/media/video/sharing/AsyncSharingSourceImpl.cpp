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

AsyncSharingSourceImpl::AsyncSharingSourceImpl(std::weak_ptr<DesktopConfiguration> desktopConfiguration,
                                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger)
    , _desktopConfiguration(std::move(desktopConfiguration))
{
    /*if (_capturer) {
        setOptions(VideoOptions{._maxFPS = DesktopConfiguration::maxFramerate(_capturer->window())});
    }*/
}

void AsyncSharingSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        if (const auto conf = _desktopConfiguration.lock()) {
            const auto devInfo = deviceInfo();
            std::unique_ptr<DesktopCapturer> capturer;
            LOCK_WRITE_SAFE_OBJ(_capturer);
            if (!_capturer.constRef()) {
                capturer = conf->createCapturer(devInfo._guid);
            }
            else if (!conf->hasTheSameType(_capturer.constRef()->selectedSource(), devInfo._guid)) {
                stopCapturer();
                capturer = conf->createCapturer(devInfo._guid);
            }
            if (capturer) {
                const auto options = this->options();
                capturer->setPreviewMode(options.preview());
                capturer->setTargetResolutuon(options._width, options._height);
                capturer->setTargetFramerate(options._maxFPS);
                capturer->setOutputSink(this);
                _capturer = std::move(capturer);
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
}

void AsyncSharingSourceImpl::resetCapturer()
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    if (_capturer.constRef()) {
        stopCapturer();
        _capturer.take()->setOutputSink(nullptr);
    }
}

void AsyncSharingSourceImpl::onOptionsChanged(const VideoOptions& options)
{
    AsyncVideoSourceImpl::onOptionsChanged(options);
    if (active()) {
        LOCK_READ_SAFE_OBJ(_capturer);
        if (const auto& capturer = _capturer.constRef()) {
            stopCapturer();
            capturer->setPreviewMode(options.preview());
            capturer->setTargetResolutuon(options._width, options._height);
            capturer->setTargetFramerate(options._maxFPS);
            startCapturer();
        }
    }
}

void AsyncSharingSourceImpl::onDeviceInfoChanged(const MediaDeviceInfo& info)
{
    bool defaultBehavior = true;
    if (const auto conf = _desktopConfiguration.lock()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        const auto& prev = _capturer.constRef();
        if (prev && conf->hasTheSameType(info._guid, prev->selectedSource())) {
            stopCapturer();
            if (prev->selectSource(info._guid)) {
                defaultBehavior = false;
                startCapturer();
            }
        }
    }
    if (defaultBehavior) {
        AsyncVideoSourceImpl::onDeviceInfoChanged(info);
    }
}

MediaDeviceInfo AsyncSharingSourceImpl::validate(MediaDeviceInfo info) const
{
    const auto conf = _desktopConfiguration.lock();
    if (conf && conf->deviceIsValid(info)) {
        return info;
    }
    return {};
}

void AsyncSharingSourceImpl::startCapturer()
{
    if (enabled() && active() && frameWanted()) {
        if (const auto& capturer = _capturer.constRef()) {
            if (capturer->selectSource(deviceInfo()._guid)) {
                if (capturer->start()) {
                    notify(&MediaDeviceListener::onMediaStarted);
                }
                else {
                    capturer->setOutputSink(nullptr);
                    // TODO: add error details
                    notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
                }
            }
            else {
                notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
            }
        }
    }
}

void AsyncSharingSourceImpl::stopCapturer()
{
    const auto& capturer = _capturer.constRef();
    if (capturer && capturer->started()) {
        capturer->stop();
        notify(&MediaDeviceListener::onMediaStopped);
    }
}

} // namespace LiveKitCpp
