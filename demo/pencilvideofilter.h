#ifndef PENCILVIDEOFILTER_H
#define PENCILVIDEOFILTER_H

#include "videofilter.h"

class PencilVideoFilter : public VideoFilter
{
public:
    PencilVideoFilter(QObject* parent = nullptr);
    static QString filterName() { return QStringLiteral("pencil"); }
protected:
    // impl. of VideoFilter
    void processFrame(QImage&& image) final;
};

#endif // PENCILVIDEOFILTER_H
