#include "mediadevicesmodel.h"

MediaDevicesModel::MediaDevicesModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

MediaDevicesModel::MediaDevicesModel(const std::vector<LiveKitCpp::MediaDeviceInfo>& infos,
                                     QObject* parent)
    : MediaDevicesModel(parent)
{
    setInfo(infos);
}

void MediaDevicesModel::setInfo(const std::vector<LiveKitCpp::MediaDeviceInfo>& infos)
{
    if (_infos.empty() && infos.empty()) {
        return;
    }
    if (_infos.empty()) {
        beginResetModel();
        _infos.assign(infos.begin(), infos.end());
        endResetModel();
    }
    else {
        beginResetModel();
        _infos.assign(infos.begin(), infos.end());
        endResetModel();
    }
}

MediaDeviceInfo MediaDevicesModel::infoAt(qsizetype index) const
{
    if (index >= 0 && index < _infos.size()) {
        return _infos.at(index);
    }
    return {};
}

qsizetype MediaDevicesModel::indexOf(const MediaDeviceInfo& info) const
{
    if (!info.isEmpty()) {
        return _infos.indexOf(info);
    }
    return -1;
}

QVariant MediaDevicesModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.row() < _infos.size()) {
        switch (role) {
            case Qt::DisplayRole:
                return _infos.at(index.row()).name();
            case MediaDeviceInfoRole:
                return QVariant::fromValue(_infos.at(index.row()));
            default:
                break;
        }
    }
    return {};
}

QHash<int, QByteArray> MediaDevicesModel::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(MediaDeviceInfoRole, QByteArrayLiteral("mediaDeviceInfo"));
    return names;
}
