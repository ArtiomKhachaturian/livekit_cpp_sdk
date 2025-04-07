#ifndef MEDIADEVICESMODEL_H
#define MEDIADEVICESMODEL_H
#include "itemmodel.h"
#include "mediadeviceinfo.h"

class MediaDevicesModel : public ItemModel<MediaDeviceInfo>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MediaDevicesModel)
    using Base = ItemModel<MediaDeviceInfo>;
public:
    MediaDevicesModel(QObject* parent = nullptr);
    Q_INVOKABLE MediaDeviceInfo itemAt(qsizetype index) const;
    Q_INVOKABLE qsizetype indexOf(const MediaDeviceInfo& item) const;
};

#endif // MEDIADEVICESMODEL_H
