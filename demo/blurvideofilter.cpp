#include "blurvideofilter.h"
#include <QPainter>

// https://stackoverflow.com/a/21301895/29318346
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT extern void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius,
                                      bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

BlurVideofilter::BlurVideofilter(QObject* parent)
    : VideoFilter(blurFilterName(), parent)
{
}

void BlurVideofilter::setQuality(bool quality)
{
    if (quality != _quality.exchange(quality)) {
        emit qualityChanged();
    }
}

void BlurVideofilter::setRadius(qreal radius)
{
    if (!qFuzzyCompare(radius, _radius.exchange(radius))) {
        emit radiusChanged();
    }
}

void BlurVideofilter::processFrame(QImage&& image)
{
    if (!image.isNull()) {
        QImage dst(image.size(), image.format());
        {
            QPainter painter(&dst);
            qt_blurImage(&painter, image, _radius, _quality, false, 0);//blur radius: 2px
        }
        sendProcessed(std::move(dst));
    }
}
