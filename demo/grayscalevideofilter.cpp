#include "grayscalevideofilter.h"

GrayscaleVideoFilter::GrayscaleVideoFilter(QObject* parent)
    : VideoFilter{filterName(), parent}
{
}

void GrayscaleVideoFilter::processFrame(QImage&& image)
{
    if (!image.isNull()) {
        sendProcessed(image.convertedTo(QImage::Format_Grayscale8));
    }
}
