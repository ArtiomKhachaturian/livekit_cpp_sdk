// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "MacAudioDevice.h"
#include "MacVoiceProcessingAudioUnit.h"
#include <modules/audio_device/fine_audio_buffer.h>
#include <rtc_base/logging.h>
#include <rtc_base/thread.h>
#include <rtc_base/arraysize.h>
#include <rtc_base/platform_thread.h>
#include <mach/mach_time.h>
#include <cmath>
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

extern "C" {
// See: https://trac.webkit.org/browser/webkit/trunk/Source/WebCore/PAL/pal/spi/cf/CoreAudioSPI.h?rev=228264
OSStatus AudioDeviceDuck(AudioDeviceID device,
                         Float32 duckedLevel,
                         const AudioTimeStamp* __nullable startTime,
                         Float32 rampDuration) __attribute__((weak_import));
}

namespace {

inline void SetCurrentThreadRealtime(bool recordThread) {
    @autoreleasepool {
        NSThread* current = [NSThread currentThread];
        if (current) {
            [current setThreadPriority: 1.0];
            if (1.0 != current.threadPriority) {
                RTC_LOG(LS_WARNING) << "Unable to set realtime priority for thread '" 
                                    << (recordThread ? "audio_record" : "audio_playout") << "'";
            }
            else {
                [current setQualityOfService: NSQualityOfServiceUserInteractive];
            }
        }
    }
}

}

namespace LiveKitCpp
{

// FineAudioBuffer takes an AudioDeviceBuffer which delivers audio data
// in chunks of 10ms. It then allows for this data to be pulled in
// a finer or coarser granularity. I.e. interacting with this class instead
// of directly with the AudioDeviceBuffer one can ask for any number of
// audio data samples. Is also supports a similar scheme for the recording
// side.
// Example: native buffer size can be 128 audio frames at 16kHz sample rate.
// WebRTC will provide 480 audio frames per 10ms but MacOS asks for 128
// in each callback (one every 8ms). This class can then ask for 128 and the
// FineAudioBuffer will ask WebRTC for new data only when needed and also
// cache non-utilized audio between callbacks. On the recording side, MacOS
// can provide audio data frames of size 128 and these are accumulated until
// enough data to supply one 10ms call exists. This 10ms chunk is then sent
// to WebRTC and the remaining part is stored.
class MacAudioDevice::FineAudioBuffer : public webrtc::FineAudioBuffer
{
public:
    FineAudioBuffer(webrtc::AudioDeviceBuffer* audioDeviceBuffer);
    void SetTypingStatus();
private:
    static webrtc::AudioDeviceBuffer* SetupAudioBuffers(webrtc::AudioDeviceBuffer* audioDeviceBuffer);
    // keypress detection
    bool KeyPressed();
private:
    // Raw pointer handle provided to us in AttachAudioBuffer(). Owned by the
    // AudioDeviceModuleImpl class and called by AudioDeviceModule::Create().
    // The AudioDeviceBuffer is a member of the AudioDeviceModuleImpl instance
    // and therefore outlives this object.
    webrtc::AudioDeviceBuffer* const _audioDeviceBuffer;
    // Typing detection
    // 0x5c is key "9", after that comes function keys.
    bool _prevKeyState[0x5d];
};

MacAudioDevice::MacAudioDevice()
    : _thread(rtc::Thread::Current())
    , _recordAudioBuffer(MacVoiceProcessingAudioUnit::_bytesPerSample, MacVoiceProcessingAudioUnit::_preferredNumberOfChannels)
    , _recordingDevice(GetDefaultDevice(true))
    , _playoutDevice(GetDefaultDevice(false))
{
    _ioThreadChecker.Detach();
    DuckAudioDevice(_playoutDevice, _speakerEnabled.load());
}

MacAudioDevice::~MacAudioDevice()
{
    Terminate();
}

void MacAudioDevice::AttachAudioBuffer(webrtc::AudioDeviceBuffer* audioBuffer)
{
    RTC_DCHECK_RUN_ON(_thread);
    if (audioBuffer) {
        _fineAudioBuffer = std::make_unique<FineAudioBuffer>(audioBuffer);
    }
    else {
        _fineAudioBuffer.reset();
    }
}

webrtc::AudioDeviceGeneric::InitStatus MacAudioDevice::Init()
{
    _ioThreadChecker.Detach();
    RTC_DCHECK_RUN_ON(_thread);
    if (!_initialized) {
        _initialized = true;
    }
    return InitStatus::OK;
}

int32_t MacAudioDevice::Terminate()
{
    RTC_DCHECK_RUN_ON(_thread);
    if (_initialized) {
        StopPlayout();
        StopRecording();
        _initialized = false;
    }
    return 0;
}

bool MacAudioDevice::Initialized() const
{
    RTC_DCHECK_RUN_ON(_thread);
    return _initialized;
}

int32_t MacAudioDevice::InitPlayout()
{
    RTC_DCHECK_RUN_ON(_thread);
    RTC_DCHECK(_initialized);
    RTC_DCHECK(!_playing.load());
    if (!InitPlayOrRecord()) {
        RTC_LOG_F(LS_ERROR) << "InitPlayOrRecord failed for InitPlayout!";
        return -1;
    }
    return 0;
}

bool MacAudioDevice::PlayoutIsInitialized() const
{
    RTC_DCHECK_RUN_ON(_thread);
    return nullptr != _audioUnit.get();
}

int32_t MacAudioDevice::InitRecording()
{
    RTC_DCHECK_RUN_ON(_thread);
    RTC_DCHECK(_initialized);
    RTC_DCHECK(!_recording.load());
    if (!InitPlayOrRecord()) {
        RTC_LOG_F(LS_ERROR) << "InitPlayOrRecord failed for InitRecording!";
        return -1;
    }
    return 0;
}

bool MacAudioDevice::RecordingIsInitialized() const
{
    return nullptr != _audioUnit.get();
}

int32_t MacAudioDevice::StartPlayout()
{
    RTC_DCHECK_RUN_ON(_thread);
    RTC_DCHECK(!_playing.load());
    RTC_DCHECK(_audioUnit);
    int32_t result = 0/*_mixerManager.OpenSpeaker(_playoutDevice)*/;
    if (0 == result) {
        if (_fineAudioBuffer) {
            _fineAudioBuffer->ResetPlayout();
        }
        if (!_recording.load() && _audioUnit->Initialized()) {
            OSStatus result = _audioUnit->Start();
            if (result != noErr) {
                //_mixerManager.CloseSpeaker();
                RTC_LOG(LS_ERROR) << "StartPlayout failed to start audio unit, reason " << result;
                result = -1;
            }
            else {
                RTC_LOG(LS_INFO) << "Voice-Processing I/O audio unit is now started";
            }
        }
        _playing.store(1, std::memory_order_release);
    }
    return result;
}

int32_t MacAudioDevice::StopPlayout()
{
    RTC_DCHECK_RUN_ON(_thread);
    if (_playing.exchange(0, std::memory_order_release)) {
        if (!_recording.load()) {
            ShutdownPlayOrRecord();
        }
        //_mixerManager.CloseSpeaker();
    }
    return 0;
}

bool MacAudioDevice::Playing() const
{
    return _playing.load();
}

int32_t MacAudioDevice::StartRecording()
{
    RTC_DCHECK_RUN_ON(_thread);
    RTC_DCHECK(!_recording.load());
    RTC_DCHECK(_audioUnit);
    int32_t result = 0 /*_mixerManager.OpenMicrophone(_recordingDevice)*/;
    if (0 == result) {
        if (_fineAudioBuffer) {
            _fineAudioBuffer->ResetRecord();
        }
        if (!_playing.load() && _audioUnit->Initialized()) {
            OSStatus result = _audioUnit->Start();
            if (result != noErr) {
                //_mixerManager.CloseMicrophone();
                RTC_LOG(LS_ERROR) << "StartRecording failed to start audio unit, reason " << result;
                result = -1;
            }
            else {
                RTC_LOG(LS_INFO) << "Voice-Processing I/O audio unit is now started";
            }
        }
        _recording.store(1, std::memory_order_release);
    }
    return result;
}

int32_t MacAudioDevice::StopRecording()
{
    RTC_DCHECK_RUN_ON(_thread);
    if (_recording.exchange(0, std::memory_order_release)) {
        if (!_playing.load()) {
            ShutdownPlayOrRecord();
        }
        //_mixerManager.CloseMicrophone();
    }
    return 0;
}

bool MacAudioDevice::Recording() const
{
    return _recording.load();
}

int32_t MacAudioDevice::PlayoutDelay(uint16_t& delayMS) const
{
    RTC_DCHECK_RUN_ON(_thread);
    if (_audioUnit) {
        int32_t renderDelayUs = _renderDelayUs;
        delayMS = static_cast<uint16_t>(1e-3 * (renderDelayUs + _audioUnit->GetRenderLatencyUs()) + 0.5);
        return 0;
    }
    return -1;
}

int32_t MacAudioDevice::ActiveAudioLayer(webrtc::AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

int32_t MacAudioDevice::PlayoutIsAvailable(bool& available)
{
    available = true;
    return 0;
}

int32_t MacAudioDevice::RecordingIsAvailable(bool& available)
{
    available = true;
    return 0;
}

int16_t MacAudioDevice::PlayoutDevices()
{
    return static_cast<int16_t>(GetDevices(false).size());
}

int16_t MacAudioDevice::RecordingDevices()
{
    return static_cast<int16_t>(GetDevices(true).size());
}

int32_t MacAudioDevice::PlayoutDeviceName(uint16_t index,
                                          char name[webrtc::kAdmMaxDeviceNameSize],
                                          char guid[webrtc::kAdmMaxGuidSize])
{
    return GetDeviceName(false, index, name, guid) ? 0 : -1;
}

int32_t MacAudioDevice::RecordingDeviceName(uint16_t index,
                                            char name[webrtc::kAdmMaxDeviceNameSize],
                                            char guid[webrtc::kAdmMaxGuidSize])
{
    return GetDeviceName(true, index, name, guid) ? 0 : -1;
}

int32_t MacAudioDevice::SetPlayoutDevice(uint16_t index)
{
    RTC_DCHECK_RUN_ON(_thread);
    const auto devices = GetDevices(false);
    int32_t result = -1;
    if (index < devices.size()) {
        if (devices.at(index) != _playoutDevice) {
            _playoutDevice = devices.at(index);
            DuckAudioDevice(_playoutDevice, _speakerEnabled.load());
            if (_audioUnit) {
                const auto status = _audioUnit->SetPlayoutDevice(_playoutDevice);
                if (noErr == status) {
                    result = 0;
                }
                else {
                    RTC_LOG(LS_ERROR) << "Failed to set playout device in audio unit: " << status;
                }
            }
            else {
                result = 0;
            }
        }
        else {
            result = 0;
        }
    }
    return result;
}

int32_t MacAudioDevice::SetPlayoutDevice(webrtc::AudioDeviceModule::WindowsDeviceType device)
{
    RTC_LOG(LS_ERROR) << "WindowsDeviceType not supported";
    return -1;
}

int32_t MacAudioDevice::SetRecordingDevice(uint16_t index)
{
    RTC_DCHECK_RUN_ON(_thread);
    const auto devices = GetDevices(true);
    int32_t result = -1;
    if (index < devices.size()) {
        if (devices.at(index) != _recordingDevice) {
            _recordingDevice = devices.at(index);
            if (_audioUnit) {
                const auto status = _audioUnit->SetRecordingDevice(_recordingDevice);
                if (noErr == status) {
                    result = 0;
                }
                else {
                    RTC_LOG(LS_ERROR) << "Failed to set recording device in audio unit: " << status;
                }
            }
            else {
                result = 0;
            }
        }
        else {
            result = 0;
        }
    }
    return result;
}

int32_t MacAudioDevice::SetRecordingDevice(webrtc::AudioDeviceModule::WindowsDeviceType device)
{
    RTC_LOG(LS_ERROR) << "WindowsDeviceType not supported";
    return -1;
}

int32_t MacAudioDevice::InitSpeaker()
{
    return 0;
}

bool MacAudioDevice::SpeakerIsInitialized() const
{
    return true;
}

int32_t MacAudioDevice::InitMicrophone()
{
    return 0;
}

bool MacAudioDevice::MicrophoneIsInitialized() const
{
    return true;
}

int32_t MacAudioDevice::SpeakerVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t MacAudioDevice::SetSpeakerVolume(uint32_t volume)
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::SpeakerVolume(uint32_t& volume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MicrophoneVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t MacAudioDevice::SetMicrophoneVolume(uint32_t volume)
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MicrophoneVolume(uint32_t& volume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const
{
    RTC_DCHECK_NOTREACHED() << "Not implemented";
    return -1;
}

int32_t MacAudioDevice::MicrophoneMuteIsAvailable(bool& available)
{
    available = true;
    return 0;
}

int32_t MacAudioDevice::SetMicrophoneMute(bool enable)
{
    _microphoneEnabled.store(enable, std::memory_order_release);
    return 0;
}

int32_t MacAudioDevice::MicrophoneMute(bool& enabled) const
{
    enabled = _microphoneEnabled.load();
    return 0;
}

int32_t MacAudioDevice::SpeakerMuteIsAvailable(bool& available)
{
    available = true;
    return 0;
}

int32_t MacAudioDevice::SetSpeakerMute(bool enable)
{
    if (enable != _speakerEnabled.exchange(enable, std::memory_order_release)) {
        DuckAudioDevice(_playoutDevice, enable);
    }
    return 0;
}

int32_t MacAudioDevice::SpeakerMute(bool& enabled) const
{
    enabled = _speakerEnabled.load();
    return 0;
}

int32_t MacAudioDevice::StereoPlayoutIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t MacAudioDevice::SetStereoPlayout(bool enable)
{
    return enable ? -1 : 0;
}

int32_t MacAudioDevice::StereoPlayout(bool& enabled) const
{
    enabled = false;
    return 0;
}

int32_t MacAudioDevice::StereoRecordingIsAvailable(bool& available)
{
    available = false;
    return 0;
}

int32_t MacAudioDevice::SetStereoRecording(bool enable)
{
    return enable ? -1 : 0;
}

int32_t MacAudioDevice::StereoRecording(bool& enabled) const
{
    enabled = false;
    return 0;
}

int32_t MacAudioDevice::GetDelayUs(bool input, UInt32 /*numFrames*/,
                                   const AudioTimeStamp* timestamp)
{
    const UInt64 hostTimeNs = GetHostTimeNs(timestamp);
    const UInt64 nowNs = GetCurrentHostTimeNs();
    UInt64 delta = 0ULL;
    if (input) {
        RTC_DCHECK_LE(hostTimeNs, nowNs);
        delta = nowNs - hostTimeNs;
    }
    else {
        RTC_DCHECK_GE(hostTimeNs, nowNs);
        delta = hostTimeNs - nowNs;
    }
    auto delay = 1e-3 * delta + 0.5;
    /*if (numFrames) {
        const auto sampleRate = input ? _recSamplesPerSec : _playSamplesPerSec;
        delay += (1.0e6 * numFrames) / MacVoiceProcessingAudioUnit::_preferredNumberOfChannels / sampleRate + 0.5;
    }*/
    return static_cast<int32_t>(std::round(delay));
}

UInt64 MacAudioDevice::GetHostTimeNs(const AudioTimeStamp* timestamp)
{
    RTC_DCHECK(timestamp);
    return AudioConvertHostTimeToNanos(timestamp->mHostTime);
}

UInt64 MacAudioDevice::GetCurrentHostTimeNs()
{
    return AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
}
    
std::vector<AudioDeviceID> MacAudioDevice::GetDevices(bool input)
{
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    UInt32 size = 0;
    OSStatus result = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                                     &propertyAddress,
                                                     0, nullptr, &size);
    if (noErr == result) {
        if (size) {
            std::vector<AudioDeviceID> availableDevices(size / sizeof(AudioDeviceID));
            result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                &propertyAddress,
                                                0, nullptr, &size,
                                                availableDevices.data());
            if (noErr == result) {
                std::vector<AudioDeviceID> matchedDevices;
                propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
                propertyAddress.mScope = input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
                propertyAddress.mElement = 0;
                // iterate over all available devices to gather information
                for (auto device : availableDevices) {
                    result = AudioObjectGetPropertyDataSize(device, &propertyAddress,
                                                            0, nullptr, &size);
                    if (result == kAudioHardwareBadDeviceError) {
                        // this device doesn't actually exist - continue iterating
                        continue;
                    }
                    else {
                        if (noErr == result) {
                            std::unique_ptr<uint8_t[]> bufferList(new uint8_t[size]);
                            result = AudioObjectGetPropertyData(device, &propertyAddress,
                                                                0, nullptr,
                                                                &size, bufferList.get());
                            if (noErr == result) {
                                if (reinterpret_cast<AudioBufferList*>(bufferList.get())->mNumberBuffers > 0) {
                                    matchedDevices.push_back(device);
                                }
                            }
                            else {
                                RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyData: " << result;
                                matchedDevices.clear();
                                break;
                            }
                        }
                        else {
                            RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyDataSize: " << result;
                            matchedDevices.clear();
                            break;
                        }
                    }
                }
                return matchedDevices;
            }
            else {
                RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyData: " << result;
            }
        }
        else {
            RTC_LOG(LS_WARNING) << "No devices";
        }
    }
    else {
        RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyDataSize: " << result;
    }
    return {};
}

AudioDeviceID MacAudioDevice::GetDefaultDevice(bool input)
{
    AudioObjectPropertyAddress propertyAddress = {
        input ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    UInt32 size = sizeof(AudioDeviceID);
    AudioDeviceID device = 0;
    const auto result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                   &propertyAddress, 0, nullptr,
                                                   &size, &device);
    if (noErr != result) {
        RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyData: " << result;
    }
    return device;
}

bool MacAudioDevice::GetDeviceName(bool input, uint16_t index,
                                   char name[webrtc::kAdmMaxDeviceNameSize],
                                   char guid[webrtc::kAdmMaxGuidSize])
{
    if (name) {
        const auto devices = GetDevices(input);
        if (index < devices.size()) {
            UInt32 len = webrtc::kAdmMaxDeviceNameSize;
            AudioObjectPropertyAddress propertyAddress = {
                kAudioDevicePropertyDeviceName,
                input ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput,
                0
            };
            std::memset(name, 0, webrtc::kAdmMaxDeviceNameSize);
            OSStatus result = AudioObjectGetPropertyData(devices[index],
                                                         &propertyAddress,
                                                         0, nullptr,
                                                         &len, name);
            if (noErr == result) {
                if (guid) {
                    std::memset(guid, 0, webrtc::kAdmMaxGuidSize);
                }
            }
            else {
                RTC_LOG(LS_ERROR) << "Error in AudioObjectGetPropertyData: " << result;
            }
            return noErr == result;
        }
    }
    return false;
}

void MacAudioDevice::MarkPlayoutAsSilence(AudioUnitRenderActionFlags* flags,
                                          UInt32 numFrames, AudioBufferList* ioData)
{
    if (ioData) {
        auto audioBuffer = &ioData->mBuffers[0];
        // produce silence and give audio unit a hint about it if playout is not activated
        const size_t sizeInBytes = audioBuffer->mDataByteSize;
        RTC_CHECK_EQ(sizeInBytes / MacVoiceProcessingAudioUnit::_bytesPerSample, numFrames);
        std::memset(audioBuffer->mData, 0, sizeInBytes);
        if (flags) {
            *flags |= kAudioUnitRenderAction_OutputIsSilence;
        }
    }
}

void MacAudioDevice::DuckAudioDevice(AudioDeviceID device, bool duck)
{
    if (_audioDuckingEnabled && AudioDeviceDuck) {
        AudioDeviceDuck(device, duck ? 0.177828f : 1.0f, nullptr, 0.1f);
    }
}

int32_t MacAudioDevice::GetFullDelayMs(bool input) const
{
    const auto& delay = input ? _captureDelayUs : _renderDelayUs;
    const auto latency = input ? _audioUnit->GetCaptureLatencyUs() : _audioUnit->GetRenderLatencyUs();
    return static_cast<int32_t>(std::floor(1e-3 * (delay.load() + latency) + 0.5));
}

bool MacAudioDevice::CreateAudioUnit()
{
    RTC_DCHECK_RUN_ON(_thread);
    if (!_audioUnit) {
        MacVoiceProcessingAudioUnitObserver* observer = this;
        auto audioUnit = std::make_unique<MacVoiceProcessingAudioUnit>(observer);
        auto status = audioUnit->Init();
        if (noErr == status) {
            status = audioUnit->SetPlayoutDevice(_playoutDevice);
            if (noErr == status) {
                status = audioUnit->SetRecordingDevice(_recordingDevice);
                if (noErr == status) {
                    _audioUnit = std::move(audioUnit);
                }
                else {
                    RTC_LOG(LS_ERROR) << "Failed to set recording device in audio unit: " << status;
                }
            }
            else {
                RTC_LOG(LS_ERROR) << "Failed to set playout device in audio unit: " << status;
            }
        }
        else {
            RTC_LOG(LS_ERROR) << "Failed to initialize audio unit: " << status;
        }
    }
    return nullptr != _audioUnit.get();
}

void MacAudioDevice::UpdateAudioUnit(bool canPlayOrRecord)
{
    RTC_DCHECK_RUN_ON(_thread);
    // if we're initialized, we must have an audio unit
    if (_audioUnit) {
        bool should_initialize_audio_unit = false;
        bool should_uninitialize_audio_unit = false;
        bool should_start_audio_unit = false;
        bool should_stop_audio_unit = false;
        switch (_audioUnit->GetState()) {
            case MacVoiceProcessingAudioUnit::State::InitRequired:
                RTC_LOG(LS_VERBOSE) << "VPAU state: InitRequired";
                RTC_DCHECK_NOTREACHED();
                break;
            case MacVoiceProcessingAudioUnit::State::Uninitialized:
                RTC_LOG(LS_VERBOSE) << "VPAU state: Uninitialized";
                should_initialize_audio_unit = canPlayOrRecord;
                should_start_audio_unit = should_initialize_audio_unit && (_playing.load() || _recording.load());
                break;
            case MacVoiceProcessingAudioUnit::State::Initialized:
                RTC_LOG(LS_VERBOSE) << "VPAU state: Initialized";
                should_start_audio_unit = canPlayOrRecord && (_playing.load() || _recording.load());
                should_uninitialize_audio_unit = !canPlayOrRecord;
                break;
            case MacVoiceProcessingAudioUnit::State::Started:
                RTC_LOG(LS_VERBOSE) << "VPAU state: Started";
                RTC_DCHECK(_playing.load() || _recording.load());
                should_stop_audio_unit = !canPlayOrRecord;
                should_uninitialize_audio_unit = should_stop_audio_unit;
                break;
        }
        if (should_initialize_audio_unit) {
            RTC_LOG(LS_VERBOSE) << "Initializing audio unit for UpdateAudioUnit";
            const auto status = _audioUnit->Initialize(_playSamplesPerSec);
            if (noErr != status) {
                RTC_LOG(LS_ERROR) << "Failed to initialize audio unit: " << status;
                return;
            }
        }
        if (should_start_audio_unit) {
            RTC_LOG(LS_VERBOSE) << "Starting audio unit for UpdateAudioUnit";
            const auto status = _audioUnit->Start();
            if (noErr != status) {
                RTC_LOG(LS_ERROR) << "Failed to start audio unit: " << status;
                return;
            }
        }
        if (should_stop_audio_unit) {
            RTC_LOG(LS_VERBOSE) << "Stopping audio unit for UpdateAudioUnit";
            const auto status = _audioUnit->Stop();
            if (noErr != status) {
                RTC_LOG(LS_ERROR) << "Failed to stop audio unit: " << status;
                return;
            }
            PrepareForNewStart();
        }
        if (should_uninitialize_audio_unit) {
            RTC_LOG(LS_VERBOSE) << "Uninitializing audio unit for UpdateAudioUnit";
            const auto status = _audioUnit->Uninitialize();
            if (noErr != status) {
                RTC_LOG(LS_ERROR) << "Failed to uninitialize audio unit: " << status;
            }
        }
    }
}

bool MacAudioDevice::InitPlayOrRecord()
{
    RTC_DCHECK_RUN_ON(_thread);
    if (CreateAudioUnit()) {
        const auto status = _audioUnit->Initialize(_playSamplesPerSec);
        if (noErr != status) {
            RTC_LOG(LS_ERROR) << "Failed to initialize audio unit: " << status;
        }
        else {
            _captureDelayUs = 0;
            _renderDelayUs = 0;
        }
        return noErr == status;
    }
    return false;
}

void MacAudioDevice::ShutdownPlayOrRecord()
{
    RTC_DCHECK_RUN_ON(_thread);
    // Stop the audio unit to prevent any additional audio callbacks.
    _audioUnit->Stop();
    // Close and delete the voice-processing I/O unit.
    _audioUnit.reset();
    // Detach thread checker for the AURemoteIO::IOThread to ensure that the
    // next session uses a fresh thread id.
    _ioThreadChecker.Detach();
}

void MacAudioDevice::PrepareForNewStart()
{
    // The audio unit has been stopped and preparations are needed for an upcoming
    // restart. It will result in audio callbacks from a new native I/O thread
    // which means that we must detach thread checkers here to be prepared for an
    // upcoming new audio stream.
    _ioThreadChecker.Detach();
}

OSStatus MacAudioDevice::OnDeliverRecordedData(AudioUnitRenderActionFlags* flags,
                                               const AudioTimeStamp* timestamp,
                                               UInt32 busNumber,
                                               UInt32 numFrames)
{
    OSStatus result = noErr;
    if (_microphoneEnabled.load()) {
        RTC_DCHECK_RUN_ON(&_ioThreadChecker);
        // simply return if recording is not enabled
        if (_recording.load(std::memory_order_acquire)) {
            // Get audio timestamp for the audio.
            // The timestamp will not have NTP time epoch, but that will be addressed by
            // the TimeStampAligner in AudioDeviceBuffer::SetRecordedBuffer().
            _captureDelayUs = GetDelayUs(true, numFrames, timestamp);
            // boost priority (?)
            static thread_local std::once_flag priority;
            std::call_once(priority, SetCurrentThreadRealtime, true);
            // Set the size of our own audio buffer and clear it first to avoid copying
            // in combination with potential reallocations.
            // On real iOS devices, the size will only be set once (at first callback).
            _recordAudioBuffer.SetFramesCount(numFrames);
            //absl::optional<int64_t> captureTimestampNs(AudioConvertHostTimeToNanos(timestamp->mHostTime));
            // Obtain the recorded audio samples by initiating a rendering cycle.
            // Since it happens on the input bus, the `io_data` parameter is a reference
            // to the preallocated audio buffer list that the audio unit renders into.
            // We can make the audio unit provide a buffer instead in io_data, but we
            // currently just use our own.
            // TODO(henrika): should error handling be improved?
            result = _audioUnit->Render(flags, timestamp, busNumber, numFrames, _recordAudioBuffer);
            if (result != noErr) {
                RTC_LOG(LS_ERROR) << "Failed to render audio: " << result;
            }
            else {
                RTC_DCHECK(_fineAudioBuffer);
                _fineAudioBuffer->SetTypingStatus();
                // Get a pointer to the recorded audio and send it to the WebRTC ADB.
                // Use the FineAudioBuffer instance to convert between native buffer size
                // and the 10ms buffer size used by WebRTC.
                _fineAudioBuffer->DeliverRecordedData(_recordAudioBuffer,
                                                      GetFullDelayMs(true));
            }
        }
    }
    return result;
}

OSStatus MacAudioDevice::OnGetPlayoutData(AudioUnitRenderActionFlags* flags,
                                          const AudioTimeStamp* timestamp,
                                          UInt32 busNumber,
                                          UInt32 numFrames,
                                          AudioBufferList* ioData)
{
    if (_speakerEnabled.load()) {
        RTC_DCHECK_RUN_ON(&_ioThreadChecker);
        if (_playing.load(std::memory_order_acquire)) {
            _renderDelayUs = GetDelayUs(false, numFrames, timestamp);
            // boost priority (?)
            static thread_local std::once_flag priority;
            std::call_once(priority, SetCurrentThreadRealtime, false);
            // Verify 16-bit, noninterleaved mono PCM signal format.
            RTC_DCHECK_EQ(1, ioData->mNumberBuffers);
            AudioBuffer* audioBuffer = &ioData->mBuffers[0];
            RTC_DCHECK_EQ(MacVoiceProcessingAudioUnit::_preferredNumberOfChannels, audioBuffer->mNumberChannels);
            const auto data = reinterpret_cast<int16_t*>(audioBuffer->mData);
            // Read decoded 16-bit PCM samples from WebRTC (using a size that matches
            // the native I/O audio unit) and copy the result to the audio buffer in the
            // `ioData` destination.
            _fineAudioBuffer->GetPlayoutData(rtc::ArrayView<int16_t>(data, numFrames),
                                             GetFullDelayMs(false));
        }
        else {
            MarkPlayoutAsSilence(flags, numFrames, ioData);
        }
    }
    else {
        MarkPlayoutAsSilence(flags, numFrames, ioData);
    }
    return noErr;
}

MacAudioDevice::FineAudioBuffer::FineAudioBuffer(webrtc::AudioDeviceBuffer* audioDeviceBuffer)
    : webrtc::FineAudioBuffer(SetupAudioBuffers(audioDeviceBuffer))
    , _audioDeviceBuffer(audioDeviceBuffer)
{
    std::memset(&_prevKeyState, 0, sizeof(_prevKeyState));
}

void MacAudioDevice::FineAudioBuffer::SetTypingStatus()
{
    _audioDeviceBuffer->SetTypingStatus(KeyPressed());
}

webrtc::AudioDeviceBuffer* MacAudioDevice::FineAudioBuffer::SetupAudioBuffers(webrtc::AudioDeviceBuffer* audioDeviceBuffer)
{
    RTC_DCHECK(audioDeviceBuffer);
    // Inform the audio device buffer (ADB) about the new audio format.
    audioDeviceBuffer->SetPlayoutSampleRate(MacAudioDevice::_playSamplesPerSec);
    audioDeviceBuffer->SetPlayoutChannels(MacVoiceProcessingAudioUnit::_preferredNumberOfChannels);
    audioDeviceBuffer->SetRecordingSampleRate(MacAudioDevice::_recSamplesPerSec);
    audioDeviceBuffer->SetRecordingChannels(MacVoiceProcessingAudioUnit::_preferredNumberOfChannels);
    return audioDeviceBuffer;
}

bool MacAudioDevice::FineAudioBuffer::KeyPressed()
{
    bool keyDown = false;
      // Loop through all Mac virtual key constant values.
    for (unsigned keyIndex = 0U; keyIndex < arraysize(_prevKeyState); ++keyIndex) {
        bool keyState = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, keyIndex);
        // A false -> true change in keymap means a key is pressed.
        keyDown |= (keyState && !_prevKeyState[keyIndex]);
        // Save current state.
        _prevKeyState[keyIndex] = keyState;
    }
    return keyDown;
}

} // namespace LiveKitCpp
