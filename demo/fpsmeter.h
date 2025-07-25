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
    quint16 fps() const { return _fps; }
signals:
    void fpsChanged();
private:
    void calculate();
    void setFps(quint16 fps);
    static constexpr float num();
private:
    quint16 _fps = 0U;
    std::atomic_bool _started = false;
    std::atomic<quint16> _framesCounter = 0U;
    QElapsedTimer _elapsedTimer;
    QTimer _timer;
};

#endif // FPSMETER_H
