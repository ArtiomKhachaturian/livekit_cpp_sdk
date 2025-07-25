#pragma once
#ifdef _WIN32
#ifdef LIVEKIT_SIGNALING_EXPORTS
    #define LIVEKIT_SIGNALING_API __declspec(dllexport)
#else
    #define LIVEKIT_SIGNALING_API __declspec(dllimport)
#endif
#else
    #define LIVEKIT_SIGNALING_API __attribute__((visibility("default")))
#endif
