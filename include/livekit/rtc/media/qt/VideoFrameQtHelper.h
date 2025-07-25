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
#include "livekit/rtc/media/VideoFrame.h"
#include "livekit/rtc/media/qt/VideoFormatsQt.h"
#if __has_include(<QAbstractVideoBuffer>)
#include <QAbstractVideoBuffer> // since QT 6.8
#define HAS_ABSTRACT_VIDEO_BUFFER
#elif __has_include(<private/qabstractvideobuffer_p.h>)
#include <private/qabstractvideobuffer_p.h>
#define HAS_ABSTRACT_VIDEO_BUFFER
#define DEPRECATED_ABSTRACT_VIDEO_BUFFER
#endif
#include <QImage>
#include <memory>

namespace LiveKitCpp
{

// API
#ifdef HAS_ABSTRACT_VIDEO_BUFFER
class QtVideoBuffer : public QAbstractVideoBuffer
{
public:
    QtVideoBuffer(std::shared_ptr<VideoFrame> frame);
    bool valid() const;
    // impl. of QAbstractVideoBuffer
    MapData map(QVideoFrame::MapMode mode) final;
    void unmap() final {}
#ifdef DEPRECATED_ABSTRACT_VIDEO_BUFFER
    QVideoFrameFormat format() const;
#else
    QVideoFrameFormat format() const final;
#endif
private:
    std::shared_ptr<VideoFrame> _frame;
    QVideoFrameFormat::PixelFormat _format = QVideoFrameFormat::Format_Invalid;
};

QVideoFrame convert(std::shared_ptr<VideoFrame> frame);

#endif

template <class TQtData>
class VideoFrameQtData : public VideoFrame
{
public:
    // impl. of VideoFrame
    int width() const final { return _data.width(); }
    int height() const final { return _data.height(); }
protected:
    VideoFrameQtData(TQtData data, VideoFrameType type, int rotation, int64_t timestampUs = 0LL);
protected:
    const TQtData _data;
};

class QImageVideoFrame : public VideoFrameQtData<QImage>
{
public:
    static std::shared_ptr<VideoFrame> create(QImage&& data, int rotation = 0,
                                              int64_t timestampUs = 0LL);
    // impl. of VideoFrame
    size_t planesCount() const final { return 1U; }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    int dataSize(size_t planeIndex) const final;
private:
    QImageVideoFrame(QImage data, VideoFrameType type, int rotation, int64_t timestampUs);
};

// implementation
#ifdef HAS_ABSTRACT_VIDEO_BUFFER
inline QtVideoBuffer::QtVideoBuffer(std::shared_ptr<VideoFrame> frame)
#ifdef DEPRECATED_ABSTRACT_VIDEO_BUFFER
    : QAbstractVideoBuffer(QVideoFrame::HandleType::NoHandle)
#endif
{
    if (frame) {
        auto format = toQVideoPixelFormat(frame->type());
        if (QVideoFrameFormat::Format_Invalid == format) {
            frame = frame->convertToI420();
            if (frame) {
                format = toQVideoPixelFormat(frame->type());
            }
        }
        if (QVideoFrameFormat::Format_Invalid != format) {
            _format = format;
            _frame = std::move(frame);
        }
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
#ifdef DEPRECATED_ABSTRACT_VIDEO_BUFFER
            mapdata.nPlanes = static_cast<int>(planes);
#else
            mapdata.planeCount = static_cast<int>(planes);
#endif
            for (size_t i = 0U; i < planes; ++i) {
                mapdata.bytesPerLine[i] = _frame->stride(i);
                const auto data = reinterpret_cast<const uchar*>(_frame->data(i));
                mapdata.data[i] = const_cast<uchar*>(data);
#ifdef DEPRECATED_ABSTRACT_VIDEO_BUFFER
                mapdata.size[i] = _frame->dataSize(i);
#else
                mapdata.dataSize[i] = _frame->dataSize(i);
#endif
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

inline QVideoFrame convert(std::shared_ptr<VideoFrame> frame)
{
    if (frame) {
        const auto rotation = frame->rotation();
        const auto timestamp = frame->timestampUs();
        auto buffer = std::make_unique<QtVideoBuffer>(std::move(frame));
        if (buffer->valid()) {
#ifdef DEPRECATED_ABSTRACT_VIDEO_BUFFER
            const auto format = buffer->format();
            QVideoFrame output(buffer.release(), format);
#else
            QVideoFrame output(std::move(buffer));
#endif
            switch (rotation) {
            case 90:
                output.setRotation(QtVideo::Rotation::Clockwise90);
                break;
            case 180:
                output.setRotation(QtVideo::Rotation::Clockwise180);
                break;
            case 270:
                output.setRotation(QtVideo::Rotation::Clockwise270);
                break;
            default:
                break;
            }
            output.setStartTime(timestamp);
            return output;
        }
    }
    return {};
}
#endif

template <class TQtData>
inline VideoFrameQtData<TQtData>::VideoFrameQtData(TQtData data,
                                                   VideoFrameType type,
                                                   int rotation,
                                                   int64_t timestampUs)
    : VideoFrame(type, rotation, timestampUs)
    , _data(std::move(data))
{
}

inline QImageVideoFrame::QImageVideoFrame(QImage data,
                                          VideoFrameType type,
                                          int rotation,
                                          int64_t timestampUs)
    : VideoFrameQtData<QImage>(std::move(data), type, rotation, timestampUs)
{
}

inline int QImageVideoFrame::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return _data.bytesPerLine();
    }
    return 0;
}

inline const std::byte* QImageVideoFrame::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(_data.bits());
    }
    return nullptr;
}

inline int QImageVideoFrame::dataSize(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return _data.sizeInBytes();
    }
    return 0;
}

inline std::shared_ptr<VideoFrame> QImageVideoFrame::create(QImage&& data,
                                                            int rotation,
                                                            int64_t timestampUs)
{
    std::shared_ptr<VideoFrame> frame;
    if (!data.isNull()) {
        auto type = fromQImageFormat(data.format());
        if (!type) {
            data = data.convertToFormat(QImage::Format_RGB888);
            type = fromQImageFormat(data.format());
        }
        if (type) {
            frame.reset(new QImageVideoFrame(std::move(data),
                                             type.value(),
                                             rotation, timestampUs));
        }
    }
    return frame;
}

} // namespace LiveKitCpp
#endif
