#ifndef MEDIADEVICEINFO_H
#define MEDIADEVICEINFO_H
#include <media/MediaDeviceInfo.h>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class MediaDeviceInfo
{
    QML_VALUE_TYPE(MediaDeviceInfo)
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

Q_DECLARE_METATYPE(MediaDeviceInfo)

#endif // MEDIADEVICEINFO_H
