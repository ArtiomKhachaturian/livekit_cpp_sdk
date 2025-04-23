#include "fpsmeter.h"

void FpsMeter::start(QObject* target)
{
    _framesCounter = 0U;
    _elapsedTimer.start();
    _timer.start(1000, target);
}

void FpsMeter::stop()
{
    _elapsedTimer.invalidate();
    _framesCounter = 0U;
    _timer.stop();
}

void FpsMeter::addFrame()
{
    _framesCounter.fetch_add(1U);
}

quint16 FpsMeter::restart()
{
    const auto frames = _framesCounter.exchange(0U);
    const auto elapsed = float(_elapsedTimer.restart()); // in ms
    return (1000U * frames) / elapsed;
}
