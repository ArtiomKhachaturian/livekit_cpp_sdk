#pragma once
#ifdef _WIN32
#ifdef LIVEKIT_CLIENT_EXPORTS
    #define LIVEKIT_CLIENT_API __declspec(dllexport)
#else
    #define LIVEKIT_CLIENT_API __declspec(dllimport)
#endif
#else
    #define LIVEKIT_CLIENT_API
#endif