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
#pragma once // VideoFormatsQt.h
#if __has_include(<QVideoFrame>)
#include "livekit/rtc/media/VideoFrameType.h"
#include <QVideoFrame>
#include <QImage>
#include <optional>

namespace LiveKitCpp
{

QVideoFrameFormat::PixelFormat toQVideoPixelFormat(VideoFrameType type);
std::optional<VideoFrameType> fromQVideoPixelFormat(QVideoFrameFormat::PixelFormat format);
std::optional<VideoFrameType> fromQImageFormat(QImage::Format format);

inline QVideoFrameFormat::PixelFormat toQVideoPixelFormat(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::BGRA32:
            return QVideoFrameFormat::Format_BGRA8888;
        case VideoFrameType::ARGB32:
            return QVideoFrameFormat::Format_ARGB8888;
        case VideoFrameType::RGBA32:
            return QVideoFrameFormat::Format_RGBA8888;
        case VideoFrameType::ABGR32:
            return QVideoFrameFormat::Format_ABGR8888;
        case VideoFrameType::MJPEG:
            return QVideoFrameFormat::Format_Jpeg;
        case VideoFrameType::UYVY:
            return QVideoFrameFormat::Format_UYVY;
        case VideoFrameType::NV12:
            return QVideoFrameFormat::Format_NV12;
        case VideoFrameType::I420:
            return QVideoFrameFormat::Format_YUV420P;
        case VideoFrameType::I422:
            return QVideoFrameFormat::Format_YUV422P;
        case VideoFrameType::YV12:
            return QVideoFrameFormat::Format_YV12;
        default:
            break;
    }
    return QVideoFrameFormat::Format_Invalid;
}

inline std::optional<VideoFrameType> fromQVideoPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
        // The frame is stored using a 32-bit BGRA format (0xBBGGRRAA)
        case QVideoFrameFormat::Format_BGRA8888:
            return VideoFrameType::BGRA32;
        // The frame is stored using a ARGB format with 8 bits per component
        case QVideoFrameFormat::Format_ARGB8888:
            return VideoFrameType::ARGB32;
        // The frame is stored in memory as the bytes R, G, B, A/X, with R at the lowest address and A/X at the highest address
        case QVideoFrameFormat::Format_RGBA8888:
            return VideoFrameType::RGBA32;
        // The frame is stored using a 32-bit ABGR format (0xAABBGGRR)
        case QVideoFrameFormat::Format_ABGR8888:
            return  VideoFrameType::ABGR32;
        case QVideoFrameFormat::Format_Jpeg:
            return VideoFrameType::MJPEG;
        case QVideoFrameFormat::Format_UYVY:
            return VideoFrameType::UYVY;
        case QVideoFrameFormat::Format_NV12:
            return VideoFrameType::NV12;
        case QVideoFrameFormat::Format_YUV420P:
            return VideoFrameType::I420;
        case QVideoFrameFormat::Format_YUV422P:
            return VideoFrameType::I422;
        case QVideoFrameFormat::Format_YV12:
            return VideoFrameType::YV12;
        default:
            break;
    }
    return std::nullopt;
}

inline std::optional<VideoFrameType> fromQImageFormat(QImage::Format format)
{
    switch (format) {
        case QImage::Format_RGB888:
            return VideoFrameType::RGB24;
        case QImage::Format_BGR888:
            return VideoFrameType::BGR24;
        // The image is stored using a 32-bit ARGB format (0xAARRGGBB)
        case QImage::Format_ARGB32:
            return VideoFrameType::ARGB32;
        case QImage::Format_ARGB8565_Premultiplied:
            return VideoFrameType::RGB565;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
#endif
