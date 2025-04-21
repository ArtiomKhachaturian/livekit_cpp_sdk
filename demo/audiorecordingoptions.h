#ifndef AUDIORECORDINGOPTIONS_H
#define AUDIORECORDINGOPTIONS_H
#include <livekit/rtc/media/AudioRecordingOptions.h>
#include <QQmlEngine>

class AudioRecordingOptions
{
    Q_GADGET
public:
    Q_PROPERTY(Qt::CheckState echoCancellation READ echoCancellation WRITE setEchoCancellation)
    Q_PROPERTY(Qt::CheckState autoGainControl READ autoGainControl WRITE setAutoGainControl)
    Q_PROPERTY(Qt::CheckState noiseSuppression READ noiseSuppression WRITE setNoiseSuppression)
    Q_PROPERTY(Qt::CheckState highpassFilter READ highpassFilter WRITE setHighpassFilter)
    Q_PROPERTY(Qt::CheckState stereoSwapping READ stereoSwapping WRITE setStereoSwapping)
public:
    AudioRecordingOptions() = default;
    AudioRecordingOptions(const AudioRecordingOptions&) = default;
    AudioRecordingOptions(AudioRecordingOptions&&) noexcept = default;
    AudioRecordingOptions(const LiveKitCpp::AudioRecordingOptions& src);
    AudioRecordingOptions& operator = (const AudioRecordingOptions&) = default;
    AudioRecordingOptions& operator = (AudioRecordingOptions&&) noexcept = default;
    AudioRecordingOptions& operator = (const LiveKitCpp::AudioRecordingOptions& src);
    bool operator == (const AudioRecordingOptions& other) const noexcept;
    bool operator == (const LiveKitCpp::AudioRecordingOptions& other) const noexcept;
    bool operator != (const AudioRecordingOptions& other) const noexcept;
    bool operator != (const LiveKitCpp::AudioRecordingOptions& other) const noexcept;
    operator LiveKitCpp::AudioRecordingOptions() const { return _impl; }
    operator QString() const { return toString(); }
    Q_INVOKABLE Qt::CheckState echoCancellation() const { return getState(_impl._echoCancellation); }
    Q_INVOKABLE Qt::CheckState autoGainControl() const { return getState(_impl._autoGainControl); }
    Q_INVOKABLE Qt::CheckState noiseSuppression() const { return getState(_impl._noiseSuppression); }
    Q_INVOKABLE Qt::CheckState highpassFilter() const { return getState(_impl._highpassFilter); }
    Q_INVOKABLE Qt::CheckState stereoSwapping() const { return getState(_impl._stereoSwapping); }
    Q_INVOKABLE void setEchoCancellation(Qt::CheckState state) { setState(state, _impl._echoCancellation); }
    Q_INVOKABLE void setAutoGainControl(Qt::CheckState state) { setState(state, _impl._autoGainControl); }
    Q_INVOKABLE void setNoiseSuppression(Qt::CheckState state) { setState(state, _impl._noiseSuppression); }
    Q_INVOKABLE void setHighpassFilter(Qt::CheckState state) { setState(state, _impl._highpassFilter); }
    Q_INVOKABLE void setStereoSwapping(Qt::CheckState state) { setState(state, _impl._stereoSwapping); }
    Q_INVOKABLE QString toString() const;
private:
    static Qt::CheckState getState(const std::optional<bool>& val);
    void setState(Qt::CheckState state, std::optional<bool>& val);
    static QString stateToString(Qt::CheckState state);
private:
    LiveKitCpp::AudioRecordingOptions _impl;
};

Q_DECLARE_METATYPE(AudioRecordingOptions)

#endif // AUDIORECORDINGOPTIONS_H
