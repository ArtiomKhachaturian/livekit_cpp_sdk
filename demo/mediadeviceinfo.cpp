#include "mediadeviceinfo.h"


MediaDeviceInfo::MediaDeviceInfo(QString name, QString guid)
    : _name(std::move(name))
    , _guid(std::move(guid))
{
}

MediaDeviceInfo::MediaDeviceInfo(const LiveKitCpp::MediaDeviceInfo& src)
    : MediaDeviceInfo(QString::fromStdString(src._name), QString::fromStdString(src._guid))
{
}

MediaDeviceInfo& MediaDeviceInfo::operator = (const LiveKitCpp::MediaDeviceInfo& src)
{
    _name = QString::fromStdString(src._name);
    _guid = QString::fromStdString(src._guid);
    return *this;
}

bool MediaDeviceInfo::operator == (const MediaDeviceInfo& other) const
{
    return this == &other || (name() == other.name() && guid() == other.guid());
}

bool MediaDeviceInfo::operator == (const LiveKitCpp::MediaDeviceInfo& other) const
{
    return name() == QString::fromStdString(other._name) && guid() == QString::fromStdString(other._guid);
}

bool MediaDeviceInfo::operator != (const MediaDeviceInfo& other) const
{
    if (this != &other) {
        return name() != other.name() || guid() != other.guid();
    }
    return false;
}

bool MediaDeviceInfo::operator != (const LiveKitCpp::MediaDeviceInfo& other) const
{
    return name() !=  QString::fromStdString(other._name) || guid() != QString::fromStdString(other._guid);
}

MediaDeviceInfo::operator LiveKitCpp::MediaDeviceInfo() const
{
    return {name().toStdString(), guid().toStdString()};
}
