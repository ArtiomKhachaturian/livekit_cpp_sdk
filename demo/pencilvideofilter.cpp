#include "pencilvideofilter.h"

PencilVideoFilter::PencilVideoFilter(QObject *parent)
    : VideoFilter{filterName(), parent}
{
}

void PencilVideoFilter::processFrame(QImage&& image)
{
    for (int i = 0; i < image.width(); i++) {
        for (int j = 0; j < image.height(); j++) {
            const QColor oldColor(image.pixel(i, j));
            int averagevalue = (oldColor.red() + oldColor.green() + oldColor.blue()) / 3;
            if (averagevalue <= 127) {
                image.setPixel(i, j,qRgb(0, 0, 0));
            }
            else if (averagevalue >= 128) {
                image.setPixel(i, j,qRgb(255, 255, 255));
            }
        }
    }
    sendProcessed(std::move(image));
}
