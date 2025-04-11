#pragma once
#ifdef _WIN32
#ifdef LIVEKIT_RTC_EXPORTS
    #define LIVEKIT_RTC_API __declspec(dllexport)
#else
    #define LIVEKIT_RTC_API __declspec(dllimport)
#endif
#else
    #define LIVEKIT_RTC_API
#endif
