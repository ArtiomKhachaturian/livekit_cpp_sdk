#include "audiodevicewrapper.h"
#include "media/AudioDevice.h"

AudioDeviceWrapper::AudioDeviceWrapper(std::shared_ptr<LiveKitCpp::AudioDevice> device,
                                       QObject *parent)
    : QObject(parent)
    , _device(std::move(device))
{
}

AudioDeviceWrapper::~AudioDeviceWrapper()
{
}
