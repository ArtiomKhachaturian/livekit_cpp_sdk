#include "grayscalevideofilter.h"

GrayscaleVideoFilter::GrayscaleVideoFilter(QObject* parent)
    : VideoFilter(grayscaleFilterName(), parent)
{
}

void GrayscaleVideoFilter::processFrame(QImage&& image)
{
    if (!image.isNull()) {
        sendProcessed(image.convertedTo(QImage::Format_Grayscale8));
    }
}
