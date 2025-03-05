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
#include "MacVoiceProcessingAudioUnit.h"
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/system/arch.h>

namespace {

#ifndef NDEBUG
inline void LogStreamDescription(AudioStreamBasicDescription description) {
    char formatIdString[5];
    UInt32 formatId = CFSwapInt32HostToBig(description.mFormatID);
    bcopy(&formatId, formatIdString, 4);
    formatIdString[4] = '\0';
    RTC_LOG_F(LS_VERBOSE) <<
        "AudioStreamBasicDescription: {\n" <<
        "  mSampleRate: " << description.mSampleRate << "\n" <<
        "  formatIDString: " << formatIdString << "\n" <<
        "  mFormatFlags: " << static_cast<unsigned int>(description.mFormatFlags) << "\n" <<
        "  mBytesPerPacket: " << static_cast<unsigned int>(description.mBytesPerPacket) << "\n" <<
        "  mFramesPerPacket: " << static_cast<unsigned int>(description.mFramesPerPacket) << "\n" <<
        "  mBytesPerFrame: " << static_cast<unsigned int>(description.mBytesPerFrame) << "\n" <<
        "  mChannelsPerFrame: " << static_cast<unsigned int>(description.mChannelsPerFrame) << "\n" <<
        "  mBitsPerChannel: " << static_cast<unsigned int>(description.mBitsPerChannel) << "\n" <<
        "  mReserved: " << static_cast<unsigned int>(description.mReserved) << "\n}";
}
#endif

// Returns true if the format flags in |formatFlags| has the "non-interleaved"
// flag (kAudioFormatFlagIsNonInterleaved) cleared (set to 0).
inline bool FormatIsInterleaved(UInt32 formatFlags) {
    return !(formatFlags & kAudioFormatFlagIsNonInterleaved);
}

}

namespace LiveKitCpp
{

MacVoiceProcessingAudioUnit::MacVoiceProcessingAudioUnit(MacVoiceProcessingAudioUnitObserver* observer)
    : _observer(observer)
{
    RTC_DCHECK(_observer);
}

MacVoiceProcessingAudioUnit::~MacVoiceProcessingAudioUnit()
{
    DisposeAudioUnit();
}

OSStatus MacVoiceProcessingAudioUnit::Init()
{
    RTC_DCHECK_EQ(_state, State::InitRequired);
    // Create an audio component description to identify the Voice Processing
    // I/O audio unit.
    AudioComponentDescription vpioUnitDescription;
    vpioUnitDescription.componentType = kAudioUnitType_Output;
    vpioUnitDescription.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
    vpioUnitDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    vpioUnitDescription.componentFlags = 0;
    vpioUnitDescription.componentFlagsMask = 0;
    // Obtain an audio unit instance given the description.
    AudioComponent foundVpioUnitRef = AudioComponentFindNext(nullptr, &vpioUnitDescription);
    // Create a Voice Processing IO audio unit.
    OSStatus result = noErr;
    result = AudioComponentInstanceNew(foundVpioUnitRef, &_vpioUnit);
    if (result != noErr) {
        _vpioUnit = nullptr;
        RTC_LOG_F(LS_ERROR) << "AudioComponentInstanceNew failed. Error=" << result;
        return result;
    }
    // Specify the callback function that provides audio samples to the audio
    // unit.
    AURenderCallbackStruct renderCallback;
    renderCallback.inputProc = OnGetPlayoutData;
    renderCallback.inputProcRefCon = this;
    result = SetAudioUnitProperty<false>(kAudioUnitProperty_SetRenderCallback,
                                         renderCallback, kAudioUnitScope_Input);
    if (result != noErr) {
        DisposeAudioUnit();
        RTC_LOG_F(LS_ERROR) << "Failed to specify the render callback on the output bus. Error=" << result;
        return result;
    }
    // Disable AU buffer allocation for the recorder, we allocate our own.
    result = SetAudioUnitProperty<true, UInt32>(kAudioUnitProperty_ShouldAllocateBuffer,
                                                0U, kAudioUnitScope_Output);
    if (result != noErr) {
        DisposeAudioUnit();
        RTC_LOG_F(LS_ERROR) << "Failed to disable buffer allocation on the input bus. Error=" << result;
        return result;
    }
    // Enable input on the input scope of the input element.
    /*result = SetAudioUnitProperty<true, UInt32>(kAudioOutputUnitProperty_EnableIO,
                                                 1U, kAudioUnitScope_Input);
    if (result != noErr) {
        DisposeAudioUnit();
        RTC_LOG_F(LS_ERROR) << "Failed to enable input on input scope of input element. Error=%ld." << result;
        return result;
    }
    // Enable output on the output scope of the output element.
    result = SetAudioUnitProperty<false, UInt32>(kAudioOutputUnitProperty_EnableIO,
                                                 1U, kAudioUnitScope_Output);*/
    if (result != noErr) {
        DisposeAudioUnit();
        RTC_LOG_F(LS_ERROR) << "Failed to enable output on output scope of output element. Error=%ld." << result;
        return result;
    }
    // Specify the callback to be called by the I/O thread to us when input audio
    // is available. The recorded samples can then be obtained by calling the
    // AudioUnitRender() method.
    AURenderCallbackStruct inputCallback;
    inputCallback.inputProc = OnDeliverRecordedData;
    inputCallback.inputProcRefCon = this;
    result = SetAudioUnitProperty<true>(kAudioOutputUnitProperty_SetInputCallback, inputCallback);
    if (result != noErr) {
        DisposeAudioUnit();
        RTC_LOG_F(LS_ERROR) << "Failed to specify the input callback on the input bus. Error=" << result;
    }
    else {
        _state = State::Uninitialized;
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::SetPlayoutDevice(AudioDeviceID device)
{
    const auto status = SetCurrentDevice<false>(device);
    if (noErr == status) {
        RTC_DCHECK(_vpioUnit);
        const auto latencyStatus = GetAudioUnitLatency<false>(_renderLatencyUs);
        if (noErr != latencyStatus) {
            RTC_LOG_F(LS_ERROR) << "Failed to get renderer (playout) latency. Error=" << latencyStatus;
        }
    }
    return status;
}

OSStatus MacVoiceProcessingAudioUnit::SetRecordingDevice(AudioDeviceID device)
{
    const auto status = SetCurrentDevice<true>(device);
    if (noErr == status) {
        RTC_DCHECK(_vpioUnit);
        const auto latencyStatus = GetAudioUnitLatency<true>(_captureLatencyUs);
        if (noErr != latencyStatus) {
            RTC_LOG_F(LS_ERROR) << "Failed to get capturer (recording) latency. Error=" << latencyStatus;
        }
    }
    return status;
}

OSStatus MacVoiceProcessingAudioUnit::Initialize(Float64 sampleRate)
{
    RTC_DCHECK_GE(_state, State::Uninitialized);
    RTC_LOG_F(LS_VERBOSE) << "Initializing audio unit with sample rate: " << sampleRate;
    OSStatus result = noErr;
    AudioStreamBasicDescription format = GetFormat(sampleRate);
#if !defined(NDEBUG)
    LogStreamDescription(format);
#endif
    // Set the format on the output scope of the input element/bus.
    result = SetAudioUnitProperty<true>(kAudioUnitProperty_StreamFormat,
                                        format, kAudioUnitScope_Output);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to set format on output scope of input bus. Error=" << result;
        return result;
    }
    // Set the format on the input scope of the output element/bus.
    result = SetAudioUnitProperty<false>(kAudioUnitProperty_StreamFormat,
                                         format, kAudioUnitScope_Input);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to set format on input scope of output bus. Error=" << result;
        return result;
    }
    // Initialize the Voice Processing I/O unit instance.
    // Calls to AudioUnitInitialize() can fail if called back-to-back on
    // different ADM instances. The error message in this case is -66635 which is
    // undocumented. Tests have shown that calling AudioUnitInitialize a second
    // time, after a short sleep, avoids this issue.
    // See webrtc:5166 for details.
    size_t failedInitalizeAttempts = 0U;
    result = AudioUnitInitialize(_vpioUnit);
    while (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to initialize the Voice Processing I/O unit. Error=" << result;
        ++failedInitalizeAttempts;
        if (failedInitalizeAttempts == _maxNumberOfAudioUnitInitializeAttempts) {
            // Max number of initialization attempts exceeded, hence abort.
            RTC_LOG_F(LS_ERROR) << "Too many initialization attempts.";
            return result;
        }
        RTC_LOG_F(LS_VERBOSE) << "Pause 100ms and try audio unit initialization again..";
        [NSThread sleepForTimeInterval:0.1f];
        result = AudioUnitInitialize(_vpioUnit);
    }
    if (result == noErr) {
        RTC_LOG_F(LS_VERBOSE) << "Voice Processing I/O unit is now initialized.";
    }
    // AGC should be enabled by default for Voice Processing I/O units but it is
    // checked below and enabled explicitly if needed. This scheme is used
    // to be absolutely sure that the AGC is enabled since we have seen cases
    // where only zeros are recorded and a disabled AGC could be one of the
    // reasons why it happens.
    bool agcWasEnabledByDefault = false;
    UInt32 agcIsEnabled = 0U;
    result = GetAGCState(agcIsEnabled);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to get AGC state (1st attempt). Error=" << result;
    } else if (agcIsEnabled) {
        // Remember that the AGC was enabled by default. Will be used in UMA.
        agcWasEnabledByDefault = true;
    } else {
        // AGC was initially disabled => try to enable it explicitly.
        result = SetAudioUnitProperty<true, UInt32>(kAUVoiceIOProperty_VoiceProcessingEnableAGC, 1U);
        if (result != noErr) {
            RTC_LOG_F(LS_ERROR) << "Failed to enable the built-in AGC. Error=" << result;
        }
        result = GetAGCState(agcIsEnabled);
        if (result != noErr) {
            RTC_LOG_F(LS_ERROR) << "Failed to get AGC state (2nd attempt). Error=" << result;
        }
    }
    // get latencies
    result = GetAudioUnitLatency<true>(_captureLatencyUs);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to get capturer (recording) latency. Error=" << result;
    }
    result = GetAudioUnitLatency<false>(_renderLatencyUs);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to get renderer (playout) latency. Error=" << result;
    }
    // Track if the built-in AGC was enabled by default (as it should) or not.
    RTC_LOG_F(LS_VERBOSE) << "WebRTC.Audio.BuiltInAGCWasEnabledByDefault: " << agcWasEnabledByDefault;
    // As a final step, add an UMA histogram for tracking the AGC state.
    // At this stage, the AGC should be enabled, and if it is not, more work is
    // needed to find out the root cause.
    RTC_LOG_F(LS_VERBOSE) << "WebRTC.Audio.BuiltInAGCIsEnabled: " << agcIsEnabled;
    _state = State::Initialized;
    return noErr;
}

OSStatus MacVoiceProcessingAudioUnit::Start()
{
    RTC_DCHECK_GE(_state, State::Uninitialized);
    RTC_LOG_F(LS_VERBOSE) << "Starting audio unit";
    OSStatus result = AudioOutputUnitStart(_vpioUnit);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to start audio unit. Error=" << result;
    } else {
        RTC_LOG_F(LS_VERBOSE) << "Started audio unit";
        _state = State::Started;
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::Stop()
{
    RTC_DCHECK_GE(_state, State::Uninitialized);
    RTC_LOG_F(LS_VERBOSE) << "Stopping audio unit";
    OSStatus result = AudioOutputUnitStop(_vpioUnit);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to stop audio unit. Error=" << result;
    } else {
        RTC_LOG_F(LS_VERBOSE) << "Stopped audio unit";
        _state = State::Initialized;
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::Uninitialize()
{
    RTC_DCHECK_GE(_state, State::Uninitialized);
    RTC_LOG_F(LS_VERBOSE) << "Unintializing audio unit";
    OSStatus result = AudioUnitUninitialize(_vpioUnit);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to uninitialize audio unit. Error=" << result;
    } else {
        RTC_LOG_F(LS_VERBOSE) << "Uninitialized audio unit";
        _state = State::Uninitialized;
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::Render(AudioUnitRenderActionFlags* flags, 
                                             const AudioTimeStamp* timestamp,
                                             UInt32 outputBusNumber, UInt32 numFrames, 
                                             AudioBufferList* ioData)
{
    RTC_DCHECK(_vpioUnit) << "Init() not called.";
    OSStatus result = AudioUnitRender(_vpioUnit, flags, timestamp,
                                      outputBusNumber, numFrames, ioData);
    if (result != noErr) {
        RTC_LOG_F(LS_ERROR) << "Failed to render audio unit. Error=" << result;
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::GetAGCState(UInt32& enabled) const
{
    const auto result = GetAudioUnitProperty<true>(kAUVoiceIOProperty_VoiceProcessingEnableAGC,
                                                   enabled);
    if (noErr == result) {
        RTC_LOG_F(LS_VERBOSE) << "VPIO unit AGC:" << enabled;
    }
    return result;
}

template <bool inputBus, typename T>
OSStatus MacVoiceProcessingAudioUnit::GetAudioUnitProperty(AudioUnitPropertyID propId,
                                                           T& value,
                                                           AudioUnitScope scope) const
{
    RTC_DCHECK(_vpioUnit);
    UInt32 size = sizeof(T);
    return AudioUnitGetProperty(_vpioUnit, propId, scope,
                                inputBus ? _inputBus : _outputBus,
                                &value, &size);
}

template <bool inputBus, typename T>
OSStatus MacVoiceProcessingAudioUnit::SetAudioUnitProperty(AudioUnitPropertyID propId,
                                                           const T& value,
                                                           AudioUnitScope scope)
{
    RTC_DCHECK(_vpioUnit);
    UInt32 size = sizeof(T);
    return AudioUnitSetProperty(_vpioUnit, propId, scope,
                                inputBus ? _inputBus : _outputBus,
                                &value, size);
}

template <bool inputBus>
OSStatus MacVoiceProcessingAudioUnit::GetAudioUnitLatency(UInt32& latencyUs) const
{
    constexpr auto scope = inputBus ? kAudioUnitScope_Output : kAudioUnitScope_Input;
    AudioStreamBasicDescription format;
    auto result = GetAudioUnitProperty<inputBus>(kAudioUnitProperty_StreamFormat, format, scope);
    if (result == noErr) {
        UInt32 latency = 0U;
        result = GetAudioUnitProperty<inputBus>(kAudioDevicePropertyLatency, latency);
        if (result == noErr) {
            latencyUs = (UInt32)((1.0e6 * latency) / format.mSampleRate);
            result = GetAudioUnitProperty<inputBus>(kAudioStreamPropertyLatency, latency);
            if (result == noErr) {
                latencyUs += (UInt32)((1.0e6 * latency) / format.mSampleRate);
            }
            else {
                latencyUs = 0U;
            }
        }
    }
    return result;
}

OSStatus MacVoiceProcessingAudioUnit::OnGetPlayoutData(void* inRefCon,
                                                       AudioUnitRenderActionFlags* flags,
                                                       const AudioTimeStamp* timestamp,
                                                       UInt32 busNumber, UInt32 numFrames,
                                                       AudioBufferList* ioData)
{
    if (const auto unit = static_cast<MacVoiceProcessingAudioUnit*>(inRefCon)) {
        return unit->_observer->OnGetPlayoutData(flags, timestamp, busNumber, numFrames, ioData);
    }
    return paramErr;
}

OSStatus MacVoiceProcessingAudioUnit::OnDeliverRecordedData(void* inRefCon,
                                                            AudioUnitRenderActionFlags* flags,
                                                            const AudioTimeStamp* timestamp,
                                                            UInt32 busNumber, UInt32 numFrames,
                                                            AudioBufferList* ioData)
{
    if (const auto unit = static_cast<MacVoiceProcessingAudioUnit*>(inRefCon)) {
        return unit->_observer->OnDeliverRecordedData(flags, timestamp, busNumber, numFrames);
    }
    return paramErr;
}

template <bool inputBus>
OSStatus MacVoiceProcessingAudioUnit::SetCurrentDevice(AudioDeviceID device)
{
    return SetAudioUnitProperty<inputBus>(kAudioOutputUnitProperty_CurrentDevice, device);
}

AudioStreamBasicDescription MacVoiceProcessingAudioUnit::GetFormat(Float64 sampleRate) const
{
    // Set the application formats for input and output:
    // - use same format in both directions
    // - avoid resampling in the I/O unit by using the hardware sample rate
    // - linear PCM => noncompressed audio data format with one frame per packet
    // - no need to specify interleaving since only mono is supported
    AudioStreamBasicDescription format;
    RTC_DCHECK_EQ(1, _preferredNumberOfChannels);
    std::memset(&format, 0, sizeof(format));
    format.mSampleRate = sampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    RTC_DCHECK(FormatIsInterleaved(format.mFormatFlags));
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    format.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
    format.mBytesPerPacket = _bytesPerSample;
    format.mFramesPerPacket = 1;  // uncompressed.
    format.mBytesPerFrame = _bytesPerSample;
    format.mChannelsPerFrame = _preferredNumberOfChannels;
    format.mBitsPerChannel = 8 * _bytesPerSample;
    return format;
}

void MacVoiceProcessingAudioUnit::DisposeAudioUnit()
{
    if (_vpioUnit) {
        switch (_state) {
            case State::Started:
                Stop();
                [[fallthrough]];
            case State::Initialized:
                Uninitialize();
                break;
            case State::Uninitialized:
            case State::InitRequired:
                break;
        }
        RTC_LOG_F(LS_VERBOSE) << "Disposing audio unit";
        const auto result = AudioComponentInstanceDispose(_vpioUnit);
        if (result != noErr) {
            RTC_LOG_F(LS_ERROR) << "AudioComponentInstanceDispose failed. Error=" << result;
        }
        _vpioUnit = nullptr;
      }
}

} // namespace LiveKitCpp
