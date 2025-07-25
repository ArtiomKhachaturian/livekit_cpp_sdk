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

AsyncSharingSourceImpl::AsyncSharingSourceImpl(bool previewMode,
                                               std::weak_ptr<DesktopConfiguration> desktopConfiguration,
                                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger, defaultSharingContentHint())
    , _previewMode(previewMode)
    , _desktopConfiguration(std::move(desktopConfiguration))
{
}

bool AsyncSharingSourceImpl::changeContentHint(VideoContentHint hint)
{
    switch (hint) {
        case VideoContentHint::None:
        case VideoContentHint::Detailed:
        case VideoContentHint::Text:
            return AsyncVideoSourceImpl::changeContentHint(hint);
        default:
            break;
    }
    return false;
}

void AsyncSharingSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        if (const auto conf = _desktopConfiguration.lock()) {
            const auto devInfo = deviceInfo();
            std::unique_ptr<DesktopCapturer> capturer;
            LOCK_WRITE_SAFE_OBJ(_capturer);
            if (!_capturer.constRef()) {
                capturer = conf->createCapturer(devInfo._guid, _previewMode, framesPool());
            }
            else if (!conf->hasTheSameType(_capturer.constRef()->selectedSource(), devInfo._guid)) {
                stopCapturer(_capturer.constRef());
                capturer = conf->createCapturer(devInfo._guid, _previewMode, framesPool());
            }
            if (capturer) {
                applyOptions(capturer.get(), options());
                capturer->setOutputSink(this);
                _capturer = std::move(capturer);
                if (enabled()) {
                    startCapturer(_capturer.constRef());
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
    if (auto capturer = _capturer.take()) {
        stopCapturer(capturer);
        capturer->setOutputSink(nullptr);
    }
}

void AsyncSharingSourceImpl::updateAfterContentHintChanges(VideoContentHint hint)
{
    AsyncVideoSourceImpl::updateAfterContentHintChanges(hint);
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        capturer->updateQualityToContentHint();
    }
}

std::string_view AsyncSharingSourceImpl::logCategory() const
{
    return DesktopConfiguration::logCategory();
}

void AsyncSharingSourceImpl::onOptionsChanged(const VideoOptions& options)
{
    AsyncVideoSourceImpl::onOptionsChanged(options);
    if (active()) {
        LOCK_READ_SAFE_OBJ(_capturer);
        if (const auto& capturer = _capturer.constRef()) {
            applyOptions(capturer.get(), options);
        }
    }
}

void AsyncSharingSourceImpl::onDeviceInfoChanged(const MediaDeviceInfo& info)
{
    bool defaultBehavior = true;
    if (const auto conf = _desktopConfiguration.lock()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        const auto& capturer = _capturer.constRef();
        if (capturer && conf->hasTheSameType(info._guid, capturer->selectedSource())) {
            stopCapturer(capturer);
            if (capturer->selectSource(info._guid)) {
                defaultBehavior = false;
                startCapturer(capturer);
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

void AsyncSharingSourceImpl::startCapturer(const std::unique_ptr<DesktopCapturer>& capturer) const
{
    if (capturer && enabled() && active() && frameWanted()) {
        const auto name = deviceInfo()._name;
        if (capturer->selectSource(deviceInfo()._guid)) {
            auto optionsText = toString(options());
            if (capturer->start()) {
                if (canLogVerbose()) {
                    logVerbose(capturer, "has been started with caps [" + optionsText + "]");
                }
                notify(&MediaDeviceListener::onMediaStarted);
                capturer->focusOnSelectedSource();
            }
            else {
                if (canLogError()) {
                    logError(capturer, "failed to start with caps [" + optionsText + "]");
                }
                notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
            }
        }
        else {
            if (canLogError()) {
                logError(capturer, "failed to select source '" + name + "'");
            }
            notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
        }
    }
}

void AsyncSharingSourceImpl::stopCapturer(const std::unique_ptr<DesktopCapturer>& capturer) const
{
    if (capturer && capturer->started()) {
        capturer->stop();
        logVerbose(capturer, "has been started stopped");
        notify(&MediaDeviceListener::onMediaStopped);
    }
}

void AsyncSharingSourceImpl::logError(const std::unique_ptr<DesktopCapturer>& capturer,
                                      const std::string& message) const
{
    if (capturer && canLogError()) {
        AsyncVideoSourceImpl::logError(formatLogMessage(capturer, message));
    }
}

void AsyncSharingSourceImpl::logVerbose(const std::unique_ptr<DesktopCapturer>& capturer,
                                        const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        AsyncVideoSourceImpl::logVerbose(formatLogMessage(capturer, message));
    }
}

std::string AsyncSharingSourceImpl::formatLogMessage(const std::unique_ptr<DesktopCapturer>& capturer,
                                                     const std::string& message)
{
    if (!message.empty()) {
        return capturerTitle(capturer) + " - " + message;
    }
    return message;
}

std::string AsyncSharingSourceImpl::capturerTitle(const std::unique_ptr<DesktopCapturer>& capturer)
{
    std::string title, type;
    if (capturer) {
        title = capturer->title(capturer->selectedSource()).value_or(std::string{});
        type = capturer->window() ? "window " : "screen ";
    }
    if (title.empty()) {
        title = "unknown";
    }
    return "'" + title + "' " + type + "capturer";
}

void AsyncSharingSourceImpl::applyOptions(DesktopCapturer* capturer,
                                          const VideoOptions& options)
{
    if (capturer) {
        capturer->setTargetResolution(options._width, options._height);
        capturer->setTargetFramerate(DesktopConfiguration::boundFramerate(options._maxFPS));
    }
}

} // namespace LiveKitCpp
