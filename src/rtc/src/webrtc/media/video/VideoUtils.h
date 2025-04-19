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
#pragma once // VideoUtils.h
#include "livekit/rtc/media/VideoContentHint.h"
#include <api/media_stream_interface.h>
#include <api/video/video_frame.h>
#include <optional>

namespace LiveKitCpp
{

// new frames factory
// webrtc::VideoFrameBuffer::Type::kNative & webrtc::VideoFrameBuffer::Type::kI420A types
// are ignored - output frame will be with webrtc::VideoFrameBuffer::Type::kI420 buffer
std::optional<webrtc::VideoFrame> createVideoFrame(int width, int height,
                                                   webrtc::VideoFrameBuffer::Type type = webrtc::VideoFrameBuffer::Type::kI420,
                                                   int64_t timeStampMicro = 0LL,
                                                   uint16_t id = 0U,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace = {});
std::optional<webrtc::VideoFrame> createVideoFrame(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                                   int64_t timeStampMicro = 0LL,
                                                   uint16_t id = 0U,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace = {});
std::optional<webrtc::VideoFrame> createBlackVideoFrame(int width, int height,
                                                        int64_t timeStampMicro = 0LL,
                                                        uint16_t id = 0U,
                                                        const std::optional<webrtc::ColorSpace>& colorSpace = {});
webrtc::VideoTrackInterface::ContentHint map(VideoContentHint hint);
VideoContentHint map(webrtc::VideoTrackInterface::ContentHint hint);
	
} // namespace LiveKitCpp
