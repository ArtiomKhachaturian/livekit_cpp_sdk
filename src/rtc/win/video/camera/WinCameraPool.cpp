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
#include "WinCameraPool.h"
#include "CameraCapturerProxy.h"
#include "CapturerProxySink.h"
#include "CameraManager.h"
#include "Listeners.h"
#include "RtcObject.h"
#include "SafeObjAliases.h"
#include "WinCameraCapturer.h"
#include <rtc_base/ref_counted_object.h>
#include <modules/video_capture/windows/device_info_ds.h>
#include <unordered_map>

namespace {

inline size_t deviceKey(std::string_view deviceGuid) { return std::hash<std::string_view>()(deviceGuid); }

} // namespace

namespace LiveKitCpp 
{

class WinCameraPool::ReleaseManager
{
public:
    virtual ~ReleaseManager() = default;
    virtual void release(std::string_view deviceGuid, std::shared_ptr<CameraCapturerProxy> proxy) = 0;
};

class WinCameraPool::Impl : public WinCameraPool::ReleaseManager
{
    using CacheMap = std::unordered_map<size_t, std::shared_ptr<CameraCapturerProxy>>;

public:
    std::shared_ptr<CameraCapturerProxy> acquire(const MediaDeviceInfo& device,
                                                 VideoFrameBufferPool framesPool = {},
                                                 const std::shared_ptr<Bricks::Logger>& logger = {});
    // impl. of ReleaseManager
    void release(std::string_view deviceGuid, std::shared_ptr<CameraCapturerProxy> proxy) final;
private:
    Bricks::SafeObj<CacheMap> _cache;
};

class WinCameraPool::CameraWrapper : public CameraCapturer, 
                                     private RtcObject<CameraCapturerProxy, CapturerProxySink>
{
public:
    CameraWrapper(const MediaDeviceInfo& device,
                  const std::weak_ptr<ReleaseManager>& releaseManagerRef,
                  std::shared_ptr<CameraCapturerProxy> proxyImpl);
    ~CameraWrapper() override;
    void setObserver(CapturerObserver* observer) { _observer = observer;  }
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    bool CaptureStarted() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
private:
    // impl. of CameraObserver
    void onStateChanged(CapturerState state) final;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
private:
    const std::weak_ptr<ReleaseManager> _releaseManagerRef;
    Bricks::Listener<CapturerObserver*> _observer;
    std::atomic_bool _captureStarted = false;
};

rtc::scoped_refptr<CameraCapturer> WinCameraPool::
    create(const MediaDeviceInfo& device,
           VideoFrameBufferPool framesPool,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!device._guid.empty()) {
        const auto& impl = implementation();
        if (auto proxy = impl->acquire(device, std::move(framesPool), logger)) {
            return rtc::make_ref_counted<CameraWrapper>(device, impl, std::move(proxy));
        }
    }
    return {};
}

const std::shared_ptr<WinCameraPool::Impl>& WinCameraPool::implementation()
{
    static const std::shared_ptr<Impl> impl = std::make_shared<Impl>();
    return impl;
}

std::shared_ptr<CameraCapturerProxy> WinCameraPool::Impl::
    acquire(const MediaDeviceInfo& device, 
            VideoFrameBufferPool framesPool,
            const std::shared_ptr<Bricks::Logger>& logger)
{
    const auto& guid = device._guid;
    if (!guid.empty()) {
        const auto key = deviceKey(guid);
        LOCK_WRITE_SAFE_OBJ(_cache);
        auto& cache = _cache.ref();
        auto it = cache.find(key);
        if (it == cache.end()) {
            if (auto proxy = CameraCapturerProxy::create(WinCameraCapturer::create(device, 
                                                                                   std::move(framesPool),
                                                                                   logger))) {
                it = cache.insert({key, std::move(proxy)}).first;
            }
        }
        if (it != cache.end()) {
            return it->second;
        }
    }
    return nullptr;
}

void WinCameraPool::Impl::release(std::string_view deviceGuid, std::shared_ptr<CameraCapturerProxy> proxy)
{
    if (!deviceGuid.empty()) {
        const auto key = deviceKey(deviceGuid);
        LOCK_WRITE_SAFE_OBJ(_cache);
        auto& cache = _cache.ref();
        auto it = cache.find(key);
        if (it != cache.end()) {
            std::shared_ptr<CameraCapturerProxy>().swap(proxy);
            if (1L == it->second.use_count()) {
                cache.erase(it);
            }
        }
    }
}

WinCameraPool::CameraWrapper::CameraWrapper(const MediaDeviceInfo& device,
                                            const std::weak_ptr<ReleaseManager>& releaseManagerRef,
                                            std::shared_ptr<CameraCapturerProxy> proxyImpl)
    : CameraCapturer(device)
    , RtcObject<CameraCapturerProxy, CapturerProxySink>(std::move(proxyImpl))
    , _releaseManagerRef(releaseManagerRef)
{
}

WinCameraPool::CameraWrapper::~CameraWrapper()
{
    if (const auto releaseManager = _releaseManagerRef.lock()) {
        releaseManager->release(guid(), dispose());
    }
}

int32_t WinCameraPool::CameraWrapper::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    int result = -1;
    if (const auto proxy = loadImpl()) {
        result = proxy->startCapture(capability, this);
        _captureStarted = 0 == result;
    }
    return result;
}

int32_t WinCameraPool::CameraWrapper::StopCapture()
{
    int result = -1;
    if (const auto proxy = loadImpl()) {
        result = proxy->stopCapture(this);
        if (0 == result) {
            _captureStarted = false;
        }
    }
    return result;
}

bool WinCameraPool::CameraWrapper::CaptureStarted()
{
    if (_captureStarted.load()) {
        const auto proxy = loadImpl();
        return proxy && proxy->started();
    }
    return false;
}

int32_t WinCameraPool::CameraWrapper::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    if (CaptureStarted()) {
        const auto proxy = loadImpl();
        if (proxy && proxy->captureCapability(settings)) {
            return 0;
        }
    }
    return -1;
}

void WinCameraPool::CameraWrapper::onStateChanged(CapturerState state)
{
    _observer.invoke(&CapturerObserver::onStateChanged, state);
}

void WinCameraPool::CameraWrapper::OnFrame(const webrtc::VideoFrame& frame)
{
    if (CaptureStarted()) {
        sendFrame(frame);
    }
}

void WinCameraPool::CameraWrapper::OnDiscardedFrame()
{
    if (CaptureStarted()) {
        discardFrame();
    }
}

std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> CameraManager::createDeviceInfo()
{
    using Module = std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo>;
    return std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo>{webrtc::videocapturemodule::DeviceInfoDS::Create()};
}

rtc::scoped_refptr<CameraCapturer> CameraManager::
    createCapturer(const MediaDeviceInfo& device, 
                   VideoFrameBufferPool framesPool,
                   const std::shared_ptr<Bricks::Logger>& logger) const
{
    return WinCameraPool::create(device, std::move(framesPool), logger);
}

} // namespace LiveKitCpp
