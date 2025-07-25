# LiveKit C++ SDK

The **LiveKit C++ SDK** provides native C++ access to the [LiveKit](https://livekit.io) open-source WebRTC platform.  
It enables developers to build high-performance, real-time audio and video applications using C++ on **macOS** and **Windows** platforms.

> ⚠️ Linux is not yet supported. This SDK is under active development.

## Overview

The SDK is divided into two main modules:

### 1. Signaling Library

A lightweight library that handles the WebSocket-based signaling protocol with a LiveKit server.  
It manages:
- Establishing and maintaining the WebSocket connection
- Request/response signaling exchange

It does **not** process server responses internally. Instead, it exposes callback-based mechanisms to allow users to handle signaling messages directly.

### 2. RTC Library

A full-featured WebRTC client implementation that builds on top of the signaling library.  
It manages:
- Media (audio/video) negotiation and transmission
- Data channels
- Peer connection management
- Audio/video filters (custom filters can be implemented by the user)

## Features

- Connect to LiveKit rooms via WebSocket
- Publish and subscribe to audio and video tracks
- Support for custom audio/video filters
- Send and receive data via data channels
- Cross-platform: macOS & Windows (Linux support coming soon)
- Sample Qt demo client included

## Requirements

- **CMake** (>= 3.14)
- **C++17**
- **Xcode 16+** (for macOS)
- **MS Visual Studio 2019+** (for Windows)
- **Protobuf**
- **Abseil**
- **WebRTC static libraries** (must be provided and linked manually)

> Note: You must provide the WebRTC static libraries and headers separately and point to them via CMake variables or environment settings.

## Demo Application

A sample Qt-based demo app is included with the SDK.  
It demonstrates how to:

- Connect to a LiveKit server
- Publish audio/video
- Display remote participants
- Send/receive messages via data channels

## License

This project is licensed under the Apache License 2.0.  
See the [LICENSE](LICENSE) file for full details.

## Resources

- [LiveKit Docs](https://docs.livekit.io)
- [LiveKit Server GitHub](https://github.com/livekit/livekit-server)
- [LiveKit Swift SDK](https://github.com/livekit/client-sdk-swift)
- [LiveKit JS SDK](https://github.com/livekit/client-sdk-js)

## Author

[Artiom Khachaturian](https://github.com/ArtiomKhachaturian)
