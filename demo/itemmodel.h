#ifndef ITEMMODEL_H
#define ITEMMODEL_H
#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <algorithm>
#include <type_traits>

template<class T>
struct ItemModelStringify
{
    static QString toString(const T& item) { return item; }
};

template<class T>
struct ItemModelStringify<T*>
{
    static QString toString(const T* item) {
        if (item) {
            return item->name();
        }
        return {};
    }
};

template <typename T>
class ItemModel : public QAbstractListModel
{
public:
    enum Roles {
        ItemRole = Qt::UserRole + 1,
        NextRole,
    };
public:
    void clear() { setItems(QList<T>{});}
    template <template <typename...> class TContainer, typename U = T>
    void setItems(TContainer<U> items);
    T itemAt(qsizetype index) const;
    template <typename U = T>
    qsizetype indexOf(const U& item) const;
    template <typename U = T>
    qsizetype append(U item); // return index of inserted item
    template <template <typename...> class TContainer, typename U = T>
    void append(TContainer<U> items);
    template <typename U = T>
    QList<T> take(const U& item);
    template <typename U = T>
    bool remove(const U& item);
    bool removeAt(qsizetype index);
    // override of QAbstractListModel
    int columnCount(const QModelIndex&) const final { return 1; }
    int rowCount(const QModelIndex&) const final { return _items.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    ~ItemModel() override { destroyItems(); }
protected:
    ItemModel(QObject* parent = nullptr);
    const QList<T>& items() const noexcept { return _items; }
    virtual QByteArray itemRoleName() const { return QByteArrayLiteral("item"); }
private:
    template <typename U = T>
    bool takeItem(qsizetype index, U& item);
    void destroyItems();
    template <typename U = T>
    static void destroyItem(U item);
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
    destroyItems();
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
template <typename U>
inline qsizetype ItemModel<T>::indexOf(const U& item) const
{
    return _items.indexOf(item);
}

template <typename T>
template <typename U>
inline qsizetype ItemModel<T>::append(U item)
{
    beginInsertRows(QModelIndex(), _items.size(), _items.size());
    _items.append(std::move(item));
    endInsertRows();
    return _items.size() - 1;
}

template <typename T>
template <template <typename...> class TContainer, typename U>
inline void ItemModel<T>::append(TContainer<U> items)
{
    if (const auto s = std::size(items)) {
        beginInsertRows(QModelIndex(), _items.size(), _items.size() + s - 1);
        for (auto it = std::make_move_iterator(items.begin());
             it != std::make_move_iterator(items.end()); ++it) {
            _items.append(std::move(*it));
        }
        endInsertRows();
    }
}


template <typename T>
inline bool ItemModel<T>::removeAt(qsizetype index)
{
    T item;
    if (takeItem(index, item)) {
        destroyItem(std::move(item));
        return true;
    }
    return false;
}

template <typename T>
template <typename U>
inline bool ItemModel<T>::remove(const U& item)
{
    size_t removed = 0U;
    while (removeAt(_items.indexOf(item))) {
        ++removed;
    }
    return removed > 0U;
}

template <typename T>
template <typename U>
inline QList<T> ItemModel<T>::take(const U& item)
{
    QList<T> orphans;
    U orphan;
    while (takeItem(_items.indexOf(item), orphan)) {
        orphans.append(std::move(orphan));
    }
    return orphans;
}

template <typename T>
inline QVariant ItemModel<T>::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.row() < _items.size()) {
        switch (role) {
            case Qt::DisplayRole:
                return ItemModelStringify<T>::toString(_items.at(index.row()));
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

template <typename T>
template <typename U>
inline bool ItemModel<T>::takeItem(qsizetype index, U& item)
{
    if (index >= 0 && index < _items.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        item = std::move(_items[index]);
        _items.remove(index);
        endRemoveRows();
        return true;
    }
    return false;
}

template <typename T>
inline void ItemModel<T>::destroyItems()
{
    if (!_items.isEmpty()) {
        std::for_each(std::make_move_iterator(_items.begin()),
                      std::make_move_iterator(_items.end()),
                      destroyItem<T>);
        _items.clear();
    }
}

template <typename T>
template <typename U>
inline void ItemModel<T>::destroyItem(U item)
{
    if constexpr (std::is_pointer_v<U>) {
        delete item;
    }
}

#endif // ITEMMODEL_H
