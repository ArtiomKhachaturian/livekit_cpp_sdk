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
#pragma once // MFVideoDecoderPipeline.h
#include "MFNV12VideoPipeline.h"
#include "MFMediaBufferLocker.h"
#include <api/scoped_refptr.h>

namespace webrtc {
class NV12BufferInterface;
}

namespace LiveKitCpp 
{

class MFVideoDecoderPipeline : public MFNV12VideoPipeline
{
public:
    // sync MFT by default
    static constexpr bool syncMFT() { return true; }
    static CompletionStatusOr<MFVideoDecoderPipeline> create(bool hardwareAccellerated,
                                                             webrtc::VideoCodecType codecType,
                                                             UINT32 width = 0U,
                                                             UINT32 height = 0U,
                                                             bool sync = syncMFT());
    MFVideoDecoderPipeline() = default;
    MFVideoDecoderPipeline(const MFVideoDecoderPipeline&) = default;
    MFVideoDecoderPipeline(MFVideoDecoderPipeline&&) = default;
    ~MFVideoDecoderPipeline() override;
    MFVideoDecoderPipeline& operator = (MFVideoDecoderPipeline&&) = default;
    MFVideoDecoderPipeline& operator = (const MFVideoDecoderPipeline&) = default;
    bool hardwareAccellerated() const noexcept final;
    CompletionStatusOrScopedRefPtr<webrtc::VideoFrameBuffer> createBuffer(CComPtr<IMFMediaBuffer> inputBuffer,
                                                                          UINT32 width = 0U, UINT32 height = 0U) const;
    CompletionStatus setMaxCodedWidth(UINT32 maxCodedWidth);
    CompletionStatus setMaxCodedHeight(UINT32 maxCodedHeight);
    CompletionStatus setSoftwareDynamicFormatChange(bool set);
    CompletionStatus setSampleTimeMetrics(const CComPtr<IMFSample>& sample, const webrtc::EncodedImage& from);
    CompletionStatusOr<std::pair<UINT32, UINT32>> uncompressedFrameSize() const;
    CompletionStatusOr<MFVideoArea> minimumDisplayAperture() const;
protected:
    MFVideoDecoderPipeline(MFPipeline impl, webrtc::VideoCodecType codecType, bool dxvaAccelerated);
private:
    bool _dxvaAccelerated = false;
};

} // namespace LiveKitCpp