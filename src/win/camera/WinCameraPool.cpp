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
#ifdef WEBRTC_WIN
#include "WinCameraPool.h"
#include "CameraCapturerProxy.h"
#include "CameraCapturerProxySink.h"
#include "CameraManager.h"
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
    std::shared_ptr<CameraCapturerProxy> acquire(const MediaDevice& device,
                                                 const std::shared_ptr<Bricks::Logger>& logger);
    // impl. of ReleaseManager
    void release(std::string_view deviceGuid, std::shared_ptr<CameraCapturerProxy> proxy) final;
private:
    Bricks::SafeObj<CacheMap> _cache;
};

class WinCameraPool::CameraWrapper : public CameraCapturer, private CameraCapturerProxySink
{
public:
    CameraWrapper(const MediaDevice& device,
                  CameraObserver* observer,
                  const std::weak_ptr<ReleaseManager>& releaseManagerRef,
                  std::shared_ptr<CameraCapturerProxy> proxyImpl);
    ~CameraWrapper() override;
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    bool CaptureStarted() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
private:
    // impl. of CameraObserver
    void onStateChanged(CameraState state) final;
    // impl. of ::rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
private:
    CameraObserver* const _observer;
    const std::weak_ptr<ReleaseManager> _releaseManagerRef;
    Bricks::SafeSharedPtr<CameraCapturerProxy> _proxyImpl;
    std::atomic_bool _captureStarted = false;
};

::rtc::scoped_refptr<CameraCapturer> WinCameraPool::
    create(const MediaDevice& device, const std::shared_ptr<Bricks::Logger>& logger, CameraObserver* observer)
{
    if (!device._guid.empty()) {
        const auto& impl = implementation();
        if (auto proxy = impl->acquire(device, logger)) {
            return rtc::make_ref_counted<CameraWrapper>(device, observer, impl, std::move(proxy));
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
    acquire(const MediaDevice& device, const std::shared_ptr<Bricks::Logger>& logger)
{
    const auto& guid = device._guid;
    if (!guid.empty()) {
        const auto key = deviceKey(guid);
        LOCK_WRITE_SAFE_OBJ(_cache);
        auto& cache = _cache.ref();
        auto it = cache.find(key);
        if (it == cache.end()) {
            if (auto proxy = CameraCapturerProxy::create(WinCameraCapturer::create(device, logger))) {
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

WinCameraPool::CameraWrapper::CameraWrapper(const MediaDevice& device,
                                            CameraObserver* observer,
                                            const std::weak_ptr<ReleaseManager>& releaseManagerRef,
                                            std::shared_ptr<CameraCapturerProxy> proxyImpl)
    : CameraCapturer(device)
    , _observer(observer)
    , _releaseManagerRef(releaseManagerRef)
    , _proxyImpl(std::move(proxyImpl))
{
}

WinCameraPool::CameraWrapper::~CameraWrapper()
{
    if (const auto releaseManager = _releaseManagerRef.lock()) {
        LOCK_WRITE_SAFE_OBJ(_proxyImpl);
        releaseManager->release(guid(), _proxyImpl.take());
    }
}

int32_t WinCameraPool::CameraWrapper::StartCapture(const webrtc::VideoCaptureCapability& capability)
{
    int result = -1;
    LOCK_READ_SAFE_OBJ(_proxyImpl);
    if (const auto& proxyImpl = _proxyImpl.constRef()) {
        result = proxyImpl->startCapture(capability, this);
        _captureStarted = 0 == result;
    }
    return result;
}

int32_t WinCameraPool::CameraWrapper::StopCapture()
{
    int result = -1;
    LOCK_READ_SAFE_OBJ(_proxyImpl);
    if (const auto& proxyImpl = _proxyImpl.constRef()) {
        result = proxyImpl->stopCapture(this);
        if (0 == result) {
            _captureStarted = false;
        }
    }
    return result;
}

bool WinCameraPool::CameraWrapper::CaptureStarted()
{
    auto started = _captureStarted.load(std::memory_order_relaxed);
    if (started) {
        LOCK_READ_SAFE_OBJ(_proxyImpl);
        if (const auto& proxyImpl = _proxyImpl.constRef()) {
            started = proxyImpl->started();
        }
    }
    return started;
}

int32_t WinCameraPool::CameraWrapper::CaptureSettings(webrtc::VideoCaptureCapability& settings)
{
    if (CaptureStarted()) {
        LOCK_READ_SAFE_OBJ(_proxyImpl);
        if (const auto& proxyImpl = _proxyImpl.constRef()) {
            if (proxyImpl->captureCapability(settings)) {
                return 0;
            }
        }
    }
    return -1;
}

void WinCameraPool::CameraWrapper::onStateChanged(CameraState state)
{
    if (_observer) {
        _observer->onStateChanged(state);
    }
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

rtc::scoped_refptr<CameraCapturer> CameraManager::
    createCapturer(const MediaDevice& device, const std::shared_ptr<Bricks::Logger>& logger)
{
    return WinCameraPool::create(device, logger);
}

webrtc::VideoCaptureModule::DeviceInfo* CameraManager::deviceInfo()
{
    using Module = std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo>;
    static const Module info(webrtc::videocapturemodule::DeviceInfoDS::Create());
    return info.get();
}

} // namespace LiveKitCpp
#endif