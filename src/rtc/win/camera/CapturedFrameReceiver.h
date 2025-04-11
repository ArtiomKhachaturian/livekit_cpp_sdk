/********************************************************************************************************************
**                                                                                                               **
** Copyright (C) 2022, Dark Matter LLC. All rights Reserved                                                      **
** This software and/or source code may be used, copied and/or disseminated only with the written                **
** permission of Dark Matter LLC, or in accordance with the terms and conditions stipulated in the               **
** agreement/contract under which the software and/or source code has been                                       **
** supplied by Dark Matter LLC or its affiliates. Unauthorized use, copying, or dissemination of this file, via  **
** any medium, is strictly prohibited, and will constitute an infringement of copyright.                         **
**                                                                                                               **
*********************************************************************************************************************/
#pragma once
#ifdef WEBRTC_WIN
#include <atlbase.h> //CComPtr support
#include <stddef.h>
#include <strmif.h>

namespace webrtc {
struct VideoCaptureCapability;
}

namespace LiveKitCpp 
{

class CapturedFrameReceiver
{
public:
    virtual void deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                              DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                              const webrtc::VideoCaptureCapability& frameInfo) = 0;

protected:
    virtual ~CapturedFrameReceiver() = default;
};

} // namespace LiveKitCpp
#endif