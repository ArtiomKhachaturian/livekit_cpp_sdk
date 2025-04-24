#ifndef FPSMETER_H
#define FPSMETER_H
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>

class FpsMeter final : public QObject
{
    Q_OBJECT
public:
    FpsMeter();
    ~FpsMeter();
    void start();
    void stop();
    void addFrame();
    bool isActive() const { return _timer.isActive(); }
signals:
    void fpsChanged(quint16 fps);
private:
    void calculate();
    // return fps
    quint16 restart();
private:
    std::atomic_bool _started = false;
    std::atomic<quint16> _framesCounter = 0U;
    QElapsedTimer _elapsedTimer;
    QTimer _timer;
};

#endif // FPSMETER_H
