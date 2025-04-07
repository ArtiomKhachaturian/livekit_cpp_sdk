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
#pragma once // VideoFrameQtHelper.h
#if __has_include(<QVideoFrame>)
#include "media/VideoFrame.h"
#include <QAbstractVideoBuffer> // since QT 6.8
#include <QVideoFrame>
#include <memory>

namespace LiveKitCpp
{

// API

class QtVideoBuffer : public QAbstractVideoBuffer
{
public:
    QtVideoBuffer(std::shared_ptr<VideoFrame> frame);
    bool valid() const;
    // impl. of QAbstractVideoBuffer
    MapData map(QVideoFrame::MapMode mode) final;
    void unmap() final {}
    QVideoFrameFormat format() const final;
private:
    std::shared_ptr<VideoFrame> _frame;
    QVideoFrameFormat::PixelFormat _format = QVideoFrameFormat::Format_Invalid;
};

QVideoFrameFormat::PixelFormat qtVideoPixelFormat(VideoFrameType type);

QVideoFrame convert(const std::shared_ptr<VideoFrame>& frame);

// implementation

inline QtVideoBuffer::QtVideoBuffer(std::shared_ptr<VideoFrame> frame)
{
    if (frame) {
        _format = qtVideoPixelFormat(frame->type());
        if (QVideoFrameFormat::Format_Invalid == _format) {
            frame = frame->convertToI420();
            _format = qtVideoPixelFormat(frame->type());
        }
        _frame = std::move(frame);
    }
}

inline bool QtVideoBuffer::valid() const
{
    if (_frame && QVideoFrameFormat::Format_Invalid != _format) {
        return _frame->planesCount() > 0U;
    }
    return false;
}

inline QAbstractVideoBuffer::MapData QtVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapdata;
    if (_frame && QVideoFrame::MapMode::ReadOnly == (mode & QVideoFrame::MapMode::ReadOnly)) {
        if (const auto planes = _frame->planesCount()) {
            mapdata.planeCount = static_cast<int>(planes);
            for (size_t i = 0U; i < planes; ++i) {
                mapdata.bytesPerLine[i] = _frame->stride(i);
                const auto data = reinterpret_cast<const uchar*>(_frame->data(i));
                mapdata.data[i] = const_cast<uchar*>(data);
                mapdata.dataSize[i] = _frame->dataSize(i);
            }
        }
    }
    return mapdata;
}

inline QVideoFrameFormat QtVideoBuffer::format() const
{
    if (_frame && QVideoFrameFormat::Format_Invalid != _format) {
        return QVideoFrameFormat(QSize(_frame->width(), _frame->height()), _format);
    }
    return {};
}

inline QVideoFrame convert(const std::shared_ptr<VideoFrame>& frame)
{
    if (frame) {
        auto buffer = std::make_unique<QtVideoBuffer>(frame);
        if (buffer->valid()) {
            QVideoFrame rtcFrame(std::move(buffer));
            switch (frame->rotation()) {
                case 90:
                    rtcFrame.setRotation(QtVideo::Rotation::Clockwise90);
                    break;
                case 180:
                    rtcFrame.setRotation(QtVideo::Rotation::Clockwise180);
                    break;
                case 270:
                    rtcFrame.setRotation(QtVideo::Rotation::Clockwise270);
                    break;
                default:
                    break;
            }
            return rtcFrame;
        }
    }
    return {};
}

inline QVideoFrameFormat::PixelFormat qtVideoPixelFormat(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::RGB24:
            return QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format_RGB888);
        case VideoFrameType::BGR24:
            return QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format_BGR888);
        case VideoFrameType::BGRA32:
            return QVideoFrameFormat::Format_BGRA8888;
        case VideoFrameType::ARGB32:
            return QVideoFrameFormat::Format_ARGB8888;
        case VideoFrameType::RGBA32:
            return QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format_RGBA8888);
        case VideoFrameType::ABGR32:
            return QVideoFrameFormat::Format_ABGR8888;
        case VideoFrameType::I420:
            return QVideoFrameFormat::Format_YUV420P;
        case VideoFrameType::I422:
            return QVideoFrameFormat::Format_YUV422P;
        case VideoFrameType::I444:
            break;
        case VideoFrameType::I010:
        case VideoFrameType::I210:
        case VideoFrameType::I410:
            return QVideoFrameFormat::Format_P010;
        case VideoFrameType::NV12:
            return QVideoFrameFormat::Format_NV12;
        default:
            break;
    }
    return QVideoFrameFormat::Format_Invalid;
}

} // namespace LiveKitCpp

#endif
