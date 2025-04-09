#ifndef SAFEOBJ_H // safeobj.h
#define SAFEOBJ_H
#include "lockable.h"

template <typename T>
class SafeObj
{
public:
    SafeObj() = default;
    explicit SafeObj(T val);
    template <class... Args>
    SafeObj(Args&&... args);
    ~SafeObj() { set({}); }
    template <typename U = T>
    SafeObj& operator=(U src) noexcept;
    operator T() const { return get(); }
    template <typename U = T>
    void set(U src);
    template <typename U = T>
    bool exchange(U src);
    T get() const;
    T take();
private:
    Lockable<T> _data;
};

template <typename T>
inline SafeObj<T>::SafeObj(T val)
    : _data(std::move(val))
{
}

template <typename T>
template <class... Args>
inline SafeObj<T>::SafeObj(Args&&... args)
    : _data(std::forward<Args>(args)...)
{
}

template <typename T>
template <typename U >
inline SafeObj<T>& SafeObj<T>::operator=(U src) noexcept
{
    set(std::move(src));
    return *this;
}

template <typename T>
template <typename U>
inline void SafeObj<T>::set(U src)
{
    const QWriteLocker locker(&_data._lock);
    _data._val = std::move(src);
}

template <typename T>
template <typename U>
inline bool SafeObj<T>::exchange(U src)
{
    const QWriteLocker locker(&_data._lock);
    if (src != _data._val) {
        _data._val = std::move(src);
        return true;
    }
    return false;
}

template <typename T>
inline T SafeObj<T>::get() const
{
    const QReadLocker locker(&_data._lock);
    return _data._val;
}

template <typename T>
inline T SafeObj<T>::take()
{
    const QWriteLocker locker(&_data._lock);
    return std::move(_data._val);
}

#endif // SAFEOBJ_H
