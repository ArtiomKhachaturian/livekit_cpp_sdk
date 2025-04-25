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
#include "MetalRGBScaler.h"
#include "Utils.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#define MTL_STRINGIFY(s) @ #s

namespace
{

static NSString* const rgbShaderSource = MTL_STRINGIFY(
   using namespace metal;
   
   kernel void scale_image(texture2d<float, access::read> srcTexture [[texture(0)]],
                           texture2d<float, access::write> dstTexture [[texture(1)]],
                           constant int &srcWidth [[buffer(0)]],
                           constant int &srcHeight [[buffer(1)]],
                           constant int &dstWidth [[buffer(2)]],
                           constant int &dstHeight [[buffer(3)]],
                           uint2 gid [[thread_position_in_grid]]) {
           if (gid.x >= dstWidth || gid.y >= dstHeight) {
               return;
           }
           
           float scaleX = float(srcWidth) / float(dstWidth);
           float scaleY = float(srcHeight) / float(dstHeight);
           
           int srcX = int(gid.x * scaleX);
           int srcY = int(gid.y * scaleY);
           
           float4 pixel = srcTexture.read(uint2(srcX, srcY));
           dstTexture.write(pixel, gid);
       }
   );

}

namespace LiveKitCpp
{

struct MetalRGBScaler::Impl
{
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    id<MTLLibrary> _defaultLibrary;
    id<MTLComputePipelineState> _pipelineState;
    MTLTextureDescriptor* _srcDescriptor;
    MTLTextureDescriptor* _dstDescriptor;
    Impl();
    ~Impl();
};

MetalRGBScaler::MetalRGBScaler()
    : _impl(create())
{
}

MetalRGBScaler::~MetalRGBScaler()
{
}

bool MetalRGBScaler::valid()
{
    return nullptr != _impl;
}

bool MetalRGBScaler::scale(const std::byte* src, int srcStride,
                           int srcWidth, int srcHeight,
                           std::byte* dst, int dstStride,
                           int dstWidth, int dstHeight)
{
    if (valid() && src && dst && srcStride > 0 &&
        srcWidth > 0 && srcHeight > 0 && dstStride > 0 &&
        dstWidth > 0 && dstHeight > 0) {
        @autoreleasepool {
            _impl->_srcDescriptor.width = srcWidth;
            _impl->_srcDescriptor.height = srcHeight;
            _impl->_dstDescriptor.width = dstWidth;
            _impl->_dstDescriptor.height = dstHeight;

            id<MTLTexture> srcTexture = [_impl->_device newTextureWithDescriptor:_impl->_srcDescriptor];
            id<MTLTexture> dstTexture = [_impl->_device newTextureWithDescriptor:_impl->_dstDescriptor];
            [srcTexture replaceRegion:MTLRegionMake2D(0, 0, srcWidth, srcHeight)
                          mipmapLevel:0
                            withBytes:src
                          bytesPerRow:srcStride];
            
            id<MTLCommandBuffer> commandBuffer = [_impl->_commandQueue commandBuffer];
            id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
            
            [computeEncoder setComputePipelineState:_impl->_pipelineState];
            [computeEncoder setTexture:srcTexture atIndex:0];
            [computeEncoder setTexture:dstTexture atIndex:1];
            
            [computeEncoder setBytes:&srcWidth length:sizeof(int) atIndex:0];
            [computeEncoder setBytes:&srcHeight length:sizeof(int) atIndex:1];
            [computeEncoder setBytes:&dstWidth length:sizeof(int) atIndex:2];
            [computeEncoder setBytes:&dstHeight length:sizeof(int) atIndex:3];
            
            MTLSize gridSize = MTLSizeMake(dstWidth, dstHeight, 1);
            MTLSize threadGroupSize = MTLSizeMake(8, 8, 1); // Размер группы потоков
            [computeEncoder dispatchThreads:gridSize threadsPerThreadgroup:threadGroupSize];
            
            [computeEncoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            
            [dstTexture getBytes:dst
                     bytesPerRow:dstStride
                      fromRegion:MTLRegionMake2D(0, 0, dstWidth, dstHeight)
                     mipmapLevel:0];
        }
        return true;
    }
    return false;
}

std::unique_ptr<MetalRGBScaler::Impl> MetalRGBScaler::create()
{
    auto impl = std::make_unique<Impl>();
    @autoreleasepool {
        impl->_device = MTLCreateSystemDefaultDevice();
        if (!impl->_device) {
            return {};
        }
        impl->_commandQueue = [impl->_device newCommandQueue];
        if (!impl->_commandQueue) {
            return {};
        }
        NSError* error = nil;
        impl->_defaultLibrary = [impl->_device newLibraryWithSource:rgbShaderSource
                                                            options:nil
                                                              error:&error];
        if (!impl->_defaultLibrary) {
            const auto err = toString(error);
            return {};
        }
        id<MTLFunction> kernelFunction = [impl->_defaultLibrary newFunctionWithName:@"scale_image"];
        if (!kernelFunction) {
            return {};
        }
        impl->_pipelineState = [impl->_device newComputePipelineStateWithFunction:kernelFunction error:nil];
        if (!impl->_pipelineState) {
            return {};
        }
    }
    return impl;
}

MetalRGBScaler::Impl::Impl()
{
    _srcDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                        width:0
                                                                       height:0
                                                                    mipmapped:NO];
    _srcDescriptor.usage = MTLTextureUsageShaderRead;
    _dstDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                        width:0
                                                                       height:0
                                                                    mipmapped:NO];
    _dstDescriptor.usage = MTLTextureUsageShaderWrite;
}

MetalRGBScaler::Impl::~Impl()
{
    _device = nil;
    _commandQueue = nil;
    _defaultLibrary = nil;
    _pipelineState = nil;
    _srcDescriptor = nil;
    _dstDescriptor = nil;
}

} // namespace LiveKitCpp
