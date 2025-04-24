#include "fpsmeter.h"
#include <QTimerEvent>


FpsMeter::FpsMeter()
{
    QObject::connect(&_timer, &QTimer::timeout, this, &FpsMeter::calculate);
}

FpsMeter::~FpsMeter()
{
    stop();
    _timer.disconnect(this);
}

void FpsMeter::start()
{
    if (!_started.exchange(true)) {
        _framesCounter = 0U;
        _elapsedTimer.start();
        _timer.start(1000);
    }
}

void FpsMeter::stop()
{
    if (_started.exchange(false)) {
        _timer.stop();
        calculate();
        _elapsedTimer.invalidate();
    }
}

void FpsMeter::addFrame()
{
    if (_started) {
        _framesCounter.fetch_add(1U);
    }
}

void FpsMeter::calculate()
{
    emit fpsChanged(restart());
}

quint16 FpsMeter::restart()
{
    const auto frames = _framesCounter.exchange(0U);
    return (1000U * frames) / float(_elapsedTimer.restart());
}
