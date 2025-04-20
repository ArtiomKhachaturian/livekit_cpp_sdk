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
#include "ScreenCaptureOutput.h"
#include "ScreenCaptureFramesReceiver.h"

@implementation ScreenCaptureOutput {
    std::weak_ptr<LiveKitCpp::ScreenCaptureFramesReceiver> _frameReceiver;
}

- (instancetype) initWith:(const std::weak_ptr<LiveKitCpp::ScreenCaptureFramesReceiver>&) frameReceiver
{
    if (!frameReceiver.expired()) {
        if (self = [super init]) {
            _frameReceiver = frameReceiver;
        }
        return self;
    }
    return nil;
}

- (void) stream:(SCStream*) stream didOutputSampleBuffer:(CMSampleBufferRef) sampleBuffer
         ofType:(SCStreamOutputType) type
{
    if (SCStreamOutputTypeScreen == type && sampleBuffer) {
        if (const auto frameReceiver = _frameReceiver.lock()) {
            frameReceiver->deliverFrame(stream, sampleBuffer);
        }
    }
}

@end
