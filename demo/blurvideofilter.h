#ifndef BLURVIDEOFILTER_H
#define BLURVIDEOFILTER_H
#include "videofilter.h"

class BlurVideofilter : public VideoFilter
{
public:
    BlurVideofilter() = default;
protected:
    // impl. of VideoFilter
    void processFrame(QImage&& image) final;
private:
    std::atomic_bool _quality = true;
    std::atomic<qreal> _radius = 25.;
};

#endif // BLURVIDEOFILTER_H
