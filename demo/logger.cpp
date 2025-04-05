#include "logger.h"
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QScopedPointer>

Logger::Logger()
{
}

bool Logger::canLog(Bricks::LoggingSeverity severity) const
{
    switch (severity) {
        case Bricks::LoggingSeverity::Verbose:
            return QLoggingCategory::defaultCategory()->isDebugEnabled();
        case Bricks::LoggingSeverity::Info:
            return QLoggingCategory::defaultCategory()->isInfoEnabled();
        case Bricks::LoggingSeverity::Warning:
            return QLoggingCategory::defaultCategory()->isWarningEnabled();
        case Bricks::LoggingSeverity::Error:
            return QLoggingCategory::defaultCategory()->isCriticalEnabled();
        default:
            break;
    }
    return true;
}

void Logger::log(Bricks::LoggingSeverity severity,
                 std::string_view message,
                 std::string_view category)
{
    if (!message.empty()) {
        QScopedPointer<QLoggingCategory> customCat;
        if (!category.empty()) {
            customCat.reset(new QLoggingCategory(category.data()));
        }
        const auto cat = customCat ? customCat.data() : QLoggingCategory::defaultCategory();
        const QMutexLocker locker(&_mutex);
        switch (severity) {
            case Bricks::LoggingSeverity::Verbose:
                qCDebug(*cat).noquote() << message;
                break;
            case Bricks::LoggingSeverity::Info:
                qCInfo(*cat).noquote() << message;
                break;
            case Bricks::LoggingSeverity::Warning:
                qCWarning(*cat).noquote() << message;
                break;
            case Bricks::LoggingSeverity::Error:
                qCCritical(*cat).noquote() << message;
                break;
        }
    }
}
