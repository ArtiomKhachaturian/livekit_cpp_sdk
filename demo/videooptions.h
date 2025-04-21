#ifndef VIDEOOPTIONS_H
#define VIDEOOPTIONS_H
#include <livekit/rtc/media/VideoOptions.h>
#include <QCoreApplication>
#include <QSize>
#include <QtQml/qqmlregistration.h>

class VideoOptions
{
    Q_GADGET
public:
    Q_PROPERTY(qint32 width READ width WRITE setWidth)
    Q_PROPERTY(qint32 height READ height WRITE setHeight)
    Q_PROPERTY(qint32 maxFPS READ maxFPS WRITE setMaxFPS)
    Q_PROPERTY(bool interlaced READ interlaced WRITE setInterlaced)
    Q_PROPERTY(bool preview READ preview WRITE setPreview)
    Q_PROPERTY(QSize resolution READ resolution WRITE setResolution)
public:
    VideoOptions() = default;
    VideoOptions(const VideoOptions&) = default;
    VideoOptions(VideoOptions&&) noexcept = default;
    VideoOptions(qint32 width, qint32 height, qint32 maxFps);
    VideoOptions(const LiveKitCpp::VideoOptions& src);
    VideoOptions& operator = (const VideoOptions&) = default;
    VideoOptions& operator = (VideoOptions&&) noexcept = default;
    VideoOptions& operator = (const LiveKitCpp::VideoOptions& src);
    bool operator == (const VideoOptions& other) const noexcept;
    bool operator == (const LiveKitCpp::VideoOptions& other) const noexcept;
    bool operator != (const VideoOptions& other) const noexcept;
    bool operator != (const LiveKitCpp::VideoOptions& other) const noexcept;
    operator LiveKitCpp::VideoOptions() const { return _impl; }
    operator QString() const { return toString(); }
    explicit operator bool () const noexcept { return isValid(); }
    Q_INVOKABLE qint32 width() const noexcept { return _impl._width; }
    Q_INVOKABLE qint32 height() const noexcept { return _impl._height; }
    Q_INVOKABLE qint32 maxFPS() const noexcept { return _impl._maxFPS; }
    Q_INVOKABLE bool interlaced() const noexcept { return _impl.interlaced(); }
    Q_INVOKABLE bool preview() const noexcept { return _impl.preview(); }
    Q_INVOKABLE void setInterlaced(bool interlaced) { _impl.setInterlaced(interlaced); }
    Q_INVOKABLE void setPreview(bool preview) { _impl.setPreview(preview); }
    Q_INVOKABLE void setWidth(qint32 width) noexcept { _impl._width = width; }
    Q_INVOKABLE void setHeight(qint32 height) noexcept { _impl._height = height; }
    Q_INVOKABLE void setMaxFPS(qint32 maxFps) noexcept { _impl._maxFPS = maxFps; }
    Q_INVOKABLE void setResolution(qint32 width, qint32 height) noexcept;
    Q_INVOKABLE void setResolution(const QSize& resolution) noexcept;
    Q_INVOKABLE QSize resolution() const noexcept { return {width(), height()}; }
    QString toString() const;
    Q_INVOKABLE bool isValid() const noexcept { return !_impl.null(); }
private:
    LiveKitCpp::VideoOptions _impl;
};

Q_DECLARE_METATYPE(VideoOptions)

#endif // VIDEOOPTIONS_H
