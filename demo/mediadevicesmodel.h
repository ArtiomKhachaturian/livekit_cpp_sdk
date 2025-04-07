#ifndef MEDIADEVICESMODEL_H
#define MEDIADEVICESMODEL_H
#include "mediadeviceinfo.h"
#include <media/MediaDeviceInfo.h>
#include <QAbstractListModel>
#include <QQmlEngine>
#include <vector>

class MediaDevicesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MediaDevicesModel)
public:
    enum Roles { MediaDeviceInfoRole = Qt::UserRole + 1 };
public:
    MediaDevicesModel(QObject* parent = nullptr);
    MediaDevicesModel(const std::vector<LiveKitCpp::MediaDeviceInfo>& infos, QObject* parent = nullptr);
    void setInfo(const std::vector<LiveKitCpp::MediaDeviceInfo>& infos);
    Q_INVOKABLE MediaDeviceInfo infoAt(qsizetype index) const;
    Q_INVOKABLE qsizetype indexOf(const MediaDeviceInfo& info) const;
    // override of QAbstractListModel
    int columnCount(const QModelIndex&) const final { return 1; }
    int rowCount(const QModelIndex&) const final { return _infos.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const final;
    QHash<int, QByteArray> roleNames() const final;
private:
    QList<MediaDeviceInfo> _infos;
};

Q_DECLARE_METATYPE(MediaDevicesModel*)

#endif // MEDIADEVICESMODEL_H
