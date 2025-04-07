#include "cameraoptions.h"

CameraOptions::CameraOptions(const CameraOptions& src)
    : VideoOptions(src)
    , _type(src.type())
    , _interlaced(src.interlaced())
{
}

CameraOptions::CameraOptions(CameraOptions&& tmp) noexcept
    : VideoOptions(std::move(tmp))
    , _type(tmp.type())
    , _interlaced(tmp.interlaced())
{
}

CameraOptions::CameraOptions(qint32 width, qint32 height, qint32 maxFps,
                             std::optional<LiveKitCpp::VideoFrameType> type,
                             bool interlaced)
    : VideoOptions(width, height, maxFps)
    , _type(std::move(type))
    , _interlaced(interlaced)
{
}

CameraOptions::CameraOptions(const LiveKitCpp::CameraOptions& src)
    : CameraOptions(src._width, src._height, src._maxFPS,
                    src._type, src._interlaced)
{
}

CameraOptions& CameraOptions::operator = (const CameraOptions& src)
{
    if (this != &src) {
        VideoOptions::operator=(src);
        setType(src.type());
        setInterlaced(src.interlaced());
    }
    return *this;
}

CameraOptions& CameraOptions::operator = (CameraOptions&& tmp) noexcept
{
    if (this != &tmp) {
        VideoOptions::operator=(std::move(tmp));
        setType(tmp.type());
        setInterlaced(tmp.interlaced());
    }
    return *this;
}

CameraOptions& CameraOptions::operator = (const LiveKitCpp::CameraOptions& src)
{
    VideoOptions::operator=(src);
    setType(src._type);
    setInterlaced(src._interlaced);
    return *this;
}

bool CameraOptions::operator == (const CameraOptions& other) const noexcept
{
    if (VideoOptions::operator==(other)) {
        return type() == other.type() && interlaced() == other.interlaced();
    }
    return false;
}

bool CameraOptions::operator == (const LiveKitCpp::CameraOptions& other) const noexcept
{
    if (VideoOptions::operator==(other)) {
        return type() == other._type && interlaced() == other._interlaced;
    }
    return false;
}

bool CameraOptions::operator != (const CameraOptions& other) const noexcept
{
    if (VideoOptions::operator!=(other)) {
        return type() != other.type() || interlaced() != other.interlaced();
    }
    return false;
}

bool CameraOptions::operator != (const LiveKitCpp::CameraOptions& other) const noexcept
{
    if (VideoOptions::operator!=(other)) {
        return type() != other._type || interlaced() != other._interlaced;
    }
    return false;
}

CameraOptions::operator LiveKitCpp::CameraOptions() const
{
    LiveKitCpp::CameraOptions options;
    options._width = width();
    options._height = height();
    options._maxFPS = maxFPS();
    options._type = type();
    options._interlaced = interlaced();
    return options;
}

void CameraOptions::setType(std::optional<LiveKitCpp::VideoFrameType> type) noexcept
{
    _type = std::move(type);
}

bool CameraOptions::isValid() const noexcept
{
    return VideoOptions::isValid() && type().has_value();
}

QString CameraOptions::toString() const
{
    return VideoOptions::toString() + QStringLiteral(", ") + toString(type());
}

QString CameraOptions::toString(const std::optional<LiveKitCpp::VideoFrameType>& type)
{
    if (type.has_value()) {
        switch (type.value()) {
            case LiveKitCpp::VideoFrameType::RGB24:
                return QStringLiteral("RGB24");
            case LiveKitCpp::VideoFrameType::BGR24:
                return QStringLiteral("BGR24");
            case LiveKitCpp::VideoFrameType::BGRA32:
                return QStringLiteral("BGRA32");
            case LiveKitCpp::VideoFrameType::ARGB32:
                return QStringLiteral("ARGB32");
            case LiveKitCpp::VideoFrameType::RGBA32:
                return QStringLiteral("RGBA32");
            case LiveKitCpp::VideoFrameType::ABGR32:
                return QStringLiteral("ABGR32");
            case LiveKitCpp::VideoFrameType::RGB565:
                return QStringLiteral("ABGR32");
            case LiveKitCpp::VideoFrameType::MJPEG:
                return QStringLiteral("MJPEG");
            case LiveKitCpp::VideoFrameType::UYVY:
                return QStringLiteral("UYVY");
            case LiveKitCpp::VideoFrameType::YUY2:
                return QStringLiteral("YUY2");
            case LiveKitCpp::VideoFrameType::NV12:
                return QStringLiteral("NV12");
            case LiveKitCpp::VideoFrameType::I420:
                return QStringLiteral("I420");
            case LiveKitCpp::VideoFrameType::I422:
                return QStringLiteral("I422");
            case LiveKitCpp::VideoFrameType::I444:
                return QStringLiteral("I444");
            case LiveKitCpp::VideoFrameType::I010:
                return QStringLiteral("I010");
            case LiveKitCpp::VideoFrameType::I210:
                return QStringLiteral("I210");
            case LiveKitCpp::VideoFrameType::I410:
                return QStringLiteral("I410");
            case LiveKitCpp::VideoFrameType::YV12:
                return QStringLiteral("YV12");
            case LiveKitCpp::VideoFrameType::IYUV:
                return QStringLiteral("IYUV");
            default:
                Q_ASSERT(false);
                break;
        }
    }
    return tr("n/a");
}

