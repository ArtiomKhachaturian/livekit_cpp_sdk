#ifndef MEDIADEVICEINFO_H
#define MEDIADEVICEINFO_H
#include <livekit/rtc/media/MediaDeviceInfo.h>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class MediaDeviceInfo
{
    QML_VALUE_TYPE(mediaDeviceInfo)
public:
    MediaDeviceInfo() = default;
    MediaDeviceInfo(const MediaDeviceInfo&) = default;
    MediaDeviceInfo(MediaDeviceInfo&&) = default;
    MediaDeviceInfo(QString name, QString guid);
    MediaDeviceInfo(const LiveKitCpp::MediaDeviceInfo& src);
    MediaDeviceInfo& operator = (const MediaDeviceInfo&) = default;
    MediaDeviceInfo& operator = (MediaDeviceInfo&&) = default;
    MediaDeviceInfo& operator = (const LiveKitCpp::MediaDeviceInfo& src);
    bool operator == (const MediaDeviceInfo& other) const;
    bool operator == (const LiveKitCpp::MediaDeviceInfo& other) const;
    bool operator != (const MediaDeviceInfo& other) const;
    bool operator != (const LiveKitCpp::MediaDeviceInfo& other) const;
    explicit operator bool () const noexcept { return !isEmpty(); }
    operator const QString& () const noexcept { return name(); }
    operator LiveKitCpp::MediaDeviceInfo() const;
    const QString& name() const noexcept { return _name; }
    const QString& guid() const noexcept { return _guid; }
    void setName(const QString& name) { _name = name; }
    void setGuid(const QString& guid) { _guid = guid; }
    bool isEmpty() const noexcept { return name().isEmpty() && guid().isEmpty(); }
private:
    QString _name;
    QString _guid;
};

#endif // MEDIADEVICEINFO_H
