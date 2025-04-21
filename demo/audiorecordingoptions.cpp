#include "audiorecordingoptions.h"
#include <tuple>

namespace  {

inline bool equal(const LiveKitCpp::AudioRecordingOptions& l,
                  const LiveKitCpp::AudioRecordingOptions& r) {
    return std::tie(l._autoGainControl, l._echoCancellation, l._highpassFilter, l._noiseSuppression, l._stereoSwapping) ==
           std::tie(r._autoGainControl, r._echoCancellation, r._highpassFilter, r._noiseSuppression, r._stereoSwapping);
}

inline bool notEqual(const LiveKitCpp::AudioRecordingOptions& l,
                    const LiveKitCpp::AudioRecordingOptions& r) {
    return std::tie(l._autoGainControl, l._echoCancellation, l._highpassFilter, l._noiseSuppression, l._stereoSwapping) !=
           std::tie(r._autoGainControl, r._echoCancellation, r._highpassFilter, r._noiseSuppression, r._stereoSwapping);
}

}

AudioRecordingOptions::AudioRecordingOptions(const LiveKitCpp::AudioRecordingOptions& src)
    : _impl(src)
{
}

AudioRecordingOptions& AudioRecordingOptions::operator = (const LiveKitCpp::AudioRecordingOptions& src)
{
    if (&src != &_impl) {
        _impl = src;
    }
    return *this;
}

bool AudioRecordingOptions::operator == (const AudioRecordingOptions& other) const noexcept
{
    return this == &other || &_impl == &other._impl || equal(_impl, other._impl);
}

bool AudioRecordingOptions::operator == (const LiveKitCpp::AudioRecordingOptions& other) const noexcept
{
    return &_impl == &other || equal(_impl, _impl);
}

bool AudioRecordingOptions::operator != (const AudioRecordingOptions& other) const noexcept
{
    return notEqual(_impl, other._impl);
}

bool AudioRecordingOptions::operator != (const LiveKitCpp::AudioRecordingOptions& other) const noexcept
{
    return notEqual(_impl, other);
}

QString AudioRecordingOptions::toString() const
{
    QStringList states;
    states.append(QStringLiteral("AEC - ") + stateToString(echoCancellation()));
    states.append(QStringLiteral("AGC - ") + stateToString(autoGainControl()));
    states.append(QStringLiteral("NS - ") + stateToString(noiseSuppression()));
    states.append(QStringLiteral("HPF - ") + stateToString(highpassFilter()));
    states.append(QStringLiteral("STS - ") + stateToString(stereoSwapping()));
    return states.join(",");
}

Qt::CheckState AudioRecordingOptions::getState(const std::optional<bool>& val)
{
    if (val.has_value()) {
        return val.value() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    }
    return Qt::CheckState::PartiallyChecked;
}

void AudioRecordingOptions::setState(Qt::CheckState state, std::optional<bool>& val)
{
    switch (state) {
        case Qt::Unchecked:
            val = false;
            break;
        case Qt::PartiallyChecked:
            val = std::nullopt;
            break;
        case Qt::Checked:
            val = true;
            break;
    }
}

QString AudioRecordingOptions::stateToString(Qt::CheckState state)
{
    switch (state) {
        case Qt::Unchecked:
            return QStringLiteral("off");
        case Qt::PartiallyChecked:
            break;
        case Qt::Checked:
            return QStringLiteral("on");
    }
    return QStringLiteral("?");
}
