#ifndef LOGGER_H
#define LOGGER_H
#include <logger/Logger.h>
#include <QMutex>

class Logger : public Bricks::Logger
{
public:
    Logger();
    // impl. of Bricks::Logger
    bool canLog(Bricks::LoggingSeverity severity) const final;
    void log(Bricks::LoggingSeverity severity,
             std::string_view message,
             std::string_view category) final;
private:
    QMutex _mutex;
};

#endif // LOGGER_H
