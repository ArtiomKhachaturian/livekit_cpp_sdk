#ifndef CAMERAOPTIONS_H
#define CAMERAOPTIONS_H
#include <livekit/rtc/media/CameraOptions.h>
#include "videooptions.h"

class CameraOptions : public VideoOptions
{
    QML_VALUE_TYPE(cameraOptions)
public:
    CameraOptions() = default;
    CameraOptions(const CameraOptions& src);
    CameraOptions(CameraOptions&& tmp) noexcept;
    CameraOptions(qint32 width, qint32 height, qint32 maxFps,
                  std::optional<LiveKitCpp::VideoFrameType> type,
                  bool interlaced);
    CameraOptions(const LiveKitCpp::CameraOptions& src);
    CameraOptions& operator = (const CameraOptions& src);
    CameraOptions& operator = (CameraOptions&& tmp) noexcept;
    CameraOptions& operator = (const LiveKitCpp::CameraOptions& src);
    bool operator == (const CameraOptions& other) const noexcept;
    bool operator == (const LiveKitCpp::CameraOptions& other) const noexcept;
    bool operator != (const CameraOptions& other) const noexcept;
    bool operator != (const LiveKitCpp::CameraOptions& other) const noexcept;
    operator LiveKitCpp::CameraOptions() const;
    const auto& type() const & noexcept { return _type; }
    std::optional<LiveKitCpp::VideoFrameType> type() && noexcept { return std::move(_type); }
    void setType(std::optional<LiveKitCpp::VideoFrameType> type) noexcept;
    bool interlaced() const noexcept { return _interlaced; }
    void setInterlaced(bool interlaced) noexcept { _interlaced = interlaced; }
    // overrides of VideoOptions
    bool isValid() const noexcept final;
    QString toString() const final;
    static CameraOptions defaultOptions();
    static QString toString(const std::optional<LiveKitCpp::VideoFrameType>& type);
private:
    std::optional<LiveKitCpp::VideoFrameType> _type;
    bool _interlaced = false;
};

#endif // CAMERAOPTIONS_H
