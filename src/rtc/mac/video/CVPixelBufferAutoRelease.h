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
#pragma once
#include "CFAutoRelease.h"
#include <CoreMedia/CMSampleBuffer.h> // for CMSampleBufferRef
#include <CoreVideo/CVPixelBuffer.h>

namespace LiveKitCpp
{

template <>
class CFAutoReleaseTraits<CVPixelBufferRef>
{
public:
    static constexpr CVPixelBufferRef invalidValue() { return nullptr; }
    static CVPixelBufferRef retain(CVPixelBufferRef ref) { return ref ? CVPixelBufferRetain(ref) : nullptr; }
    static void release(CVPixelBufferRef ref);
};

class CVPixelBufferAutoRelease : public CFAutoRelease<CVPixelBufferRef>
{
    using BaseClass = CFAutoRelease<CVPixelBufferRef>;
public:
    CVPixelBufferAutoRelease();
    CVPixelBufferAutoRelease(std::nullptr_t);
    CVPixelBufferAutoRelease(CVPixelBufferRef buffer, bool retain = false);
    explicit CVPixelBufferAutoRelease(CMSampleBufferRef buffer);
    CVPixelBufferAutoRelease(CVPixelBufferAutoRelease&& tmp);
    CVPixelBufferAutoRelease(const CVPixelBufferAutoRelease& other);
    static OSType pixelFormat(CVPixelBufferRef buffer);
    // the caller does not own the returned CVImageBufferRef, and must retain it explicitly
    // if the caller needs to maintain a reference to it
    static CVImageBufferRef imageBuffer(CMSampleBufferRef buffer);
    bool lock() const; // for read-only
    bool unlock() const;
    OSType pixelFormat() const { return pixelFormat(ref()); }
    size_t dataSize() const;
    size_t planesCount() const;
    size_t width() const;
    size_t width(size_t planeIndex) const;
    size_t height() const;
    size_t height(size_t planeIndex) const;
    size_t stride() const;
    size_t stride(size_t planeIndex) const;
    uint8_t* planeAddress(size_t planeIndex) const;
    uint8_t* baseAddress() const;
};

inline void CFAutoReleaseTraits<CVPixelBufferRef>::release(CVPixelBufferRef ref)
{
    if (ref) {
        CVBufferRelease(ref);
    }
}

} // namespace LiveKitCpp
