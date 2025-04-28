#ifndef SEPIAVIDEOFILTER_H
#define SEPIAVIDEOFILTER_H

#include "videofilter.h"

class SepiaVideoFilter : public VideoFilter
{
public:
    SepiaVideoFilter(QObject* parent = nullptr);
    static QString filterName() { return QStringLiteral("sepia"); }
protected:
    // impl. of VideoFilter
    void processFrame(QImage&& image) final;
};

#endif // SEPIAVIDEOFILTER_H
