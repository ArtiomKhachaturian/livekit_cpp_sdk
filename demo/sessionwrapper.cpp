#include "sessionwrapper.h"

SessionWrapper::SessionWrapper(std::unique_ptr<LiveKitCpp::Session> impl,
                               const QString& clientId, QObject *parent)
    : QObject{parent}
    , _impl(std::move(impl))
    , _clientId(clientId)
{
    if (_impl) {
        _impl->setListener(this);
    }
}

SessionWrapper::~SessionWrapper()
{
    disconnectFromSfu();
    if (_impl) {
        _impl->setListener(nullptr);
    }
}

bool SessionWrapper::connectToSfu(const QString& url, const QString& token)
{
    return _impl && _impl->connect(url.toStdString(), token.toStdString());
}

void SessionWrapper::disconnectFromSfu()
{
    if (_impl) {
        _impl->disconnect();
    }
}

QString SessionWrapper::sid() const
{
    if (_impl) {
        return QString::fromStdString(_impl->sid());
    }
    return {};
}

QString SessionWrapper::identity() const
{
    if (_impl) {
        return QString::fromStdString(_impl->identity());
    }
    return {};
}

QString SessionWrapper::name() const
{
    if (_impl) {
        return QString::fromStdString(_impl->name());
    }
    return {};
}

void SessionWrapper::onError(LiveKitCpp::LiveKitError error, const std::string& what)
{
    emit this->error(_clientId,
                     QString::fromStdString(LiveKitCpp::toString(error)),
                     QString::fromStdString(what));
}
