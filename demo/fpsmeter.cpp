#include "fpsmeter.h"
#include <QTimerEvent>
#include <QThread>
#include <cmath>


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
    if (QThread::currentThread() == thread()) {
        if (!_started.exchange(true)) {
            _framesCounter = _fps = 0U;
            _elapsedTimer.start();
            _timer.start(1000);
        }
    }
    else {
        QMetaObject::invokeMethod(this, &FpsMeter::start);
    }
}

void FpsMeter::stop()
{
    if (QThread::currentThread() == thread()) {
        if (_started.exchange(false)) {
            _timer.stop();
            calculate();
            _elapsedTimer.invalidate();
            setFps(0U);
        }
    }
    else {
        QMetaObject::invokeMethod(this, &FpsMeter::stop);
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
    const auto frames = _framesCounter.exchange(0U);
    const auto fps = std::roundf((1000U * frames) / float(_elapsedTimer.restart()));
    setFps(static_cast<quint16>(fps));

}

void FpsMeter::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}
