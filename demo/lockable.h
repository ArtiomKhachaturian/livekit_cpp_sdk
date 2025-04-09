#ifndef LOCKABLE_H
#define LOCKABLE_H
// lockable.h
#include <QReadWriteLock>

template <typename T>
struct Lockable
{
    T _val;
    mutable QReadWriteLock _lock;
    Lockable();
    explicit Lockable(T val);
    template <class... Args>
    Lockable(Args&&... args);
};

template <typename T>
inline Lockable<T>::Lockable()
    : _lock(QReadWriteLock::RecursionMode::Recursive)
{
}

template <typename T>
inline Lockable<T>::Lockable(T val)
    : _val(std::move(val))
    , _lock(QReadWriteLock::RecursionMode::Recursive)
{
}

template <typename T>
template <class... Args>
inline Lockable<T>::Lockable(Args&&... args)
    : _val(std::forward<Args>(args)...)
    , _lock(QReadWriteLock::RecursionMode::Recursive)
{
}

#endif // LOCKABLE_H
