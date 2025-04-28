#include "sepiavideofilter.h"

SepiaVideoFilter::SepiaVideoFilter(QObject* parent)
    : VideoFilter{filterName(), parent}
{
}

void SepiaVideoFilter::processFrame(QImage&& image)
{
    if (!image.isNull()) {
        int sepia, blue;
        for (int i = 0; i < image.width(); i++) {
            for (int j = 0; j < image.height(); j++) {
                const QColor oldColor(image.pixel(i, j));
                int averagevalue = (oldColor.red() + oldColor.green() + oldColor.blue()) / 3;
                sepia = averagevalue + 50;
                sepia = qBound(0, sepia, 255);
                blue = qBound(0, averagevalue, 255);
                image.setPixel(i, j, qRgb(sepia, sepia - 20, blue));
            }
        }
        sendProcessed(std::move(image));
    }
}
