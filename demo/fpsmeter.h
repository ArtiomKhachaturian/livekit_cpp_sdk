#ifndef FPSMETER_H
#define FPSMETER_H
#include <QBasicTimer>
#include <QElapsedTimer>
#include <atomic>

class FpsMeter
{
public:
    FpsMeter() = default;
    void start(QObject* target);
    void stop();
    // return fps
    quint16 restart();
    void addFrame();
    int timerId() const { return _timer.timerId(); }
    bool isActive() const { return _timer.isActive(); }
private:
    std::atomic<quint16> _framesCounter = 0U;
    QElapsedTimer _elapsedTimer;
    QBasicTimer _timer;
};

#endif // FPSMETER_H
