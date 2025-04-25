#include "grayscalevideofilter.h"

void GrayscaleVideoFilter::processFrame(QImage&& image)
{
    if (!image.isNull()) {
        sendProcessed(image.convertedTo(QImage::Format_Grayscale8));
    }
}
