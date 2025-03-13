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
#include "CameraVideoSource.h"
#include "CameraManager.h"
#include "CameraCapturer.h"
#include "Utils.h"
#include <api/video/i420_buffer.h>

namespace {

inline rtc::scoped_refptr<const webrtc::I420BufferInterface> toI420(const webrtc::VideoFrame& frame) {
    if (const auto buffer = frame.video_frame_buffer()) {
        if (webrtc::VideoFrameBuffer::Type::kI420 == buffer->type()) {
            return rtc::scoped_refptr<const webrtc::I420BufferInterface>(buffer->GetI420());
        }
        return buffer->ToI420();
    }
    return {};
}

}

namespace LiveKitCpp
{

CameraVideoSource::CameraVideoSource()
    : _adapter(2)
    , _capability(CameraManager::defaultCapability())
    , _state(webrtc::MediaSourceInterface::kEnded)
{
}

CameraVideoSource::~CameraVideoSource()
{
    setCapturer({});
}

void CameraVideoSource::setCapturer(rtc::scoped_refptr<CameraCapturer> capturer)
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    if (capturer != _capturer.constRef()) {
        if (auto prev = _capturer.take()) {
            prev->StopCapture();
            prev->DeRegisterCaptureDataCallback();
            prev->setObserver(nullptr);
        }
        _capturer = std::move(capturer);
        if (const auto& capturer = _capturer.constRef()) {
            capturer->RegisterCaptureDataCallback(this);
            capturer->setObserver(this);
            if (_broadcaster.frame_wanted()) {
                capturer->StartCapture(_capability());
            }
        }
    }
}

void CameraVideoSource::enableBlackFrames(bool enable)
{
    auto wants = _broadcaster.wants();
    wants.black_frames = enable;
    _adapter.OnSinkWants(wants);
}

bool CameraVideoSource::GetStats(Stats* stats)
{
    if (stats && _hasLastResolution) {
        const auto lastResolution = _lastResolution.load();
        stats->input_width = extractHiWord(lastResolution);
        stats->input_height = extractLoWord(lastResolution);
        return true;
    }
    return false;
}

void CameraVideoSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    _broadcaster.ProcessConstraints(c);
}

void CameraVideoSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                        const rtc::VideoSinkWants& wants)
{
    if (sink) {
        const auto frameWanted = _broadcaster.frame_wanted();
        _broadcaster.AddOrUpdateSink(sink, wants);
        _adapter.OnSinkWants(_broadcaster.wants());
        if (frameWanted != _broadcaster.frame_wanted()) {
            LOCK_READ_SAFE_OBJ(_capturer);
            if (const auto& capturer = _capturer.constRef()) {
                capturer->StartCapture(_capability());
            }
        }
    }
}

void CameraVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        _broadcaster.RemoveSink(sink);
        _adapter.OnSinkWants(_broadcaster.wants());
        if (!_broadcaster.frame_wanted()) {
            LOCK_READ_SAFE_OBJ(_capturer);
            if (const auto& capturer = _capturer.constRef()) {
                capturer->StopCapture();
            }
        }
    }
}

void CameraVideoSource::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.add(observer);
}

void CameraVideoSource::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.remove(observer);
}

void CameraVideoSource::onStateChanged(CameraState state)
{
    switch (state) {
        case CameraState::Stopped:
            changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
            break;
        case CameraState::Starting:
            changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
            break;
        case CameraState::Started:
            changeState(webrtc::MediaSourceInterface::SourceState::kLive);
            break;
        default:
            break;
    }
}

void CameraVideoSource::OnFrame(const webrtc::VideoFrame& frame)
{
    if (const auto buffer = frame.video_frame_buffer()) {
        int adaptedWidth, adaptedHeight, cropWidth, cropHeight, cropX, cropY;
        if (adaptFrame(frame.width(), frame.height(), frame.timestamp_us(),
                       adaptedWidth, adaptedHeight,
                       cropWidth, cropHeight, cropX, cropY)) {
            if (adaptedWidth == frame.width() && adaptedHeight == frame.height()) {
                // No adaption - optimized path.
                broadcast(frame);
            }
            else {
                if (const auto srcI420 = toI420(frame)) {
                    auto dstI420 = webrtc::I420Buffer::Create(adaptedWidth, adaptedHeight);
                    dstI420->CropAndScaleFrom(*srcI420, cropX, cropY, cropWidth, cropHeight);
                    webrtc::VideoFrame adapted(frame);
                    adapted.set_video_frame_buffer(dstI420);
                    broadcast(adapted);
                }
                else {
                    // TODO: report error
                }
            }
        }
    }
}

void CameraVideoSource::OnDiscardedFrame()
{
    _broadcaster.OnDiscardedFrame();
}

void CameraVideoSource::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& /*constraints*/)
{
    /*LOCK_READ_SAFE_OBJ(_sinks);
    for (const auto sink : _sinks.constRef()) {
        sink->OnConstraintsChanged(constraints);
    }*/
}

bool CameraVideoSource::applyRotation() const
{
    return _broadcaster.wants().rotation_applied;
}

void CameraVideoSource::broadcast(const webrtc::VideoFrame& frame)
{
    if (applyRotation() && frame.rotation() != webrtc::kVideoRotation_0) {
        if (const auto srcI420 = toI420(frame)) {
            webrtc::VideoFrame rotatedFrame(frame);
            rotatedFrame.set_video_frame_buffer(webrtc::I420Buffer::Rotate(*srcI420, frame.rotation()));
            rotatedFrame.set_rotation(webrtc::kVideoRotation_0);
            _broadcaster.OnFrame(rotatedFrame);
        }
        else {
            // TODO: report error
        }
        
    } else {
        _broadcaster.OnFrame(frame);
    }
}

void CameraVideoSource::changeState(webrtc::MediaSourceInterface::SourceState state)
{
    if (state != _state.exchange(state)) {
        switch (state) {
            case webrtc::MediaSourceInterface::SourceState::kEnded:
                if (_hasLastResolution.exchange(false)) {
                    _lastResolution = 0ULL;
                }
                break;
            default:
                break;
        }
        _observers.invoke(&webrtc::ObserverInterface::OnChanged);
        return true;
    }
    return false;
}

bool CameraVideoSource::adaptFrame(int width, int height, int64_t timeUs,
                                   int& outWidth, int& outHeight,
                                   int& cropWidth, int& cropHeight,
                                   int& cropX, int& cropY)
{
    _lastResolution = clueToUint64(width, height);
    _hasLastResolution = true;

    if (!_broadcaster.frame_wanted()) {
        return false;
    }

    if (!_adapter.AdaptFrameResolution(width, height,
                                       timeUs * rtc::kNumNanosecsPerMicrosec,
                                       &cropWidth, &cropHeight,
                                       &outWidth, &outHeight)) {
        _broadcaster.OnDiscardedFrame();
        // VideoAdapter dropped the frame.
        return false;
    }

    cropX = (width - cropWidth) / 2;
    cropY = (height - cropHeight) / 2;
    return true;
}


} // namespace LiveKitCpp
