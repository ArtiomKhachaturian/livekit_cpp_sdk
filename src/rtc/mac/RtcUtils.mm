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
#include "RtcUtils.h"
#include "Utils.h"
#include <rtc_base/time_utils.h>
#include <api/units/timestamp.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <CoreMedia/CMFormatDescription.h>
#import <CoreAudio/AudioHardwareBase.h>
#import <VideoToolbox/VideoToolbox.h>

namespace {

inline std::optional<webrtc::Timestamp> toTimestamp(const CMTime& time)
{
    if (0 != CMTimeCompare(kCMTimeInvalid, time) && (time.flags & kCMTimeFlags_Valid)) {
        return webrtc::Timestamp::Seconds(CMTimeGetSeconds(time));
    }
    return std::nullopt;
}

}

namespace LiveKitCpp
{

int64_t cmTimeToMicro(const CMTime& time)
{
    const auto ts = toTimestamp(time);
    if (ts.has_value()) {
        return ts->us<int64_t>();
    }
    return 0LL;
}

int32_t cmTimeToMilli(const CMTime& time)
{
    const auto ts = toTimestamp(time);
    if (ts.has_value()) {
        return ts->ms<int32_t>();
    }
    return 0;
}

CFStringRefAutoRelease stringToCFString(std::string_view str)
{
    if (!str.empty()) {
        return CFStringCreateWithCString(kCFAllocatorDefault, str.data(), kCFStringEncodingUTF8);
    }
    return nullptr;
}

std::string osStatusToString(OSStatus status)
{
    if (noErr != status) {
        switch (status) {
            case kAudioHardwareNotRunningError:
                return "audio hardware isn't running";
            case kAudioHardwareUnspecifiedError:
                return "audio hardware unspecified error";
            case kAudioHardwareUnknownPropertyError:
                return "audio object doesn't know about the property at the given address";
            case kAudioHardwareBadPropertySizeError:
                return "an improperly sized buffer was provided when accessing the data of a property";
            case kAudioHardwareIllegalOperationError:
                return "requested operation couldn't be completed";
            case kAudioHardwareBadObjectError:
                return "audio object ID doesn't map to a valid audio object";
            case kAudioHardwareBadDeviceError:
                return "audio object ID doesn't map to a valid audio device";
            case kAudioHardwareBadStreamError:
                return "audio object ID doesn't map to a valid audio stream";
            case kAudioHardwareUnsupportedOperationError:
                return "audio object doesn't support the requested operation";
            case kAudioDeviceUnsupportedFormatError:
                return "audio stream doesn't support the requested format";
            case kAudioDevicePermissionsError:
                return "requested operation can't be completed because the process doesn't have permission";
            case kVTPropertyNotSupportedErr:
                return "property not supported";
            case kVTPropertyReadOnlyErr:
                return "property read only";
            case kVTParameterErr:
                return "parameter error";
            case kVTInvalidSessionErr:
                return "invalid video codec session";
            case kVTAllocationFailedErr:
                return "allocation failed";
            case kVTPixelTransferNotSupportedErr:
                return "pixel transfer not supported";
            case kVTCouldNotFindVideoDecoderErr:
                return "could not find video decoder";
            case kVTCouldNotCreateInstanceErr:
                return "could not create instance";
            case kVTCouldNotFindVideoEncoderErr:
                return "could not find video encoder";
            case kVTVideoDecoderBadDataErr:
                return "video decoder bad data";
            case kVTVideoDecoderUnsupportedDataFormatErr:
                return "video decoder unsupported data format";
            case kVTVideoDecoderMalfunctionErr:
                return "video decoder malfunction";
            case kVTVideoEncoderMalfunctionErr:
                return "video encoder malfunction";
            case kVTVideoDecoderNotAvailableNowErr:
                return "video decoder not available now";
            case kVTImageRotationNotSupportedErr:
                return "image rotation not supported";
            case kVTVideoEncoderNotAvailableNowErr:
                return "video encoder not available now";
            case kVTFormatDescriptionChangeNotSupportedErr:
                return "video format description change not supported";
            case kVTInsufficientSourceColorDataErr:
                return "insufficient source color data";
            case kVTCouldNotCreateColorCorrectionDataErr:
                return "could not create color correction data";
            case kVTColorSyncTransformConvertFailedErr:
                return "color sync transform convert failed";
            case kVTVideoDecoderAuthorizationErr:
                return "video decoder authorization error";
            case kVTVideoEncoderAuthorizationErr:
                return "video encoder authorization error";
            case kVTColorCorrectionPixelTransferFailedErr:
                return "color correction pixel transfer failed";
            case kVTMultiPassStorageIdentifierMismatchErr:
                return "multi-pass storage identifier mismatch";
            case kVTMultiPassStorageInvalidErr:
                return "multi-pass storage invalid";
            case kVTFrameSiloInvalidTimeStampErr:
                return "frame silo invalid time stamp";
            case kVTFrameSiloInvalidTimeRangeErr:
                return "frame silo invalid time range";
            case kVTCouldNotFindTemporalFilterErr:
                return "could not find temporal filter";
            case kVTPixelTransferNotPermittedErr:
                return "pixel transfer not permitted";
            case kVTColorCorrectionImageRotationFailedErr:
                return "color correction image rotation failed";
            case kVTVideoDecoderRemovedErr:
                return "video decoder removed";
            case kVTSessionMalfunctionErr:
                return "video codec session malfunction";
            case kVTVideoDecoderNeedsRosettaErr:
                return "video decoder needs Rosetta";
            case kVTVideoEncoderNeedsRosettaErr:
                return "video encoder needs Rosetta";
            case kCMSampleBufferError_AlreadyHasDataBuffer:
                return "attempt to set data on a sample buffer failed because that buffer already contains media data";
            case kCMSampleBufferError_ArrayTooSmall:
                return "output array isn’t large enough to hold the requested array";
            case kCMSampleBufferError_BufferHasNoSampleSizes:
                return "request for sample sizes on a buffer failed because the buffer doesn’t provide that information";
            case kCMSampleBufferError_BufferHasNoSampleTimingInfo:
                return "request for sample timing on a buffer failed because the buffer doesn’t contain that information";
            case kCMSampleBufferError_BufferNotReady:
                return "system can’t make the buffer’s data ready for use";
            case kCMSampleBufferError_CannotSubdivide:
                return "sample buffer doesn’t contain sample sizes";
            case kCMSampleBufferError_DataCanceled:
                return "sample buffer canceled its data-loading operation";
            case kCMSampleBufferError_DataFailed:
                return "sample buffer failed to load its data";
            case kCMSampleBufferError_InvalidEntryCount:
                return "timing or size value isn’t within the allowed range";
            case kCMSampleBufferError_InvalidMediaFormat:
                return "media format doesn’t match the sample buffer’s format description";
            case kCMSampleBufferError_InvalidMediaTypeForOperation:
                return "media type that the format description defines isn’t a value for the requested operation";
            case kCMSampleBufferError_InvalidSampleData:
                return "sample buffer contains bad data";
            case kCMSampleBufferError_Invalidated:
                return "sample buffer invalidated its data";
            case kCMSampleBufferError_RequiredParameterMissing:
                return "required parameter’s value is invalid";
            case kCMSampleBufferError_SampleIndexOutOfRange:
                return "sample index is outside the range of samples that the buffer contains";
            case kCMBlockBufferBadCustomBlockSourceErr:
                return "custom block source is invalid";
            case kCMBlockBufferBadLengthParameterErr:
                return "block length is zero or doesn’t equal the size of the memory block";
            case kCMBlockBufferBadOffsetParameterErr:
                return "offset doesn’t point to the location of data in the memory block";
            case kCMBlockBufferBadPointerParameterErr:
                return "block buffer reference is invalid";
            case kCMBlockBufferBlockAllocationFailedErr:
                return "block allocator failed to allocate a memory block";
            case kCMBlockBufferInsufficientSpaceErr:
                return "system failed to create a new buffer because of insufficient space at the buffer out location";
            case kCMBlockBufferStructureAllocationFailedErr:
                return "structure allocator failed to allocate a block buffer";
            case kCMBlockBufferUnallocatedBlockErr:
                return "system encountered an unallocated memory block";
            default:
                break;
        }
        @autoreleasepool {
            if (NSError* error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil]) {
                return toString(error);
            }
        }
        return "OS error #" + std::to_string(status);
    }
    return {};
}

webrtc::RTCError toRtcError(OSStatus status, webrtc::RTCErrorType type)
{
    if (noErr != status) {
        if (webrtc::RTCErrorType::NONE == type) {
            type = webrtc::RTCErrorType::UNSUPPORTED_OPERATION;
        }
        return webrtc::RTCError(type, osStatusToString(status));
    }
    return {};
}

} // namespace LiveKitCpp

