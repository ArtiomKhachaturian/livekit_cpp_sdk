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
#pragma once // MFNV12VideoPipeline.h
#include "MFPipeline.h"
#include "MFSampleTimeLine.h"
#include "VideoFrameBufferPool.h"
#include <api/video/video_codec_type.h>

namespace LiveKitCpp 
{

class VideoFrameBufferPoolSource;

class MFNV12VideoPipeline
{
public:
    virtual ~MFNV12VideoPipeline() = default;
    explicit operator bool() const { return valid(); }
    bool valid() const { return _impl.valid();  }
    bool encoder() const noexcept { return _impl.encoder(); }
    bool hardwareAccellerated() const noexcept { return _impl.hardwareAccellerated(); }
    bool directXAccelerationSupported() const;
    bool direct3D11Supported() const;
    bool sync() const { return _impl.sync(); }
    bool started() const noexcept { return _impl.started(); }
    const auto& friendlyName() const noexcept { return _impl.friendlyName(); }
    auto codecType() const noexcept { return _codecType; }
    LONGLONG lastTimestampHns() const { return inputFramesTimeline().lastTimestampHns(); }
    CompletionStatus setCompressedFramerate(UINT32 frameRate);
    CompletionStatus setCompressedFrameSize(UINT32 width, UINT32 height);
    CompletionStatus setUncompressedFramerate(UINT32 frameRate);
    CompletionStatus setUncompressedFrameSize(UINT32 width, UINT32 height);
    CompletionStatus beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState = NULL);
    CompletionStatus endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent);
    CompletionStatusOrComPtr<IMFMediaEvent> asyncEvent(DWORD flags = MF_EVENT_FLAG_NO_WAIT) const;
    CompletionStatusOrComPtr<IMFMediaType> compressedMediaType() const { return _impl.compressedMediaType(); }
    CompletionStatusOrComPtr<IMFMediaType> uncompressedMediaType() const { return _impl.uncompressedMediaType(); }
    CompletionStatus setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    CompletionStatus setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    CompletionStatus selectUncompressedMediaType();
    CompletionStatus setLowLatencyMode(bool set);
    CompletionStatus setRealtimeContent(bool set);
    CompletionStatus setNumWorkerThreads(UINT32 numThreads);
    CompletionStatus start() { return _impl.start(); }
    CompletionStatus stop();
    CompletionStatus flush() { return _impl.flush(); }
    CompletionStatus drain() { return _impl.drain(); }
    CompletionStatusOr<DWORD> compressedStatus() const { return _impl.compressedStatus(); }
    CompletionStatusOr<DWORD> uncompressedStatus() const;
    CompletionStatus processInput(const CComPtr<IMFSample>& sample, DWORD flags = 0UL);
    CompletionStatusOrComPtr<IMFSample> createSampleWitMemoryBuffer(bool input) const;
    CompletionStatusOrComPtr<IMFSample> processOutput(const CComPtr<IMFSample>& sample,
                                                         const CComPtr<IMFCollection>& events = {});
    CompletionStatusOr<MFT_INPUT_STREAM_INFO> inputStreamInfo() const { return _impl.inputStreamInfo(); }
    CompletionStatusOr<MFT_OUTPUT_STREAM_INFO> outputStreamInfo() const { return _impl.outputStreamInfo(); }
    CompletionStatus processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param = NULL);
    // formats
    static const GUID& compressedFormat(webrtc::VideoCodecType codecType);
    // media type
    static const GUID& uncompressedType(); // NV12
    static CompletionStatusOrComPtr<IMFMediaType> createUncompressedMediaType();
    static CompletionStatusOrComPtr<IMFMediaType> createCompressedMediaType(webrtc::VideoCodecType codecType);
    static CompletionStatusOrComPtr<IMFMediaType> createMediaType(const GUID& subType);
    static CompletionStatus setAvgBitrate(const CComPtr<IMFMediaType>& mediaType, UINT32 bitsPerSecond);
    static CompletionStatus setFrameParameters(const CComPtr<IMFMediaType>& mediaType,
                                             UINT32 width, UINT32 height,
                                             UINT32 frameRate, MFVideoInterlaceMode im,
                                             bool allSamplesIndependent);
    static CompletionStatus setVideoInterlaceMode(const CComPtr<IMFMediaType>& mediaType,
                                                  MFVideoInterlaceMode im);
protected:
    MFNV12VideoPipeline() = default;
    MFNV12VideoPipeline(const MFNV12VideoPipeline&) = default;
    MFNV12VideoPipeline(MFNV12VideoPipeline&&) = default;
    MFNV12VideoPipeline(MFPipeline impl, webrtc::VideoCodecType codecType);
    MFNV12VideoPipeline& operator = (MFNV12VideoPipeline&&) = default;
    MFNV12VideoPipeline& operator = (const MFNV12VideoPipeline&) = default;
    MFSampleTimeLine& inputFramesTimeline() { return _inputFramesTimeline; }
    const MFSampleTimeLine& inputFramesTimeline() const { return _inputFramesTimeline; }
    VideoFrameBufferPool framesPool() const;
    HRESULT setUINT32Attr(const GUID& attribute, UINT32 value);
    static CompletionStatusOr<MFPipeline> createImpl(webrtc::VideoCodecType codecType,
                                                     bool encoder, bool sync, bool hardwareAccellerated,
                                                     MFTransformConfigurator* configurator = nullptr);
private:
    static bool isWin32LockedDown();
private:
    MFPipeline _impl;
    webrtc::VideoCodecType _codecType = {};
    std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    MFSampleTimeLine _inputFramesTimeline;
};

} // namespace LiveKitCpp