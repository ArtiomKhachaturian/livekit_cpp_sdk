#ifndef DEMOAPP_H
#define DEMOAPP_H
#include <QGuiApplication>
#include <QScopedPointer>
#include <QPointer>
#include <optional>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace LiveKitCpp {
class Service;
enum class ServiceState;
}

class DemoApp : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
public:
    DemoApp(int &argc, char **argv);
    ~DemoApp() override;
public slots:
    void setAppWindow(QObject* appWindow, const QUrl&);
    Q_INVOKABLE void registerClient(const QString& id, bool reg);
    Q_INVOKABLE bool isValid() const;
signals:
    void showErrorMessage(const QString& message);
private:
    QScopedPointer<LiveKitCpp::Service> _service;
    std::optional<LiveKitCpp::ServiceState> _serviceInitFailure;
    QPointer<QObject> _appWindow;
};

#endif // DEMOAPP_H
