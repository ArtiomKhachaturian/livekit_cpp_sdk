#ifndef BLURVIDEOFILTER_H
#define BLURVIDEOFILTER_H
#include "videofilter.h"

class BlurVideofilter : public VideoFilter
{
    Q_OBJECT
    Q_PROPERTY(bool quality READ quality WRITE setQuality NOTIFY qualityChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
public:
    BlurVideofilter(QObject* parent = nullptr);
    static QString filterName() { return QStringLiteral("blur"); }
    bool quality() const { return _quality; }
    qreal radius() const { return _radius; }
public slots:
    void setQuality(bool quality);
    void setRadius(qreal radius);
signals:
    void qualityChanged();
    void radiusChanged();
protected:
    // impl. of VideoFilter
    void processFrame(QImage&& image) final;
private:
    std::atomic_bool _quality = true;
    std::atomic<qreal> _radius = 25.;
};

#endif // BLURVIDEOFILTER_H
