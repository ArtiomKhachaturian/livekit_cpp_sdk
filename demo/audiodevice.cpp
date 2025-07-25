#include "audiodevice.h"
#include <livekit/rtc/media/AudioDevice.h>

AudioDevice::AudioDevice(std::shared_ptr<LiveKitCpp::AudioDevice> device,
                         QObject *parent)
    : QObject(parent)
    , _device(std::move(device))
{
}

AudioDevice::~AudioDevice()
{
}
