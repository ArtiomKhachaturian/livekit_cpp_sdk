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
#include "CVPixelBufferAutoRelease.h"
#include <utility>

namespace LiveKitCpp
{

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease()
    : CVPixelBufferAutoRelease(nullptr)
{
}

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease(std::nullptr_t)
    : BaseClass(nullptr)
{
}

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease(CVPixelBufferRef buffer, bool retain)
    : BaseClass(buffer, retain)
{
}

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease(CMSampleBufferRef buffer)
    : CVPixelBufferAutoRelease(imageBuffer(buffer), true)
{
}

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease(CVPixelBufferAutoRelease&& tmp)
    : BaseClass(std::move(tmp))
{
}

CVPixelBufferAutoRelease::CVPixelBufferAutoRelease(const CVPixelBufferAutoRelease& other)
    : BaseClass(other)
{
}

OSType CVPixelBufferAutoRelease::pixelFormat(CVPixelBufferRef buffer)
{
    return CVPixelBufferGetPixelFormatType(buffer);
}

CVImageBufferRef CVPixelBufferAutoRelease::imageBuffer(CMSampleBufferRef buffer)
{
    if (buffer && CMSampleBufferDataIsReady(buffer) && CMSampleBufferIsValid(buffer) &&
        1L == CMSampleBufferGetNumSamples(buffer)) {
            return CMSampleBufferGetImageBuffer(buffer);
    }
    return nil;
}

bool CVPixelBufferAutoRelease::lock() const
{
    return valid() && kCVReturnSuccess == CVPixelBufferLockBaseAddress(ref(), kCVPixelBufferLock_ReadOnly);
}

bool CVPixelBufferAutoRelease::unlock() const
{
    return valid() && kCVReturnSuccess == CVPixelBufferUnlockBaseAddress(ref(), kCVPixelBufferLock_ReadOnly);
}

size_t CVPixelBufferAutoRelease::dataSize() const
{
    return CVPixelBufferGetDataSize(ref());
}

size_t CVPixelBufferAutoRelease::planesCount() const
{
    return CVPixelBufferGetPlaneCount(ref());
}

size_t CVPixelBufferAutoRelease::width() const
{
    return CVPixelBufferGetWidth(ref());
}

size_t CVPixelBufferAutoRelease::width(size_t planeIndex) const
{
    return CVPixelBufferGetWidthOfPlane(ref(), planeIndex);
}

size_t CVPixelBufferAutoRelease::height() const
{
    return CVPixelBufferGetHeight(ref());
}

size_t CVPixelBufferAutoRelease::height(size_t planeIndex) const
{
    return CVPixelBufferGetHeightOfPlane(ref(), planeIndex);
}

size_t CVPixelBufferAutoRelease::stride() const
{
    return CVPixelBufferGetBytesPerRow(ref());
}

size_t CVPixelBufferAutoRelease::stride(size_t planeIndex) const
{
    return CVPixelBufferGetBytesPerRowOfPlane(ref(), planeIndex);
}

uint8_t* CVPixelBufferAutoRelease::planeAddress(size_t planeIndex) const
{
    return static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(ref(), planeIndex));
}

uint8_t* CVPixelBufferAutoRelease::baseAddress() const
{
    return static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(ref()));
}

} // namespace LiveKitCpp
