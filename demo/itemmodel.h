#ifndef ITEMMODEL_H
#define ITEMMODEL_H
#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

template <typename T>
class ItemModel : public QAbstractListModel
{
public:
    enum Roles {
        ItemRole = Qt::UserRole + 1,
        NextRole,
    };
public:
    template <template <typename...> class TContainer, typename U = T>
    void setItems(TContainer<U> items);
    T itemAt(qsizetype index) const;
    qsizetype indexOf(const T& item) const;
    // override of QAbstractListModel
    int columnCount(const QModelIndex&) const final { return 1; }
    int rowCount(const QModelIndex&) const final { return _items.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
protected:
    ItemModel(QObject* parent = nullptr);
    const QList<T>& items() const noexcept { return _items; }
    virtual QByteArray itemRoleName() const { return QByteArrayLiteral("item"); }
    virtual QString toString(const T& item) const { return item; }
private:
    QList<T> _items;
};

template <typename T>
inline ItemModel<T>::ItemModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

template <typename T>
template <template <typename...> class TContainer, typename U>
inline void ItemModel<T>::setItems(TContainer<U> items)
{
    beginResetModel();
    _items.assign(std::make_move_iterator(items.begin()),
                  std::make_move_iterator(items.end()));
    endResetModel();
}

template <typename T>
inline T ItemModel<T>::itemAt(qsizetype index) const
{
    if (index >= 0 && index < _items.size()) {
        return _items.at(index);
    }
    return {};
}

template <typename T>
inline qsizetype ItemModel<T>::indexOf(const T& item) const
{
    return _items.indexOf(item);
}

template <typename T>
inline QVariant ItemModel<T>::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.row() < _items.size()) {
        switch (role) {
            case Qt::DisplayRole:
                return toString(_items.at(index.row()));
            case ItemRole:
                return QVariant::fromValue(_items.at(index.row()));
            default:
                break;
            }
    }
    return {};
}

template <typename T>
inline QHash<int, QByteArray> ItemModel<T>::roleNames() const
{
    auto names = QAbstractListModel::roleNames();
    names.insert(ItemRole, itemRoleName());
    return names;
}

#endif // ITEMMODEL_H
