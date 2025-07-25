#include "mediadevicesmodel.h"

MediaDevicesModel::MediaDevicesModel(QObject* parent)
    : Base(parent)
{
}

MediaDeviceInfo MediaDevicesModel::itemAt(qsizetype index) const
{
    return Base::itemAt(index);
}

qsizetype MediaDevicesModel::indexOf(const MediaDeviceInfo& item) const
{
    return Base::indexOf(item);
}
