#ifndef GRAYSCALEVIDEOFILTER_H
#define GRAYSCALEVIDEOFILTER_H
#include "videofilter.h"

class GrayscaleVideoFilter : public VideoFilter
{
public:
    GrayscaleVideoFilter(QObject* parent = nullptr);
    static QString filterName() { return QStringLiteral("gray"); }
protected:
    // impl. of VideoFilter
    void processFrame(QImage&& image) final;
};

#endif // GRAYSCALEVIDEOFILTER_H
