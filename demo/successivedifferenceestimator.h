#ifndef SUCCESSIVEDIFFERENCEESTIMATOR_H
#define SUCCESSIVEDIFFERENCEESTIMATOR_H
#include <QtTypes>

template <typename T>
class SuccessiveDifferenceEstimator
{
public:
    SuccessiveDifferenceEstimator() { reset(); }
    explicit SuccessiveDifferenceEstimator(const T& initialValue);
    void reset();
    uint64_t counter() const { return _counter; }
    bool hasEstimation() const { return counter() > 1ULL; }
    T diff() const;
    T current() const { return _current; }
    T previous() const { return _previous; }
    // return true if value changed
    bool setCurrent(T current, bool force = false);
    void add() { setCurrent(current() + one()); }
private:
    static constexpr T zero() { return static_cast<T>(0); }
    static constexpr T one() { return static_cast<T>(1); }
private:
    T _previous;
    T _current;
    quint64 _counter = 1ULL;
};

template <typename T>
SuccessiveDifferenceEstimator<T>::SuccessiveDifferenceEstimator(const T& initialValue)
    : _previous(initialValue)
    , _current(initialValue)
{
}

template <typename T>
void SuccessiveDifferenceEstimator<T>::reset()
{
    _previous = _current = zero();
    _counter = 0ULL;
}

template <typename T>
T SuccessiveDifferenceEstimator<T>::diff() const
{
    if (hasEstimation() && current() > previous()) {
        return current() - previous();
    }
    return zero();
}

template <typename T>
bool SuccessiveDifferenceEstimator<T>::setCurrent(T current, bool force)
{
    ++_counter;
    if (force || current != _current) {
        _previous = _current;
        _current = current;
        return true;
    }
    return false;
}

#endif // SUCCESSIVEDIFFERENCEESTIMATOR_H
